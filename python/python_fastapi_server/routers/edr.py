"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

import logging
from datetime import datetime, timezone
import traceback

from edr_pydantic.capabilities import (
    ConformanceModel,
    Contact,
    LandingPageModel,
    Provider,
)
from edr_pydantic.collections import Collection, Collections
from edr_pydantic.link import Link
from fastapi import FastAPI, Request, Response
from fastapi.openapi.utils import get_openapi
from fastapi.responses import JSONResponse

from .edr_cube import router as cube_router
from .utils.edr_exception import EdrException, exc_no_datasets
from .edr_locations import router as locations_router
from .edr_position import router as position_router
from .edr_instances import router as instances_router

from .utils.edr_utils import (
    generate_max_age,
    get_base_url,
    get_collectioninfo_from_md,
    get_time_values_for_range,
    get_metadata,
)

logger = logging.getLogger(__name__)
logger.debug("Starting EDR")

edrApiApp = FastAPI(debug=False)
edrApiApp.include_router(cube_router)
edrApiApp.include_router(position_router)
edrApiApp.include_router(locations_router)
edrApiApp.include_router(instances_router)


@edrApiApp.exception_handler(EdrException)
async def edr_exception_handler(_, exc: EdrException):
    """
    Handler for EDR exceptions
    """

    return JSONResponse(
        status_code=exc.status_code,
        content={"code": str(exc.status_code), "description": exc.detail},
    )


DEFAULT_CRS_OBJECT = {
    "crs": "EPSG:4326",
    "wkt": 'GEOGCS["WGS 84",DATUM["WGS_1984",SPHEROID["WGS 84",6378137,298.257223563,AUTHORITY["EPSG","7030"]],AUTHORITY["EPSG","6326"]],PRIMEM["Greenwich",0,AUTHORITY["EPSG","8901"]],UNIT["degree",0.01745329251994328,AUTHORITY["EPSG","9122"]],AUTHORITY["EPSG","4326"]]',
}


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


@edrApiApp.get(
    "/collections", response_model=Collections, response_model_exclude_none=True
)
async def rest_get_edr_collections(request: Request, response: Response):
    """
    GET /collections, returns all available collections
    """
    links: list[Link] = []
    collection_url = get_base_url(request) + "/edr/collections"
    self_link = Link(href=collection_url, rel="self", type="application/json")

    links.append(self_link)
    collections: list[Collection] = []
    ttl_set = set()
    metadata = await get_metadata()
    if metadata is None:
        raise exc_no_datasets()
    for dataset_name in metadata.keys():
        try:
            colls = get_collectioninfo_from_md(metadata[dataset_name], dataset_name)
            if colls:
                collections.extend(colls)
            else:
                logger.warning("Unable to fetch WMS GetMetadata for %s", dataset_name)
        except Exception:
            print("ERR", dataset_name, traceback.format_exc())
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
    metadata = await get_metadata(collection_name)
    if metadata is None:
        raise EdrException(
            code=400,
            description=f"Unknown or unconfigured collection {collection_name}",
        )

    ttl = None

    collection = get_collectioninfo_from_md(metadata[collection_name], collection_name)[
        0
    ]
    if ttl is not None:
        response.headers["cache-control"] = generate_max_age(ttl)
    if collection is None:
        raise EdrException(code=400, description="Unknown or unconfigured collection")
    return collection


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
