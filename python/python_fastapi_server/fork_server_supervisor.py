import asyncio
import logging
import os
import signal

from configure_logging import configure_logging
from adaguc.runAdaguc import runAdaguc
from adaguc.fork_settings import get_fork_socket_path

configure_logging(logging)
logger = logging.getLogger(__name__)


class ForkServerSupervisor:
    def __init__(self, interval: int = 5, startup_timeout: float = 10.0):
        """Initialize supervisor with binary path and health check interval.

        ADAGUC_CONFIG and ADAGUC_ONLINERESOURCE need to be set manually
        """

        self.interval = interval
        self.startup_timeout = startup_timeout
        self.process: asyncio.subprocess.Process | None = None

        self.env = os.environ.copy()
        adaguc_env = runAdaguc().getAdagucEnv()
        self.env.update({k: str(v) for k, v in adaguc_env.items()})
        self.env["ADAGUC_CONFIG"] = os.environ.get(
            "ADAGUC_CONFIG", f"{self.env.get('ADAGUC_PATH')}/python/lib/adaguc/adaguc-server-config-python-postgres.xml"
        )
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

        # Isolate the mother and its future forked children in a process group separate from FastAPI.
        self.process = await asyncio.create_subprocess_exec(self.adaguc_binary_path, env=self.env, start_new_session=True)

    async def wait_for_process_group_exit(self, process_group_id: int, timeout: float) -> bool:
        """Wait until a process group no longer contains any processes."""

        loop = asyncio.get_running_loop()
        deadline = loop.time() + timeout
        while True:
            try:
                os.killpg(process_group_id, 0)
            except ProcessLookupError:
                return True
            if loop.time() >= deadline:
                return False
            await asyncio.sleep(min(0.05, deadline - loop.time()))

    async def stop_process(self):
        """Terminate the subprocess gracefully, force kill if needed."""

        if not self.process:
            return

        logger.info("Stopping forkserver")
        loop = asyncio.get_running_loop()
        deadline = loop.time() + 2

        try:
            # Signal the isolated process group, including the mother and every forked request child.
            os.killpg(self.process.pid, signal.SIGTERM)
        except ProcessLookupError:
            pass

        if self.process.returncode is None:
            try:
                await asyncio.wait_for(self.process.wait(), timeout=max(0, deadline - loop.time()))
            except asyncio.TimeoutError:
                pass

        group_exited = await self.wait_for_process_group_exit(self.process.pid, timeout=max(0, deadline - loop.time()))
        if not group_exited:
            logger.warning("Force killing forkserver process group")
            try:
                # Force-stop anything in the group that did not handle SIGTERM within the grace period.
                os.killpg(self.process.pid, signal.SIGKILL)
            except ProcessLookupError:
                pass

        if self.process.returncode is None:
            await self.process.wait()

        try:
            os.unlink(get_fork_socket_path())
        except FileNotFoundError:
            pass

        self.process = None

    async def restart_process(self):
        """Restart the subprocess safely."""

        if self._stopping:
            return

        await self.stop_process()
        await self.start_process()
        if not await self.wait_until_ready():
            logger.error("Forkserver did not become ready after restart")
            await self.stop_process()

    async def wait_until_ready(self) -> bool:
        """Wait until the forkserver answers health checks or the startup deadline expires."""

        loop = asyncio.get_running_loop()
        deadline = loop.time() + self.startup_timeout

        while not self._stopping and loop.time() < deadline:
            if not self.process or self.process.returncode is not None:
                return False
            if await self.health_check_mother():
                return True
            await asyncio.sleep(0.05)

        return False

    async def health_check_mother(self, timeout: float = 1.0) -> bool:
        """Check forkserver health via UNIX socket ping."""

        try:
            # Use asyncio socket IO so the health check never blocks the FastAPI event loop.
            async with asyncio.timeout(timeout):
                reader, writer = await asyncio.open_unix_connection(get_fork_socket_path())
                try:
                    writer.write(b"PING\n")
                    await writer.drain()
                    data = await reader.readline()
                    return data.strip() == b"PONG"
                finally:
                    writer.close()
                    await writer.wait_closed()
        except OSError, asyncio.TimeoutError:
            return False

    async def _loop(self):
        """Main supervision loop handling restarts and health checks."""

        try:
            while self._running:
                if self._stopping:
                    break

                # Restart if process crashed
                if self.process and self.process.returncode is not None:
                    await self.restart_process()

                # Health check
                elif not await self.health_check_mother():
                    await self.restart_process()

                await asyncio.sleep(self.interval)

        except asyncio.CancelledError:
            pass

    async def start_monitoring(self):
        """Start supervision loop and initial subprocess."""

        self._running = True
        self._stopping = False
        await self.start_process()
        if not await self.wait_until_ready():
            await self.stop_process()
            self._running = False
            raise RuntimeError("Forkserver did not become ready during application startup")
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
