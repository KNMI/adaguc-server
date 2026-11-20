import json

import brotli
import pytest
from starlette.applications import Starlette
from starlette.datastructures import Headers
from starlette.responses import Response, StreamingResponse
from starlette.routing import Route
from starlette.testclient import TestClient

from routers.ContentTypeBasedBrotli import COMPRESS_ON_THREAD_BYTES, ContentTypeBasedBrotli, get_accepted_encodings


@pytest.mark.parametrize(
    "accept_encoding, expected",
    [
        ("", set()),
        ("gzip, deflate, br, zstd", {"gzip", "deflate", "br", "zstd"}),
        ("br;q=1.0, gzip;q=0.8, *;q=0.1", {"br", "gzip", "*"}),
        ("GZIP", {"gzip"}),
        ("gzip;q=0, br", {"br"}),
        ("gzip; q=0.000", set()),
        # Not a substring match: "brotli" or "x-gzip" are no br or gzip
        ("brotli, x-gzip", {"brotli", "x-gzip"}),
    ],
)
def test_get_accepted_encodings(accept_encoding: str, expected: set[str]):
    assert get_accepted_encodings(Headers({"Accept-Encoding": accept_encoding})) == expected


def test_get_accepted_encodings_without_header():
    assert get_accepted_encodings(Headers({})) == set()


# A JSON body larger than COMPRESS_ON_THREAD_BYTES, compressed on a thread instead of on the event loop
LARGE_BODY = json.dumps({"values": [n / 7 for n in range(200_000)]}).encode()
SMALL_BODY = json.dumps({"values": [1, 2, 3] * 300}).encode()


def make_client() -> TestClient:
    async def single(request):
        return Response(LARGE_BODY if request.query_params.get("size") == "large" else SMALL_BODY, media_type="application/json")

    async def streaming(_):
        # A part larger than COMPRESS_ON_THREAD_BYTES (compressed on a thread), then smaller parts (on the event loop)
        async def parts():
            yield LARGE_BODY[: COMPRESS_ON_THREAD_BYTES + 10]
            for start in range(COMPRESS_ON_THREAD_BYTES + 10, len(LARGE_BODY), 400_000):
                yield LARGE_BODY[start : start + 400_000]

        return StreamingResponse(parts(), media_type="application/json")

    app = Starlette(routes=[Route("/single", single), Route("/streaming", streaming)])
    app.add_middleware(ContentTypeBasedBrotli, gzip_fallback=True)
    return TestClient(app)


@pytest.mark.parametrize(
    "path, expected",
    [("/single?size=small", SMALL_BODY), ("/single?size=large", LARGE_BODY), ("/streaming", LARGE_BODY)],
)
def test_brotli_responses(path: str, expected: bytes):
    assert len(LARGE_BODY) > COMPRESS_ON_THREAD_BYTES
    # The raw body, decompressed here: letting the test client decompress it crashes in the brotli module
    with make_client().stream("GET", path, headers={"Accept-Encoding": "br"}) as response:
        assert response.status_code == 200
        assert response.headers["content-encoding"] == "br"
        body = b"".join(response.iter_raw())
    assert brotli.decompress(body) == expected
