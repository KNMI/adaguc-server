import logging

from starlette.responses import PlainTextResponse
from uvicorn._types import ASGI3Application, ASGIReceiveCallable, ASGISendCallable, Scope


class DisableRangeRequests:

    def __init__(self, app: ASGI3Application) -> None:
        self.app = app

    async def __call__(self, scope: Scope, receive: ASGIReceiveCallable, send: ASGISendCallable) -> None:
        headers: dict[bytes, bytes] = dict(scope["headers"])
        logging.info(str(headers))
        if b"range" in headers:
            response = PlainTextResponse("Range requests are not supported", status_code=400)
            return await response(scope, receive, send)
        return await self.app(scope, receive, send)
