"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

import logging
from fastapi import APIRouter

router = APIRouter()
logger = logging.getLogger(__name__)
logger.debug("Starting EDR")


@router.get("/collections/{_coll}/locations")
@router.get("/collections/{_coll}/instances/{instance}/locations")
async def get_locations(_coll: str):
    """
    Returns locations where you could query data.
    """
    return {
        "features": [
            {
                "id": "100683",
                "type": "Feature",
                "geometry": {"coordinates": [5.2, 52.0], "type": "Point"},
                "properties": {
                    "name": "De Bilt",
                },
            }
        ]
    }
