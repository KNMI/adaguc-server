from fastapi.encoders import jsonable_encoder
from fastapi.responses import JSONResponse


class CovJSONResponse(JSONResponse):
    media_type = "application/prs.coverage+json"

