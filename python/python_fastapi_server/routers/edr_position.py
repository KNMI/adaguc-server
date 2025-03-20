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
from .utils.edr_exception import EdrException, exc_unknown_collection
from .utils.edr_utils import (
    call_adaguc,
    generate_max_age,
    get_custom,
    get_dataset_from_collection,
    get_instance,
    get_metadata,
    get_parameters,
    get_ttl_from_adaguc_headers,
    get_vertical,
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
    parameter_name_par: Annotated[
        str, Query(alias="parameter-name", min_length=1)
    ] = None,
    z_par: Annotated[str, Query(alias="z", min_length=1)] = None,
) -> Coverage:
    """
    returns data for the EDR /position endpoint
    """
    allowed_params = ["coords", "datetime", "parameter-name", "z", "f", "crs"]

    metadata = await get_metadata(collection_name)

    dataset_name = get_dataset_from_collection(metadata, collection_name)
    instance = get_instance(metadata, collection_name, instance)
    parameter_names = get_parameters(metadata, collection_name, parameter_name_par)
    print("PARAMS:", parameter_names)

    _, vertical_dim = get_vertical(metadata, collection_name, parameter_names[0], z_par)

    custom_dims = get_custom(
        request.query_params,
        allowed_params,
    )

    latlons = wkt.loads(coords)
    coord = {"lat": latlons["coordinates"][1], "lon": latlons["coordinates"][0]}

    resp, headers = await get_point_value(
        dataset_name,
        instance,
        [coord["lon"], coord["lat"]],
        parameter_names,
        datetime_par,
        vertical_dim,
        custom_dims,
    )
    if resp:
        dat = json.loads(resp)
        ttl = get_ttl_from_adaguc_headers(headers)
        if ttl is not None:
            response.headers["cache-control"] = generate_max_age(ttl)
        return covjson_from_resp(
            dat,
            metadata[collection_name],
        )

    raise EdrException(code=404, description="No data")


async def get_point_value(
    dataset_name: str,
    instance: str,
    coords: list[float],
    parameters: list[str],
    datetime_par: str,
    vertical_dim: str = None,
    custom_dims: str = None,
):
    """Returns information in EDR format for a given collection and position"""
    urlrequest = (
        f"SERVICE=WMS&VERSION=1.3.0&REQUEST=GetPointValue&CRS=EPSG:4326"
        f"&DATASET={dataset_name}&QUERY_LAYERS={','.join(parameters)}"
        f"&X={coords[0]}&Y={coords[1]}&INFO_FORMAT=application/json"
    )
    if datetime_par:
        urlrequest += f"&TIME={datetime_par}"

    if instance:
        urlrequest += f"&DIM_reference_time={instance_to_iso(instance)}"
    if vertical_dim:
        urlrequest += f"&{vertical_dim}"
    if custom_dims:
        urlrequest += custom_dims

    status, response, headers = await call_adaguc(url=urlrequest.encode("UTF-8"))
    if status == 0:
        return response.getvalue(), headers
    return None, None
