from typing import Any, Dict, Optional, Tuple
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

from .ogcapi_tools import callADAGUC

logger = logging.getLogger(__name__)
logger.debug("Starting EDR")

edrApiApp = FastAPI(debug=True)


def get_datasets(adagucDataSetDir: str = os.environ["ADAGUC_DATASET_DIR"]):
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


def get_subdatasets_for_dataset(
    dataset: str, dataset_dir: str = os.environ["ADAGUC_DATASET_DIR"]
):
    fn = f"{dataset_dir}/{dataset}.xml"

    subdatasets = {}
    tree = parse(fn)
    root = tree.getroot()
    for ogcapi_edr in root.iter("OgcApiEdr"):
        for edr_instance in ogcapi_edr.iter("EdrInstance"):
            subdatasets[edr_instance.attrib.get("name")] = {
                "dataset": dataset,
                "name": edr_instance.attrib.get("name"),
                "time_interval": edr_instance.attrib.get("time_interval"),
                "z_interval": edr_instance.attrib.get("z_interval"),
                "parameters": [
                    inst.strip() for inst in edr_instance.text.strip().split(",")
                ],
            }
    return subdatasets


def extract_dataset_from_request(request: Request):
    return request.url.path.strip("/").split("/")[1]


def extract_baseurl_from_request(request: Request):
    dataset = extract_dataset_from_request(request)
    url = request.url
    return f"{url.scheme}://{url.hostname}:{url.port}/maps/{dataset}"


def extract_parameters_for_dataset(dataset: str) -> list[str]:
    return ["air_temperature__at_2m"]


def getPointValue(
    dataset: str,
    instance: str,
    coords: list[float],
    parameters: list[str],
    t: list[str],
    z: str = None,
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
    if z:
        urlrequest += f"&ELEVATION={z}"

    logger.info("URL: %s", urlrequest)
    status, response = callADAGUC(url=urlrequest.encode("UTF-8"))
    logger.info("status: %d", status)
    if status == 0:
        return response.getvalue()


@edrApiApp.get(
    "/collections/{collection_name}/position",
    response_model=Coverage,
    response_model_exclude_none=True,
)
async def get_collection_position(
    collection_name: str,
    #    instance: str,
    request: Request,
    coords: str,
    datetime: Optional[str] = None,
    parameter_name: str = Query(alias="parameter-name"),
):
    dataset = collection_name.split("-")[0]
    latlons = wkt.loads(coords)
    logger.info("latlons:%s", latlons)
    coord = {"lat": latlons["coordinates"][1], "lon": latlons["coordinates"][0]}
    resp = getPointValue(
        dataset, None, [coord["lon"], coord["lat"]], parameter_name, datetime
    )
    if resp:
        dat = json.loads(resp)
        return GeoJSONResponse(covjson_from_resp(dat))
    return None


@edrApiApp.get(
    "/collections/{collection_name}/instances/{instance}/position",
    response_model=Coverage,
    response_model_exclude_none=True,
)
async def get_collection_instance_position(
    collection_name: str,
    instance: str,
    request: Request,
    coords: str,
    datetime: Optional[str] = None,
    parameter_name: str = Query(alias="parameter-name"),
    z: Optional[str] = None,
):
    dataset = collection_name.split("-")[0]

    latlons = wkt.loads(coords)
    coord = {"lat": latlons["coordinates"][1], "lon": latlons["coordinates"][0]}
    resp = getPointValue(
        dataset, instance, [coord["lon"], coord["lat"]], parameter_name, datetime, z
    )
    dat = json.loads(resp)
    return GeoJSONResponse(covjson_from_resp(dat))


DEFAULT_CRS_OBJECT = {
    "crs": "EPSG:4326",
    "wkt": 'GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4326"]]',
}


def get_collectioninfo_for_id(
    collection_name: str,
    base_url: str,
    dataset: str,
    subdataset: str = None,
    instance: str = None,
) -> Collection:
    links: list[Link] = []
    links.append(Link(href=f"{base_url}", rel="self", type="application/json"))
    ref_times = None
    if not instance:
        ref_times = get_reference_times_for_dataset(dataset, None, None)
        if ref_times and len(ref_times) > 0:
            instance_link = Link(
                href=f"{base_url}/instances", rel="collection", type="application/json"
            )
            links.append(instance_link)

    bbox = get_extent(dataset)  # TODO from subdataset?
    print("bbox:", bbox)
    crs = CRSOptions.wgs84
    spatial = Spatial(bbox=bbox, crs=crs)
    (interval, time_values) = get_times_for_dataset(dataset)

    print("T:", interval, time_values)
    customlist = get_custom_dims_for_dataset(dataset)
    print("CUSTOM1:", customlist)
    custom = []
    if customlist:
        for c in customlist:
            custom.append(Custom(**c))
    print("CUSTOM2:", customlist)
    temporal = Temporal(
        interval=interval,  # [["2022-06-30T09:00:00Z", "2022-07-02T06:00:00Z"]],
        trs='TIMECRS["DateTime",TDATUM["Gregorian Calendar"],CS[TemporalDateTime,1],AXIS["Time (T)",future]',
        values=time_values,  # ["R49/2022-06-30T06:00:00/PT60M"],
    )

    extent = Extent(
        spatial=spatial,
        temporal=temporal,
        custom=custom,
    )

    crs_object = CrsObject(
        crs="EPSG:4326",
        wkt='GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4326"]]',
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

    data_queries = DataQueries(
        position=position_data_query_link, instances=instances_data_query_link
    )
    parameter_names = get_parameters_for_subdataset(dataset, subdataset=subdataset)

    crs = ["EPSG:4326"]

    dataset_name = dataset
    if subdataset:
        dataset_name = subdataset

    output_formats = ["CoverageJSON", "GeoJSON"]
    collection = Collection(
        links=links,
        id=instance if instance else collection_name,
        extent=extent,
        data_queries=data_queries,
        parameter_names=parameter_names,
        crs=crs,
        output_formats=output_formats,
    )
    return collection


def get_parameters_for_subdataset(
    dataset: str, subdataset: str
) -> dict[str, ParameterName]:
    parameter_names = dict()
    if subdataset:
        subdatasets = get_subdatasets_for_dataset(dataset)
        for param_name in subdatasets[subdataset]["parameters"]:
            p = ParameterName(
                id=param_name,
                observedProperty=ObservedPropertyCollection(
                    id=param_name, label="title: " + param_name
                ),
                type="Parameter",
                unit=Units(symbol="mm"),
                label="title: " + param_name,
            )
            parameter_names[param_name] = p
    else:
        parameter_names = get_parameters_for_dataset(dataset)
    return parameter_names


def get_parameters_for_dataset(dataset: str) -> dict[str, ParameterName]:
    wms = get_capabilities(dataset)
    parameter_names = dict()
    for l in list(wms):
        layer = wms[l]
        p = ParameterName(
            id=layer.name,
            observedProperty=ObservedPropertyCollection(
                id=layer.name, label=layer.title
            ),
            type="Parameter",
            unit=Units(symbol="mm"),
            label=layer.title,
        )
        parameter_names[layer.name] = p
    return parameter_names


def get_times_for_dataset(
    dataset: str, parameter: str = None
) -> Tuple[list[list[str]], list[str]]:
    wms = get_capabilities(dataset)
    if parameter and parameter in wms:
        layer = wms[parameter]
    else:
        layer = wms[list(wms)[0]]

    if "time" in layer.dimensions:
        time_dim = layer.dimensions["time"]
        print("timedim:", time_dim, time_dim["values"])
        terms = time_dim["values"][0].split("/")
        interval = [
            [
                datetime.strptime(terms[0], "%Y-%m-%dT%H:%M:%SZ"),
                datetime.strptime(terms[1], "%Y-%m-%dT%H:%M:%SZ"),
            ]
        ]
        return (interval, [f"R49/{terms[0]}/PT60M"])
    return (None, None)


def get_custom_dims_for_dataset(dataset: str, parameter: str = None):
    wms = get_capabilities(dataset)
    custom = []
    if parameter and parameter in list(wms):
        layer = wms[parameter]
    else:
        layer = wms[list(wms)[0]]
        print("LAYER:", parameter, layer)
    for dim_name in layer.dimensions:
        if dim_name not in ["reference_time", "time"]:
            print("CDIM for ", layer.name, layer.dimensions[dim_name])
            custom_dim = {
                "id": dim_name,
                "interval": [],
                "values": layer.dimensions[dim_name]["values"],
                "reference": "custom",
            }
            custom.append(custom_dim)
    return custom if len(custom) > 0 else None


def get_parameters_for_dataset_fixed(dataset: str) -> dict[str, ParameterName]:
    if dataset == "RADAR":
        parameter_names = {
            "precipitation": ParameterName(
                id="precipitation",
                observedProperty=ObservedPropertyCollection(
                    id="precip", label="precipitation"
                ),
                type="Parameter",
                unit=Units(symbol="mm"),
                label="precipitation",
            )
        }
        return parameter_names
    elif dataset == "HARM_N25":
        parameter_names = {
            "air_temperature__at_2m": ParameterName(
                id="air_temperature__at_2m",
                observedProperty=ObservedPropertyCollection(
                    id="temp", label="temperature"
                ),
                type="Parameter",
                unit=Units(symbol="C"),
                label="air_temperature__at_2m",
            ),
        }
        return parameter_names


@edrApiApp.get(
    "/collections", response_model=CollectionsModel, response_model_exclude_none=True
)
async def get_collections(request: Request):
    links: list[Link] = []
    base_url = request.url_for("get_collections")
    self_link = Link(
        href=request.url_for("get_collections"), rel="self", type="application/json"
    )

    datasets = get_datasets()

    links.append(self_link)
    collections: list[Collection] = []
    for ds in datasets.keys():
        subdatasets = get_subdatasets_for_dataset(ds)
        print("DS:", ds, "SDS:", subdatasets)
        if len(subdatasets) > 0:
            for sds in subdatasets:
                print("ds:", ds, "sds:", sds)
                coll = get_collectioninfo_for_id(sds, f"{base_url}/{sds}", ds, sds)
                collections.append(coll)
        else:
            coll = get_collectioninfo_for_id(ds, f"{base_url}/{ds}", ds, None)
            collections.append(coll)
    collections_data = CollectionsModel(links=links, collections=collections)
    return collections_data


@edrApiApp.get("/collections/{collection_name}", response_model=Collection)
async def get_collection_by_id(collection_name: str, request: Request):
    base_url = request.url_for("get_collection_by_id", collection_name=collection_name)

    dataset = collection_name.split("-")[0]
    subdataset = collection_name
    if collection_name == dataset:
        subdataset = None
    collection = get_collectioninfo_for_id(
        collection_name, base_url, dataset, subdataset
    )

    return collection


def get_capabilities(collname):
    """
    Get the collectioninfo from the WMS GetCapabilities
    """
    datasets = get_datasets().get(collname)
    if "dataset" in datasets:
        logger.info("callADAGUC by dataset")
        dataset = datasets["dataset"]
        urlrequest = (
            f"dataset={dataset}&service=wms&version=1.3.0&request=getcapabilities"
        )
        status, response = callADAGUC(url=urlrequest.encode("UTF-8"))
        logger.info("status: %d", status)
        if status == 0:
            xml = response.getvalue()
            wms = WebMapService(datasets["service"], xml=xml, version="1.3.0")
        else:
            logger.error("status: %d", status)
            return {}
    else:
        logger.info("callADAGUC by service %s", dataset)
        wms = WebMapService(dataset["service"], version="1.3.0")
    return wms.contents


def get_reference_times_for_dataset(
    dataset: str, wms_url: str, layer: str
) -> list[str]:
    url = f"{wms_url}?DATASET={dataset}&SERVICE=WMS&VERSION=1.3.0&request=getreferencetimes&LAYER={layer}"
    logger.info("getreftime_url(%s,%s): %s", dataset, layer, wms_url)
    if dataset.startswith("HARM_N25"):
        return ["2022-06-30T06:00:00Z", "2022-06-30T09:00:00Z"]
    return []


def get_extent(coll):
    """
    Get the boundingbox extent from the WMS GetCapabilities
    """
    print(f"get_extent({coll})")
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
async def get_collection_instances_for_dataset(collection_name: str, request: Request):
    print(f"get_collection_instances_for_dataset({collection_name})")
    dataset = collection_name.split("-")[0]
    base_url = request.url_for(
        "get_collection_instances_for_dataset", collection_name=collection_name
    )
    instances: list(Instance) = []
    wms_url = request.url_for("get_landing_page").replace("/edr", "/wms")
    subdatasets = get_subdatasets_for_dataset(dataset)

    ref_times = get_reference_times_for_dataset(
        dataset, wms_url, subdatasets[collection_name]["parameters"][0]
    )
    print("REF:", ref_times, dataset, subdatasets[collection_name]["parameters"][0])
    links: list(Link) = []
    links.append(Link(href=base_url, rel="self"))
    extent = Extent()
    for instance in list(ref_times):
        print("INSTANCE:", instance)
        instance_links: list(Link) = []
        instance_link = Link(href=f"{base_url}/{instance}", rel="self")
        instance_links.append(instance_link)
        # instance = Collection(
        #     id=instance, links=instance_links, extent=extent, crs=CRSOptions.wgs84, data_queries=
        # )
        instance_info = get_collectioninfo_for_id(
            collection_name,
            f"{base_url}/{instance}",
            dataset,
            collection_name,
            instance,
        )

        instances.append(instance_info)
        # base_url = request.url_for(
        #     "get_collection_instance_by_dataset_and_instance",
        #     dataset=dataset,
        #     instance=instance,
        # )
        # coll = get_collectioninfo_for_id(dataset, base_url, True)
        # colls.append(coll)

    instances_data = InstancesModel(instances=instances, links=links)
    return instances_data


@edrApiApp.get(
    "/collections/{collection_name}/instances/{instance}",
    response_model=Collection,
    response_model_exclude_none=True,
)
async def get_collection_instance_by_name_and_instance(
    collection_name: str, instance, request: Request
):
    base_url = request.url_for(
        "get_collection_instance_by_name_and_instance",
        collection_name=collection_name,
        instance=instance,
    )
    dataset = collection_name.split("-")[0]
    coll = get_collectioninfo_for_id(
        collection_name, base_url, dataset, collection_name, instance
    )
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


def makedimsORG(dims, data):
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


def covjson_from_resp(dats):
    print("DAT:", dats)
    dat = dats[0]
    (lon, lat) = dat["point"]["coords"].split(",")
    lat = float(lat)
    lon = float(lon)
    dims = makedims(dat["dims"], dat["data"])
    print("dims: ", dims, "reference_time" in dims)
    time_steps = getdimvals(dims, "time")
    print("TSTEPS:", time_steps)
    vertical_steps = getdimvals(dims, "elevation")
    reference_time = getdimvals(dims, "reference_time")
    if reference_time:
        single_reference_time = reference_time[0]
    else:
        single_reference_time = None
    print("REF:", single_reference_time, vertical_steps)

    values = []
    if reference_time:
        for t in time_steps:
            if vertical_steps:
                for v in vertical_steps:
                    values.append(float(dat["data"][single_reference_time][v][t]))
            else:
                values.append(float(dat["data"][single_reference_time][t]))
    else:
        if vertical_steps:
            for v in vertical_steps:
                values.append(float(dat["data"][v][t]))
        else:
            for t in time_steps:
                values.append(float(dat["data"][t]))
    print("VAL:", values)

    parameters: Dict(str, CovJsonParameter) = dict()
    ranges = dict()
    for dt in [dat]:
        param = CovJsonParameter(
            id=dt["name"],
            observedProperty=ObservedProperty(label={"en": dt["name"]}),
        )
        parameters[dt["name"]] = param
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
        print("_range: ", _range)
        ranges[dt["name"]] = _range

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
    domain = Domain(domainType=domainType, axes=axes, referencing=referencing)
    covjson = Coverage(
        id="test",
        domain=domain,
        ranges=ranges,
        parameters=parameters,
    )
    return covjson


def feature_from_dat(dats, observedPropertyName, name):
    """
    feature_from_dat
    """
    print("DAT:", dats)
    features = []
    for dat in dats:
        print("dims:", dat["dims"])
        dims = makedimsORG(dat["dims"], dat["data"])
        timeSteps = getdimvalsORG(dims, "time")
        print("timeSteps:", timeSteps)
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
        print("D:", dims, dims_without_time)
        tuples = list(itertools.product(*valstack))

        # features = []
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
