"""EDR 2D trajectories sampled in one native ADAGUC data request."""

import math
import re

from fastapi import APIRouter, Query, Request
from geomet import wkt
from typing_extensions import Annotated

from .covjsonresponse import CovJSONResponse
from .trajectory_covjson import SectionCoverage, SectionCoverageCollection, sections_from_response
from .utils.edr_exception import EdrException, native_json_response
from .utils.edr_utils import (
    call_adaguc,
    get_custom,
    get_dataset_from_collection,
    get_instance,
    get_metadata,
    get_parameters,
    get_vertical,
    instance_to_iso,
    native_query,
    propagate_data_headers,
    supports_section,
)

router = APIRouter()


def validate_coords(coords):
    """Reject unsupported WKT forms before invoking the native sampler."""
    try:
        number = r"[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?"
        pair = rf"{number}\s+{number}"
        if coords is None or re.fullmatch(rf"\s*LINESTRING\s*\(\s*{pair}(?:\s*,\s*{pair})+\s*\)\s*", coords, re.IGNORECASE) is None:
            raise ValueError
        geometry = wkt.loads(coords)
        points = geometry["coordinates"]
        if geometry["type"] != "LineString" or len(points) < 2 or len(points) > 100000:
            raise ValueError
        if any(len(point) != 2 or not all(math.isfinite(v) for v in point) for point in points):
            raise ValueError
        if any(not (-180 <= lon <= 180 and -90 <= lat <= 90) for lon, lat in points):
            raise ValueError
    except (TypeError, ValueError, KeyError, IndexError, StopIteration, AttributeError) as exc:
        raise EdrException(400, "coords must be a 2D WKT LINESTRING with at least two finite longitude/latitude coordinates") from exc


@router.get(
    "/collections/{collection_name}/trajectory",
    response_model=SectionCoverage | SectionCoverageCollection,
    response_class=CovJSONResponse,
    response_model_exclude_none=True,
)
@router.get(
    "/collections/{collection_name}/instances/{instance}/trajectory",
    response_model=SectionCoverage | SectionCoverageCollection,
    response_class=CovJSONResponse,
    response_model_exclude_none=True,
)
async def get_trajectory(
    collection_name: str,
    request: Request,
    response: CovJSONResponse,
    coords: str | None = None,
    instance: str | None = None,
    datetime_par: str | None = Query(default=None, alias="datetime"),
    parameter_name: Annotated[str | None, Query(alias="parameter-name", min_length=1)] = None,
    z_par: Annotated[str | None, Query(alias="z", min_length=1)] = None,
    crs: str = "EPSG:4326",
    f: str = "CoverageJSON",
):
    validate_coords(coords)
    if crs != "EPSG:4326":
        raise EdrException(400, "Trajectory currently supports crs=EPSG:4326")
    if f not in ("CoverageJSON", "application/prs.coverage+json"):
        raise EdrException(400, "Trajectory currently supports CoverageJSON")
    if "resolution_x" in request.query_params or "resolution_y" in request.query_params:
        raise EdrException(400, "Trajectory samples native grid cells; resolution_x and resolution_y are not supported")

    metadata = await get_metadata(collection_name, response=response)
    dataset = get_dataset_from_collection(metadata, collection_name)
    instance = get_instance(metadata, collection_name, instance)
    parameters = get_parameters(metadata, collection_name, parameter_name)

    vertical_dims = set()
    for parameter in parameters:
        if not supports_section(metadata[collection_name][parameter]):
            raise EdrException(400, f"Section requires time and a vertical coordinate for {parameter}")
        _, dim = get_vertical(metadata, collection_name, parameter, z_par)
        if dim:
            vertical_dims.add(dim)

    custom_dims = get_custom(request.query_params, ["coords", "datetime", "parameter-name", "z", "f", "crs"])
    query = {
        "dataset": dataset,
        "service": "wms",
        "version": "1.3.0",
        "request": "GetTrajectory",
        "info_format": "application/json",
        "crs": "EPSG:4326",
        "coords": coords,
        "query_layers": ",".join(parameters),
    }
    if datetime_par:
        query["time"] = datetime_par
    if instance:
        query["dim_reference_time"] = instance_to_iso(instance)
    status, body, headers = await call_adaguc(url=native_query(query, custom_dims, sorted(vertical_dims)))
    data = native_json_response(status, body, "Trajectory")
    propagate_data_headers(response, headers)
    return sections_from_response(data, metadata[collection_name], collection_name)
