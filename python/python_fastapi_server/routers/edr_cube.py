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

from .covjsonresponse import CovJSONResponse, create_coveragejson_response
from .utils.edr_utils import (
    get_custom,
    get_dataset_from_collection,
    get_instance,
    get_parameters,
    get_vertical_query,
    instance_to_iso,
    encode_native_query,
    propagate_data_headers,
    get_metadata,
)


from .netcdf_to_covjson import netcdf_to_covjson
from .utils.native_grid import MAX_POSITIONS, NATIVE_CRS, check_crs, get_native_grid_query
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
    response: Response,
    bbox: str,
    datetime_par: str | None = Query(default=None, alias="datetime"),
    parameter_name: Annotated[str | None, Query(alias="parameter-name", min_length=1)] = None,
    z_par: Annotated[str | None, Query(alias="z", min_length=1)] = None,
    resolution_x: Annotated[float | None, Query(gt=0, allow_inf_nan=False)] = None,
    resolution_y: Annotated[float | None, Query(gt=0, allow_inf_nan=False)] = None,
    positions_x: Annotated[int | None, Query(alias="resolution-x", ge=0, le=MAX_POSITIONS)] = None,
    positions_y: Annotated[int | None, Query(alias="resolution-y", ge=0, le=MAX_POSITIONS)] = None,
    crs: str | None = None,
) -> CovJSONResponse:
    """Returns information in EDR format for a given collection and position"""
    return await get_coll_inst_cube(
        collection_name,
        request,
        response=response,
        bbox=bbox,
        instance=None,
        datetime_par=datetime_par,
        parameter_name_par=parameter_name,
        z_par=z_par,
        resolution_x=resolution_x,
        resolution_y=resolution_y,
        positions_x=positions_x,
        positions_y=positions_y,
        crs=crs,
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
    datetime_par: str | None = Query(default=None, alias="datetime"),
    parameter_name_par: Annotated[str | None, Query(alias="parameter-name", min_length=1)] = None,
    z_par: Annotated[str | None, Query(alias="z", min_length=1)] = None,
    resolution_x: Annotated[float | None, Query(gt=0, allow_inf_nan=False)] = None,
    resolution_y: Annotated[float | None, Query(gt=0, allow_inf_nan=False)] = None,
    positions_x: Annotated[int | None, Query(alias="resolution-x", ge=0, le=MAX_POSITIONS)] = None,
    positions_y: Annotated[int | None, Query(alias="resolution-y", ge=0, le=MAX_POSITIONS)] = None,
    crs: str | None = None,
) -> CovJSONResponse:
    """Returns information in EDR format for a given collection, instance and position.
    With crs=native the data is returned on the grid of the source data, for example a rotated pole grid, instead of in EPSG:4326.

    resolution-x / resolution-y (EDR): the number of positions in x and y. With crs=native every n-th cell of the native
    grid, for at most that many positions, so the values stay cells of the native grid; 0 gives every cell. Otherwise the
    width and height of the EPSG:4326 grid. resolution_x / resolution_y (ADAGUC extension, not EDR): the cell size in the
    units of the crs: degrees for EPSG:4326, native units (like metres or rotated degrees) for crs=native, where it is
    rounded to a whole number of native cells. Without crs=native, x and y must be given together."""
    allowed_params = [
        "bbox",
        "datetime",
        "parameter-name",
        "z",
        "f",
        "crs",
        "resolution_x",
        "resolution_y",
        "resolution-x",
        "resolution-y",
    ]

    check_crs(crs, allow_native=True, query_type="cube")
    # A resolution-x or resolution-y of 0 means the default resolution, the same as leaving it out
    positions_x = positions_x or None
    positions_y = positions_y or None
    if (resolution_x is not None or resolution_y is not None) and (positions_x is not None or positions_y is not None):
        raise exc_failed_call("Use either resolution-x and resolution-y (number of positions) or resolution_x and resolution_y (cell size)")
    # EPSG:4326 output needs both a width and a height, or both a cell size in x and y. With crs=native an axis without
    # them keeps every cell.
    if crs != NATIVE_CRS and (positions_x is None) != (positions_y is None):
        raise exc_failed_call("resolution-x and resolution-y must be given together without crs=native (0 is the same as leaving them out)")
    if crs != NATIVE_CRS and (resolution_x is None) != (resolution_y is None):
        raise exc_failed_call("resolution_x and resolution_y must be given together without crs=native")

    metadata = await get_metadata(collection_name, response=response)

    dataset_name = get_dataset_from_collection(metadata, collection_name)
    instance = get_instance(metadata, collection_name, instance)
    parameter_names = get_parameters(metadata, collection_name, parameter_name_par)

    custom_dims = get_custom(
        request.query_params,
        allowed_params,
    )

    logger.info("callADAGUC by dataset")

    translate_names = get_translate_names(metadata[collection_name])
    translate_dims = get_translate_dims(metadata[collection_name])

    coveragejsons = []
    parameters = {}
    datetime_arg = datetime_par
    if datetime_par is None:
        datetime_arg = "*"
    for parameter_name in parameter_names:
        vertical_dim = get_vertical_query(metadata, collection_name, parameter_name, z_par)

        query = {
            "dataset": dataset_name,
            "service": "wcs",
            "version": "1.1.1",
            "request": "getcoverage",
            "format": "NetCDF4",
            "crs": "EPSG:4326",
            "coverage": parameter_name,
            "bbox": bbox,
            "time": datetime_arg,
        }
        if instance:
            query["dim_reference_time"] = instance_to_iso(instance)
        if crs == NATIVE_CRS:
            gridspec = metadata[collection_name][parameter_name]["layer"].get("gridspec")
            query.update(get_native_grid_query(gridspec, bbox, positions_x, positions_y, resolution_x, resolution_y))
        elif positions_x is not None and positions_y is not None:
            query.update(width=positions_x, height=positions_y)
        elif resolution_x is not None and resolution_y is not None:
            query.update(get_cellsize_query(bbox, resolution_x, resolution_y))
        urlrequest = encode_native_query(query, [*custom_dims, vertical_dim])

        start = time.time()
        status, wcs_response, headers = await call_adaguc(url=urlrequest)
        propagate_data_headers(response, headers)

        logger.info("status: %d [%f]", status, time.time() - start)
        if status != 0:
            continue
        try:
            with Dataset(f"{parameter_name}.nc", memory=wcs_response) as result_dataset:
                coveragejson = netcdf_to_covjson(metadata[collection_name], result_dataset, translate_names, translate_dims)
        except OSError as exc:
            # adaguc-server can refuse a request (like a grid above its size limit) with an error message instead of NetCDF
            raise exc_failed_call(f"cube call failed for parameter {parameter_name}") from exc
        if coveragejson is not None:
            coveragejsons.extend(coveragejson)
            for covjson in coveragejson:
                parameters = parameters | covjson.parameters

    if len(coveragejsons) == 0:
        raise exc_failed_call(f"cube call failed for parameters {','.join(parameter_names)} [{status}]")
    if len(coveragejsons) == 1:
        return create_coveragejson_response(coveragejsons[0], response=response)

    return create_coveragejson_response(CoverageCollection(coverages=coveragejsons, parameters=parameters), response=response)


def get_cellsize_query(bbox: str, resolution_x: float, resolution_y: float) -> dict:
    """
    WCS parameters for a cell size (resolution_x/resolution_y) in the units of the bbox. A cell size larger than the bbox
    gives one cell, as with crs=native; without this adaguc-server rounds the number of cells down to 0 and returns an
    empty grid. An invalid bbox is left for adaguc-server to refuse.
    """
    try:
        x_min, y_min, x_max, y_max = (float(value) for value in bbox.split(","))
    except ValueError:
        return {"resx": resolution_x, "resy": resolution_y}
    width, height = abs(x_max - x_min), abs(y_max - y_min)
    # A bbox without a width or height is also left for adaguc-server
    return {
        "resx": min(resolution_x, width) if width > 0 else resolution_x,
        "resy": min(resolution_y, height) if height > 0 else resolution_y,
    }


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
