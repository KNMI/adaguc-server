import asyncio
import logging
import socket
import os
import signal

from configure_logging import configure_logging
from adaguc.runAdaguc import runAdaguc
from adaguc.fork_settings import ADAGUC_FORK_SOCKET_PATH

configure_logging(logging)
logger = logging.getLogger(__name__)


class ForkServerSupervisor:
    def __init__(self, interval: int = 5):
        """Initialize supervisor with binary path and health check interval.

        ADAGUC_CONFIG and ADAGUC_ONLINERESOURCE need to be set manually
        """

        self.interval = interval
        self.process: asyncio.subprocess.Process | None = None

        self.env = os.environ.copy()
        adaguc_env = runAdaguc().getAdagucEnv()
        self.env.update({k: str(v) for k, v in adaguc_env.items()})
        self.env["ADAGUC_CONFIG"] = f"{self.env.get('ADAGUC_PATH')}/python/lib/adaguc/adaguc-server-config-python-postgres.xml"
        self.env["ADAGUC_ONLINERESOURCE"] = os.environ.get("EXTERNALADDRESS", "") + "/adaguc-server?"

        self.adaguc_binary_path = f"{self.env.get('ADAGUC_PATH')}/bin/adagucserver"

        self._running = False
        self._stopping = False
        self._task: asyncio.Task | None = None

    async def start_process(self):
        """Asynchronously start the forkserver subprocess."""

        if self._stopping:
            return

        logger.info("Starting forkserver")
        self.process = await asyncio.create_subprocess_exec(self.adaguc_binary_path, env=self.env)

    async def stop_process(self):
        """Terminate the subprocess gracefully, force kill if needed."""

        if not self.process:
            return

        logger.info("Stopping forkserver")
        if self.process.returncode is None:
            self.process.send_signal(signal.SIGTERM)
            try:
                await asyncio.wait_for(self.process.wait(), timeout=2)
            except asyncio.TimeoutError:
                logger.warning("Force killing forkserver")
                self.process.kill()
                await self.process.wait()

        self.process = None

    async def restart_process(self):
        """Restart the subprocess safely."""

        if self._stopping:
            return

        await self.stop_process()
        await self.start_process()

    def health_check_mother(self, timeout=10) -> bool:
        """Check forkserver health via UNIX socket ping."""

        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
                s.settimeout(timeout)
                s.connect(ADAGUC_FORK_SOCKET_PATH)
                s.sendall(b"PING\n")
                data = s.recv(1024)
                return data.strip() == b"PONG"
        except Exception:
            return False

    async def _loop(self):
        """Main supervision loop handling restarts and health checks."""

        try:
            while self._running:
                if self._stopping:
                    break

                # Restart if process crashed
                if self.process and self.process.returncode is not None:
                    await self.start_process()

                # Health check
                elif not self.health_check_mother():
                    await self.restart_process()

                await asyncio.sleep(self.interval)

        except asyncio.CancelledError:
            pass

    async def start_monitoring(self):
        """Start supervision loop and initial subprocess."""

        self._running = True
        self._stopping = False
        await self.start_process()
        self._task = asyncio.create_task(self._loop())

    async def stop_monitoring(self):
        """Stop supervision loop and terminate subprocess."""

        self._stopping = True
        self._running = False

        if self._task:
            self._task.cancel()
            try:
                await self._task
            except asyncio.CancelledError:
                pass

        await self.stop_process()
