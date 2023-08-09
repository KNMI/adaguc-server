from typing import Any, Dict, Optional, Tuple
from fastapi import APIRouter, Request, Response, FastAPI, Query, HTTPException
from fastapi.responses import JSONResponse
from fastapi.encoders import jsonable_encoder

import json
import itertools
import sys

from owslib.wms import WebMapService
from .setup_adaguc import setup_adaguc
from defusedxml.ElementTree import fromstring, parse, ParseError

import re

from pyproj import CRS, Transformer
import logging
import os
import time
import datetime as datetime_class
from datetime import datetime, timedelta, timezone

from cachetools import cached, TTLCache

from covjson_pydantic.coverage import Coverage
from covjson_pydantic.parameter import Parameter as CovJsonParameter
from covjson_pydantic.unit import Unit as CovJsonUnit, Symbol
from covjson_pydantic.i18n import i18n
from covjson_pydantic.domain import Domain, ValuesAxis
from covjson_pydantic.observed_property import ObservedProperty
from covjson_pydantic.i18n import i18n
from covjson_pydantic.reference_system import (
    ReferenceSystem,
    ReferenceSystemConnectionObject,
)
from edr_pydantic_classes.generic_models import (
    Custom,
    Link,
    ObservedPropertyCollection,
    Spatial,
    Temporal,
    Units,
    Vertical,
    CRSOptions,
    ParameterName,
)
from edr_pydantic_classes.capabilities import (
    ConformanceModel,
    LandingPageModel,
    Contact,
    Provider,
)

from edr_pydantic_classes.instances import (
    Instance,
    InstancesDataQueryLink,
    InstancesLink,
    InstancesModel,
    Collection,
    CollectionsModel,
    Extent,
    DataQueries,
    InstancesVariables,
    PositionLink,
    PositionDataQueryLink,
    PositionVariables,
    CrsObject,
)

from geomet import wkt
from .geojsonresponse import GeoJSONResponse

from .ogcapi_tools import call_adaguc

logger = logging.getLogger(__name__)
logger.debug("Starting EDR")

edrApiApp = FastAPI(debug=True)

class EdrException(Exception):
    def __init__(self, code: str, description: str):
        self.code = code
        self.description = description

@edrApiApp.exception_handler(EdrException)
async def edr_exception_handler(request: Request, exc: EdrException):
    return JSONResponse(status_code = exc.code, content={"code": str(exc.code), "description": exc.description})

def get_edr_collections(
        adagucDataSetDir: str = os.environ["ADAGUC_DATASET_DIR"]):
    """
    Return all possible OGCAPI EDR datasets, based on the dataset directory
    """
    datasetFiles = [
        f for f in os.listdir(adagucDataSetDir)
        if os.path.isfile(os.path.join(adagucDataSetDir, f))
        and f.endswith(".xml")
    ]

    edr_collections = {}
    for datasetFile in datasetFiles:
        dataset=datasetFile.replace(".xml", "")
        try:
            tree = parse(os.path.join(adagucDataSetDir, datasetFile))
            root = tree.getroot()
            for ogcapi_edr in root.iter("OgcApiEdr"):
                for edr_collection in ogcapi_edr.iter("EdrCollection"):
                    edr_collections[edr_collection.attrib.get("name")] = {
                        "dataset":
                        dataset,
                        "name":
                        edr_collection.attrib.get("name"),
                        "service":
                        "http://localhost:8000/wms",
                        "time_interval":
                        edr_collection.attrib.get("time_interval"),
                        "z_interval":
                        edr_collection.attrib.get("z_interval"),
                        "parameters": [
                            inst.strip()
                            for inst in edr_collection.text.strip().split(",")
                        ],
                    }
        except ParseError:
            pass
    return edr_collections


def get_edr_collections_for_datasetOLD(
        dataset: str, dataset_dir: str = os.environ["ADAGUC_DATASET_DIR"]):
    logger.info(">>get_edr_collections_for_dataset %s", dataset)
    filename = f"{dataset_dir}/{dataset}.xml"

    edr_collections = {}
    tree = parse(filename)
    root = tree.getroot()
    for ogcapi_edr in root.iter("OgcApiEdr"):
        for edr_collection in ogcapi_edr.iter("EdrCollection"):
            edr_collections[edr_collection.attrib.get("name")] = {
                "dataset":
                dataset,
                "name":
                edr_collection.attrib.get("name"),
                "url":
                "http://localhost:8000/wms",
                "time_interval":
                edr_collection.attrib.get("time_interval"),
                "z_interval":
                edr_collection.attrib.get("z_interval"),
                "parameters": [
                    inst.strip()
                    for inst in edr_collection.text.strip().split(",")
                ],
            }
    return edr_collections


def get_point_value(
    dataset: str,
    instance: str,
    coords: list[float],
    parameters: str,
    t: str,
    z: str = None,
    extra_dims="",
):
    urlrequest = (
        f"SERVICE=WMS&VERSION=1.3.0&REQUEST=GetPointValue&CRS=EPSG:4326"
        f"&DATASET={dataset}&QUERY_LAYERS={parameters}"
        f"&X={coords[0]}&Y={coords[1]}&INFO_FORMAT=application/json")
    if t:
        urlrequest += f"&TIME={t}"

    if instance:
        urlrequest += f"&DIM_reference_time={instance_to_iso(instance)}"
    if z:
        urlrequest += f"&ELEVATION={z}"
    if extra_dims:
        urlrequest += extra_dims

    logger.info("URL: %s", urlrequest)
    status, response = call_adaguc(url=urlrequest.encode("UTF-8"))
    logger.info("status: %d", status)
    if status == 0:
        return response.getvalue()
    return None


@edrApiApp.get(
    "/collections/{collection_name}/position",
    response_model=Coverage,
    response_model_exclude_none=True,
)
async def get_collection_position(
        collection_name: str,
        request: Request,
        coords: str,
        datetime: Optional[str] = None,
        parameter_name: str = Query(alias="parameter-name"),
):
    allowed_params = ["coords", "datatime", "parameter-name"]
    extra_params = [k for k in request.query_params if k not in allowed_params]
    extra_dims = ""
    if len(extra_params):
        for extra_param in extra_params:
            extra_dims += f"&DIM_{extra_param}={request.query_params[extra_param]}"
    dataset = collection_name.split("-")[0]
    latlons = wkt.loads(coords)
    logger.info("latlons:%s", latlons)
    coord = {
        "lat": latlons["coordinates"][1],
        "lon": latlons["coordinates"][0]
    }
    resp = get_point_value(
        dataset,
        None,
        [coord["lon"], coord["lat"]],
        parameter_name,
        datetime,
        extra_dims,
    )
    if resp:
        dat = json.loads(resp)
        return GeoJSONResponse(covjson_from_resp(dat))

    raise EdrException(code=400, description="No data")


@edrApiApp.get(
    "/collections/{collection_name}/instances/{instance}/position",
    response_model=Coverage,
    response_model_exclude_none=True,
)
async def edr_get_collection_instance_position(
    collection_name: str,
    instance: str,
    request: Request,
    response: Response,
    coords: str,
    datetime: Optional[str] = None,
    parameter_name: str = Query(alias="parameter-name"),
    z: Optional[str] = None,
):

    collections_info = get_edr_collections()
    collection_info = collections_info[collection_name]
    dataset = collection_info["dataset"]

    latlons = wkt.loads(coords)
    coord = {
        "lat": latlons["coordinates"][1],
        "lon": latlons["coordinates"][0]
    }
    resp = get_point_value(
        dataset,
        instance,
        [coord["lon"], coord["lat"]],
        parameter_name,
        datetime,
        z,
    )
    if resp:
        dat = json.loads(resp)
        return GeoJSONResponse(covjson_from_resp(dat))

    raise EdrException(code=400, description="No data")

DEFAULT_CRS_OBJECT = {
    "crs":
    "EPSG:4326",
    "wkt":
    'GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4326"]]',
}


def get_collectioninfo_for_id(
    edr_collection: str,
    base_url: str,
    instance: str = None,
) -> Collection:
    logger.info("get_collection_info_for_id(%s, %s, %s)", edr_collection , base_url, instance)
    links: list[Link] = []
    links.append(Link(href=f"{base_url}", rel="collection", type="application/json"))

    edr_collectioninfo = get_edr_collections()[edr_collection]
    dataset = edr_collectioninfo["dataset"]
    logger.info("%s=>%s", edr_collection, dataset)

    ref_times = None
    if not instance:
        ref_times = get_reference_times_for_dataset(
            dataset, edr_collectioninfo["parameters"][0])
        if ref_times and len(ref_times) > 0:
            instances_link = Link(href=f"{base_url}/instances",
                                 rel="collection",
                                 type="application/json")
            links.append(instances_link)

    bbox = get_extent(edr_collection)
    crs = CRSOptions.wgs84
    spatial = Spatial(bbox=bbox, crs=crs)
    (interval,
     time_values) = get_times_for_collection(edr_collection,
                                          edr_collectioninfo["parameters"][0])

    customlist = get_custom_dims_for_dataset(edr_collection,
                                             edr_collectioninfo["parameters"][0])
    custom = []
    if customlist:
        for c in customlist:
            custom.append(Custom(**c))

    vertical = None
    vertical_dim = get_vertical_dim_for_dataset(
        edr_collection, edr_collectioninfo["parameters"][0])
    if vertical_dim:
        vertical = Vertical(**vertical_dim)

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

    crs_object = CrsObject(
        crs="EPSG:4326",
        wkt=
        'GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4326"]]',
    )
    position_variables = PositionVariables(
        crs_details=[crs_object],
        default_output_format="CoverageJSON",
        output_formats=["CoverageJSON", "GeoJSON"],
    )
    position_link = PositionLink(
        href=f"{base_url}/position",
        rel="data",
        hreflang="en",
        title="Position query",
        variables=position_variables,
    )
    if ref_times and len(ref_times) > 0:
        instances_variables = InstancesVariables(
            crs_details=[crs_object],
            default_output_format="CoverageJSON",
            output_formats=["CoverageJSON", "GeoJSON"],
        )
        instances_link = InstancesLink(
            href=f"{base_url}/instances",
            rel="collection",
            hreflang="en",
            title="Instances query",
            variables=instances_variables,
        )
        instances_data_query_link = InstancesDataQueryLink(link=instances_link)
    else:
        instances_data_query_link = None

    position_data_query_link = PositionDataQueryLink(link=position_link)

    data_queries = DataQueries(position=position_data_query_link,
                               instances=instances_data_query_link)
    parameter_names = get_parameters_for_edr_collection(edr_collection=edr_collection)

    crs = ["EPSG:4326"]

    output_formats = ["CoverageJSON", "GeoJSON"]
    collection = Collection(
        links=links,
        id=instance if instance else edr_collection,
        extent=extent,
        data_queries=data_queries,
        parameter_names=parameter_names,
        crs=crs,
        output_formats=output_formats,
    )
    return collection


def get_parameters_for_edr_collection(edr_collection: str) -> dict[str, ParameterName]:
    parameter_names = dict()
    edr_collections = get_edr_collections()
    for param_name in edr_collections[edr_collection]["parameters"]:
        p = ParameterName(
            id=param_name,
            observedProperty=ObservedPropertyCollection(id=param_name,
                                                        label="title: " +
                                                        param_name),
            type="Parameter",
            unit=Units(symbol="mm"), #TODO Find real untis
            label="title: " + param_name,
        )
        parameter_names[param_name] = p
    return parameter_names


def parse_iso(dts: str) -> datetime:
    parsed_dt = None
    try:
        parsed_dt = datetime.strptime(
            dts, "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc)
    except ValueError as exc:
        logger.error("err: %s %s", dts, exc)
    return parsed_dt


def parse_instance_time(dts: str) -> datetime:
    parsed_dt = None
    try:
        parsed_dt = datetime.strptime(
            dts, "%Y%m%d%H%M").replace(tzinfo=timezone.utc)
    except ValueError as exc:
        logger.error("err: %s %s", dts, exc)
    return parsed_dt

def instance_to_iso(instance_dt: str):
    parsed_dt=parse_instance_time(instance_dt)
    if parsed_dt:
        return parsed_dt.strftime("%Y-%m-%dT%H:%M:%SZ")
    return None

def timepar_to_iso(instance_dt: str):
    parsed_dt=parse_instance_time(instance_dt)
    if parsed_dt:
        return parsed_dt.strftime("%Y-%m-%dT%H:%M:%SZ")
    return None

def get_time_values_for_range(rng) -> list[str]:
    els = rng.split("/")
    st = parse_iso(els[0])
    end = parse_iso(els[1])
    step = els[2]
    timediff = (end - st).total_seconds()
    tstep = None
    m = re.match("PT(\\d+)M", step)
    if m:
        tstep = int(m.group(1)) * 60
    m = re.match("PT(\\d+)H", step)
    if m:
        tstep = int(m.group(1)) * 3600
    m = re.match("P(\\d+)D", step)
    if m:
        tstep = int(m.group(1)) * 3600 * 24
    if not tstep:
        tstep = 3600
    nsteps = int(timediff / tstep) + 1
    return [f"R{nsteps}/{st}/{step}"]


def get_times_for_collection(
        collection_name: str,
        parameter: str = None) -> Tuple[list[list[str]], list[str]]:
    logger.info(f"get_times_for_dataset({collection_name},{parameter}")
    wms = get_capabilities(collection_name)
    if parameter and parameter in wms:
        layer = wms[parameter]
    else:
        layer = wms[list(wms)[0]]

    if "time" in layer.dimensions:
        time_dim = layer.dimensions["time"]
        if "/" in time_dim["values"][0]:
            terms = time_dim["values"][0].split("/")
            interval = [[
                datetime.strptime(terms[0], "%Y-%m-%dT%H:%M:%SZ"),
                datetime.strptime(terms[1], "%Y-%m-%dT%H:%M:%SZ"),
            ]]
            return (interval, get_time_values_for_range(time_dim["values"][0]))
        else:
            interval = [[
                datetime.strptime(time_dim["values"][0], "%Y-%m-%dT%H:%M:%SZ"),
                datetime.strptime(time_dim["values"][-1],
                                  "%Y-%m-%dT%H:%M:%SZ"),
            ]]
            return interval, time_dim["values"]
    return (None, None)


def get_custom_dims_for_dataset(dataset: str, parameter: str = None):
    wms = get_capabilities(dataset)
    custom = []
    if parameter and parameter in list(wms):
        layer = wms[parameter]
    else:
        # default to first layer
        layer = wms[list(wms)[0]]
    for dim_name in layer.dimensions:
        if dim_name not in ["reference_time", "time", "elevation"]:
            custom_dim = {
                "id": dim_name,
                "interval": [],
                "values": layer.dimensions[dim_name]["values"],
                "reference": "custom",
            }
            custom.append(custom_dim)
    return custom if len(custom) > 0 else None


def get_vertical_dim_for_dataset(dataset: str, parameter: str = None):
    wms = get_capabilities(dataset)
    if parameter and parameter in list(wms):
        layer = wms[parameter]
    else:
        layer = wms[list(wms)[0]]
    for dim_name in layer.dimensions:
        if dim_name in ["elevation"]:
            vertical_dim = {
                "interval": [],
                "values": layer.dimensions[dim_name]["values"],
                "vrs": "customvrs",
            }
            return vertical_dim
    return None


@edrApiApp.get("/collections",
               response_model=CollectionsModel,
               response_model_exclude_none=True)
async def edr_get_collections(request: Request):
    links: list[Link] = []
    base_url = str(request.url_for("edr_get_collections"))
    self_link = Link(href=base_url, rel="self", type="application/json")

    datasets = get_edr_collections()

    links.append(self_link)
    collections: list[Collection] = []
    edr_collections = get_edr_collections()
    for edr_coll in edr_collections:
        coll = get_collectioninfo_for_id(edr_coll, f"{base_url}/{edr_coll}")
        collections.append(coll)
    collections_data = CollectionsModel(links=links, collections=collections)
    return collections_data


@edrApiApp.get("/collections/{collection_name}",
               response_model=Collection,
               response_model_exclude_none=True)
async def edr_get_collection_by_id(collection_name: str, request: Request):
    base_url = request.url_for("edr_get_collection_by_id",
                               collection_name=collection_name)

    dataset = collection_name.split("-")[0]
    edr_collection = collection_name
    if collection_name == dataset:
        edr_collection = None
    collection = get_collectioninfo_for_id(collection_name, base_url)

    return collection


@cached(cache=TTLCache(maxsize=1024, ttl=60))
def get_capabilities(collname):
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
        status, response = call_adaguc(url=urlrequest.encode("UTF-8"))
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
    return wms.contents


def get_reference_times_for_dataset(dataset: str,
                                    layer: str) -> list[str]:
    url = f"?DATASET={dataset}&SERVICE=WMS&VERSION=1.3.0&request=getreferencetimes&LAYER={layer}"
    logger.info("getreftime_url(%s,%s): %s", dataset, layer, url)
    status, response = call_adaguc(url=url.encode("UTF-8"))
    if status == 0:
        ref_times = json.loads(response.getvalue())
        instance_ids=[parse_iso(reft).strftime("%Y%m%d%H%M") for reft in ref_times]
        return instance_ids
    return []


def get_extent(coll):
    """
    Get the boundingbox extent from the WMS GetCapabilities
    """
    contents = get_capabilities(coll)
    if len(contents):
        bbox = contents[next(iter(contents))].boundingBoxWGS84

        return [[bbox[0], bbox[1]], [bbox[2], bbox[3]]]
    return None


@edrApiApp.get(
    "/collections/{collection_name}/instances",
    response_model=InstancesModel,
    response_model_exclude_none=True,
)
async def edr_get_instances_for_collection(collection_name: str,
                                                   request: Request):
    base_url = str(
        request.url_for("edr_get_instances_for_collection",
                        collection_name=collection_name))
    instances: list(Instance) = []
    edr_collections = get_edr_collections()

    ref_times = get_reference_times_for_dataset(
        edr_collections[collection_name]["dataset"], edr_collections[collection_name]["parameters"][0])
    links: list(Link) = []
    links.append(Link(href=base_url, rel="collection"))
    for instance in list(ref_times):
        instance_links: list(Link) = []
        instance_link = Link(href=f"{base_url}/{instance}", rel="collection")
        instance_links.append(instance_link)
        instance_info = get_collectioninfo_for_id(
            collection_name,
            f"{base_url}/{instance}",
            instance,
        )

        instances.append(instance_info)

    instances_data = InstancesModel(instances=instances, links=links)
    return instances_data


@edrApiApp.get(
    "/collections/{collection_name}/instances/{instance}",
    response_model=Collection,
    response_model_exclude_none=True,
)
async def edr_get_collection_instance_by_name_and_instance(
        collection_name: str, instance, request: Request):
    base_url = request.url_for(
        "edr_get_collection_instance_by_name_and_instance",
        collection_name=collection_name,
        instance=instance,
    )
    coll = get_collectioninfo_for_id(collection_name, base_url,
                                     instance)
    return coll


@edrApiApp.get("/",
               response_model=LandingPageModel,
               response_model_exclude_none=True)
async def edr_get_landing_page(request: Request):
    cfg = set_edr_config()
    contact = Contact(**cfg["contact"])
    provider = Provider(**cfg["provider"])

    description = cfg.get("description")
    title = cfg.get("title")
    keywords = cfg.get("keywords")
    base_url = str(request.url_for("edr_get_landing_page"))
    conformance_url = str(request.url_for("edr_get_conformance"))
    collections_url = str(request.url_for("edr_get_collections"))
    links: list[Link] = []
    l = Link(href=base_url, rel="self", type="application/json")
    links.append(l)
    links.append(
        Link(href=conformance_url, rel="conformance", type="application/json"))
    links.append(
        Link(href=collections_url, rel="data", type="application/json"))
    openapi_url = f"{base_url}api"
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


@edrApiApp.get(
    "/api", )
def get_fixed_api():
    api = edrApiApp.openapi()
    for pth in api["paths"].values():
        if "parameters" in pth["get"]:
            for param in pth["get"]["parameters"]:
                if param["in"] == "query" and param["name"] == "datetime":
                    param["style"] = "form"
                    param["explode"] = False
                    param["schema"] = {
                        "type": "string",
                    }
    if "CompactAxis" in api["components"]["schemas"]:
        comp = api["components"]["schemas"]["CompactAxis"]
        if "exclusiveMinimum" in comp["properties"]["num"]:
            comp["properties"]["num"]["exclusiveMinimum"] = False

    return api


@edrApiApp.get("/conformance", response_model=ConformanceModel)
async def edr_get_conformance():
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


def getdimvalsORG(dims, name):
    """
    getdimvals
    """
    for n in dims:
        if list(n.keys())[0] == name:
            return list(n.values())[0]
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

    dt = data
    d1 = list(dt.keys())
    if len(d1) == 0:
        return []
    dimlist[dims[0]] = d1

    if len(dims) >= 2:
        d2 = list(dt[d1[0]].keys())
        dimlist[dims[1]] = d2

    if len(dims) >= 3:
        d3 = list(dt[d1[0]][d2[0]].keys())
        dimlist[dims[2]] = d3

    if len(dims) >= 4:
        d4 = list(dt[d1[0]][d2[0]][d3[0]].keys())
        dimlist[dims[3]] = d4

    if len(dims) >= 5:
        d5 = list(dt[d1[0]][d2[0]][d3[0]][d4[0]].keys())
        dimlist[dims[4]] = d5

    return dimlist


def covjson_from_resp(dats):
    first = True
    fullcovjson = None
    for dat in dats:
        (lon, lat) = dat["point"]["coords"].split(",")
        lat = float(lat)
        lon = float(lon)
        dims = makedims(dat["dims"], dat["data"])
        time_steps = getdimvals(dims, "time")
        vertical_steps = getdimvals(dims, "elevation")
        reference_time = getdimvals(dims, "reference_time")
        if reference_time:
            single_reference_time = reference_time[0]
        else:
            single_reference_time = None
        custom_dims = {
            dim: dims[dim]
            for dim in dims if dim not in
            ["reference_time", "time", "x", "y", "z", "elevation"]
        }

        valstack = []
        # dims_without_time = []
        for dim_name in dims:
            vals = getdimvals(dims, dim_name)
            valstack.append(vals)
        tuples = list(itertools.product(*valstack))
        values = []
        for t in tuples:
            v = multi_get(dat["data"], t)
            if v:
                try:
                    values.append(float(v))
                except ValueError:
                    if v == "nodata":
                        values.append(None)
                    else:
                        values.append(v)

        parameters: Dict(str, CovJsonParameter) = dict()
        ranges = dict()

        param = CovJsonParameter(
            id=dat["name"],
            observedProperty=ObservedProperty(label={"en": dat["name"]}),
        )
        #TODO: add units to CovJsonParameter
        parameters[dat["name"]] = param
        axisNames = ["x", "y", "t"]
        shape = [1, 1, len(time_steps)]
        if vertical_steps:
            axisNames = ["x", "y", "z", "t"]
            shape = [1, 1, len(vertical_steps), len(time_steps)]
        _range = dict(
            axisNames=axisNames,
            shape=shape,
            values=values,
        )
        ranges[dat["name"]] = _range

        axes: dict[str, ValuesAxis] = {
            "x": ValuesAxis(values=[lon]),
            "y": ValuesAxis(values=[lat]),
            "t": ValuesAxis(values=time_steps),
        }

        domainType = "PointSeries"
        if vertical_steps:
            axes["z"] = ValuesAxis(values=vertical_steps)
            if len(vertical_steps) > 1:
                domainType = "VerticalProfile"
        if time_steps:
            axes["t"] = ValuesAxis(values=time_steps)
            if len(time_steps) > 1 and vertical_steps and len(
                    vertical_steps) > 1:
                domainType = "Grid"

        referencing = [
            ReferenceSystemConnectionObject(
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
        domain = Domain(domainType=domainType,
                        axes=axes,
                        referencing=referencing)
        covjson = Coverage(
            id="test",
            domain=domain,
            ranges=ranges,
            parameters=parameters,
        )
        if not fullcovjson:
            fullcovjson = covjson
        else:
            for k, v in covjson.parameters.items():
                fullcovjson.parameters[k] = v
            for k, v in covjson.ranges.items():
                fullcovjson.ranges[k] = v

    return fullcovjson


def set_edr_config():
    config = {}
    config["contact"] = {"email": "info@knmi.nl"}
    config["provider"] = {"name": "KNMI", "url": "https://www.knmi.nl"}
    config["keywords"] = ["HARM_N25", "precipitation"]
    config["description"] = "EDR service for ADAGUC datasets"
    config["title"] = "ADAGUC OGCAPI EDR"
    return config
