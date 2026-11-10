"""JSON decoding for native ADAGUC responses, without caching response data."""

from pydantic_core import from_json


def loads_native_json(data: str | bytes | bytearray):
    # Reuse repeated short dictionary keys (dimension names/timestamps), as the
    # stdlib decoder does within a document. Do not cache measurement strings.
    # Preserve json.loads' acceptance of NaN/Infinity in native responses.
    return from_json(data, cache_strings="keys", allow_inf_nan=True)
