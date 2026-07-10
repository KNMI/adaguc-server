from starlette.types import Receive
from starlette.types import Scope
from starlette.types import Send
from fastapi.middleware.trustedhost import TrustedHostMiddleware


class ShortCircuitHealthCheckMiddleware(TrustedHostMiddleware):
    """Middleware to use TrustedHostMiddleware for all calls except /healthcheck

    This middleware forwards request handling to TrustedHostMiddleware except for
    the /healthcheck request.
    This is necessary because some /healthcheck calling systems sned their requests
    to an IP address (which would result in an "invalid host header" error message).
    So the check for a correct host header is always skipped for /healthcheck calls.
    """

    async def __call__(self, scope: Scope, receive: Receive, send: Send) -> None:
        if scope["type"] == "lifespan":
            await self.app(scope, receive, send)
            return
        if scope["path"] == "/healthcheck":
            await self.app(scope, receive, send)
            return
        await super().__call__(scope, receive, send)
        return
