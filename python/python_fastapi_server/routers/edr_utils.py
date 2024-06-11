"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

import json
import logging
import os
import re
from datetime import datetime, timezone

from cachetools import TTLCache, cached
from dateutil.relativedelta import relativedelta
from defusedxml.ElementTree import ParseError, parse
from edr_pydantic.collections import Collection, Instance
from edr_pydantic.data_queries import DataQueries, EDRQuery
from edr_pydantic.extent import Custom, Extent, Spatial, Temporal, Vertical
from edr_pydantic.link import EDRQueryLink, Link
from edr_pydantic.observed_property import ObservedProperty
from edr_pydantic.parameter import Parameter
from edr_pydantic.unit import Symbol, Unit
from edr_pydantic.variables import Variables
from fastapi import Request
from owslib.wms import WebMapService

from .edr_exception import EdrException
from .ogcapi_tools import call_adaguc

logger = logging.getLogger(__name__)


def parse_config_file(
    dataset_filename: str, adaguc_dataset_dir: str = os.environ["ADAGUC_DATASET_DIR"]
):
    """Return translation names for NetCDF data"""
    translate = {}
    dim_translate = {}
    filename = dataset_filename
    _, ext = os.path.splitext(filename)
    if ext == "" or ext != ".xml":
        filename = filename + ".xml"

    if os.path.isfile(os.path.join(adaguc_dataset_dir, filename)) and filename.endswith(
        ".xml"
    ):
        tree = parse(os.path.join(adaguc_dataset_dir, filename))
        root = tree.getroot()
        for lyr in root.iter("Layer"):
            layer_variables = [l.text for l in lyr.iter("Variable")]
            if len(layer_variables) == 1:
                translate[layer_variables[0]] = (
                    lyr.find("Name") or lyr.find("Variable")
                ).text
        for coll in root.iter("EdrCollection"):
            if "vertical_name" in coll.attrib:
                vertical_name = coll.attrib["vertical_name"]
                for lyr in root.iter("Layer"):
                    for dimension in lyr.iter("Dimension"):
                        if dimension.text == vertical_name:
                            dim_translate[dimension.attrib["name"]] = "z"
    return translate, dim_translate


def parse_iso(dts: str) -> datetime:
    """
    Converts a ISO8601 string into a datetime object
    """
    parsed_dt = None
    try:
        parsed_dt = datetime.strptime(dts, "%Y-%m-%dT%H:%M:%SZ").replace(
            tzinfo=timezone.utc
        )
    except ValueError as exc:
        logger.error("err: %s %s", dts, exc)
    return parsed_dt


async def get_ref_times_for_coll(edr_collectioninfo: dict, layer: str) -> list[str]:
    """
    Returns available reference times for given collection
    """
    dataset = edr_collectioninfo["dataset"]
    url = f"?DATASET={dataset}&SERVICE=WMS&VERSION=1.3.0&request=getreferencetimes&LAYER={layer}"
    # logger.info("getreftime_url(%s,%s): %s", dataset, layer, url)
    status, response, _ = await call_adaguc(url=url.encode("UTF-8"))
    if status == 0:
        ref_times = json.loads(response.getvalue())
        instance_ids = [parse_iso(reft).strftime("%Y%m%d%H%M") for reft in ref_times]
        return instance_ids
    return []


def get_base_url(req: Request = None) -> str:
    """Returns the base url of this service"""

    base_url_from_request = (
        f"{req.url.scheme}://{req.url.hostname}{(':'+str(req.url.port)) if req.url.port else ''}"
        if req
        else None
    )
    base_url = (
        os.getenv("EXTERNALADDRESS", base_url_from_request) or "http://localhost:8080"
    )

    return base_url.strip("/")


OWSLIB_DUMMY_URL = "http://localhost:8000"


def init_edr_collections(adaguc_dataset_dir: str = os.environ["ADAGUC_DATASET_DIR"]):
    """
    Return all possible OGCAPI EDR datasets, based on the dataset directory
    """
    dataset_files = sorted(
        [
            f
            for f in os.listdir(adaguc_dataset_dir)
            if os.path.isfile(os.path.join(adaguc_dataset_dir, f))
            and f.endswith(".xml")
        ]
    )

    edr_collections = {}
    for dataset_file in dataset_files:
        dataset = dataset_file.replace(".xml", "")
        try:
            tree = parse(os.path.join(adaguc_dataset_dir, dataset_file))
            root = tree.getroot()
            for ogcapi_edr in root.iter("OgcApiEdr"):
                for edr_collection in ogcapi_edr.iter("EdrCollection"):
                    edr_params = []
                    for edr_parameter in edr_collection.iter("EdrParameter"):
                        if (
                            "name" in edr_parameter.attrib
                            and "unit" in edr_parameter.attrib
                        ):
                            name = edr_parameter.attrib.get("name")
                            edr_param = {
                                "name": name,
                                "parameter_label": name,
                                "observed_property_label": name,
                                "description": name,
                                "standard_name": None,
                                "unit": "-",
                            }
                            # Try to take the standard name from the configuration
                            if "standard_name" in edr_parameter.attrib:
                                edr_param["standard_name"] = edr_parameter.attrib.get(
                                    "standard_name"
                                )
                                # If there is a standard name, set the label to the same as fallback
                                edr_param["observed_property_label"] = (
                                    edr_parameter.attrib.get("standard_name")
                                )

                            # Try to take the observed_property_label from the configuration
                            if "observed_property_label" in edr_parameter.attrib:
                                edr_param["observed_property_label"] = (
                                    edr_parameter.attrib.get("observed_property_label")
                                )
                            # Try to take the parameter_label from the Layer configuration Title
                            if "parameter_label" in edr_parameter.attrib:
                                edr_param["parameter_label"] = edr_parameter.attrib.get(
                                    "parameter_label"
                                )
                            # Try to take the parameter_label from the Layer configuration Title
                            if "unit" in edr_parameter.attrib:
                                edr_param["unit"] = edr_parameter.attrib.get("unit")

                            edr_params.append(edr_param)

                        else:
                            logger.warning(
                                "In dataset %s, skipping parameter %s: has no name or units configured",
                                dataset_file,
                                edr_parameter,
                            )

                    if len(edr_params) == 0:
                        logger.warning(
                            "Skipping dataset %s: has no EdrParameter configured",
                            dataset_file,
                        )
                    else:
                        collection_url = (
                            get_base_url()
                            + f"/edr/collections/{edr_collection.attrib.get('name')}"
                        )
                        edr_collections[edr_collection.attrib.get("name")] = {
                            "dataset": dataset,
                            "name": edr_collection.attrib.get("name"),
                            "service": f"{OWSLIB_DUMMY_URL}/wms",
                            "parameters": edr_params,
                            "vertical_name": edr_collection.attrib.get("vertical_name"),
                            "custom_name": edr_collection.attrib.get("custom_name"),
                            "base_url": collection_url,
                            "time_interval": edr_collection.attrib.get("time_interval"),
                        }
        except ParseError:
            pass
    return edr_collections


# The edr_collections information is cached locally for a maximum of 2 minutes
# It will be refreshed if older than 2 minutes
edr_cache = TTLCache(maxsize=100, ttl=120)  # TODO: Redis?


@cached(cache=edr_cache)
def get_edr_collections():
    """Returns all EDR collections"""
    return init_edr_collections()


def instance_to_iso(instance_dt: str):
    """
    Converts EDR instance time in format %Y%m%d%H%M into iso8601 time string.
    """
    parsed_dt = parse_instance_time(instance_dt)
    if parsed_dt:
        return parsed_dt.strftime("%Y-%m-%dT%H:%M:%SZ")
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
    elif max_age:
        return max_age
    return None


def generate_max_age(ttl):
    """
    Return max-age header for ttl value
    """
    if ttl >= 0:
        return f"max-age={ttl}"
    return "max-age=0"


async def get_collectioninfo_for_id(
    edr_collection: str,
    instance: str = None,
) -> tuple[Collection, datetime]:
    """
    Returns collection information for a given collection id and or instance
    Is used to obtain metadata from the dataset configuration and WMS GetCapabilities document.
    """
    logger.info("get_collectioninfo_for_id(%s, %s)", edr_collection, instance)
    edr_collectionsinfo = get_edr_collections()
    if edr_collection not in edr_collectionsinfo:
        raise EdrException(code=400, description="Unknown or unconfigured collection")

    edr_collectioninfo = edr_collectionsinfo[edr_collection]

    base_url = edr_collectioninfo["base_url"]

    if instance is not None:
        base_url += f"/instances/{instance}"

    links: list[Link] = []
    links.append(Link(href=f"{base_url}", rel="collection", type="application/json"))

    ref_times = None

    if not instance:
        ref_times = await get_ref_times_for_coll(
            edr_collectioninfo, edr_collectioninfo["parameters"][0]["name"]
        )
        if ref_times and len(ref_times) > 0:
            instances_link = Link(
                href=f"{base_url}/instances", rel="collection", type="application/json"
            )
            links.append(instances_link)

    wmslayers, ttl = await get_capabilities(edr_collectioninfo["name"])

    bbox = get_extent(edr_collectioninfo, wmslayers)
    if bbox is None:
        return None, None
    crs = 'GEOGCS["GCS_WGS_1984",DATUM["D_WGS_1984",SPHEROID["WGS_1984",6378137,298.257223563]],PRIMEM["Greenwich",0],UNIT["Degree",0.017453292519943295]]'
    spatial = Spatial(bbox=bbox, crs=crs)

    if instance is None or edr_collectioninfo["time_interval"] is None:
        (interval, time_values) = get_times_for_collection(
            wmslayers, edr_collectioninfo["parameters"][0]["name"]
        )
    else:
        (interval, time_values) = create_times_for_instance(
            edr_collectioninfo, instance
        )

    customlist: list = get_custom_dims_for_collection(
        edr_collectioninfo, wmslayers, edr_collectioninfo["parameters"][0]["name"]
    )

    # Custom can be a list of custom dimensions, like ensembles, thresholds
    custom = []
    if customlist is not None:
        for custom_el in customlist:
            custom.append(Custom(**custom_el))

    vertical = None
    vertical_dim = get_vertical_dim_for_collection(
        edr_collectioninfo, wmslayers, edr_collectioninfo["parameters"][0]["name"]
    )
    if vertical_dim:
        vertical = Vertical(**vertical_dim)

    temporal = Temporal(
        interval=interval,  # [["2022-06-30T09:00:00Z", "2022-07-02T06:00:00Z"]],
        trs='TIMECRS["DateTime",TDATUM["Gregorian Calendar"],CS[TemporalDateTime,1],AXIS["Time (T)",future]',
        values=time_values,  # ["R49/2022-06-30T06:00:00/PT60M"],
    )

    extent = Extent(
        spatial=spatial, temporal=temporal, custom=custom, vertical=vertical
    )

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

    instances_link = None
    if ref_times and len(ref_times) > 0:
        instances_variables = Variables(
            query_type="instances",
            #  crs_details=[crs_object],
            default_output_format="CoverageJSON",
            output_formats=["CoverageJSON", "GeoJSON"],
        )
        instances_link = EDRQueryLink(
            href=f"{base_url}/instances",
            rel="collection",
            hreflang="en",
            title="Instances query",
            variables=instances_variables,
        )
        data_queries = DataQueries(
            position=EDRQuery(link=position_link),
            cube=EDRQuery(link=cube_link),
            instances=EDRQuery(link=instances_link),
        )
    else:
        data_queries = DataQueries(
            position=EDRQuery(link=position_link), cube=EDRQuery(link=cube_link)
        )

    parameter_names = get_params_for_collection(
        edr_collection=edr_collection, wmslayers=wmslayers
    )

    crs = ["EPSG:4326"]

    output_formats = ["CoverageJSON"]
    if instance is None:
        collection = Collection(
            links=links,
            id=edr_collection,
            extent=extent,
            data_queries=data_queries,
            parameter_names=parameter_names,
            crs=crs,
            output_formats=output_formats,
        )
    else:
        collection = Instance(
            links=links,
            id=instance,
            extent=extent,
            data_queries=data_queries,
            parameter_names=parameter_names,
            crs=crs,
            output_formats=output_formats,
        )

    return collection, ttl


def create_times_for_instance(edr_collectioninfo: dict, instance: str):
    """
    Returns a list of times for a reference_time, derived from the time_interval EDRCollection attribute in edr_collectioninfo

    """
    ref_time = parse_instance_time(instance)
    time_interval = edr_collectioninfo["time_interval"].replace(
        "{reference_time}", ref_time.strftime("%Y-%m-%dT%H:%M:%SZ")
    )
    (repeat_s, _, time_step_s) = time_interval.split("/")
    repeat = int(repeat_s[1:])

    if time_step_s.startswith("PT"):
        pattern = re.compile(r"""PT(\d+)([HMS])""")
        match = pattern.match(time_step_s)
        step = int(match.group(1))
        step_type = match.group(2).lower()
    else:
        pattern = re.compile(r"""P(\d+)([YMD])""")
        match = pattern.match(time_step_s)
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

    times = []
    step_time = ref_time
    for _ in range(repeat):
        times.append(step_time.strftime("%Y-%m-%dT%H:%M:%SZ"))
        step_time = step_time + delta

    interval = [
        [
            datetime.strptime(times[0], "%Y-%m-%dT%H:%M:%SZ").replace(
                tzinfo=timezone.utc
            ),
            datetime.strptime(times[-1], "%Y-%m-%dT%H:%M:%SZ").replace(
                tzinfo=timezone.utc
            ),
        ]
    ]
    return interval, times


SYMBOL_TYPE_URL = "http://www.opengis.net/def/uom/UCUM"


def get_params_for_collection(
    edr_collection: str, wmslayers: dict
) -> dict[str, Parameter]:
    """
    Returns a dictionary with parameters for given EDR collection
    """
    parameter_names = {}
    edr_collections = get_edr_collections()
    for param_el in edr_collections[edr_collection]["parameters"]:
        param_id = param_el["name"]
        if not param_id in wmslayers:
            logger.warning(
                "EDR Parameter with name [%s] is not found in any of the adaguc Layer configurations. Available layers are %s",
                param_id,
                str(list(wmslayers.keys())),
            )
        else:
            param_metadata = get_param_metadata(param_id, edr_collection)
            logger.info(param_id)
            logger.info(param_metadata["wms_layer_name"])
            param = Parameter(
                id=param_metadata["wms_layer_name"],
                observedProperty=ObservedProperty(
                    id=param_metadata["observed_property_id"],
                    label=param_metadata["observed_property_label"],
                ),
                # description=param_metadata["wms_layer_title"], # TODO in follow up
                type="Parameter",
                unit=Unit(
                    symbol=Symbol(
                        value=param_metadata["parameter_unit"], type=SYMBOL_TYPE_URL
                    )
                ),
                label=param_metadata["parameter_label"],
            )
            parameter_names[param_el["name"]] = param
    return parameter_names


async def get_capabilities(collname):
    """
    Get the collectioninfo from the WMS GetCapabilities
    """
    collection_info = get_edr_collections().get(collname)
    if "dataset" in collection_info:
        logger.info("callADAGUC by dataset")
        dataset = collection_info["dataset"]
        urlrequest = (
            f"dataset={dataset}&service=wms&version=1.3.0&request=getcapabilities"
        )
        status, response, headers = await call_adaguc(url=urlrequest.encode("UTF-8"))
        ttl = get_ttl_from_adaguc_headers(headers)
        logger.info("status: %d", status)
        if status == 0:
            xml = response.getvalue()
            wms = WebMapService(collection_info["service"], xml=xml, version="1.3.0")
        else:
            logger.error("status: %d", status)
            return {}
    else:
        logger.info("callADAGUC by service %s", dataset)
        wms = WebMapService(dataset["service"], version="1.3.0")
        ttl = None

    layers = {}
    for layername, layerinfo in wms.contents.items():
        layers[layername] = {
            "name": layername,
            "title": layerinfo.title,
            "dimensions": {**layerinfo.dimensions},
            "boundingBoxWGS84": layerinfo.boundingBoxWGS84,
        }
    return layers, ttl


def get_vertical_dim_for_collection(
    edr_collectioninfo: dict, wmslayers, parameter: str = None
):
    """
    Return the verticel dimension the WMS GetCapabilities document.
    """
    if parameter and parameter in list(wmslayers):
        layer = wmslayers[parameter]
    else:
        layer = wmslayers[list(wmslayers)[0]]

    for dim_name in layer["dimensions"]:
        if dim_name in ["elevation"] or (
            "vertical_name" in edr_collectioninfo
            and dim_name == edr_collectioninfo["vertical_name"]
        ):
            vertical_dim = {
                "interval": [],
                "values": layer["dimensions"][dim_name]["values"],
                "vrs": "customvrs",
            }
            return vertical_dim
    return None


def get_custom_dims_for_collection(
    edr_collectioninfo: dict, wmslayers, parameter: str = None
):
    """
    Return the dimensions other then elevation or time from the WMS GetCapabilities document.
    """
    custom = []
    if parameter and parameter in list(wmslayers):
        layer = wmslayers[parameter]
    else:
        # default to first layer
        layer = wmslayers[list(wmslayers)[0]]
    for dim_name in layer["dimensions"]:
        # Not needed for non custom dims:
        if dim_name not in [
            "reference_time",
            "time",
            "elevation",
            edr_collectioninfo.get("vertical_name"),
        ]:
            custom_dim = {
                "id": dim_name,
                "interval": [
                    [
                        layer["dimensions"][dim_name]["values"][0],
                        layer["dimensions"][dim_name]["values"][-1],
                    ]
                ],
                "values": layer["dimensions"][dim_name]["values"],
                "reference": f"custom_{dim_name}",
            }
            if layer["dimensions"][dim_name]["values"] == ["1", "2", "3", "4", "5"]:
                custom_dim["values"] = ["R5/1/1"]
            # if dim_name == "member":
            #     custom_dim["id"] = "number"
            custom.append(custom_dim)
    return custom if len(custom) > 0 else None


def get_extent(edr_collectioninfo: dict, wmslayers):
    """
    Get the boundingbox extent from the WMS GetCapabilities
    """
    first_layer = edr_collectioninfo["parameters"][0]["name"]
    if len(wmslayers):
        if first_layer in wmslayers:
            bbox = wmslayers[first_layer]["boundingBoxWGS84"]
        else:
            # Fallback to first layer in getcapabilities
            bbox = wmslayers[next(iter(wmslayers))]["boundingBoxWGS84"]

        return [[bbox[0], bbox[1]], [bbox[2], bbox[3]]]
    return None


def get_times_for_collection(
    wmslayers, parameter: str = None
) -> tuple[list[list[str]], list[str]]:
    """
    Returns a list of times based on the time dimensions, it does a WMS GetCapabilities to the given dataset (cached)

    It does this for given parameter. When the parameter is not given it will do it for the first Layer in the GetCapabilities document.
    """
    # logger.info("get_times_for_dataset(%s,%s)", edr_collectioninfo["name"], parameter)
    if parameter and parameter in wmslayers:
        layer = wmslayers[parameter]
    else:
        layer = wmslayers[list(wmslayers)[0]]

    if "time" in layer["dimensions"]:
        time_dim = layer["dimensions"]["time"]
        if "/" in time_dim["values"][0]:
            terms = time_dim["values"][0].split("/")
            interval = [
                [
                    datetime.strptime(terms[0], "%Y-%m-%dT%H:%M:%SZ").replace(
                        tzinfo=timezone.utc
                    ),
                    datetime.strptime(terms[1], "%Y-%m-%dT%H:%M:%SZ").replace(
                        tzinfo=timezone.utc
                    ),
                ]
            ]
            return interval, get_time_values_for_range(time_dim["values"][0])
        interval = [
            [
                datetime.strptime(time_dim["values"][0], "%Y-%m-%dT%H:%M:%SZ").replace(
                    tzinfo=timezone.utc
                ),
                datetime.strptime(time_dim["values"][-1], "%Y-%m-%dT%H:%M:%SZ").replace(
                    tzinfo=timezone.utc
                ),
            ]
        ]
        return interval, time_dim["values"]
    return None, None


def get_time_values_for_range(rng: str) -> list[str]:
    """
    Converts a start/stop/res string into a ISO8601 Range object

    For example:
        "2023-01-01T00:00:00Z/2023-01-01T12:00:00Z/PT1H" into ["R13/2023-01-01T00:00:00Z/PT1H"]
    """
    els = rng.split("/")
    iso_start = parse_iso(els[0])
    iso_end = parse_iso(els[1])
    step = els[2]
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
    return [f"R{nsteps}/{iso_start.strftime('%Y-%m-%dT%H:%M:%SZ')}/{step}"]


VOCAB_ENDPOINT_URL = "https://vocab.nerc.ac.uk/standard_name/"


def get_parameter_config(edr_collection_name: str, param_id: str):
    """Gets the EDRParameter configuration based on the edr collection name and parameter id

    Args:
        edr_collection_name (str): edr collection name
        param_id (str): parameter id

    Returns:
        _type_: parameter element_
    """

    edr_collections = get_edr_collections()
    edr_collection_parameters = edr_collections[edr_collection_name]["parameters"]
    for param_el in edr_collection_parameters:
        if param_id == param_el["name"]:
            return param_el
    return None


def get_param_metadata(param_id: str, edr_collection_name) -> dict:
    """Composes parameter metadata based on the param_el and the wmslayer dictionaries

    Args:
        param_id (str): The parameter / wms layer name to find
        edr_collection_name (str): The collection name

    Returns:
        dict: dictionary with all metadata required to construct a Edr Parameter object.
    """
    param_el = get_parameter_config(edr_collection_name, param_id)
    wms_layer_name = param_el["name"]
    observed_property_id = wms_layer_name
    parameter_label = param_el["parameter_label"]
    parameter_unit = param_el["unit"]
    observed_property_label = param_el["observed_property_label"]
    if "standard_name" in param_el and param_el["standard_name"] is not None:
        observed_property_id = VOCAB_ENDPOINT_URL + param_el["standard_name"]

    return {
        "wms_layer_name": wms_layer_name,
        "observed_property_id": observed_property_id,
        "observed_property_label": observed_property_label,
        "parameter_label": parameter_label,
        "parameter_unit": parameter_unit,
    }
