"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

from __future__ import annotations
import logging
import time
from typing_extensions import Annotated

from covjson_pydantic.coverage import Coverage, CoverageCollection
from fastapi import Query, Request, APIRouter, Response
from netCDF4 import Dataset

from .utils.edr_exception import exc_failed_call

from .covjsonresponse import CovJSONResponse
from .utils.edr_utils import (
    generate_max_age,
    get_custom,
    get_dataset_from_collection,
    get_instance,
    get_parameters,
    get_ttl_from_adaguc_headers,
    get_vertical,
    instance_to_iso,
    get_metadata,
)


from .netcdf_to_covjson import netcdf_to_covjson
from .utils.ogcapi_tools import call_adaguc

router = APIRouter()

logger = logging.getLogger(__name__)
logger.debug("Starting EDR")


@router.get(
    "/collections/{collection_name}/cube",
    response_model=CoverageCollection | Coverage,
    response_class=CovJSONResponse,
    response_model_exclude_none=True,
)
async def get_collection_cube(
    collection_name: str,
    request: Request,
    bbox: str,
    datetime_par: str = Query(default=None, alias="datetime"),
    parameter_name: Annotated[str, Query(alias="parameter-name", min_length=1)] = None,
    z_par: Annotated[str, Query(alias="z", min_length=1)] = None,
    resolution_x: float | None = None,
    resolution_y: float | None = None,
) -> Coverage:
    """Returns information in EDR format for a given collection and position"""
    return await get_coll_inst_cube(
        collection_name,
        request,
        bbox,
        None,
        datetime_par,
        parameter_name,
        z_par,
        resolution_x,
        resolution_y,
    )


@router.get(
    "/collections/{collection_name}/instances/{instance}/cube",
    response_model=CoverageCollection | Coverage,
    response_class=CovJSONResponse,
    response_model_exclude_none=True,
)
async def get_coll_inst_cube(
    collection_name: str,
    request: Request,
    response: Response,
    bbox: str,
    instance: str | None = None,
    datetime_par: str = Query(default=None, alias="datetime"),
    parameter_name_par: Annotated[
        str | None, Query(alias="parameter-name", min_length=1)
    ] = None,
    z_par: Annotated[str, Query(alias="z", min_length=1)] = None,
    resolution_x: float | None = None,
    resolution_y: float | None = None,
) -> CoverageCollection | Coverage:
    """Returns information in EDR format for a given collection, instance and position"""
    allowed_params = [
        "bbox",
        "datetime",
        "parameter-name",
        "z",
        "f",
        "crs",
        "resolution_x",
        "resolution_y",
    ]

    metadata = await get_metadata(collection_name)

    dataset_name = get_dataset_from_collection(metadata, collection_name)
    instance = get_instance(metadata, collection_name, instance)
    parameter_names = get_parameters(metadata, collection_name, parameter_name_par)

    _, vertical_dim = get_vertical(metadata, collection_name, parameter_names[0], z_par)

    custom_dims = get_custom(
        request.query_params,
        allowed_params,
    )

    if resolution_x is not None and resolution_y is not None:
        res_queryterm = [f"resx={resolution_x}", f"resy={resolution_y}"]
    else:
        res_queryterm = []

    logger.info("callADAGUC by dataset")

    translate_names = get_translate_names(metadata[collection_name])
    translate_dims = get_translate_dims(metadata[collection_name])

    coveragejsons = []
    parameters = {}
    datetime_arg = datetime_par
    print("TYPE:", type(datetime_arg))
    if datetime_par is None:
        datetime_arg = "2000/3000"
    for parameter_name in parameter_names:
        urlrequest = "&".join(
            [
                f"dataset={dataset_name}",
                "service=wcs&version=1.1.1&request=getcoverage&format=NetCDF4&crs=EPSG:4326",
                f"coverage={parameter_name}",
                f"bbox={bbox}",
                f"time={datetime_arg}",
                f"dim_reference_time={instance_to_iso(instance)}" if instance else "",
                *custom_dims,
                f"{vertical_dim}" if len(vertical_dim) > 0 else "",
                *res_queryterm,
            ]
        )

        start = time.time()
        status, wcs_response, headers = await call_adaguc(
            url=urlrequest.encode("UTF-8")
        )
        ttl = get_ttl_from_adaguc_headers(headers)
        if ttl is not None:
            response.headers["cache-control"] = generate_max_age(ttl)

        logger.info("status: %d [%f]", status, time.time() - start)
        if status != 0:
            raise exc_failed_call(
                f"cube call failed for parameter {parameter_name} [{status}]"
            )
        result_dataset = Dataset(f"{parameter_name}.nc", memory=wcs_response.getvalue())

        coveragejson = netcdf_to_covjson(
            metadata[collection_name], result_dataset, translate_names, translate_dims
        )
        if coveragejson is not None:
            coveragejsons.extend(coveragejson)
            for covjson in coveragejson:
                parameters = parameters | covjson.parameters

    if len(coveragejsons) == 1:
        return coveragejsons[0]

    return CoverageCollection(coverages=coveragejsons, parameters=parameters)


def get_translate_names(metadata: dict) -> dict:
    translate_names = {}
    for layer_name in metadata:
        if "dims" in metadata[layer_name] and metadata[layer_name]["dims"] is not None:
            var_name = metadata[layer_name]["layer"]["variables"][0]["variableName"]
            translate_names[var_name] = layer_name
    return translate_names


def get_translate_dims(metadata: dict) -> dict:
    translate_dims = {}
    for layer_name in metadata:
        if "dims" in metadata[layer_name] and metadata[layer_name]["dims"] is not None:
            for dim_name in metadata[layer_name]["dims"]:
                dim_cdfname = metadata[layer_name]["dims"][dim_name]["cdfName"]
                translate_dims[dim_cdfname] = dim_name
    return translate_dims
