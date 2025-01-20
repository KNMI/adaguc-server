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
    get_metadata,
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
    z_value: Annotated[str, Query(alias="z", min_length=1)] = None,
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

    metadata = await get_metadata(collection_name)

    if metadata is not None:
        # Check parameter_name argument, at least 1 parameter should exist in collection
        if parameter_name is None:
            raise EdrException(code=400, description="No parameter-name")
        parameter_names = parameter_name.split(",")
        if not any(param in metadata[collection_name] for param in parameter_names):
            raise EdrException(code=404, description=f"Parameter-name {parameter_name}")
        cleaned_parameter_names = []
        for param in parameter_names:
            if param in metadata[collection_name]:
                cleaned_parameter_names.append(param)
        if len(cleaned_parameter_names) == 0:
            raise EdrException(
                code=404, description=f"Parameter-names unknown {parameter_name}"
            )

        vertical_dim_name = "z"
        for dim_name in metadata[collection_name][parameter_names[0]]["dims"]:
            print(
                "DIM:%s %s",
                dim_name,
                metadata[collection_name][parameter_names[0]]["dims"][dim_name],
            )
            if (
                "isvertical"
                in metadata[collection_name][parameter_names[0]]["dims"][dim_name]
                and metadata[collection_name][parameter_names[0]]["dims"][dim_name][
                    "isvertical"
                ]
                is True
            ):
                vertical_dim_name = dim_name
        latlons = wkt.loads(coords)
        coord = {"lat": latlons["coordinates"][1], "lon": latlons["coordinates"][0]}
        resp, headers = await get_point_value(
            collection_name,
            instance,
            [coord["lon"], coord["lat"]],
            cleaned_parameter_names,
            datetime_par,
            z_value,
            vertical_dim_name,
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
    else:
        raise EdrException(
            code=404, description=f"Collection {collection_name} not found"
        )
    raise EdrException(code=404, description="No data")


async def get_point_value(
    collection_name: str,
    instance: str,
    coords: list[float],
    parameters: list[str],
    datetime_par: str,
    z_value: str = None,
    z_name: str = None,
    custom_dims: str = None,
):
    """Returns information in EDR format for a given collection and position"""
    dataset_name = collection_name.rsplit(".", 1)[0]
    urlrequest = (
        f"SERVICE=WMS&VERSION=1.3.0&REQUEST=GetPointValue&CRS=EPSG:4326"
        f"&DATASET={dataset_name}&QUERY_LAYERS={','.join(parameters)}"
        f"&X={coords[0]}&Y={coords[1]}&INFO_FORMAT=application/json"
    )
    if datetime_par:
        urlrequest += f"&TIME={datetime_par}"

    if instance:
        urlrequest += f"&DIM_reference_time={instance_to_iso(instance)}"
    if z_value:
        if urlrequest.upper() != "ELEVATION":
            urlrequest += f"&DIM_{z_name}={z_value}"
        else:
            urlrequest += f"&ELEVATION={z_value}"
    if custom_dims:
        urlrequest += custom_dims

    status, response, headers = await call_adaguc(url=urlrequest.encode("UTF-8"))
    if status == 0:
        return response.getvalue(), headers
    return None, None
