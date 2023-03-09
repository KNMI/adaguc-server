from typing import Any, Dict, Optional
from fastapi import APIRouter, Request, Response, FastAPI, Query
import json
import itertools

from owslib.wms import WebMapService
from .setup_adaguc import setup_adaguc
from defusedxml.ElementTree import fromstring, parse, ParseError

from pyproj import CRS, Transformer
import logging
import os
import time
from datetime import datetime

from covjson_pydantic.coverage import Coverage
from covjson_pydantic.parameter import Parameter
from covjson_pydantic.unit import Unit, Symbol
from covjson_pydantic.i18n import i18n
from covjson_pydantic.domain import Domain, ValuesAxis
from covjson_pydantic.observed_property import ObservedProperty
from covjson_pydantic.reference_system import (
    ReferenceSystem,
    ReferenceSystemConnectionObject,
)
from edr_pydantic_classes.generic_models import Link
from edr_pydantic_classes.capabilities import (
    ConformanceModel,
    LandingPageModel,
    Contact,
    Provider,
)

from edr_pydantic_classes.instances import (
    Instance,
    InstancesModel,
    Collection,
    CollectionsModel,
    Extent,
    Spatial,
    Temporal,
    Vertical,
    CRSOptions,
    DataQueries,
    DataQueryLink,
)

from geomet import wkt
from .geojsonresponse import GeoJSONResponse

from .ogcapi_tools import callADAGUC

logger = logging.getLogger(__name__)
logger.debug("Starting EDR")

edrApiApp = FastAPI(debug=True)


def get_datasets(adagucDataSetDir):
    """
    Return all possible OGCAPI EDR datasets, based on the dataset directory
    """
    logger.info("getDatasets(%s)", adagucDataSetDir)
    datasetFiles = [
        f
        for f in os.listdir(adagucDataSetDir)
        if os.path.isfile(os.path.join(adagucDataSetDir, f)) and f.endswith(".xml")
    ]
    datasets = {}
    for datasetFile in datasetFiles:
        try:
            tree = parse(os.path.join(adagucDataSetDir, datasetFile))
            root = tree.getroot()
            for ogcapi in root.iter("OgcApiEdr"):
                logger.info("ogcapi: %s", ogcapi)
                """Note, service is just a placeholder because it is needed by OWSLib. Adaguc is still run as executable, not as service"""
                dataset = {
                    "dataset": datasetFile.replace(".xml", ""),
                    "name": datasetFile.replace(".xml", ""),
                    "title": datasetFile.replace(".xml", "").lower().capitalize(),
                    "service": "http://localhost:8080/wms?DATASET="
                    + datasetFile.replace(".xml", ""),
                }
                datasets[dataset["name"]] = dataset
        except ParseError:
            pass

    return datasets


datasets = get_datasets(os.environ["ADAGUC_DATASET_DIR"])


def extract_dataset_from_request(request: Request):
    return request.url.path.strip("/").split("/")[1]


def extract_baseurl_from_request(request: Request):
    dataset = extract_dataset_from_request(request)
    url = request.url
    return f"{url.scheme}://{url.hostname}:{url.port}/maps/{dataset}"


def extract_parameters_for_dataset(dataset: str) -> list[str]:
    return "air_temperature__at_2m"


def getPointValue(
    dataset: str,
    instance: str,
    coords: list[float],
    parameters: list[str],
    t: list[str],
):
    urlrequest = (
        f"SERVICE=WMS&VERSION=1.3.0&REQUEST=GetPointValue&CRS=EPSG:4326"
        f"&DATASET={dataset}&QUERY_LAYERS={parameters}"
        f"&X={coords[0]}&Y={coords[1]}&INFO_FORMAT=application/json"
    )
    if t:
        urlrequest += f"&TIME={t}"

    if instance:
        urlrequest += f"&DIM_reference_time={instance}"
    logger.info("URL: %s", urlrequest)
    status, response = callADAGUC(url=urlrequest.encode("UTF-8"))
    logger.info("status: %d", status)
    if status == 0:
        return response.getvalue()


@edrApiApp.get(
    "/collections/{dataset}/position",
    response_model=Coverage,
    response_model_exclude_none=True,
)
async def get_collection_position(
    dataset: str,
    #    instance: str,
    request: Request,
    coords: str,
    datetime: Optional[str] = None,
    parameter_name: str = Query(alias="parameter-name"),
):
    latlons = wkt.loads(coords)
    logger.info("latlons:%s", latlons)
    coord = {"lat": latlons["coordinates"][0], "lon": latlons["coordinates"][1]}
    instance = None
    resp = getPointValue(
        dataset, None, [coord["lon"], coord["lat"]], parameter_name, datetime
    )
    if resp:
        dat = json.loads(resp)
        return GeoJSONResponse(covjson_from_resp(dat))
    return None


@edrApiApp.get(
    "/collections/{dataset}/instances/{instance}/position",
    response_model=Coverage,
    response_model_exclude_none=True,
)
async def get_collection_instance_position(
    dataset: str,
    instance: str,
    request: Request,
    coords: str,
    datetime: Optional[str] = None,
    parameter_name: str = Query(alias="parameter-name"),
):
    latlons = wkt.loads(coords)
    coord = {"lat": latlons["coordinates"][0], "lon": latlons["coordinates"][1]}
    resp = getPointValue(
        dataset, instance, [coord["lon"], coord["lat"]], parameter_name, datetime
    )
    dat = json.loads(resp)
    return GeoJSONResponse(covjson_from_resp(dat))


def get_collectioninfo_for_id(dataset: str, base_url: str) -> Collection:
    links: list[Link] = []
    links.append(Link(href=f"{base_url}", rel="self", type="application/json"))
    bbox = get_extent(dataset)
    print("bbox:", bbox)
    crs = CRSOptions.wgs84
    spatial = Spatial(bbox=bbox, crs=crs)
    extent = Extent(spatial=spatial)
    position_link = Link(
        href=f"{base_url}/position", rel="data", type="application/geo+json"
    )
    links.append(position_link)
    position_query_link = DataQueryLink(link=position_link)
    ref_times = get_reference_times_for_dataset(dataset, None, None)
    instance_query_link = None
    if ref_times and len(ref_times) > 0:
        instance_link = Link(
            href=f"{base_url}/instances", rel="instance", type="application/json"
        )
        links.append(instance_link)
        instance_query_link = DataQueryLink(link=instance_link)
    data_queries = DataQueries(
        position=position_query_link, instances=instance_query_link
    )
    parameter_names: dict(str, Parameter) = {
        "air_temperature": Parameter(
            id="air_temperature",
            observedProperty=ObservedProperty(id="temp", label={"en": "temperature"}),
            type="Parameter",
            unit=Unit(symbol="C"),
            title="air_temperature",
        ),
    }

    crs = ["EPSG:4326"]

    output_formats = ["CoverageJSON", "GeoJSON"]
    collection = Collection(
        links=links,
        id=dataset,
        extent=extent,
        data_queries=data_queries,
        parameter_names=parameter_names,
        crs=crs,
        output_formats=output_formats,
    )
    return collection


@edrApiApp.get(
    "/collections", response_model=CollectionsModel, response_model_exclude_none=True
)
async def get_collections(request: Request):
    links: list[Link] = []
    base_url = request.url_for("get_collections")
    self_link = Link(href=request.url_for("get_collections"), rel="self", type="application/json")
    links.append(self_link)
    collections: list[Collection] = []
    for ds in datasets.keys():
        print("ds:", ds)
        coll = get_collectioninfo_for_id(ds, f"{base_url}/{ds}")
        collections.append(coll)
    collections_data = CollectionsModel(links=links, collections=collections)
    return collections_data


@edrApiApp.get("/collections/{dataset}", response_model=Collection)
async def get_collection_by_id(dataset: str, request: Request):
    base_url = request.url_for("get_collection_by_id", dataset=dataset)

    collection = get_collectioninfo_for_id(dataset, base_url)

    return collection


def generate_collections():
    """
    Generate OGC API Feature collections
    """
    collections = get_datasets(os.environ.get("ADAGUC_DATASET_DIR"))
    return collections


def get_capabilities(collname):
    """
    Get the collectioninfo from the WMS GetCapabilities
    """
    coll = generate_collections().get(collname)
    if "dataset" in coll:
        logger.info("callADAGUC by dataset")
        dataset = coll["dataset"]
        urlrequest = (
            f"dataset={dataset}&service=wms&version=1.3.0&request=getcapabilities"
        )
        status, response = callADAGUC(url=urlrequest.encode("UTF-8"))
        logger.info("status: %d", status)
        if status == 0:
            xml = response.getvalue()
            wms = WebMapService(coll["service"], xml=xml, version="1.3.0")
        else:
            logger.error("status: %d", status)
            return {}
    else:
        logger.info("callADAGUC by service %s", coll)
        wms = WebMapService(coll["service"], version="1.3.0")
    return wms.contents


def get_reference_times_for_dataset(
    dataset: str, wms_url: str, layer: str
) -> list[str]:
    url = f"{wms_url}?DATASET={dataset}&SERVICE=WMS&VERSION=1.3.0&request=getreferencetimes&LAYER={layer}"
    logger.info("getreftime_url: %s", wms_url)
    if dataset == "HARM_N25":
        return ["2022-06-30T06:00:00Z", "2022-06-30T09:00:00Z"]
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


@edrApiApp.get("/collections/{dataset}/instances", response_model=InstancesModel)
async def get_collection_instances_by_id(dataset: str, request: Request):
    base_url = request.url_for("get_collection_instances_by_id", dataset=dataset)
    instances: list(Collection) = []
    wms_url = request.url_for("get_landing_page").replace("/edr", "/wms")
    ref_times = get_reference_times_for_dataset(dataset, wms_url, "precipitation")
    links: list(Link) = []
    links.append(Link(href=base_url, rel="self"))
    extent = Extent()
    for ref_time in ref_times:
        instance_links: list(Link) = []
        instance_link = Link(href=f"{base_url}/{ref_time}", rel="self")
        instance_links.append(instance_link)
        instance = Collection(
            id=ref_time, links=instance_links, extent=extent, crs=CRSOptions.wgs84
        )
        instances.append(instance)
    instances_data = InstancesModel(instances=instances, links=links)
    return instances_data


@edrApiApp.get("/collections/{dataset}/instances/{instance}", response_model=Collection)
async def get_collection_instance_by_id_and_dataset(
    dataset: str, instance, request: Request
):
    base_url = request.url_for(
        "get_collection_instance_by_id_and_dataset", dataset=dataset, instance=instance
    )
    coll = get_collectioninfo_for_id(dataset, base_url)
    return coll


@edrApiApp.get("/", response_model=LandingPageModel, response_model_exclude_none=True)
async def get_landing_page(request: Request):
    cfg = set_edr_config()
    contact = Contact(**cfg["contact"])
    provider = Provider(**cfg["provider"])

    description = cfg.get("description")
    title = cfg.get("title")
    keywords = cfg.get("keywords")
    base_url = request.url_for("get_landing_page")
    conformance_url = request.url_for("get_conformance")
    collections_url = request.url_for("get_collections")
    links: list[Link] = []
    l = Link(href=base_url, rel="self", type="application/json")
    links.append(l)
    links.append(Link(href=conformance_url, rel="conformance", type="application/json"))
    links.append(Link(href=collections_url, rel="data", type="application/json"))
    openapi_url = f"{base_url}api"
    links.append(
        Link(
            href=openapi_url,
            rel="service-desc",
            type="application/vnd.oai.openapi+json;version=3.0",
        )
    )

    landing_page = LandingPageModel(
        links=links,
        contact=contact,
        provider=provider,
        keywords=keywords,
        description=description,
        title=title,
    )
    return landing_page


conformance = ConformanceModel(
    conformsTo=[
        "http://www.opengis.net/spec/ogcapi-edr-1/1.0/conf/core",
        "http://www.opengis.net/spec/ogcapi-common-1/1.0.conf/core",
        "http://www.opengis.net/spec/ogcapi-common-2/1.0/conf/collections",
        "http://www.opengis.net/spec/ogcapi-edr-1/1.0/conf/oas30",
        "http://www.opengis.net/spec/ogcapi-edr-1/1.0/conf/geojson",
        "http://www.opengis.net/spec/ogcapi-edr-1/1.0/conf/queries",
    ]
)


@edrApiApp.get(
    "/api",
)
def getFixedApi():
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
async def get_conformance():
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
    for n in dims:
        if list(n.keys())[0] == name:
            return list(n.values())[0]
    return None


def makedims(dims, data):
    """
    Makedims
    """
    dimlist = []
    if isinstance(dims, str) and dims == "time":
        times = list(data.keys())
        dimlist.append({"time": times})
        return dimlist

    dt = data
    d1 = list(dt.keys())
    if len(d1) == 0:
        return []
    dimlist.append({dims[0]: d1})

    if len(dims) >= 2:
        d2 = list(dt[d1[0]].keys())
        dimlist.append({dims[1]: d2})

    if len(dims) >= 3:
        d3 = list(dt[d1[0]][d2[0]].keys())
        dimlist.append({dims[2]: d3})

    if len(dims) >= 4:
        d4 = list(dt[d1[0]][d2[0]][d3[0]].keys())
        dimlist.append({dims[2]: d4})

    if len(dims) >= 5:
        d5 = list(dt[d1[0]][d2[0]][d3[0]][d4[0]].keys())
        dimlist.append({dims[2]: d5})

    return dimlist


def covjson_from_resp(dat):
    print("DAT:", dat)
    (lon, lat) = dat[0]["point"]["coords"].split(",")
    lat = float(lat)
    lon = float(lon)
    times = dat[0]
    dims = makedims(dat[0]["dims"], dat[0]["data"])
    timeSteps = getdimvals(dims, "time")
    reference_time = getdimvals(dims, "reference_time")[0]

    values = []
    for t in timeSteps:
        values.append(float(dat[0]["data"][reference_time][t]))
    print("VAL:", values)

    parameters: Dict(str, Parameter) = dict()
    ranges = dict()
    for dt in dat:
        values = []
        for t in timeSteps:
            values.append(float(dt["data"][reference_time][t]))
        param = Parameter(
            id=dt["name"], observedProperty=ObservedProperty(label={"en": dt["name"]})
        )
        parameters[dt["name"]] = param
        _range = dict(
            axisNames=["x", "y", "t"],
            shape=[1, 1, len(timeSteps)],
            values=values,
        )
        print("_range: ", _range)
        ranges[dt["name"]] = _range

    axes: dict[str, ValuesAxis] = {
        "x": ValuesAxis(values=[lon]),
        "y": ValuesAxis(values=[lat]),
        "t": ValuesAxis(values=timeSteps),
    }
    referencing = [
        ReferenceSystemConnectionObject(
            system=ReferenceSystem(
                type="GeographicCRS", id="http://www.opengis.net/def/crs/OGC/1.3/CRS84"
            ),
            coordinates=["x", "y"],
        ),
        ReferenceSystemConnectionObject(
            system=ReferenceSystem(type="TemporalRS", calendar="Gregorian"),
            coordinates=["t"],
        ),
    ]
    domain = Domain(domainType="PointSeries", axes=axes, referencing=referencing)
    covjson = Coverage(
        id="test",
        domain=domain,
        ranges=ranges,
        parameters=parameters,
    )
    return covjson


def feature_from_dat(dat, observedPropertyName, name):
    """
    feature_from_dat
    """
    print("DAT:", dat)
    print("dims:", dat["dims"])
    dims = makedims(dat["dims"], dat["data"])
    timeSteps = getdimvals(dims, "time")
    if not timeSteps or len(timeSteps) == 0:
        return []

    valstack = []
    dims_without_time = []
    for d in dims:
        dim_name = list(d.keys())[0]
        if dim_name != "time":
            dims_without_time.append(d)
            vals = getdimvals(dims, dim_name)
            valstack.append(vals)
    tuples = list(itertools.product(*valstack))

    features = []
    logger.info(
        "TUPLES: %s [%d] %s",
        json.dumps(tuples),
        len(tuples),
        json.dumps(dims_without_time),
    )

    for t in tuples:
        logger.info("T:%s", t)
        result = []
        for ts in timeSteps:
            v = multi_get(dat["data"], t + (ts,))
            if v:
                try:
                    value = float(v)
                    result.append(value)
                except ValueError:
                    result.append(v)

        feature_dims = {}
        datname = dat["name"]
        datpointcoords = dat["point"]["coords"]
        feature_id = f"{name};{datname};{datpointcoords}"
        i = 0
        for dim_value in t:
            feature_dims[list(dims_without_time[i].keys())[0]] = dim_value
            # pylint: disable=consider-using-f-string
            feature_id = feature_id + ";%s=%s" % (
                list(dims_without_time[i].keys())[0],
                dim_value,
            )
            i = i + 1

        feature_id = feature_id + f";{timeSteps[0]}${timeSteps[-1]}"
        if len(feature_dims) == 0:
            properties = {
                "timestep": timeSteps,
                "observationType": "MeasureTimeseriesObservation",
                "observedPropertyName": observedPropertyName,
                "result": result,
            }
        else:
            properties = {
                "timestep": timeSteps,
                "dims": feature_dims,
                "observationType": "MeasureTimeseriesObservation",
                "observedPropertyName": observedPropertyName,
                "result": result,
            }

        coords = dat["point"]["coords"].split(",")
        coords[0] = float(coords[0])
        coords[1] = float(coords[1])
        feature = {
            "type": "Feature",
            "geometry": {"type": "Point", "coordinates": coords},
            "properties": properties,
            "id": feature_id,
        }
        features.append(feature)
    return features


def set_edr_config():
    config = {}
    config["contact"] = {"email": "info@knmi.nl"}
    config["provider"] = {"name": "KNMI", "url": "https://www.knmi.nl"}
    config["keywords"] = ["HARM_N25", "precipitation"]
    config["description"] = "EDR service for ADAGUC datasets"
    config["title"] = "ADAGUC OGCAPI EDR"
    return config
