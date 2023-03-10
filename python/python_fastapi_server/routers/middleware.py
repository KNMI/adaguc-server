"""
Sets the request scheme to a value derived from the EXTERNALADDRESS env setting

For use behind proxies
"""
import os

from starlette.types import ASGIApp, Receive, Scope, Send


class FixSchemeMiddleware:
    """Fix https scheme behind proxy"""

    def __init__(self, app: ASGIApp):
        self.app = app
        self.scheme = None
        external_address = os.environ["EXTERNALADDRESS"]
        if external_address:
            self.scheme = external_address[: external_address.index(":")]

    async def __call__(self, scope: Scope, receive: Receive, send: Send) -> None:
        if scope["type"] == "http":
            if self.scheme:
                scope["scheme"] = self.scheme
        await self.app(scope, receive, send)
