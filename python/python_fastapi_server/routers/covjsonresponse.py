from fastapi import Response
from pydantic import BaseModel


class CovJSONResponse(Response):
    media_type = "application/prs.coverage+json"


def create_coveragejson_response(coverage: BaseModel, response: Response | None = None) -> CovJSONResponse:
    """Serialize a constructed Pydantic model, preserving backend response headers.

    Return a Response directly so FastAPI does not serialize the model again.
    Passing Headers without converting to dict preserves repeated trace headers.
    """
    return CovJSONResponse(
        coverage.model_dump_json(exclude_none=True, by_alias=True),
        headers=response.headers if response is not None else None,
        status_code=response.status_code if response is not None and response.status_code is not None else 200,
    )
