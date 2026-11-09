import pytest
from starlette.datastructures import Headers

from routers.ContentTypeBasedBrotli import get_accepted_encodings


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
