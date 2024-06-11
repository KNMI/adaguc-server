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
    parse_config_file,
    get_ref_times_for_coll,
    get_edr_collections,
    instance_to_iso,
)
from .netcdf_to_covjson import netcdf_to_covjson
from .ogcapi_tools import call_adaguc

router = APIRouter()

logger = logging.getLogger(__name__)
logger.debug("Starting EDR")


@router.get(
    "/collections/{collection_name}/cube",
    response_model=CoverageCollection,
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
    response_model=CoverageCollection,
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

    edr_collectioninfo = get_edr_collections()[collection_name]

    vertical_dim = ""
    if z_par:
        if edr_collectioninfo.get("vertical_name"):
            vertical_dim = f"{edr_collectioninfo.get('vertical_name')}={z_par}"

    custom_dims = [k for k in request.query_params if k not in allowed_params]
    custom_dim_parameter = ""
    logger.info("custom dims: %s", custom_dims)
    if len(custom_dims) > 0:
        for custom_dim in custom_dims:
            custom_dim_parameter += (
                f"&DIM_{custom_dim}={request.query_params[custom_dim]}"
            )

    dataset = edr_collectioninfo["dataset"]
    ref_times = await get_ref_times_for_coll(
        edr_collectioninfo, edr_collectioninfo["parameters"][0]["name"]
    )
    if not instance and len(ref_times) > 0:
        instance = ref_times[-1]

    parameter_names = parameter_name.split(",")

    if res_x is not None and res_y is not None:
        res_queryterm = f"&resx={res_x}&resy={res_y}"
    else:
        res_queryterm = ""

    collection_info = get_edr_collections().get(collection_name)
    if "dataset" in collection_info:
        logger.info("callADAGUC by dataset")
        dataset = collection_info["dataset"]
        translate_names, translate_dims = parse_config_file(dataset)
        coveragejsons = []
        parameters = {}
        for parameter_name in parameter_names:
            if instance is None:
                urlrequest = (
                    f"dataset={dataset}&service=wcs&version=1.1.1&request=getcoverage&format=NetCDF4&crs=EPSG:4326&coverage={parameter_name}"
                    + f"&bbox={bbox}&time={datetime_par}"
                    + (f"&{custom_dim_parameter}" if len(custom_dim_parameter)>0 else "")
                    + (f"&{vertical_dim}" if len(vertical_dim)>0 else "")
                    + res_queryterm
                )
            else:
                urlrequest = (
                    f"dataset={dataset}&service=wcs&request=getcoverage&format=NetCDF4&crs=EPSG:4326&coverage={parameter_name}"
                    + f"&bbox={bbox}&time={datetime_par}&dim_reference_time={instance_to_iso(instance)}"
                    + (f"&{custom_dim_parameter}" if len(custom_dim_parameter)>0 else "")
                    + (f"&{vertical_dim}" if len(vertical_dim)>0 else "")
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

    return CoverageCollection(coverages=coveragejsons, parameters=parameters)
