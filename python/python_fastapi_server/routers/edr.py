import itertools
import json
import logging
import os
import re
from datetime import datetime, timezone
from typing import Dict, Optional, Tuple

from cachetools import TTLCache, cached
from covjson_pydantic.coverage import Coverage
from covjson_pydantic.domain import Domain, ValuesAxis
from covjson_pydantic.observed_property import ObservedProperty
from covjson_pydantic.parameter import Parameter as CovJsonParameter
from covjson_pydantic.reference_system import (ReferenceSystem,
                                               ReferenceSystemConnectionObject)
from defusedxml.ElementTree import ParseError, parse
from edr_pydantic_classes.capabilities import (ConformanceModel, Contact,
                                               LandingPageModel, Provider)
from edr_pydantic_classes.generic_models import (CRSOptions, Custom, Link,
                                                 ObservedPropertyCollection,
                                                 ParameterName, Spatial,
                                                 Temporal, Units, Vertical)
from edr_pydantic_classes.instances import (Collection, CollectionsModel,
                                            CrsObject, DataQueries, Extent,
                                            Instance, InstancesDataQueryLink,
                                            InstancesLink, InstancesModel,
                                            InstancesVariables,
                                            PositionDataQueryLink,
                                            PositionLink, PositionVariables)
from fastapi import FastAPI, Query, Request
from fastapi.responses import JSONResponse
from geomet import wkt
from owslib.wms import WebMapService

from .covjsonresponse import CovJSONResponse
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

def init_edr_collections(
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
                    skip_dims = edr_collection.attrib.get("skip_dims")
                    if skip_dims is not None:
                        skip_dims = edr_collection.attrib.get("skip_dims").split(",")
                    edr_collections[edr_collection.attrib.get("name")] = {
                        "dataset":
                        dataset,
                        "name":
                        edr_collection.attrib.get("name"),
                        "service":
                        "http://localhost:8000/wms",
                        "time_interval":
                        edr_collection.attrib.get("time_interval"),
                        "parameters": [
                            inst.strip()
                            for inst in edr_collection.text.strip().split(",")
                        ],
                        "vertical_name": edr_collection.attrib.get("z"),
                        "skip_dims": skip_dims,
                        "base_url": f"http://localhost:8080/edr/collections/{edr_collection.attrib.get('name')}"
                    }
        except ParseError:
            pass
    return edr_collections

global_edr_collections=init_edr_collections()

def get_edr_collections():
    return global_edr_collections

def get_point_value(
    edr_collectioninfo: dict,
    instance: str,
    coords: list[float],
    parameters: list[str],
    datetime_par: str,
    z_par: str = None,
    custom_dims: str=None,
):
    dataset=edr_collectioninfo['dataset']
    urlrequest = (
        f"SERVICE=WMS&VERSION=1.3.0&REQUEST=GetPointValue&CRS=EPSG:4326"
        f"&DATASET={dataset}&QUERY_LAYERS={','.join(parameters)}"
        f"&X={coords[0]}&Y={coords[1]}&INFO_FORMAT=application/json")
    if datetime_par:
        urlrequest += f"&TIME={datetime_par}"

    if instance:
        urlrequest += f"&DIM_reference_time={instance_to_iso(instance)}"
    if z_par:
        if "vertical_name" in edr_collectioninfo and edr_collectioninfo["vertical_name"]!="ELEVATION":
          urlrequest += f"&DIM_{edr_collectioninfo['vertical_name']}={z_par}"
        else:
          urlrequest += f"&ELEVATION={z_par}"
    if custom_dims:
        urlrequest += custom_dims

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
@edrApiApp.get(
    "/collections/{collection_name}/instances/{instance}/position",
    response_model=Coverage,
    response_model_exclude_none=True,
)
async def get_collection_position(
        collection_name: str,
        request: Request,
        coords: str,
        instance:  Optional[str] = None,
        datetime_par: str = Query(default=None, alias="datetime"),
        parameter_name: str = Query(alias="parameter-name"),
        z_par: str=Query(alias="z", default=None),
        crs: str=Query(default=None),
        format: str=Query(default=None, alias="f")
):
    allowed_params = ["coords", "datetime", "parameter-name", "z", "f", "crs"]
    custom_dims = [k for k in request.query_params if k not in allowed_params]
    custom_dims = ""
    if len(custom_dims):
        for custom_dim in custom_dims:
            custom_dims += f"&DIM_{custom_dim}={request.query_params[custom_dim]}"
    edr_collections = get_edr_collections()

    parameter_names = parameter_name.split(",")
    latlons = wkt.loads(coords)
    logger.info("latlons:%s", latlons)
    coord = {
        "lat": latlons["coordinates"][1],
        "lon": latlons["coordinates"][0]
    }
    resp = get_point_value(
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
        return CovJSONResponse(covjson_from_resp(dat, edr_collections[collection_name]['vertical_name']))

    raise EdrException(code=400, description="No data")



DEFAULT_CRS_OBJECT = {
    "crs":
    "EPSG:4326",
    "wkt":
    'GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4326"]]',
}


@cached(cache=TTLCache(maxsize=1024, ttl=60))
def get_collectioninfo_for_id(
    edr_collection: str,
    instance: str = None,
) -> Collection:
    logger.info("get_collection_info_for_id(%s, %s)", edr_collection, instance)

    edr_collectioninfo = get_edr_collections()[edr_collection]
    dataset = edr_collectioninfo["dataset"]
    logger.info("%s=>%s", edr_collection, dataset)

    base_url = edr_collectioninfo["base_url"]
    if instance is not None:
        base_url += f"/instances/{instance}"

    links: list[Link] = []
    links.append(Link(href=f"{base_url}", rel="collection", type="application/json"))

    ref_times = None
    if not instance:
        ref_times = get_reference_times_for_collection(
            edr_collectioninfo, edr_collectioninfo["parameters"][0])
        if ref_times and len(ref_times) > 0:
            instances_link = Link(href=f"{base_url}/instances",
                                 rel="collection",
                                 type="application/json")
            links.append(instances_link)

    bbox = get_extent(edr_collectioninfo)
    crs = CRSOptions.wgs84
    spatial = Spatial(bbox=bbox, crs=crs)
    (interval,
     time_values) = get_times_for_collection(edr_collectioninfo,
                                          edr_collectioninfo["parameters"][0])

    customlist:list = get_custom_dims_for_collection(edr_collectioninfo,
                                             edr_collectioninfo["parameters"][0])
    custom = []
    if customlist is not None:
        for custom_el in customlist:
            custom.append(Custom(**custom_el))

    vertical = None
    vertical_dim = get_vertical_dim_for_collection(
        edr_collectioninfo, edr_collectioninfo["parameters"][0])
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
        param = ParameterName(
            id=param_name,
            observedProperty=ObservedPropertyCollection(id=param_name,
                                                        label=param_name),
            type="Parameter",
            unit=Units(symbol="mm"), #TODO Find real untis
            label=param_name,
        )
        parameter_names[param_name] = param
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
        edr_collectioninfo: Dict,
        parameter: str = None) -> Tuple[list[list[str]], list[str]]:
    logger.info("get_times_for_dataset(%s,%s)", edr_collectioninfo['name'], parameter)
    wms = get_capabilities(edr_collectioninfo['name'])
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
        interval = [[
            datetime.strptime(time_dim["values"][0], "%Y-%m-%dT%H:%M:%SZ"),
            datetime.strptime(time_dim["values"][-1],
                                "%Y-%m-%dT%H:%M:%SZ"),
        ]]
        return interval, time_dim["values"]
    return (None, None)


def get_custom_dims_for_collection(edr_collectioninfo: dict, parameter: str = None):
    wms = get_capabilities(edr_collectioninfo['name'])
    custom = []
    if parameter and parameter in list(wms):
        layer = wms[parameter]
    else:
        # default to first layer
        layer = wms[list(wms)[0]]
    for dim_name in layer.dimensions:
        if dim_name not in ["reference_time", "time", "elevation", edr_collectioninfo.get("vertical_name")]:
            custom_dim = {
                "id": dim_name,
                "interval": [],
                "values": layer.dimensions[dim_name]["values"],
                "reference": f"custom_{dim_name}",
            }
            custom.append(custom_dim)
    return custom if len(custom) > 0 else None


def get_vertical_dim_for_collection(edr_collectioninfo: dict, parameter: str = None):
    wms = get_capabilities(edr_collectioninfo['name'])
    if parameter and parameter in list(wms):
        layer = wms[parameter]
    else:
        layer = wms[list(wms)[0]]

    for dim_name in layer.dimensions:
        if dim_name in ["elevation"] or ( "vertical_name" in edr_collectioninfo and dim_name==edr_collectioninfo["vertical_name"]):
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

    links.append(self_link)
    collections: list[Collection] = []
    edr_collections = get_edr_collections()
    for edr_coll in edr_collections:
        coll = get_collectioninfo_for_id(edr_coll)
        collections.append(coll)
    collections_data = CollectionsModel(links=links, collections=collections)
    return collections_data


@edrApiApp.get("/collections/{collection_name}",
               response_model=Collection,
               response_model_exclude_none=True)
async def edr_get_collection_by_id(collection_name: str, request: Request):
    base_url = str(request.url_for("edr_get_collection_by_id",
                               collection_name=collection_name))

    collection = get_collectioninfo_for_id(collection_name)

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


def get_reference_times_for_collection(edr_collectioninfo: dict,
                                    layer: str) -> list[str]:
    dataset=edr_collectioninfo["dataset"]
    url = f"?DATASET={dataset}&SERVICE=WMS&VERSION=1.3.0&request=getreferencetimes&LAYER={layer}"
    logger.info("getreftime_url(%s,%s): %s", dataset, layer, url)
    status, response = call_adaguc(url=url.encode("UTF-8"))
    if status == 0:
        ref_times = json.loads(response.getvalue())
        instance_ids=[parse_iso(reft).strftime("%Y%m%d%H%M") for reft in ref_times]
        return instance_ids
    return []


def get_extent(edr_collectioninfo: dict):
    """
    Get the boundingbox extent from the WMS GetCapabilities
    """
    contents = get_capabilities(edr_collectioninfo['name'])
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

    ref_times = get_reference_times_for_collection(
        edr_collections[collection_name], edr_collections[collection_name]["parameters"][0])
    links: list(Link) = []
    links.append(Link(href=base_url, rel="collection"))
    for instance in list(ref_times):
        instance_links: list(Link) = []
        instance_link = Link(href=f"{base_url}/{instance}", rel="collection")
        instance_links.append(instance_link)
        instance_info = get_collectioninfo_for_id(
            collection_name,
            instance
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
    coll = get_collectioninfo_for_id(collection_name,
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


def covjson_from_resp(dats, vertical_name):
    fullcovjson = None
    for dat in dats:
        (lon, lat) = dat["point"]["coords"].split(",")
        lat = float(lat)
        lon = float(lon)
        dims = makedims(dat["dims"], dat["data"])
        time_steps = getdimvals(dims, "time")
        if vertical_name is not None:
            vertical_steps = getdimvals(dims, vertical_name)
        else:
            vertical_steps = getdimvals(dims, "elevation")
        custom_dims = {
            dim: dims[dim]
            for dim in dims if dim not in
            ["reference_time", "time", "x", "y", "z", "elevation", vertical_name]
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
            "x": ValuesAxis(values=[lon]),
            "y": ValuesAxis(values=[lat]),
            "t": ValuesAxis(values=time_steps),
        }

        domain_type = "PointSeries"
        if vertical_steps:
            axes["z"] = ValuesAxis(values=vertical_steps)
            if len(vertical_steps) > 1:
                domain_type = "VerticalProfile"
        if time_steps:
            axes["t"] = ValuesAxis(values=time_steps)
            if len(time_steps) > 1 and vertical_steps and len(
                    vertical_steps) > 1:
                domain_type = "Grid"

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
        domain = Domain(domainType=domain_type,
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
