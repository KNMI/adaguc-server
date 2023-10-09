import json
from typing import Any

from fastapi import Response
from fastapi.encoders import jsonable_encoder


class CovJSONResponse(Response):
    media_type = "application/prs.coverage+json"

    def render(self, content: Any) -> bytes:
        return content.json(exclude_none=True)

