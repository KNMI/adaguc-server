from fastapi.responses import JSONResponse


class CovJSONResponse(JSONResponse):
    media_type = "application/prs.coverage+json"
