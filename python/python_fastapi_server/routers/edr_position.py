"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

import json
from typing import Union

from covjson_pydantic.coverage import Coverage, CoverageCollection
from fastapi import APIRouter, Query, Request
from geomet import wkt
from typing_extensions import Annotated

from .covjsonresponse import CovJSONResponse
from .edr_covjson import covjson_from_resp
from .edr_exception import EdrException
from .edr_utils import (
    call_adaguc,
    generate_max_age,
    get_edr_collections,
    get_ttl_from_adaguc_headers,
    instance_to_iso,
)

router = APIRouter()


@router.get(
    "/collections/{collection_name}/position",
    response_model=Union[Coverage, CoverageCollection],
    response_class=CovJSONResponse,
    response_model_exclude_none=True,
)
async def get_collection_position(
    collection_name: str,
    request: Request,
    coords: str,
    response: CovJSONResponse,
    datetime_par: str = Query(default=None, alias="datetime"),
    parameter_name: Annotated[str, Query(alias="parameter-name", min_length=1)] = None,
    z_par: Annotated[str, Query(alias="z", min_length=1)] = None,
) -> Coverage:
    """Returns information in EDR format for a given collection, instance and position"""
    return await get_coll_inst_position(
        collection_name,
        request,
        coords,
        response,
        None,
        datetime_par,
        parameter_name,
        z_par,
    )


@router.get(
    "/collections/{collection_name}/instances/{instance}/position",
    response_model=Union[Coverage | CoverageCollection],
    response_class=CovJSONResponse,
    response_model_exclude_none=True,
)
async def get_coll_inst_position(
    collection_name: str,
    request: Request,
    coords: str,
    response: CovJSONResponse,
    instance: str = None,
    datetime_par: str = Query(default=None, alias="datetime"),
    parameter_name: Annotated[str, Query(alias="parameter-name", min_length=1)] = None,
    z_par: Annotated[str, Query(alias="z", min_length=1)] = None,
) -> Coverage:
    """
    returns data for the EDR /position endpoint
    """
    allowed_params = ["coords", "datetime", "parameter-name", "z", "f", "crs"]
    custom_params = [k for k in request.query_params if k not in allowed_params]
    custom_dims = ""
    if len(custom_params) > 0:
        for custom_param in custom_params:
            custom_dims += f"&DIM_{custom_param}={request.query_params[custom_param]}"
    edr_collections = get_edr_collections()

    if collection_name in edr_collections:
        parameter_names = parameter_name.split(",")
        latlons = wkt.loads(coords)
        coord = {"lat": latlons["coordinates"][1], "lon": latlons["coordinates"][0]}
        resp, headers = await get_point_value(
            edr_collections[collection_name],
            instance,
            [coord["lon"], coord["lat"]],
            parameter_names,
            datetime_par,
            z_par,
            custom_dims,
        )
        if resp:
            dat = json.loads(resp)
            ttl = get_ttl_from_adaguc_headers(headers)
            if ttl is not None:
                response.headers["cache-control"] = generate_max_age(ttl)
            return covjson_from_resp(
                dat,
                edr_collections[collection_name]["vertical_name"],
                edr_collections[collection_name]["custom_name"],
                collection_name,
            )

    raise EdrException(code=400, description="No data")


async def get_point_value(
    edr_collectioninfo: dict,
    instance: str,
    coords: list[float],
    parameters: list[str],
    datetime_par: str,
    z_par: str = None,
    custom_dims: str = None,
):
    """Returns information in EDR format for a given collection and position"""
    dataset = edr_collectioninfo["dataset"]
    urlrequest = (
        f"SERVICE=WMS&VERSION=1.3.0&REQUEST=GetPointValue&CRS=EPSG:4326"
        f"&DATASET={dataset}&QUERY_LAYERS={','.join(parameters)}"
        f"&X={coords[0]}&Y={coords[1]}&INFO_FORMAT=application/json"
    )
    if datetime_par:
        urlrequest += f"&TIME={datetime_par}"

    if instance:
        urlrequest += f"&DIM_reference_time={instance_to_iso(instance)}"
    if z_par:
        if (
            "vertical_name" in edr_collectioninfo
            and edr_collectioninfo["vertical_name"].upper() != "ELEVATION"
        ):
            urlrequest += f"&DIM_{edr_collectioninfo['vertical_name']}={z_par}"
        else:
            urlrequest += f"&ELEVATION={z_par}"
    if custom_dims:
        urlrequest += custom_dims

    status, response, headers = await call_adaguc(url=urlrequest.encode("UTF-8"))
    if status == 0:
        return response.getvalue(), headers
    return None, None
