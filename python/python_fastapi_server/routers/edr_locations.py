"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

import functools
import logging
import os
import json
from typing import Union
from geojson_pydantic import Feature, FeatureCollection, Point
from typing_extensions import Annotated

from fastapi import APIRouter, HTTPException, Query, Request

from routers.covjsonresponse import CovJSONResponse
from routers.edr_position import handle_coll_inst_position

from covjson_pydantic.coverage import Coverage, CoverageCollection

from .utils.edr_utils import get_custom, get_metadata

router = APIRouter()
logger = logging.getLogger(__name__)


@functools.lru_cache(maxsize=1)
def get_edr_locations() -> list[Feature]:
    locations_file_path = os.path.join(
        os.environ.get("ADAGUC_PATH"),
        "data/resources/locations/global_edr_locations.geojson",
    )

    all_locations = []
    try:
        with open(locations_file_path, "r", encoding="utf-8") as loc_f:
            feature_collection = FeatureCollection(**json.load(loc_f))
            all_locations = feature_collection.features
    except IOError:
        logger.error("failed opening: %s", locations_file_path)
    except ValueError:
        logger.error("failed parsing: %s", locations_file_path)
    except KeyError:
        logger.error("no features found: %s", locations_file_path)
    return all_locations


async def get_locations_for_collection(coll: str) -> list[Feature]:
    metadata = await get_metadata(coll)

    locations_for_coll: list[Feature] = []
    try:
        # We take the bbox from the first layer
        bbox = next(iter(metadata[coll].values()))["layer"].get("latlonbox")
        for location in get_edr_locations():
            geom: Point = location.geometry
            coords = geom.coordinates
            left, bottom, right, top = bbox
            if bottom <= coords[1] <= top:
                if left <= coords[0] <= right:
                    locations_for_coll.append(location)
        return locations_for_coll
    except KeyError:
        logger.error("failed to get collection specific locations, returning all")
        return get_edr_locations()


@router.get(
    "/collections/{collection_name}/locations",
    response_model=Union[Coverage, CoverageCollection, FeatureCollection],
    response_class=CovJSONResponse,
    response_model_exclude_none=True,
)
@router.get(
    "/collections/{collection_name}/instances/{instance}/locations",
    response_model=Union[Coverage, CoverageCollection, FeatureCollection],
    response_class=CovJSONResponse,
    response_model_exclude_none=True,
)
@router.get(
    "/collections/{collection_name}/locations/{location_id}",
    response_model=Union[Coverage, CoverageCollection, FeatureCollection],
    response_class=CovJSONResponse,
    response_model_exclude_none=True,
)
@router.get(
    "/collections/{collection_name}/instances/{instance}/locations/{location_id}",
    response_model=Union[Coverage, CoverageCollection, FeatureCollection],
    response_class=CovJSONResponse,
    response_model_exclude_none=True,
)
async def get_locations_(
    collection_name: str,
    request: Request,
    response: CovJSONResponse,
    location_id: str = None,
    instance: str = None,
    datetime_par: str = Query(default=None, alias="datetime"),
    parameter_name_par: Annotated[
        str, Query(alias="parameter-name", min_length=1)
    ] = None,
    z_par: Annotated[str, Query(alias="z", min_length=1)] = None,
) -> Union[FeatureCollection, Coverage, CoverageCollection]:
    """
    Returns locations where you could query data by id
    """

    location_list = await get_locations_for_collection(collection_name)

    if location_id is None:
        return FeatureCollection(type="FeatureCollection", features=location_list)

    allowed_params = ["datetime", "parameter-name", "z", "f"]
    custom_dims = get_custom(
        request.query_params,
        allowed_params,
    )

    for loc in location_list:
        if loc.id == location_id:
            point = loc.geometry
            coords = f"POINT({point.coordinates[0]} {point.coordinates[1]})"
            return await handle_coll_inst_position(
                collection_name,
                custom_dims,
                coords,
                response,
                instance,
                datetime_par,
                parameter_name_par,
                z_par,
            )

    raise HTTPException(status_code=404, detail=f"location {location_id} not found")
