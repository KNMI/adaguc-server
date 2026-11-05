"""EDR 2D trajectories sampled in one native ADAGUC data request."""

import json

from fastapi import APIRouter, Query, Request
from typing_extensions import Annotated

from .covjsonresponse import CovJSONResponse
from .trajectory_covjson import SectionCoverage, SectionCoverageCollection, sections_from_response
from .utils.edr_exception import EdrException, exc_failed_call
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
    supports_section,
)

router = APIRouter()


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
    points = parse_coordinates(coords, "LineString")
    if len(points) > 100000:
        raise EdrException(400, "Trajectory supports at most 100000 vertices")
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
    if parameter_name is None:
        # Collections may also contain surface or timeless layers.
        parameters = [parameter for parameter in parameters if supports_section(metadata[collection_name][parameter])]
    if not parameters:
        raise EdrException(400, "Section requires a parameter with time and a vertical coordinate")

    vertical_dims = set()
    for parameter in parameters:
        if not supports_section(metadata[collection_name][parameter]):
            raise EdrException(400, f"Section requires time and a vertical coordinate for {parameter}")
        dim = get_vertical_query(metadata, collection_name, parameter, z_par)
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
        "coords": ",".join(str(value) for point in points for value in point),
        "query_layers": ",".join(parameters),
    }
    if datetime_par:
        query["time"] = datetime_par
    if instance:
        query["dim_reference_time"] = instance_to_iso(instance)
    status, body, headers = await call_adaguc(url=encode_native_query(query, [*custom_dims, *sorted(vertical_dims)]))
    if status != 0:
        raise exc_failed_call("Trajectory call failed")
    try:
        data = json.loads(body)
        if not isinstance(data, list):
            raise ValueError
    except (ValueError, TypeError) as exc:
        raise exc_failed_call("Trajectory call returned invalid data") from exc
    propagate_data_headers(response, headers)
    return sections_from_response(data, metadata[collection_name], collection_name)
