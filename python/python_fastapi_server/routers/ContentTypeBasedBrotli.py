"""This middleware extends the BrotliMiddleware but makes it content type aware"""

from brotli_asgi import BrotliMiddleware, BrotliResponder
from starlette.datastructures import Headers
from starlette.middleware.gzip import GZipResponder
from starlette.types import Message, Receive, Scope, Send
from .is_br_encoding_needed import requires_encoding


class MyBrotliResponder(BrotliResponder):
    """This middleware extends the BrotliResponder but makes it content type aware"""

    requires_br_encoding = False

    async def __call__(self, scope: Scope, receive: Receive, send: Send) -> None:  # noqa
        self.send = send
        self.requires_br_encoding = False
        await self.app(scope, receive, self.my_send_with_brotli)

    async def my_send_with_brotli(self, message: Message) -> None:
        """Check for certain content types and apply brottly compression."""
        if self.requires_br_encoding is False and "headers" in message:
            self.requires_br_encoding = requires_encoding(Headers(raw=message["headers"]))
        if self.requires_br_encoding:
            # Use BrotliResponder to encode with brotli
            await super().send_with_brotli(message)
        else:
            # Send without any encoding
            await self.send(message)


class ContentTypeBasedbrotli(BrotliMiddleware):
    """This middleware extends the BrotliMiddleware but makes it content type aware"""

    async def __call__(self, scope: Scope, receive: Receive, send: Send) -> None:
        """Only difference is use of MyBrotliResponder"""
        if self._is_handler_excluded(scope) or scope["type"] != "http":
            return await self.app(scope, receive, send)
        headers = Headers(scope=scope)
        if "br" in headers.get("Accept-Encoding", ""):
            br_responder = MyBrotliResponder(
                self.app,
                self.quality,
                self.mode,
                self.lgwin,
                self.lgblock,
                self.minimum_size,
            )
            await br_responder(scope, receive, send)
            return
        if self.gzip_fallback and "gzip" in headers.get("Accept-Encoding", ""):
            gzip_responder = GZipResponder(self.app, self.minimum_size)
            await gzip_responder(scope, receive, send)
            return
        await self.app(scope, receive, send)
