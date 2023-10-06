import json
from typing import Any

from fastapi import Response
from fastapi.encoders import jsonable_encoder


class GeoJSONResponse(Response):
    media_type = "application/geo+json"

    def render(self, content: Any) -> bytes:
        return content.json(exclude_none=True)

