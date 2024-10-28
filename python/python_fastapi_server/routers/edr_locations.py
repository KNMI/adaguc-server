"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

import logging
from fastapi import APIRouter, HTTPException, Request
from fastapi.responses import RedirectResponse

router = APIRouter()
logger = logging.getLogger(__name__)
logger.debug("Starting EDR")

location_list = [
    {"id": "06260", "name": "De Bilt", "coordinates": [5.1797, 52.0989]},
]


@router.get("/collections/{_coll}/locations")
@router.get("/collections/{_coll}/instances/{instance}/locations")
@router.get("/collections/{_coll}/locations/{id}")
@router.get("/collections/{_coll}/instances/{instance}/locations/{id}")
async def get_locations(
    _coll: str, request: Request, instance: str = None, id: str = None
):
    """
    Returns locations where you could query data.
    """
    if id is None:
        feature_list = [
            {
                "id": loc["id"],
                "type": "Feature",
                "geometry": {"coordinates": loc["coordinates"], "type": "Point"},
                "properties": {"name": loc["name"]},
            }
            for loc in location_list
        ]
        return {"features": feature_list}

    # Redirect to /position call with coordinates filled in
    # in coords=POINT() argument if location id is known
    req_url = str(request.url)
    for loc in location_list:
        if loc["id"] == id:
            repl_url = req_url.replace(f"/locations/{id}", "/position")
            repl_url = (
                repl_url
                + ("?&" if not "?" in req_url else "&")
                + f"coords=POINT({loc['coordinates'][0]} {loc['coordinates'][1]})"
            )
            return RedirectResponse(repl_url, status_code=302)

    raise HTTPException(status_code=404, detail=f"location {id} not found")
