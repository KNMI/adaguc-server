from fastapi import Response
from pydantic import BaseModel


class CovJSONResponse(Response):
    media_type = "application/prs.coverage+json"


def create_coveragejson_response(coverage: BaseModel, response: Response | None = None) -> CovJSONResponse:
    """Serialize a constructed Pydantic model, otherwise Pydantic uses the slower json.dumps.
    Setting headers is necessary to preserve Cache-Control and X-Trace-Timings"""
    headers = response.headers if response is not None else None
    return CovJSONResponse(coverage.model_dump_json(exclude_none=True, by_alias=True), headers=headers)
