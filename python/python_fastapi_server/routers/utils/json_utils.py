from typing import Any
from pydantic_core import from_json


def pydantic_json_loads(data: str | bytes | bytearray) -> Any:
    # Like json.loads, but it uses Pydantic's JSON parser
    # Reuse short dictionary keys across parses; do not cache string values.
    # Preserve json.loads' acceptance of NaN/Infinity in native responses.
    return from_json(data, cache_strings="keys", allow_inf_nan=True)
