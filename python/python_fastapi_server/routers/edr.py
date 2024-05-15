"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

import itertools
import functools
import operator
import json
import logging
import os
import re
from datetime import datetime, timezone
from dateutil.relativedelta import relativedelta
from typing import Union
import pickle

from covjson_pydantic.coverage import Coverage, CoverageCollection
from covjson_pydantic.domain import Domain, ValuesAxis
from covjson_pydantic.observed_property import (
    ObservedProperty as CovJsonObservedProperty, )
from covjson_pydantic.parameter import Parameter as CovJsonParameter
from covjson_pydantic.reference_system import (
    ReferenceSystem,
    ReferenceSystemConnectionObject,
)
from covjson_pydantic.unit import Unit as CovJsonUnit
from defusedxml.ElementTree import ParseError, parse

from edr_pydantic.capabilities import (
    ConformanceModel,
    Contact,
    LandingPageModel,
    Provider,
)
from edr_pydantic.collections import Collection, Collections, Instance, Instances
from edr_pydantic.data_queries import DataQueries, EDRQuery
from edr_pydantic.extent import Extent, Spatial, Temporal, Vertical, Custom
from edr_pydantic.link import EDRQueryLink, Link
from edr_pydantic.observed_property import ObservedProperty
from edr_pydantic.parameter import Parameter
from edr_pydantic.unit import Unit
from edr_pydantic.variables import Variables

from fastapi import FastAPI, Query, Request, Response
from fastapi.responses import JSONResponse
from fastapi.openapi.utils import get_openapi
from geomet import wkt
from owslib.wms import WebMapService
from pydantic import AwareDatetime

from cachetools import cached, TTLCache

from .covjsonresponse import CovJSONResponse
from .ogcapi_tools import call_adaguc

logger = logging.getLogger(__name__)
logger.debug("Starting EDR")

edrApiApp = FastAPI(debug=False)

OWSLIB_DUMMY_URL = "http://localhost:8000"


def get_base_url(req: Request = None) -> str:
    """Returns the base url of this service"""

    base_url_from_request = (
        f"{req.url.scheme}://{req.url.hostname}:{req.url.port}"
        if req else None)
    base_url = (os.getenv("EXTERNALADDRESS", base_url_from_request)
                or "http://localhost:8080")

    return base_url.strip("/")


class EdrException(Exception):
    """
    Exception class for EDR
    """

    def __init__(self, code: str, description: str):
        self.code = code
        self.description = description


@edrApiApp.exception_handler(EdrException)
async def edr_exception_handler(_, exc: EdrException):
    """
    Handler for EDR exceptions
    """
    return JSONResponse(
        status_code=exc.code,
        content={
            "code": str(exc.code),
            "description": exc.description
        },
    )


def init_edr_collections(
        adaguc_dataset_dir: str = os.environ["ADAGUC_DATASET_DIR"]):
    """
    Return all possible OGCAPI EDR datasets, based on the dataset directory
    """
    dataset_files = [
        f for f in os.listdir(adaguc_dataset_dir)
        if os.path.isfile(os.path.join(adaguc_dataset_dir, f))
        and f.endswith(".xml")
    ]

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
                        if ("name" in edr_parameter.attrib
                                and "unit" in edr_parameter.attrib):
                            edr_params.append({
                                "name":
                                edr_parameter.attrib.get("name"),
                                "unit":
                                edr_parameter.attrib.get("unit"),
                            })
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
                            get_base_url() +
                            f"/edr/collections/{edr_collection.attrib.get('name')}"
                        )
                        edr_collections[edr_collection.attrib.get("name")] = {
                            "dataset":
                            dataset,
                            "name":
                            edr_collection.attrib.get("name"),
                            "service":
                            f"{OWSLIB_DUMMY_URL}/wms",
                            "parameters":
                            edr_params,
                            "vertical_name":
                            edr_collection.attrib.get("vertical_name"),
                            "base_url":
                            collection_url,
                            "time_interval":
                            edr_collection.attrib.get("time_interval"),
                        }
        except ParseError:
            pass
    return edr_collections


# The edr_collections information is cached locally for a maximum of 2 minutes
# It will be refreshed if older than 2 minutes
edr_cache = TTLCache(maxsize=100, ttl=120)


@cached(cache=edr_cache)
def get_edr_collections():
    """Returns all EDR collections"""
    return init_edr_collections()


async def get_point_value(
    edr_collectioninfo: dict,
    instance: str,
    coords: list[float],
    parameters: list[str],
    datetime_par: str,
    z_par: str = None,
    custom_dims: str = None,
):
    """Returns information in EDR format for a given location"""
    dataset = edr_collectioninfo["dataset"]
    urlrequest = (
        f"SERVICE=WMS&VERSION=1.3.0&REQUEST=GetPointValue&CRS=EPSG:4326"
        f"&DATASET={dataset}&QUERY_LAYERS={','.join(parameters)}"
        f"&X={coords[0]}&Y={coords[1]}&INFO_FORMAT=application/json")
    if datetime_par:
        urlrequest += f"&TIME={datetime_par}"

    if instance:
        urlrequest += f"&DIM_reference_time={instance_to_iso(instance)}"
    if z_par:
        if ("vertical_name" in edr_collectioninfo and
                edr_collectioninfo["vertical_name"].upper() != "ELEVATION"):
            urlrequest += f"&DIM_{edr_collectioninfo['vertical_name']}={z_par}"
        else:
            urlrequest += f"&ELEVATION={z_par}"
    if custom_dims:
        urlrequest += custom_dims

    status, response, headers = await call_adaguc(
        url=urlrequest.encode("UTF-8"))
    if status == 0:
        return response.getvalue(), headers
    return None, None


@edrApiApp.get(
    "/collections/{collection_name}/position",
    response_model=Coverage | CoverageCollection,
    response_class=CovJSONResponse,
    response_model_exclude_none=True,
)
@edrApiApp.get(
    "/collections/{collection_name}/instances/{instance}/position",
    response_model=Coverage | CoverageCollection,
    response_class=CovJSONResponse,
    response_model_exclude_none=True,
)
async def get_collection_position(
        collection_name: str,
        request: Request,
        coords: str,
        response: CovJSONResponse,
        instance: Union[str, None] = None,
        datetime_par: str = Query(default=None, alias="datetime"),
        parameter_name: str = Query(alias="parameter-name"),
        z_par: str = Query(alias="z", default=None),
) -> Coverage:
    """
    returns data for the EDR /position endpoint
    """
    allowed_params = ["coords", "datetime", "parameter-name", "z", "f", "crs"]
    custom_params = [
        k for k in request.query_params if k not in allowed_params
    ]
    custom_dims = ""
    if len(custom_params) > 0:
        for custom_param in custom_params:
            custom_dims += f"&DIM_{custom_param}={request.query_params[custom_param]}"
    edr_collections = get_edr_collections()

    parameter_names = parameter_name.split(",")
    latlons = wkt.loads(coords)
    logger.info("latlons:%s", latlons)
    coord = {
        "lat": latlons["coordinates"][1],
        "lon": latlons["coordinates"][0]
    }
    resp, headers = await get_point_value(
        edr_collections[collection_name],
        instance,
        [coord["lon"], coord["lat"]],
        parameter_names,
        datetime_par,
        z_par,
        custom_dims,
    )
    if resp:
        dat = json.loads(resp)
        ttl = get_ttl_from_adaguc_headers(headers)
        if ttl is not None:
            response.headers["cache-control"] = generate_max_age(ttl)
        return covjson_from_resp(
            dat, edr_collections[collection_name]["vertical_name"])

    raise EdrException(code=400, description="No data")


DEFAULT_CRS_OBJECT = {
    "crs":
    "EPSG:4326",
    "wkt":
    'GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4326"]]',
}


async def get_collectioninfo_for_id(
    edr_collection: str,
    instance: str = None,
) -> tuple[Collection, datetime]:
    """
    Returns collection information for a given collection id and or instance
    Is used to obtain metadata from the dataset configuration and WMS GetCapabilities document.
    """
    logger.info("get_collectioninfo_for_id(%s, %s)", edr_collection, instance)
    edr_collectioninfo = get_edr_collections()[edr_collection]
    dataset = edr_collectioninfo["dataset"]
    logger.info("%s=>%s", edr_collection, dataset)

    base_url = edr_collectioninfo["base_url"]
    logger.info("! %s", base_url)

    if instance is not None:
        base_url += f"/instances/{instance}"

    links: list[Link] = []
    links.append(
        Link(href=f"{base_url}", rel="collection", type="application/json"))

    ref_times = None

    if not instance:
        ref_times = await get_ref_times_for_coll(
            edr_collectioninfo, edr_collectioninfo["parameters"][0]["name"])
        if ref_times and len(ref_times) > 0:
            instances_link = Link(href=f"{base_url}/instances",
                                  rel="collection",
                                  type="application/json")
            links.append(instances_link)

    wmslayers, ttl = await get_capabilities(edr_collectioninfo["name"])

    bbox = get_extent(edr_collectioninfo, wmslayers)
    if bbox is None:
        return None, None
    crs = 'GEOGCS["GCS_WGS_1984",DATUM["D_WGS_1984",SPHEROID["WGS_1984",6378137,298.257223563]],PRIMEM["Greenwich",0],UNIT["Degree",0.017453292519943295]]'
    spatial = Spatial(bbox=bbox, crs=crs)

    if instance is None or edr_collectioninfo["time_interval"] is None:
        (interval, time_values) = get_times_for_collection(
            edr_collectioninfo, wmslayers,
            edr_collectioninfo["parameters"][0]["name"])
    else:
        (interval,
         time_values) = create_times_for_instance(edr_collectioninfo,
                                                  wmslayers, instance)

    customlist: list = get_custom_dims_for_collection(
        edr_collectioninfo, wmslayers,
        edr_collectioninfo["parameters"][0]["name"])

    # Custom can be a list of custom dimensions, like ensembles, thresholds
    custom = []
    if customlist is not None:
        for custom_el in customlist:
            custom.append(Custom(**custom_el))

    vertical = None
    vertical_dim = get_vertical_dim_for_collection(
        edr_collectioninfo, wmslayers,
        edr_collectioninfo["parameters"][0]["name"])
    if vertical_dim:
        vertical = Vertical(**vertical_dim)

    print("interval:", interval)
    temporal = Temporal(
        interval=interval,  # [["2022-06-30T09:00:00Z", "2022-07-02T06:00:00Z"]],
        trs=
        'TIMECRS["DateTime",TDATUM["Gregorian Calendar"],CS[TemporalDateTime,1],AXIS["Time (T)",future]',
        values=time_values,  # ["R49/2022-06-30T06:00:00/PT60M"],
    )

    extent = Extent(spatial=spatial,
                    temporal=temporal,
                    custom=custom,
                    vertical=vertical)

    #   crs_details=[crs_object],
    position_variables = Variables(
        query_type="position",
        default_output_format="CoverageJSON",
        output_formats=["CoverageJSON", "GeoJSON"],
    )
    position_link = EDRQueryLink(
        href=f"{base_url}/position",
        rel="data",
        hreflang="en",
        title="Position query",
        variables=position_variables,
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
            instances=EDRQuery(link=instances_link),
        )
    else:
        data_queries = DataQueries(position=EDRQuery(link=position_link))

    parameter_names = get_params_for_collection(edr_collection=edr_collection)

    crs = ["EPSG:4326"]

    output_formats = ["CoverageJSON", "GeoJSON"]
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


def get_params_for_collection(edr_collection: str) -> dict[str, Parameter]:
    """
    Returns a dictionary with parameters for given EDR collection
    """
    parameter_names = {}
    edr_collections = get_edr_collections()
    for param_el in edr_collections[edr_collection]["parameters"]:
        # Use name as default for label
        if "label" in param_el:
            label = param_el["label"]
        else:
            label = param_el["name"]

        param = Parameter(
            id=param_el["name"],
            observedProperty=ObservedProperty(id=param_el["name"],
                                              label=label),
            type="Parameter",
            unit=Unit(symbol=param_el["unit"]),
            label=label,
        )
        parameter_names[param_el["name"]] = param
    return parameter_names


def parse_iso(dts: str) -> datetime:
    """
    Converts a ISO8601 string into a datetime object
    """
    parsed_dt = None
    try:
        parsed_dt = datetime.strptime(
            dts, "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc)
    except ValueError as exc:
        logger.error("err: %s %s", dts, exc)
    return parsed_dt


def parse_instance_time(dts: str) -> datetime:
    """
    Converts EDR instance time in format %Y%m%d%H%M into datetime.
    """
    parsed_dt = None
    try:
        parsed_dt = datetime.strptime(
            dts, "%Y%m%d%H%M").replace(tzinfo=timezone.utc)
    except ValueError as exc:
        logger.error("err: %s %s", dts, exc)
    return parsed_dt


def instance_to_iso(instance_dt: str):
    """
    Converts EDR instance time in format %Y%m%d%H%M into iso8601 time string.
    """
    parsed_dt = parse_instance_time(instance_dt)
    if parsed_dt:
        return parsed_dt.strftime("%Y-%m-%dT%H:%M:%SZ")
    return None


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


def get_times_for_collection(
        edr_collectioninfo: dict,
        wmslayers,
        parameter: str = None) -> tuple[list[list[str]], list[str]]:
    """
    Returns a list of times based on the time dimensions, it does a WMS GetCapabilities to the given dataset (cached)

    It does this for given parameter. When the parameter is not given it will do it for the first Layer in the GetCapabilities document.
    """
    logger.info("get_times_for_dataset(%s,%s)", edr_collectioninfo["name"],
                parameter)
    if parameter and parameter in wmslayers:
        layer = wmslayers[parameter]
    else:
        layer = wmslayers[list(wmslayers)[0]]

    if "time" in layer["dimensions"]:
        time_dim = layer["dimensions"]["time"]
        if "/" in time_dim["values"][0]:
            terms = time_dim["values"][0].split("/")
            interval = [[
                datetime.strptime(
                    terms[0],
                    "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc),
                datetime.strptime(
                    terms[1],
                    "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc),
            ]]
            return interval, get_time_values_for_range(time_dim["values"][0])
        interval = [[
            datetime.strptime(
                time_dim["values"][0],
                "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc),
            datetime.strptime(
                time_dim["values"][-1],
                "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc),
        ]]
        return interval, time_dim["values"]
    return None, None


def create_times_for_instance(edr_collectioninfo: dict, wmslayers,
                              instance: str):
    """
    Returns a list of times for a reference_time, derived from the time_interval EDRCollection attribute in edr_collectioninfo

    """
    ref_time = parse_instance_time(instance)
    time_interval = edr_collectioninfo["time_interval"].replace(
        "{reference_time}", ref_time.strftime("%Y-%m-%dT%H:%M:%SZ"))
    (repeat_s, ref_t_s, time_step_s) = time_interval.split("/")
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
    for _d in range(repeat):
        times.append(step_time.strftime("%Y-%m-%dT%H:%M:%SZ"))
        step_time = step_time + delta

    interval = [[
        datetime.strptime(times[0],
                          "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc),
        datetime.strptime(times[-1],
                          "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc)
    ]]
    return interval, times


def get_custom_dims_for_collection(edr_collectioninfo: dict,
                                   wmslayers,
                                   parameter: str = None):
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
                "id":
                dim_name,
                "interval": [
                    layer["dimensions"][dim_name]["values"][0],
                    layer["dimensions"][dim_name]["values"][-1]
                ],
                "values":
                layer["dimensions"][dim_name]["values"],
                "reference":
                f"custom_{dim_name}",
            }
            custom.append(custom_dim)
    return custom if len(custom) > 0 else None


def get_vertical_dim_for_collection(edr_collectioninfo: dict,
                                    wmslayers,
                                    parameter: str = None):
    """
    Return the verticel dimension the WMS GetCapabilities document.
    """
    if parameter and parameter in list(wmslayers):
        layer = wmslayers[parameter]
    else:
        layer = wmslayers[list(wmslayers)[0]]

    for dim_name in layer["dimensions"]:
        if dim_name in [
                "elevation"
        ] or ("vertical_name" in edr_collectioninfo
              and dim_name == edr_collectioninfo["vertical_name"]):
            vertical_dim = {
                "interval": [],
                "values": layer["dimensions"][dim_name]["values"],
                "vrs": "customvrs",
            }
            return vertical_dim
    return None


@edrApiApp.get("/collections",
               response_model=Collections,
               response_model_exclude_none=True)
async def rest_get_edr_collections(request: Request, response: Response):
    """
    GET /collections, returns a list of available collections
    """
    links: list[Link] = []
    collection_url = get_base_url(request) + "/edr/collections"
    self_link = Link(href=collection_url, rel="self", type="application/json")

    links.append(self_link)
    collections: list[Collection] = []
    ttl_set = set()
    edr_collections = get_edr_collections()
    for edr_coll in edr_collections:
        coll, ttl = await get_collectioninfo_for_id(edr_coll)
        if coll:
            collections.append(coll)
            if ttl is not None:
                ttl_set.add(ttl)
        else:
            logger.warning("Unable to fetch WMS GetCapabilities for %s",
                           edr_coll)
    collections_data = Collections(links=links, collections=collections)
    if ttl_set:
        response.headers["cache-control"] = generate_max_age(min(ttl_set))
    return collections_data


@edrApiApp.get(
    "/collections/{collection_name}",
    response_model=Collection,
    response_model_exclude_none=True,
)
async def rest_get_edr_collection_by_id(collection_name: str,
                                        response: Response):
    """
    GET Returns collection information for given collection id
    """
    collection, ttl = await get_collectioninfo_for_id(collection_name)
    if ttl is not None:
        response.headers["cache-control"] = generate_max_age(ttl)
    return collection


def get_ttl_from_adaguc_headers(headers):
    """Derives an age value from the max-age/age cache headers that ADAGUC executable returns"""
    try:
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
    except:
        pass
    return None


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
        status, response, headers = await call_adaguc(
            url=urlrequest.encode("UTF-8"))
        ttl = get_ttl_from_adaguc_headers(headers)
        logger.info("status: %d", status)
        if status == 0:
            xml = response.getvalue()
            wms = WebMapService(collection_info["service"],
                                xml=xml,
                                version="1.3.0")
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
            "dimensions": {
                **layerinfo.dimensions
            },
            "boundingBoxWGS84": layerinfo.boundingBoxWGS84,
        }

    return layers, ttl


async def get_ref_times_for_coll(edr_collectioninfo: dict,
                                 layer: str) -> list[str]:
    """
    Returns available reference times for given collection
    """
    dataset = edr_collectioninfo["dataset"]
    url = f"?DATASET={dataset}&SERVICE=WMS&VERSION=1.3.0&request=getreferencetimes&LAYER={layer}"
    logger.info("getreftime_url(%s,%s): %s", dataset, layer, url)
    status, response, _ = await call_adaguc(url=url.encode("UTF-8"))
    if status == 0:
        ref_times = json.loads(response.getvalue())
        instance_ids = [
            parse_iso(reft).strftime("%Y%m%d%H%M") for reft in ref_times
        ]
        return instance_ids
    return []


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


@edrApiApp.get(
    "/collections/{collection_name}/instances",
    response_model=Instances,
    response_model_exclude_none=True,
)
async def rest_get_edr_inst_for_coll(collection_name: str, request: Request,
                                     response: Response):
    """
    GET: Returns all available instances for the collection
    """
    instances_url = (get_base_url(request) +
                     f"/edr/collections/{collection_name}/instances")

    instances: list[Instance] = []
    edr_collections = get_edr_collections()

    ref_times = await get_ref_times_for_coll(
        edr_collections[collection_name],
        edr_collections[collection_name]["parameters"][0]["name"],
    )
    links: list[Link] = []
    links.append(Link(href=instances_url, rel="collection"))
    ttl_set = set()
    for instance in list(ref_times):
        instance_links: list[Link] = []
        instance_link = Link(href=f"{instances_url}/{instance}",
                             rel="collection")
        instance_links.append(instance_link)
        instance_info, ttl = await get_collectioninfo_for_id(
            collection_name, instance)
        if ttl is not None:
            ttl_set.add(ttl)
        instances.append(instance_info)

    instances_data = Instances(instances=instances, links=links)
    if ttl_set:
        response.headers["cache-control"] = generate_max_age(min(ttl_set))
    return instances_data


@edrApiApp.get(
    "/collections/{collection_name}/instances/{instance}",
    response_model=Collection,
    response_model_exclude_none=True,
)
async def rest_get_collection_info(collection_name: str, instance,
                                   response: Response):
    """
    GET  "/collections/{collection_name}/instances/{instance}"
    """
    coll, ttl = await get_collectioninfo_for_id(collection_name, instance)
    if ttl is not None:
        response.headers["cache-control"] = generate_max_age(ttl)
    return coll


def generate_max_age(ttl):
    if ttl >= 0:
        return f"max-age={ttl}"
    return f"max-age=0"


@edrApiApp.get("/",
               response_model=LandingPageModel,
               response_model_exclude_none=True)
async def rest_get_edr_landing_page(request: Request):
    """
    GET / : Index of EDR service
    """
    cfg = set_edr_config()
    contact = Contact(**cfg["contact"])
    provider = Provider(**cfg["provider"])

    description = cfg.get("description")
    title = cfg.get("title")
    keywords = cfg.get("keywords")

    landingpage_url = get_base_url(request) + "/edr"
    conformance_url = get_base_url(request) + "/edr/conformance"
    collections_url = get_base_url(request) + "/edr/collections"

    links: list[Link] = []
    link = Link(href=landingpage_url, rel="self", type="application/json")
    links.append(link)
    links.append(
        Link(href=conformance_url, rel="conformance", type="application/json"))
    links.append(
        Link(href=collections_url, rel="data", type="application/json"))
    openapi_url = f"{landingpage_url}/api"
    links.append(
        Link(
            href=openapi_url,
            rel="service-desc",
            type="application/vnd.oai.openapi+json;version=3.0",
        ))

    landing_page = LandingPageModel(
        links=links,
        contact=contact,
        provider=provider,
        keywords=keywords,
        description=description,
        title=title,
    )
    return landing_page


conformance = ConformanceModel(conformsTo=[
    "http://www.opengis.net/spec/ogcapi-edr-1/1.0/conf/core",
    "http://www.opengis.net/spec/ogcapi-common-1/1.0.conf/core",
    "http://www.opengis.net/spec/ogcapi-common-2/1.0/conf/collections",
    "http://www.opengis.net/spec/ogcapi-edr-1/1.0/conf/oas30",
    "http://www.opengis.net/spec/ogcapi-edr-1/1.0/conf/geojson",
    "http://www.opengis.net/spec/ogcapi-edr-1/1.0/conf/queries",
])


@edrApiApp.get("/collections/{coll}/locations")
def get_locations(_coll: str):
    """
    Returns locations where you could query data.
    """
    return {
        "features": [{
            "id": "100683",
            "type": "Feature",
            "geometry": {
                "coordinates": [5.2, 52.0],
                "type": "Point"
            },
            "properties": {
                "name": "De Bilt",
            },
        }]
    }


@edrApiApp.get("/api", )
def get_fixed_api():
    """
    Fix the API! This is needed for the OGC conformance tests.
    """
    api = get_openapi(
        title=edrApiApp.title,
        version=edrApiApp.openapi_version,
        routes=edrApiApp.routes,
    )
    for pth in api["paths"].values():
        if "parameters" in pth["get"]:
            for param in pth["get"]["parameters"]:
                if param["in"] == "query" and param["name"] == "datetime":
                    param["style"] = "form"
                    param["explode"] = False
                    param["schema"] = {
                        "type": "string",
                    }
                if "schema" in param:
                    if "anyOf" in param["schema"]:
                        for itany in param["schema"]["anyOf"]:
                            if itany.get("type") == "null":
                                print("NULL found p")

    if "CompactAxis" in api["components"]["schemas"]:
        comp = api["components"]["schemas"]["CompactAxis"]
        if "exclusiveMinimum" in comp["properties"]["num"]:
            comp["properties"]["num"]["exclusiveMinimum"] = False

    return api


@edrApiApp.get("/conformance", response_model=ConformanceModel)
async def rest_get_edr_conformance():
    """
    GET /conformance: EDR Conformanc endpoint
    """
    return conformance


def multi_get(dict_obj, attrs, default=None):
    """
    multi_get
    """
    result = dict_obj
    for attr in attrs:
        if attr not in result:
            return default
        result = result[attr]
    return result


def getdimvals(dims, name):
    """
    getdimvals
    """
    if name in dims:
        return dims[name]
    return None


def makedims(dims, data):
    """
    Makedims
    """
    dimlist = {}
    if isinstance(dims, str) and dims == "time":
        times = list(data.keys())
        dimlist["time"] = times
        return dimlist

    data1 = list(data.keys())
    if len(data1) == 0:
        return []
    dimlist[dims[0]] = data1

    if len(dims) >= 2:
        data2 = list(data[data1[0]].keys())
        dimlist[dims[1]] = data2

    if len(dims) >= 3:
        data3 = list(data[data1[0]][data2[0]].keys())
        dimlist[dims[2]] = data3

    if len(dims) >= 4:
        data4 = list(data[data1[0]][data2[0]][data3[0]].keys())
        dimlist[dims[3]] = data4

    if len(dims) >= 5:
        data5 = list(data[data1[0]][data2[0]][data3[0]][data4[0]].keys())
        dimlist[dims[4]] = data5

    return dimlist


def covjson_from_resp(dats, vertical_name):
    """
    Returns a coverage json from a Adaguc WMS GetFeatureInfo request
    """
    covjson_list = []
    for dat in dats:
        if len(dat["data"]):
            (lon, lat) = dat["point"]["coords"].split(",")
            lat = float(lat)
            lon = float(lon)
            dims = makedims(dat["dims"], dat["data"])
            time_steps = getdimvals(dims, "time")
            if vertical_name is not None:
                vertical_steps = getdimvals(dims, vertical_name)
            else:
                vertical_steps = getdimvals(dims, "elevation")

            valstack = []
            # Translate the adaguc GFI object in something we can handle in Python
            for dim_name in dims:
                vals = getdimvals(dims, dim_name)
                valstack.append(vals)
            tuples = list(itertools.product(*valstack))
            values = []
            for mytuple in tuples:
                value = multi_get(dat["data"], mytuple)
                if value:
                    try:
                        values.append(float(value))
                    except ValueError:
                        if value == "nodata":
                            values.append(None)
                        else:
                            values.append(value)

            parameters: dict[str, CovJsonParameter] = {}
            ranges = {}

            unit = CovJsonUnit(symbol=dat["units"])
            param = CovJsonParameter(
                id=dat["name"],
                observedProperty=CovJsonObservedProperty(
                    label={"en": dat["name"]}),
                unit=unit,
            )
            # TODO: add units to CovJsonParameter
            parameters[dat["name"]] = param
            axis_names = ["x", "y", "t"]
            shape = [1, 1, len(time_steps)]
            if vertical_steps:
                axis_names = ["x", "y", "z", "t"]
                shape = [1, 1, len(vertical_steps), len(time_steps)]
            _range = {
                "axisNames": axis_names,
                "shape": shape,
                "values": values,
            }
            ranges[dat["name"]] = _range

            axes: dict[str, ValuesAxis] = {
                "x":
                ValuesAxis[float](values=[lon]),
                "y":
                ValuesAxis[float](values=[lat]),
                "t":
                ValuesAxis[AwareDatetime](values=[
                    datetime.strptime(t, "%Y-%m-%dT%H:%M:%SZ").replace(
                        tzinfo=timezone.utc) for t in time_steps
                ]),
            }
            domain_type = "PointSeries"
            if vertical_steps:
                axes["z"] = ValuesAxis[float](values=vertical_steps)
                if len(vertical_steps) > 1:
                    domain_type = "VerticalProfile"
            if time_steps:
                axes["t"] = ValuesAxis[AwareDatetime](values=[
                    datetime.strptime(t, "%Y-%m-%dT%H:%M:%SZ").replace(
                        tzinfo=timezone.utc) for t in time_steps
                ])
                if len(time_steps) > 1 and vertical_steps and len(
                        vertical_steps) > 1:
                    domain_type = "Grid"

            referencing = [
                ReferenceSystemConnectionObject(
                    system=ReferenceSystem(
                        type="GeographicCRS",
                        id="http://www.opengis.net/def/crs/OGC/1.3/CRS84",
                    ),
                    coordinates=["x", "y", "z"],
                ) if vertical_steps else ReferenceSystemConnectionObject(
                    system=ReferenceSystem(
                        type="GeographicCRS",
                        id="http://www.opengis.net/def/crs/OGC/1.3/CRS84",
                    ),
                    coordinates=["x", "y"],
                ),
                ReferenceSystemConnectionObject(
                    system=ReferenceSystem(type="TemporalRS",
                                           calendar="Gregorian"),
                    coordinates=["t"],
                ),
            ]
            domain = Domain(domainType=domain_type,
                            axes=axes,
                            referencing=referencing)
            covjson = Coverage(
                id="test",
                domain=domain,
                ranges=ranges,
                parameters=parameters,
            )
            covjson_list.append(covjson)

    if len(covjson_list) == 1:
        return covjson_list[0]
    if len(covjson_list) == 0:
        raise EdrException(code=400, description="No data")

    coverages = []
    covjson_list.sort(key=lambda x: x.model_dump_json())
    for _domain, group in itertools.groupby(covjson_list, lambda x: x.domain):
        _ranges = {}
        _parameters = {}
        for cov in group:
            param_id = next(iter(cov.parameters))
            _parameters[param_id] = cov.parameters[param_id]
            _ranges[param_id] = cov.ranges[param_id]
        coverages.append(
            Coverage(
                domain=_domain,
                ranges=_ranges,
                parameters=_parameters,
            ))
    if len(coverages) == 1:
        return coverages[0]

    parameter_union = functools.reduce(operator.ior,
                                       (c.parameters for c in coverages), {})
    coverage_collection = CoverageCollection(coverages=coverages,
                                             parameters=parameter_union)

    return coverage_collection


def set_edr_config():
    """
    Returns metadata for the EDR service
    """
    config = {}
    config["contact"] = {"email": "gstf@knmi.nl"}
    config["provider"] = {"name": "KNMI", "url": "https://www.knmi.nl"}
    config["keywords"] = ["OGCAPI EDR"]
    config["description"] = "EDR service for ADAGUC datasets"
    config["title"] = "ADAGUC OGCAPI EDR"
    return config
