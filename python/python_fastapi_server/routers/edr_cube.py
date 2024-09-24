"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

import logging
from typing import Union


from covjson_pydantic.coverage import Coverage, CoverageCollection
from fastapi import Query, Request, APIRouter
from netCDF4 import Dataset

from .covjsonresponse import CovJSONResponse
from .edr_utils import (
    get_ref_times_for_coll,
    instance_to_iso,
    get_metadata,
)
from .netcdf_to_covjson import netcdf_to_covjson
from .ogcapi_tools import call_adaguc

router = APIRouter()

logger = logging.getLogger(__name__)
logger.debug("Starting EDR")


@router.get(
    "/collections/{collection_name}/cube",
    response_model=Union[CoverageCollection, Coverage],
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
    res_x: Union[float, None] = None,
    res_y: Union[float, None] = None,
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
    response_model=Union[CoverageCollection, Coverage],
    response_class=CovJSONResponse,
    response_model_exclude_none=True,
)
async def get_coll_inst_cube(
    collection_name: str,
    request: Request,
    bbox: str,
    instance: Union[str, None] = None,
    datetime_par: str = Query(default=None, alias="datetime"),
    parameter_name: str = Query(alias="parameter-name"),
    z_par: str = Query(alias="z", default=None),
    res_x: Union[float, None] = None,
    res_y: Union[float, None] = None,
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

    vertical_dim = ""
    custom_name = None
    first_layer_name = parameter_name.split(",")[0]
    for param_dim in metadata[collection_name][first_layer_name]["dims"].values():
        if not param_dim["hidden"]:
            if "isvertical" in param_dim:
                vertical_name = param_dim["name"]
            else:
                custom_name = param_dim["name"]
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

    parameter_names = parameter_name.split(",")

    if res_x is not None and res_y is not None:
        res_queryterm = f"&resx={res_x}&resy={res_y}"
    else:
        res_queryterm = ""

    logger.info("callADAGUC by dataset")
    dataset = collection_name

    translate_names = get_translate_names(metadata[collection_name])
    translate_dims = get_translate_dims(metadata[collection_name])

    coveragejsons = []
    parameters = {}
    datetime_arg = datetime_par
    if datetime_arg is None:
        datetime_arg = "*"
    for parameter_name in parameter_names:
        if instance is None:
            urlrequest = (
                f"dataset={dataset}&service=wcs&version=1.1.1&request=getcoverage&format=NetCDF4&crs=EPSG:4326&coverage={parameter_name}"
                + f"&bbox={bbox}&time={datetime_arg}"
                + (f"&{custom_dim_parameter}" if len(custom_dim_parameter) > 0 else "")
                + (f"&{vertical_dim}" if len(vertical_dim) > 0 else "")
                + res_queryterm
            )
        else:
            urlrequest = (
                f"dataset={dataset}&service=wcs&request=getcoverage&format=NetCDF4&crs=EPSG:4326&coverage={parameter_name}"
                + f"&bbox={bbox}&time={datetime_arg}&dim_reference_time={instance_to_iso(instance)}"
                + (f"&{custom_dim_parameter}" if len(custom_dim_parameter) > 0 else "")
                + (f"&{vertical_dim}" if len(vertical_dim) > 0 else "")
                + res_queryterm
            )

        status, response, _ = await call_adaguc(url=urlrequest.encode("UTF-8"))
        logger.info("status: %d", status)
        result_dataset = Dataset(f"{parameter_name}.nc", memory=response.getvalue())

        coveragejson = netcdf_to_covjson(
            result_dataset, translate_names, translate_dims
        )
        if coveragejson is not None:
            coveragejsons.append(coveragejson)
            parameters = parameters | coveragejson.parameters

    if len(coveragejsons) == 1:
        return coveragejsons[0]

    return CoverageCollection(coverages=coveragejsons, parameters=parameters)


def get_translate_names(metadata: dict) -> dict:
    translated_names = {}
    for layer_name in metadata:
        var_name = metadata[layer_name]["layer"]["variables"][0]["variableName"]
        translated_names[var_name] = layer_name
    return {}


def get_translate_dims(metadata: dict) -> dict:
    translated_dims = {}
    for layer_name in metadata:
        for dim_name in metadata[layer_name]["dims"]:
            dim_cdfname = metadata[layer_name]["dims"][dim_name]["cdfName"]
            translated_dims[dim_cdfname] = dim_cdfname
    return {}
