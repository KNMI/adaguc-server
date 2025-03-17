"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

from __future__ import annotations
import logging
import time

from covjson_pydantic.coverage import Coverage, CoverageCollection
from fastapi import HTTPException, Query, Request, APIRouter
from netCDF4 import Dataset

from .covjsonresponse import CovJSONResponse
from .utils.edr_utils import (
    get_ref_times_for_coll,
    instance_to_iso,
    get_metadata,
)
from .utils.edr_exception import EdrException

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
    parameter_name: str = Query(alias="parameter-name"),
    z_par: str = Query(alias="z", default=None),
    res_x: float | None = None,
    res_y: float | None = None,
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
        res_x,
        res_y,
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
    bbox: str,
    instance: str | None = None,
    datetime_par: str = Query(default=None, alias="datetime"),
    parameter_name_par: str = Query(alias="parameter-name"),
    z_par: str = Query(alias="z", default=None),
    resolution_x: float | None = None,
    resolution_y: float | None = None,
) -> Coverage:
    """Returns information in EDR format for a given collection, instance and position"""
    allowed_params = [
        "bbox",
        "datetime",
        "parameter-name",
        "z",
        "f",
        "crs",
        "res_x",
        "res_y",
    ]

    metadata = await get_metadata(collection_name)
    if metadata is None:
        raise EdrException(code=400, description=f"{collection_name} unknown")

    dataset_name = collection_name.rsplit(".", 1)[-1]
    vertical_dim = ""
    vertical_name = None
    custom_name = None
    first_requested_layer_name = parameter_name_par.split(",")[0]
    for param_dim in metadata[collection_name][first_requested_layer_name][
        "dims"
    ].values():
        if not param_dim["hidden"]:
            if param_dim["type"] == "dimtype_vertical":
                vertical_name = param_dim["serviceName"]
            elif param_dim["type"] == "dimtype_custom":
                custom_name = param_dim["serviceName"]
    if z_par:
        if vertical_name is not None:
            if vertical_name.upper() == "ELEVATION":
                vertical_dim = f"{vertical_name}={z_par}"
            else:
                vertical_dim = f"DIM_{vertical_name}={z_par}"

    custom_dims = [k for k in request.query_params if k not in allowed_params]
    custom_dim_parameter = ""
    logger.info("custom dims: %s %s", custom_dims, custom_name)
    if len(custom_dims) > 0:
        for custom_dim in custom_dims:
            custom_dim_parameter += (
                f"&DIM_{custom_dim}={request.query_params[custom_dim]}"
            )

    ref_times = get_ref_times_for_coll(metadata[collection_name])
    if not instance and len(ref_times) > 0:
        instance = ref_times[-1]

    parameter_names = parameter_name_par.split(",")

    if resolution_x is not None and resolution_y is not None:
        res_queryterm = f"&resx={resolution_x}&resy={resolution_y}"
    else:
        res_queryterm = ""

    logger.info("callADAGUC by dataset")

    translate_names = get_translate_names(metadata[collection_name])
    translate_dims = get_translate_dims(metadata[collection_name])

    coveragejsons = []
    parameters = {}
    datetime_arg = datetime_par
    print("TYPE:", type(datetime_arg))
    if datetime_par is None:
        datetime_arg = "*"
    for parameter_name in parameter_names:
        if instance is None:
            urlrequest = (
                f"dataset={dataset_name}&service=wcs&version=1.1.1&request=getcoverage&format=NetCDF4&crs=EPSG:4326&coverage={parameter_name}"
                + f"&bbox={bbox}&time={datetime_arg}"
                + (f"&{custom_dim_parameter}" if len(custom_dim_parameter) > 0 else "")
                + (f"&{vertical_dim}" if len(vertical_dim) > 0 else "")
                + res_queryterm
            )
        else:
            urlrequest = (
                f"dataset={dataset_name}&service=wcs&request=getcoverage&format=NetCDF4&crs=EPSG:4326&coverage={parameter_name}"
                + f"&bbox={bbox}&time={datetime_arg}&dim_reference_time={instance_to_iso(instance)}"
                + (f"&{custom_dim_parameter}" if len(custom_dim_parameter) > 0 else "")
                + (f"&{vertical_dim}" if len(vertical_dim) > 0 else "")
                + res_queryterm
            )

        start = time.time()
        status, response, _ = await call_adaguc(url=urlrequest.encode("UTF-8"))
        logger.info("status: %d [%f]", status, time.time() - start)
        if status != 0:
            raise HTTPException(
                status_code=404,
                detail=f"cube call failed for parameter {parameter_name} [{status}]",
            )
        result_dataset = Dataset(f"{parameter_name}.nc", memory=response.getvalue())

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
