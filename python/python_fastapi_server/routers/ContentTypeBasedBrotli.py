"""This middleware extends the BrotliMiddleware but makes it content type aware"""

import asyncio

from brotli_asgi import BrotliMiddleware, BrotliResponder
from starlette.datastructures import Headers
from starlette.middleware.gzip import GZipResponder
from starlette.types import Message, Receive, Scope, Send
from .is_br_encoding_needed import requires_encoding

# The default level 9 is about ten times slower than 6 for a few percent smaller responses, e.g. 2.6s instead of 0.23s for 13MB of CoverageJSON
GZIP_COMPRESSLEVEL = 6

# Responses from this size are compressed on a thread: compressing tens of megabytes, like a large EDR cube, takes long enough to
# hold up all other requests when it is done on the event loop
COMPRESS_ON_THREAD_BYTES = 1024 * 1024


def get_accepted_encodings(headers: Headers) -> set[str]:
    """The content codings of the Accept-Encoding header, like {"br", "gzip"}, in lower case and without parameters like q=0.5.
    A coding with q=0 is refused by the client, so it is left out."""
    encodings = set()
    for item in headers.get("Accept-Encoding", "").split(","):
        coding, *params = (part.strip() for part in item.split(";"))
        quality = 1.0
        for param in params:
            name, _, value = param.partition("=")
            if name.strip().lower() == "q":
                try:
                    quality = float(value)
                except ValueError:
                    pass
        if coding and quality > 0:
            encodings.add(coding.lower())
    return encodings


class MyBrotliResponder(BrotliResponder):
    """This middleware extends the BrotliResponder but makes it content type aware"""

    requires_br_encoding = False
    _precompressed: tuple[bytes, bytes] | None = None  # A body and its compressed bytes, see my_send_with_brotli

    def _process(self, body):
        """The compressed body: from my_send_with_brotli when it compressed this body on a thread, otherwise compressed here"""
        if self._precompressed is not None and self._precompressed[0] is body:
            compressed = self._precompressed[1]
            self._precompressed = None
            return compressed
        return super()._process(body)

    async def __call__(self, scope: Scope, receive: Receive, send: Send) -> None:  # noqa
        self.send = send
        self.requires_br_encoding = False
        await self.app(scope, receive, self.my_send_with_brotli)

    async def my_send_with_brotli(self, message: Message) -> None:
        """Check for certain content types and apply brottly compression."""
        if self.requires_br_encoding is False and "headers" in message:
            self.requires_br_encoding = requires_encoding(Headers(raw=message["headers"]))
        if self.requires_br_encoding:
            body = message.get("body", b"")
            if message["type"] == "http.response.body" and not self.content_encoding_set and len(body) >= COMPRESS_ON_THREAD_BYTES:
                # BrotliResponder compresses on the event loop, which holds up all other requests for a large body, like an EDR
                # cube of tens of megabytes. Brotli releases the GIL, so on a thread the event loop goes on; _process then
                # gives BrotliResponder this result.
                self._precompressed = (body, await asyncio.to_thread(BrotliResponder._process, self, body))
            # Use BrotliResponder to encode with brotli
            await super().send_with_brotli(message)
        else:
            # Send without any encoding
            await self.send(message)


class ContentTypeBasedBrotli(BrotliMiddleware):
    """This middleware extends the BrotliMiddleware but makes it content type aware"""

    async def __call__(self, scope: Scope, receive: Receive, send: Send) -> None:
        """Only difference is use of MyBrotliResponder"""
        if self._is_handler_excluded(scope) or scope["type"] != "http":
            return await self.app(scope, receive, send)
        accepted_encodings = get_accepted_encodings(Headers(scope=scope))
        if "br" in accepted_encodings:
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
        if self.gzip_fallback and "gzip" in accepted_encodings:
            gzip_responder = GZipResponder(self.app, self.minimum_size, compresslevel=GZIP_COMPRESSLEVEL)
            await gzip_responder(scope, receive, send)
            return
        await self.app(scope, receive, send)
