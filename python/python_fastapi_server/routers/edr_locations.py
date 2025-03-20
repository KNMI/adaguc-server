"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

import logging
from fastapi import APIRouter, HTTPException, Request
from fastapi.responses import RedirectResponse

from .utils.edr_utils import location_list

router = APIRouter()
logger = logging.getLogger(__name__)
logger.debug("Starting EDR")


@router.get("/collections/{_coll}/locations")
@router.get("/collections/{_coll}/instances/{instance}/locations")
@router.get("/collections/{_coll}/locations/{id}")
@router.get("/collections/{_coll}/instances/{instance}/locations/{location_id}")
async def get_locations(
    _coll: str, request: Request, instance: str = None, location_id: str = None
):
    """
    Returns locations where you could query data.
    """
    if location_id is None:
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
        if loc["id"] == location_id:
            if instance:
                repl_url = req_url.replace(
                    f"/locations/{instance}/{location_id}", "/position"
                )
            else:
                repl_url = req_url.replace(f"/locations/{location_id}", "/position")
            repl_url = (
                repl_url
                + ("?&" if "?" not in req_url else "&")
                + f"coords=POINT({loc['coordinates'][0]} {loc['coordinates'][1]})"
            )
            return RedirectResponse(repl_url, status_code=302)

    raise HTTPException(status_code=404, detail=f"location {location_id} not found")
