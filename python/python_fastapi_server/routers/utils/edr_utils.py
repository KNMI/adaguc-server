"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

from __future__ import annotations
import json
import logging
import os
import re
from datetime import datetime, timezone

from dateutil.relativedelta import relativedelta
from edr_pydantic.collections import Collection, Instance
from edr_pydantic.data_queries import DataQueries, EDRQuery
from edr_pydantic.extent import Custom, Extent, Spatial, Temporal, Vertical
from edr_pydantic.link import EDRQueryLink, Link
from edr_pydantic.observed_property import ObservedProperty
from edr_pydantic.parameter import Parameter
from edr_pydantic.unit import Symbol, Unit
from edr_pydantic.variables import Variables
from fastapi import Request
from fastapi.datastructures import QueryParams

# TODO; this import should be possible!
# from python.lib.adaguc.CGIRunner import HTTP_STATUSCODE_404_NOT_FOUND
HTTP_STATUSCODE_404_NOT_FOUND = 32

from .edr_exception import (
    AdagucErrorCode,
    exc_failed_call,
    exc_unknown_collection,
    exc_incorrect_instance,
    exc_unknown_parameter,
)

from .ogcapi_tools import call_adaguc

logger = logging.getLogger(__name__)

location_list = [
    {"id": "06260", "name": "De Bilt", "coordinates": [5.1797, 52.0989]},
]


SYMBOL_TYPE_URL = "http://www.opengis.net/def/uom/UCUM"


DATETIME_ISO8601_FMT = "%Y-%m-%dT%H:%M:%SZ"


def parse_iso(dts: str) -> datetime:
    """
    Converts a ISO8601 string into a datetime object
    """
    parsed_dt = None
    try:
        parsed_dt = datetime.strptime(dts, DATETIME_ISO8601_FMT).replace(tzinfo=timezone.utc)
    except ValueError as exc:
        logger.error("err: %s %s", dts, exc)
    return parsed_dt


def get_ref_times_for_coll(metadata) -> list[str]:
    """
    Returns available reference times for given collection
    """

    ref_times = set()
    for param in metadata:
        if param in ["baselayer", "overlay"]:
            continue
        if metadata[param]["layer"].get("enable_edr", False) is False:
            continue

        # dims can be `None`
        if (dims := metadata[param].get("dims")) is None:
            continue

        ref_time_values = dims.get("reference_time", {}).get("values", "")
        if not ref_time_values:
            continue

        ref_times.update(ref_time_values.split(","))

    try:
        return [parse_iso(reft).strftime("%Y%m%d%H%M") for reft in sorted(list(ref_times))]
    except Exception:
        error_msg = "Could not parse reference times from metadata"
        logger.exception(error_msg)
        raise exc_failed_call(error_msg)


def get_base_url(req: Request = None) -> str:
    """Returns the base url of this service"""

    base_url_from_request = f"{req.url.scheme}://{req.url.hostname}{(':'+str(req.url.port)) if req.url.port else ''}" if req else None
    base_url = os.getenv("EXTERNALADDRESS", base_url_from_request) or "http://localhost:8080"

    return base_url.strip("/")


OWSLIB_DUMMY_URL = "http://localhost:8000"


def instance_to_iso(instance_dt: str):
    """
    Converts EDR instance time in format %Y%m%d%H%M into iso8601 time string.
    """
    parsed_dt = parse_instance_time(instance_dt)
    if parsed_dt:
        return parsed_dt.strftime(DATETIME_ISO8601_FMT)
    return None


def parse_instance_time(dts: str) -> datetime:
    """
    Converts EDR instance time in format %Y%m%d%H%M into datetime.
    """
    parsed_dt = None
    try:
        parsed_dt = datetime.strptime(dts, "%Y%m%d%H%M").replace(tzinfo=timezone.utc)
    except ValueError as exc:
        logger.error("err: %s %s", dts, exc)
    return parsed_dt


def get_ttl_from_adaguc_headers(headers):
    """Derives an age value from the max-age/age cache headers that ADAGUC executable returns"""
    max_age = None
    age = None
    for hdr in headers:
        hdr_terms = hdr.split(":")
        if hdr_terms[0].lower() == "cache-control":
            for cache_control_terms in hdr_terms[1].split(","):
                terms = cache_control_terms.split("=")
                if terms[0].lower() == "max-age":
                    max_age = int(terms[1])
        elif hdr_terms[0].lower() == "age":
            age = int(hdr_terms[1])
    if max_age and age:
        return max_age - age
    return max_age


def generate_max_age(ttl):
    """
    Return max-age header for ttl value
    """
    if ttl >= 0:
        return f"max-age={ttl}"
    return "max-age=0"


def get_extent_from_md(metadata: dict, parameter: str):

    bbox = [metadata[parameter]["layer"]["latlonbox"]]
    if bbox is None:
        return None, None
    crs = 'GEOGCS["GCS_WGS_1984",DATUM["D_WGS_1984",SPHEROID["WGS_1984",6378137,298.257223563]],PRIMEM["Greenwich",0],UNIT["Degree",0.017453292519943295]]'
    spatial = Spatial(bbox=bbox, crs=crs)

    interval, time_values = get_times_for_collection(metadata)

    customlist: list = get_custom_dims_for_collection(metadata, parameter)

    # Custom can be a list of custom dimensions, like ensembles, thresholds
    custom = None
    if customlist is not None:
        custom = []
        for custom_el in customlist:
            custom.append(Custom(**custom_el))

    vertical = None
    vertical_dim = get_vertical_dim_for_collection(metadata, parameter)
    if vertical_dim is not None:
        vertical = Vertical(**vertical_dim)

    temporal = None
    if interval:
        temporal = Temporal(
            interval=interval,  # [["2022-06-30T09:00:00Z", "2022-07-02T06:00:00Z"]],
            trs='TIMECRS["DateTime",TDATUM["Gregorian Calendar"],CS[TemporalDateTime,1],AXIS["Time (T)",future]',
            values=time_values,  # ["R49/2022-06-30T06:00:00/PT60M"],
        )
    # logger.info("TEMPORAL [%s]: %s, %s", parameter, interval, time_values)

    extent = Extent(spatial=spatial, temporal=temporal, custom=custom, vertical=vertical)

    return extent


def get_collectioninfo_from_md(
    metadata: dict,
    collection_name: str,
    instance: str = None,
) -> Collection:
    """
    Returns collection information for a given collection id and or instance
    Is used to obtain metadata from the dataset configuration and WMS GetCapabilities document.
    """
    # logger.info("get_collectioninfo_from_md(%s, %s)", collection_name, instance)

    dataset_name = collection_name.rsplit(".", 1)[0]
    terms = collection_name.rsplit(".", 1)
    if len(terms) == 1:
        subcollection_name = None
    else:
        subcollection_name = terms[1]

    if metadata is None:
        return None

    if not any(l for l in metadata if metadata[l]["layer"]["enable_edr"] is True):
        return []

    # Identify first parameter with enable_edr==True
    first_param = None
    for param in metadata:
        if param not in ["baselayer", "overlay"] and metadata[param]["layer"].get("enable_edr") is True:
            first_param = param
            break
    if first_param is None or metadata[first_param]["layer"]["variables"] is None:
        return []

    base_url = get_base_url() + f"/edr/collections/{collection_name}"

    if instance is not None:
        base_url += f"/instances/{instance}"

    links: list[Link] = []
    links.append(Link(href=f"{base_url}", rel="collection", type="application/json"))

    has_instances = False
    if not instance:
        ref_times = get_ref_times_for_coll(metadata)
        if ref_times and len(ref_times) > 0:
            has_instances = True
            instances_link = Link(href=f"{base_url}/instances", rel="collection", type="application/json")
            links.append(instances_link)

    primary_extent = get_extent_from_md(metadata, first_param)

    #   crs_details=[crs_object],
    position_variables = Variables(
        query_type="position",
        default_output_format="CoverageJSON",
        output_formats=["CoverageJSON"],
    )
    position_link = EDRQueryLink(
        href=f"{base_url}/position",
        rel="data",
        hreflang="en",
        title="Position query",
        variables=position_variables,
    )
    cube_variables = Variables(
        query_type="position",
        default_output_format="CoverageJSON",
        output_formats=["CoverageJSON"],
    )
    cube_link = EDRQueryLink(
        href=f"{base_url}/cube",
        rel="data",
        hreflang="en",
        title="Cube query",
        variables=cube_variables,
    )
    locations_link = None
    if len(location_list) > 0:
        locations_variables = Variables(
            query_type="locations",
            default_output_format="GeoJSON",
            output_formats=["GeoJSON"],
        )
        locations_link = EDRQueryLink(
            href=f"{base_url}/locations",
            rel="data",
            hreflang="en",
            title="Locations query",
            variables=locations_variables,
        )

    instances_query_link = None
    if has_instances:
        instances_variables = Variables(
            query_type="instances",
            #  crs_details=[crs_object],
            default_output_format="CoverageJSON",
            output_formats=["CoverageJSON", "GeoJSON"],
        )
        instances_query_link = EDRQueryLink(
            href=f"{base_url}/instances",
            rel="collection",
            hreflang="en",
            title="Instances query",
            variables=instances_variables,
        )
        if locations_link is not None:
            data_queries = DataQueries(
                position=EDRQuery(link=position_link),
                cube=EDRQuery(link=cube_link),
                instances=EDRQuery(link=instances_query_link),
                locations=EDRQuery(link=locations_link),
            )
        else:
            data_queries = DataQueries(
                position=EDRQuery(link=position_link),
                cube=EDRQuery(link=cube_link),
                instances=EDRQuery(link=instances_query_link),
            )
    else:
        if locations_link is not None:
            data_queries = DataQueries(
                position=EDRQuery(link=position_link),
                cube=EDRQuery(link=cube_link),
                locations=EDRQuery(link=locations_link),
            )
        else:
            data_queries = DataQueries(
                position=EDRQuery(link=position_link),
                cube=EDRQuery(link=cube_link),
            )

    parameter_info = get_params_for_dataset(metadata, collection_name, primary_extent)
    collections = []

    crs = ["EPSG:4326"]

    output_formats = ["CoverageJSON"]
    if subcollection_name is not None:
        collection_names = [f"{dataset_name}.{subcollection_name}"]
    else:
        collection_names = list(parameter_info.keys())
    for collection_name_ in collection_names:
        if instance is None:
            collection = Collection(
                links=links,
                id=collection_name_,
                extent=primary_extent,
                data_queries=data_queries,
                parameter_names=parameter_info[collection_name_],
                crs=crs,
                output_formats=output_formats,
            )
        else:
            collection = Instance(
                links=links,
                id=f"{instance}",
                extent=primary_extent,
                data_queries=data_queries,
                parameter_names=parameter_info[collection_name_],
                crs=crs,
                output_formats=output_formats,
            )

        collections.append(collection)

    return collections


def parse_period_string(period: str):
    """
    Parse an ISO8601 period string (for example P60M) and return a relativedelta
    """
    if period.startswith("PT"):
        pattern = re.compile(r"""PT(\d+)([HMS])""")
        match = pattern.match(period)
        step = int(match.group(1))
        step_type = match.group(2).lower()
    else:
        pattern = re.compile(r"""P(\d+)([YMD])""")
        match = pattern.match(period)
        step = int(match.group(1))
        step_type = match.group(2)

    if step_type == "Y":
        delta = relativedelta(years=step)
    elif step_type == "M":
        delta = relativedelta(months=step)
    elif step_type == "D":
        delta = relativedelta(days=step)
    elif step_type == "h":
        delta = relativedelta(hours=step)
    elif step_type == "m":
        delta = relativedelta(minutes=step)
    elif step_type == "s":
        delta = relativedelta(seconds=step)
    else:
        return None  # Should never be reached
    return delta


def parse_interval_string(time_interval: str, ref_time: datetime):
    """
    Parse an interval string and return an array of time values

    The interval string has the format "R<repeat>/{reference_time}+1/<period> or
    R<repeat>/2024-06-01T00:00:00Z/<period>
    where repeat is the number of repeats/values,
    {reference_time} is a placeholder that gets replaced with the actual reference_time,
    period is an ISO8601 repeat period (for example PT60M)
    """
    offset_steps = 0
    time_interval_term = time_interval.split("/")[1]
    offset_pattern = re.compile(r"""(\{.*\})((-|\+)(\d+))""")
    match = offset_pattern.match(time_interval_term)
    if match and match.group(2):
        offset_steps = int(match.group(2))
    # "{reference_time}", ref_time.strftime(DATETIME_ISO8601_FMT))
    (repeat_s, _, period) = time_interval.split("/")
    repeat = int(repeat_s[1:])

    delta = parse_period_string(period)

    times = []
    step_time = ref_time + delta * offset_steps
    for _ in range(repeat):
        times.append(step_time.strftime(DATETIME_ISO8601_FMT))
        step_time = step_time + delta
    return times


def create_times_for_instance(edr_collectioninfo: dict, instance: str):
    """
    Returns a list of times for a reference_time, derived from the time_interval EDRCollection attribute in edr_collectioninfo

    """
    ref_time = parse_instance_time(instance)
    time_interval = edr_collectioninfo["time_interval"]

    times = parse_interval_string(time_interval=time_interval, ref_time=ref_time)

    interval = [
        [
            datetime.strptime(times[0], DATETIME_ISO8601_FMT).replace(tzinfo=timezone.utc),
            datetime.strptime(times[-1], DATETIME_ISO8601_FMT).replace(tzinfo=timezone.utc),
        ]
    ]
    logger.info(
        "create_times_for_instance %s, %s, %s, %s, %s",
        ref_time,
        edr_collectioninfo["time_interval"],
        time_interval,
        interval,
        times,
    )
    return interval, times


def get_params_for_dataset(metadata: dict, dataset_name: str, primary_extent: Extent) -> dict[str, Parameter]:
    """
    Returns a dictionary with parameters for given EDR collection
    """
    parameter_names = {}
    for param_id in metadata:
        if metadata[param_id]["dims"] is None:
            continue
        if metadata[param_id]["layer"]["enable_edr"] is False:
            continue
        current_extent = get_extent_from_md(metadata, param_id)
        param_metadata = get_param_metadata(metadata[param_id], dataset_name)
        observed_property_id = param_metadata.get("observed_property_id", param_id)
        param = Parameter(
            id=param_id,
            observedProperty=ObservedProperty(
                id=observed_property_id,
                label=param_metadata["observed_property_label"],
            ),
            description=param_metadata["description"],
            type="Parameter",
            unit=Unit(symbol=Symbol(value=param_metadata["parameter_unit"], type=SYMBOL_TYPE_URL)),
            label=param_metadata["parameter_label"],
            extent=current_extent if current_extent != primary_extent else None,
        )
        if param_metadata["collection"] not in parameter_names:
            parameter_names[param_metadata["collection"]] = {}
        parameter_names[param_metadata["collection"]][param_id] = param
    return parameter_names


def handle_metadata(metadata: dict):
    collections = {}
    for dataset in metadata:
        # Check if there is something configured for this dataset, otherwise continue.
        if not metadata[dataset]:
            continue
        for layername, layerdata in metadata[dataset].items():
            if layerdata:
                if "collection" in layerdata["layer"] and len(layerdata["layer"]["collection"]) > 0:
                    collection_name = dataset + "." + layerdata["layer"]["collection"]
                elif "." in dataset:
                    # Note, no collections were configured. There is a "." in the datasetname, so we cannot make an EDR dataset out of it.
                    collection_name = None
                else:
                    collection_name = dataset
                # Add the collection to the dictionary
                if collection_name:
                    if not collection_name in collections:
                        collections[collection_name] = {}
                    collections[collection_name][layername] = layerdata
    return collections


async def get_metadata(collection_name: str = "", instance: str = "") -> dict:
    """Get metadata from ADAGUC.

    This method will either return a dictionary representing the metadata, or throw an exception
    """

    urlrequest = "service=wms&version=1.3.0&request=getmetadata&format=application/json"
    if collection_name:
        dataset_name = collection_name.rsplit(".", 1)[0]
        urlrequest = f"{urlrequest}&dataset={dataset_name}"
    if instance:
        reference_time = instance_to_iso(instance)
        urlrequest = f"{urlrequest}&dim_reference_time={reference_time}"

    status, response, _ = await call_adaguc(url=urlrequest.encode("UTF-8"))
    logger.info("status for %s: %d", urlrequest, status)

    raw_response = response.getvalue().decode("UTF-8")
    try:
        parsed_json = json.loads(raw_response)
    except json.JSONDecodeError:
        raise exc_failed_call("Failed to query metadata: couldn't parse json response")

    # If metadata call failed, try to decipher response
    if status != 0:
        # Try to parse error code into AdagucErrorCode enum
        error_msg = parsed_json.get("error")
        try:
            adaguc_error_code = AdagucErrorCode(error_msg)
        except ValueError:
            raise exc_failed_call(f"Unknown error code: {error_msg}")

        # Handle specific exceptions for wrong instance/dataset.
        if adaguc_error_code == AdagucErrorCode.InvalidDimensionValue:
            raise exc_incorrect_instance(collection_name, instance)
        if adaguc_error_code == AdagucErrorCode.InvalidDataset:
            raise exc_unknown_collection(collection_name)

        raise exc_failed_call(f"Failed to query metadata: {error_msg}")

    collection_metadata = handle_metadata(parsed_json)
    if collection_metadata is None:
        raise exc_unknown_collection(collection_name)

    # Return all metadata if no collection_name is specified
    if not collection_name:
        return collection_metadata

    coll = collection_metadata.get(collection_name, None)
    if coll is None:
        raise exc_unknown_collection(collection_name)

    return {collection_name: coll}


def get_vertical_dim_for_collection(metadata: dict, parameter: str = None):
    """
    Return the vertical dimension the WMS GetCapabilities document.
    """
    if parameter and parameter in list(metadata):
        layer = metadata[parameter]
    else:
        layer = metadata[list(metadata)[0]]

    if "dims" in layer and layer["dims"] is not None:
        for dim_name in layer["dims"]:
            if not layer["dims"][dim_name]["hidden"] and (
                dim_name in ["elevation"] or layer["dims"][dim_name]["type"] == "dimtype_vertical"
            ):
                values = layer["dims"][dim_name]["values"].split(",")
                vertical_dim = {
                    "interval": [[values[0], values[-1]]],
                    "values": values,
                    "vrs": "customvrs",
                }
                return vertical_dim
    return None


def try_numeric_conversion(values: list[str | float]):
    try:
        new_values = [float(v) for v in values]
        return new_values
    except ValueError:
        return values


def get_custom_dims_for_collection(metadata: dict, parameter: str = None):
    """
    Return the dimensions other then elevation or time from the WMS GetCapabilities document.
    """
    custom = []
    if parameter and parameter in list(metadata):
        layer = metadata[parameter]
    else:
        # default to first layer
        layer = metadata[list(metadata)[0]]

    if "dims" in layer and layer["dims"] is not None:
        for dim_name in layer["dims"]:
            if not layer["dims"][dim_name]["hidden"] and not layer["dims"][dim_name]["type"] == "dimtype_vertical":
                # Not needed for non custom dims:
                if dim_name not in [
                    "reference_time",
                    "time",
                    "elevation",
                ]:
                    dim_values = layer["dims"][dim_name]["values"].split(",")
                    custom_dim = {
                        "id": dim_name,
                        "interval": [
                            [
                                dim_values[0],
                                dim_values[-1],
                            ]
                        ],
                        "values": dim_values,  ##["R51/0/1"],  # dim_values,
                        "reference": "https://www.ecmwf.int/sites/default/files/elibrary/2012/14557-ecmwf-ensemble-prediction-system.pdf",  # f"custom_{dim_name}",
                    }
                    custom.append(custom_dim)
        return custom if len(custom) > 0 else None
    return None


def get_times_for_collection(metadata: dict) -> tuple[list[list[datetime]], list[str]]:
    """
    Return the first timestep, the last timestep, and an ISO 8601 range timestamp. Use data from the metadata dict.

    For every parameter in the metadata dict, find the earliest timestamp. Use that timestamp.
    """

    min_start = None
    min_item = None
    list_values = None

    for param in metadata:
        if param in ["baselayer", "overlay"]:
            continue
        if metadata[param]["layer"].get("enable_edr", False) is False:
            continue

        # dims can be `None`
        if (dims := metadata[param].get("dims")) is None:
            continue

        time_values = dims.get("time", {}).get("values", "")
        if not time_values:
            continue

        if "/" in time_values:
            # Slash separated
            start, end, period = time_values.split("/", 2)

            if min_start is None or start < min_start:
                min_start = start
                min_item = (start, end, period)
        else:
            # Comma separated
            list_values = time_values

    # Get time values for the lowest time step
    if min_item:
        start, end, period = min_item
        range_values = get_time_values_for_range(start, end, period)
    elif list_values:
        terms = list_values.split(",")
        start, end = terms[0], terms[-1]
        range_values = terms
    else:
        return [], []

    try:
        interval = [
            [
                datetime.strptime(start, DATETIME_ISO8601_FMT).replace(tzinfo=timezone.utc),
                datetime.strptime(end, DATETIME_ISO8601_FMT).replace(tzinfo=timezone.utc),
            ]
        ]
        return interval, range_values
    except Exception:
        error_msg = "Could not parse time values from metadata"
        logger.exception(error_msg)
        raise exc_failed_call(error_msg)


def get_time_values_for_range(start: str, end: str, period: str) -> list[str]:
    """
    Converts a start/stop/res string into a ISO8601 Range object

    For example:
        "2023-01-01T00:00:00Z/2023-01-01T12:00:00Z/PT1H" into ["R13/2023-01-01T00:00:00Z/PT1H"]
    """

    iso_start = parse_iso(start)
    iso_end = parse_iso(end)
    step = period

    timediff = (iso_end - iso_start).total_seconds()
    tstep = None
    regexpmatch = re.match("PT(\\d+)M", step)
    if regexpmatch:
        tstep = int(regexpmatch.group(1)) * 60
    regexpmatch = re.match("PT(\\d+)H", step)
    if regexpmatch:
        tstep = int(regexpmatch.group(1)) * 3600
    regexpmatch = re.match("P(\\d+)D", step)
    if regexpmatch:
        tstep = int(regexpmatch.group(1)) * 3600 * 24
    if not tstep:
        tstep = 3600
    nsteps = int(timediff / tstep) + 1
    return [f"R{nsteps}/{iso_start.strftime(DATETIME_ISO8601_FMT)}/{step}"]


VOCAB_ENDPOINT_URL = "https://vocab.nerc.ac.uk/standard_name/"


def get_param_metadata(param_metadata: dict, dataset_name: str) -> dict:
    """Composes parameter metadata based on the param_el and the wmslayer dictionaries

    Args:
        param_metadata (dict): The parameter / wms layer name to find
        dataset_name (str): The name of the dataset

    Returns:
        dict: dictionary with all metadata required to construct a Edr Parameter object.
    """
    wms_layer_name = param_metadata["layer"]["layername"]
    observed_property_id = wms_layer_name
    if "standard_name" in param_metadata["layer"]["variables"][0]:
        observed_property_id = VOCAB_ENDPOINT_URL + param_metadata["layer"]["variables"][0]["standard_name"]

    parameter_label = param_metadata["layer"]["title"]
    parameter_unit = param_metadata["layer"]["variables"][0]["units"]
    if len(parameter_unit) == 0:
        parameter_unit = "-^-"

    observed_property_label = param_metadata["layer"]["variables"][0]["label"]

    abstract = param_metadata["layer"]["abstract"]
    if abstract == "":
        abstract = param_metadata["layer"]["title"]

    return {
        "description": abstract,
        "observed_property_id": observed_property_id,
        "observed_property_label": observed_property_label,
        "parameter_label": parameter_label,
        "parameter_unit": parameter_unit,
        "collection": dataset_name,
    }


def get_dataset_from_collection(metadata: dict, collection_name: str) -> str:
    if metadata is None:
        raise exc_unknown_collection(collection_name)

    dataset_name = collection_name.rsplit(".", 1)[0]
    return dataset_name


def get_instance(metadata: dict, collection_name: str, instance: str | None = None) -> str:
    """Find the correct EDR instance from a given metadata"""

    ref_times = get_ref_times_for_coll(metadata[collection_name])
    if len(ref_times) == 0:
        if instance is not None:
            raise exc_incorrect_instance(collection_name, instance)
    elif not instance:
        instance = ref_times[-1]
    elif instance not in ref_times:
        raise exc_incorrect_instance(collection_name, instance)

    return instance


def get_parameters(metadata: dict, collection_name: str, parameter_name_par: str | None) -> list[str]:
    if parameter_name_par is None:
        return list(metadata[collection_name].keys())

    requested_parameters = parameter_name_par.split(",")
    parameters = []
    for param in requested_parameters:
        if param in metadata[collection_name]:
            parameters.append(param)

    if len(parameters) == 0:
        raise exc_unknown_parameter(collection_name, parameter_name_par)
    return parameters


def get_vertical(metadata: dict, collection_name: str, requested_param: str, z_par: str | None) -> tuple[str, str]:
    vertical_name = None
    vertical_dim = ""
    for param_dim in metadata[collection_name][requested_param]["dims"].values():
        if not param_dim["hidden"]:
            if param_dim["type"] == "dimtype_vertical":
                vertical_name = param_dim["serviceName"]
                break

    if z_par:
        if vertical_name is not None:
            vertical_dim = f"DIM_{vertical_name}={z_par}"
    elif vertical_name is not None:
        vertical_dim = f"DIM_{vertical_name}=*"
    return (vertical_name, vertical_dim)


def get_custom(
    query_params: QueryParams,
    allowed_params: list[str],
) -> list[str]:

    custom_name = None
    custom_dims = [k for k in query_params if k not in allowed_params]
    logger.info("custom dims: %s %s", custom_dims, custom_name)
    custom_dim_parameters = []
    for custom_dim in custom_dims:
        custom_dim_parameters.append(f"DIM_{custom_dim}={query_params[custom_dim]}")

    return custom_dim_parameters
