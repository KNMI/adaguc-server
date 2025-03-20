from fastapi.responses import JSONResponse


class GeoJSONResponse(JSONResponse):
    media_type = "application/geo+json"
