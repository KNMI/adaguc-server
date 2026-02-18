import asyncio
import logging
import socket
import os
import signal

from configure_logging import configure_logging
from adaguc.runAdaguc import runAdaguc

configure_logging(logging)

logger = logging.getLogger(__name__)


class ForkServerSupervisor:
    def __init__(self, socket_path: str, binary_path: str, interval: int = 5):
        self.socket_path = socket_path
        self.binary_path = binary_path
        self.interval = interval
        self.process = None

        self.env = os.environ.copy()
        adaguc_env = runAdaguc().getAdagucEnv()
        self.env.update({k: str(v) for k, v in adaguc_env.items()})
        self.env["ADAGUC_CONFIG"] = f"{self.env['ADAGUC_PATH']}/python/lib/adaguc/adaguc-server-config-python-postgres.xml"
        self.env["ADAGUC_ONLINERESOURCE"] = os.environ.get("EXTERNALADDRESS", "") + "/adaguc-server?"

        self._task = None
        self._running = False

    async def start_process(self):
        logger.info("Starting forkserver")
        self.process = await asyncio.create_subprocess_exec(
            self.binary_path,
            env=self.env,
            # stdout=asyncio.subprocess.PIPE,
            # stderr=asyncio.subprocess.PIPE,
        )

    async def stop_process(self):
        logger.info("Stopping forkserver")
        if self.process and self.process.returncode is None:
            self.process.send_signal(signal.SIGTERM)
            await self.process.wait()

    def check(self, timeout=10) -> bool:
        try:
            with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as s:
                s.settimeout(timeout)
                s.connect(self.socket_path)
                s.sendall(b"PING\n")
                data = s.recv(1024)
                return data.strip() == b"PONG"
        except Exception:
            return False

    async def _loop(self):
        try:
            while self._running:
                # Restart if process crashed
                if self.process and self.process.returncode is not None:
                    await self.start_process()

                # Health check
                is_running = self.check()
                # logger.info("@ fork server running:", is_running)
                if not is_running:
                    await self.stop_process()
                    await self.start_process()

                await asyncio.sleep(self.interval)
        except asyncio.CancelledError:
            pass

    async def start(self):
        self._running = True
        await self.start_process()
        self._task = asyncio.create_task(self._loop())

    async def stop(self):
        # TODO: when ctrl-c fastapi, fork server should not hang and exit immediately (in general, fork server should exit asap when requested)
        self._running = False
        if self._task:
            self._task.cancel()
            # await self._task
        await self.stop_process()
