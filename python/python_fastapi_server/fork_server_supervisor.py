import asyncio
import socket


class ForkServerSupervisor:
    def __init__(self, socket_path: str, interval: int = 5):
        self.socket_path = socket_path
        self.interval = interval
        self._task = None
        self._running = False

    def check(self, timeout: int = 2) -> bool:
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
        while self._running:
            status = self.check()
            print("Fork server running:", status)
            await asyncio.sleep(self.interval)

    def start(self):
        self._running = True
        self._task = asyncio.create_task(self._loop())

    async def stop(self):
        self._running = False
        if self._task:
            await self._task
