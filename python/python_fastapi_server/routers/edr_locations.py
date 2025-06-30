"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

import logging
import os
import json

from fastapi import APIRouter, HTTPException, Request
from fastapi.responses import RedirectResponse

from .utils.edr_utils import get_metadata

router = APIRouter()
logger = logging.getLogger(__name__)
logger.debug("Starting EDR")

GLOBAL_LOCATIONS = []


def get_edr_locations():
    global GLOBAL_LOCATIONS
    if len(GLOBAL_LOCATIONS) > 0:
        return GLOBAL_LOCATIONS

    locations_file_path = os.path.join(
        os.environ.get("ADAGUC_PATH"),
        "data/resources/locations/global_edr_locations.geojson",
    )

    GLOBAL_LOCATIONS = []
    try:
        with open(locations_file_path, "r", encoding="utf-8") as loc_f:
            feature_collection = json.load(loc_f)
            GLOBAL_LOCATIONS = feature_collection["features"]
    except OSError:
        logger.error("failed opening: %s", locations_file_path)
    except ValueError:
        logger.error("failed parsing: %s", locations_file_path)
    except KeyError:
        logger.error("no features found: %s", locations_file_path)
    return GLOBAL_LOCATIONS


async def get_locations_for_collection(coll: str):
    metadata = await get_metadata(coll)

    locations_for_coll = []
    try:
        bbox = metadata[coll][next(iter(metadata[coll]))]["layer"].get("latlonbox")
        for location in get_edr_locations():
            coords = location["geometry"]["coordinates"]
            if coords[1] >= bbox[1] and coords[1] <= bbox[3]:
                if coords[0] >= bbox[0] and coords[0] <= bbox[2]:
                    locations_for_coll.append(location)
        return locations_for_coll
    except KeyError:
        return GLOBAL_LOCATIONS


@router.get("/collections/{coll}/locations")
@router.get("/collections/{coll}/instances/{instance}/locations")
@router.get("/collections/{coll}/locations/{location_id}")
@router.get("/collections/{coll}/instances/{instance}/locations/{location_id}")
async def get_locations(
    coll: str, request: Request, instance: str = None, location_id: str = None
):
    """
    Returns locations where you could query data by id
    """

    location_list = await get_locations_for_collection(coll)

    if location_id is None:
        return {"features": location_list}

    # Redirect to /position call with coordinates filled in
    # in coords=POINT() argument if location id is known
    req_url = str(request.url)
    for loc in location_list:
        if loc["id"] == location_id:
            if instance:
                repl_url = req_url.replace(
                    f"/instances/{instance}/locations/{location_id}",
                    f"/instances/{instance}/position",
                )
            else:
                repl_url = req_url.replace(f"/locations/{location_id}", "/position")
            repl_url = (
                repl_url
                + ("?&" if "?" not in req_url else "&")
                + f"coords=POINT({loc['geometry']['coordinates'][0]} {loc['geometry']['coordinates'][1]})"
            )
            return RedirectResponse(repl_url, status_code=302)

    raise HTTPException(status_code=404, detail=f"location {location_id} not found")
