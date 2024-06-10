"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

import logging
import re
from datetime import datetime, timezone

from dateutil.relativedelta import relativedelta
from edr_pydantic.capabilities import (
    ConformanceModel,
    Contact,
    LandingPageModel,
    Provider,
)
from edr_pydantic.collections import Collection, Collections, Instance, Instances
from edr_pydantic.data_queries import DataQueries, EDRQuery
from edr_pydantic.extent import Custom, Extent, Spatial, Temporal, Vertical
from edr_pydantic.link import EDRQueryLink, Link
from edr_pydantic.observed_property import ObservedProperty
from edr_pydantic.parameter import Parameter
from edr_pydantic.unit import Symbol, Unit
from edr_pydantic.variables import Variables
from fastapi import FastAPI, Request, Response
from fastapi.openapi.utils import get_openapi
from fastapi.responses import JSONResponse
from owslib.wms import WebMapService

from .edr_utils import (
    get_base_url,
    get_edr_collections,
    get_ref_times_for_coll,
    parse_iso,
    parse_instance_time,
    get_ttl_from_adaguc_headers,
    generate_max_age,
)
from .edr_covjson import get_param_metadata
from .edr_exception import EdrException

from .edr_locations import router as locations_routers
from .edr_cube import router as cube_router
from .edr_position import router as position_router


from .ogcapi_tools import call_adaguc

logger = logging.getLogger(__name__)
logger.debug("Starting EDR")

edrApiApp = FastAPI(debug=False)
edrApiApp.include_router(cube_router)
edrApiApp.include_router(position_router)
edrApiApp.include_router(locations_routers)


SYMBOL_TYPE_URL = "http://www.opengis.net/def/uom/UCUM"


@edrApiApp.exception_handler(EdrException)
async def edr_exception_handler(_, exc: EdrException):
    """
    Handler for EDR exceptions
    """
    return JSONResponse(
        status_code=exc.code,
        content={"code": str(exc.code), "description": exc.description},
    )


DEFAULT_CRS_OBJECT = {
    "crs": "EPSG:4326",
    "wkt": 'GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4326"]]',
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
                custom_dim["values"] = ["R5/0/1"]
            # if dim_name == "member":
            #     custom_dim["id"] = "number"
            custom.append(custom_dim)
    return custom if len(custom) > 0 else None


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


@edrApiApp.get(
    "/collections", response_model=Collections, response_model_exclude_none=True
)
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
            logger.warning("Unable to fetch WMS GetCapabilities for %s", edr_coll)
    collections_data = Collections(links=links, collections=collections)
    if ttl_set:
        response.headers["cache-control"] = generate_max_age(min(ttl_set))
    return collections_data


@edrApiApp.get(
    "/collections/{collection_name}",
    response_model=Collection,
    response_model_exclude_none=True,
)
async def rest_get_edr_collection_by_id(collection_name: str, response: Response):
    """
    GET Returns collection information for given collection id
    """
    collection, ttl = await get_collectioninfo_for_id(collection_name)
    if ttl is not None:
        response.headers["cache-control"] = generate_max_age(ttl)
    if collection is None:
        raise EdrException(code=400, description="Unknown or unconfigured collection")
    return collection


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
async def rest_get_edr_inst_for_coll(
    collection_name: str, request: Request, response: Response
):
    """
    GET: Returns all available instances for the collection
    """
    instances_url = (
        get_base_url(request) + f"/edr/collections/{collection_name}/instances"
    )

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
        instance_link = Link(href=f"{instances_url}/{instance}", rel="collection")
        instance_links.append(instance_link)
        instance_info, ttl = await get_collectioninfo_for_id(collection_name, instance)
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
async def rest_get_collection_info(collection_name: str, instance, response: Response):
    """
    GET  "/collections/{collection_name}/instances/{instance}"
    """
    coll, ttl = await get_collectioninfo_for_id(collection_name, instance)
    if ttl is not None:
        response.headers["cache-control"] = generate_max_age(ttl)
    return coll


@edrApiApp.get("/", response_model=LandingPageModel, response_model_exclude_none=True)
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
    links.append(Link(href=conformance_url, rel="conformance", type="application/json"))
    links.append(Link(href=collections_url, rel="data", type="application/json"))
    openapi_url = f"{landingpage_url}/api"
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
    """
    Fix the API! This is needed for the OGC conformance tests.
    """
    api = get_openapi(
        title=edrApiApp.title,
        version=edrApiApp.version,
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
