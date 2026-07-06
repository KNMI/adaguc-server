"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

import logging
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
from .utils.edr_exception import EdrException, exc_unknown_collection
from .edr_locations import router as locations_router
from .edr_position import router as position_router
from .edr_instances import router as instances_router

from .utils.edr_utils import generate_max_age, get_base_url, get_collectioninfo_from_md, get_instance, get_metadata

logger = logging.getLogger(__name__)
logger.debug("Starting EDR")

edrApiApp = FastAPI(redirect_slashes=False, debug=False)
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


@edrApiApp.get("/collections", response_model=Collections, response_model_exclude_none=True)
@edrApiApp.get("/collections/", response_model=Collections, response_model_exclude_none=True)
async def rest_get_edr_collections(request: Request, response: Response):
    """
    GET /collections, returns all available collections
    """
    links: list[Link] = []
    collection_url = get_base_url(request) + "/edr/collections"
    self_link = Link(href=collection_url, rel="self", type="application/json")

    base_url = get_base_url(request)
    links.append(self_link)
    collections: list[Collection] = []
    ttl_set = set()
    metadata = await get_metadata()

    for dataset_name in metadata.keys():
        try:
            colls = get_collectioninfo_from_md(metadata[dataset_name], dataset_name, base_url)
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
async def rest_get_edr_collection_by_id(collection_name: str, req: Request, response: Response):
    """
    GET Returns the most recent EDR collection for given collection id
    """

    base_url = get_base_url(req)

    # Query metadata to find last reference time
    metadata = await get_metadata(collection_name)
    instance = get_instance(metadata, collection_name)

    # Query metadata again with the most recent reference time, so time.values matches the reference time
    # TODO: it would be good if we can prevent the extra call to metadata.
    metadata = await get_metadata(collection_name, instance)
    collection = get_collectioninfo_from_md(metadata[collection_name], collection_name, base_url)[0]

    if collection is None:
        raise exc_unknown_collection(collection_name)

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
