import os
import typing
from starlette.types import ASGIApp, Receive, Send, Scope

"""
Sets the request scheme to a value derived from the EXTERNALADDRESS env setting

For use behind proxies
"""


class FixSchemeMiddleware:
    def __init__(self, app: ASGIApp):
        self.app = app
        self.scheme = None
        ea = os.environ["EXTERNALADDRESS"]
        if ea:
            self.scheme = ea[: ea.index(":")]

    async def __call__(self, scope: Scope, receive: Receive, send: Send) -> None:
        if scope["type"] == "http":
            if self.scheme:
                scope["scheme"] = self.scheme
        await (self.app(scope, receive, send))
