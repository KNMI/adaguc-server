"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

from typing import Any, Union

from covjson_pydantic.coverage import Coverage, CoverageCollection
from fastapi import APIRouter, Query, Request, Response
from typing_extensions import Annotated

from .covjsonresponse import CovJSONResponse, create_coveragejson_response
from .edr_covjson import covjson_from_resp
from .utils.edr_exception import EdrException
from .utils.json_utils import loads_native_json
from .utils.edr_utils import (
    call_adaguc,
    get_custom,
    get_dataset_from_collection,
    get_instance,
    get_metadata,
    get_parameters,
    get_vertical_query,
    instance_to_iso,
    encode_native_query,
    parse_coordinates,
    propagate_data_headers,
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
    response: Response,
    datetime_par: str | None = Query(default=None, alias="datetime"),
    parameter_name: Annotated[str | None, Query(alias="parameter-name", min_length=1)] = None,
    z_par: Annotated[str | None, Query(alias="z", min_length=1)] = None,
) -> CovJSONResponse:
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
    response: Response,
    instance: str | None = None,
    datetime_par: str | None = Query(default=None, alias="datetime"),
    parameter_name_par: Annotated[str | None, Query(alias="parameter-name", min_length=1)] = None,
    z_par: Annotated[str | None, Query(alias="z", min_length=1)] = None,
) -> CovJSONResponse:
    """
    returns data for the EDR /position endpoint
    """
    allowed_params = ["coords", "datetime", "parameter-name", "z", "f", "crs"]
    custom_dims = get_custom(
        request.query_params,
        allowed_params,
    )

    metadata = await get_metadata(collection_name, response=response)

    coverage = await handle_coll_inst_position(
        collection_name,
        custom_dims,
        coords,
        response,
        metadata,
        instance,
        datetime_par,
        parameter_name_par,
        z_par,
    )
    return create_coveragejson_response(coverage, response=response)


async def handle_coll_inst_position(
    collection_name: str,
    custom_dims: list[str],
    coords: str,
    response: Response,
    metadata: dict[str, Any],
    instance: str | None = None,
    datetime_par: str | None = Query(default=None, alias="datetime"),
    parameter_name_par: Annotated[str | None, Query(alias="parameter-name", min_length=1)] = None,
    z_par: Annotated[str | None, Query(alias="z", min_length=1)] = None,
) -> Coverage | CoverageCollection:
    """
    Handle the EDR position endpoint call
    """

    dataset_name = get_dataset_from_collection(metadata, collection_name)
    instance = get_instance(metadata, collection_name, instance)
    parameter_names = get_parameters(metadata, collection_name, parameter_name_par)

    vertical_dim = get_vertical_query(metadata, collection_name, parameter_names[0], z_par)

    point = parse_coordinates(coords, "Point")[0]

    resp, headers = await get_point_value(
        dataset_name,
        instance,
        point,
        parameter_names,
        datetime_par,
        vertical_dim,
        custom_dims,
    )
    if resp:
        dat = loads_native_json(resp)
        propagate_data_headers(response, headers)
        return covjson_from_resp(dat, metadata[collection_name], collection_name)

    raise EdrException(code=404, description="No data")


async def get_point_value(
    dataset_name: str,
    instance: str,
    coords: list[float],
    parameters: list[str],
    datetime_par: str,
    vertical_dim: str = None,
    custom_dims: list[str] = None,
) -> tuple[bytes, list[str]]:
    """Returns information in EDR format for a given collection and position"""
    custom_dims = [] if custom_dims is None else custom_dims
    query = {
        "dataset": dataset_name,
        "service": "wms",
        "version": "1.3.0",
        "request": "GetPointValue",
        "info_format": "application/json",
        "crs": "EPSG:4326",
        "query_layers": ",".join(parameters),
        "X": coords[0],
        "Y": coords[1],
    }
    if datetime_par:
        query["time"] = datetime_par
    if instance:
        query["dim_reference_time"] = instance_to_iso(instance)
    urlrequest = encode_native_query(query, [*custom_dims, vertical_dim])

    status, response, headers = await call_adaguc(url=urlrequest)
    if status == 0:
        return response, headers
    return None, None
