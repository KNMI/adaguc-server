"""ogcApiApp"""
import itertools
import traceback
import types
from typing import Any, Dict, List, Union, Sequence, Type
from enum import Enum
import weakref
import yaml

import pydantic

from cachetools import cached, TTLCache

cache = TTLCache(maxsize=1000, ttl=30)

from .setup_adaguc import setup_adaguc

# from routers.cacher import init_cache

from fastapi import (
    Body,
    Depends,
    HTTPException,
    Query,
    Request,
    APIRouter,
    Response,
    Request,
    FastAPI,
    status,
)
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
from fastapi.encoders import jsonable_encoder

from pydantic import ValidationError, BaseModel, validator


from .models.ogcapifeatures_1_model import (
    GeometryGeoJSON,
    LandingPage,
    Link,
    Collections,
    Collection,
    ConfClasses,
    FeatureCollectionGeoJSON,
    FeatureGeoJSON,
    PointGeoJSON,
    Type,
    Type1,
    Type2,
    Type3,
    Type7,
    NumberMatched,
    NumberReturned,
    Extent,
    Spatial,
    Temporal,
)
from .ogcapi_tools import generate_collections, get_capabilities, get_extent, make_bbox

import sys
import os
import logging
import json
import urllib.parse

# ogcApiApp = APIRouter(
#     responses={404: {"description": "Not found at all"}},
#     redirect_slashes=False,
# )


logger = logging.getLogger(__name__)

ogcApiApp = FastAPI()


@ogcApiApp.exception_handler(RequestValidationError)
async def validation_exception_handler(request: Request, exc: RequestValidationError):
    return JSONResponse(
        status_code=status.HTTP_400_BAD_REQUEST,
        content=jsonable_encoder({"detail": exc.errors(), "body": exc.body}),
    )


@ogcApiApp.get("/", response_model=LandingPage)
@ogcApiApp.get("", response_model=LandingPage)
async def handleOgcApiRoot(req: Request, f: str = "json"):
    links: List[Link] = []
    links.append(
        Link(
            href=req.url_for("handleOgcApiRoot"),
            rel="self",
            title="This document in JSON",
            type="application/json",
        )
    )
    links.append(
        Link(
            href=req.url_for("handleOgcApiRoot") + "?f=html",
            rel="self",
            title="This document in HTML",
            type="text/html",
        )
    )
    links.append(
        Link(
            href=req.url_for("getConformance"),
            rel="conformance",
            title="This document in HTML",
            type="application/json",
        )
    )
    links.append(
        Link(
            href=req.url_for("getCollections"),
            rel="data",
            title="Collections",
            type="application/json",
        )
    )
    links.append(
        Link(
            href=req.url_for("getOpenApi"),
            rel="service-desc",
            title="The OpenAPI definition as JSON",
            type="application/vnd.oai.openapi+json;version=3.0",
        )
    )
    links.append(
        Link(
            href=req.url_for("getOpenApiYaml"),
            rel="service-desc",
            title="The OpenAPI definition as YAML",
            type="application/vnd.oai.openapi;version=3.0",
        )
    )
    landingPage = LandingPage(title="dd", description="dd description", links=links)
    if f == "json":
        return landingPage
    return landingPage


def getCollectionLinks(url, collection=None):
    links: List[Link] = []
    links.append(
        Link(
            href=url + "?f=json",
            rel="self",
            title="This document in JSON",
            type="application/json",
        )
    )
    links.append(
        Link(
            href=url + "?f=html",
            rel="alternate",
            title="This document in HTML",
            type="text/html",
        )
    )
    links.append(
        Link(
            href=url + "/items?f=json",
            rel="items",
            title="Items of this collection",
            type="application/geo+json",
        )
    )
    links.append(
        Link(
            href=url + "/items?f=html",
            rel="items",
            title="Items of this collection in HTML",
            type="text/html",
        )
    )
    return links


def getCollectionsLinks(url):
    links: List[Link] = []
    links.append(
        Link(
            href=url + "?f=json",
            rel="self",
            title="This document in JSON",
            type="application/json",
        )
    )
    links.append(
        Link(
            href=url + "?f=html",
            rel="alternate",
            title="This document in HTML",
            type="text/html",
        )
    )
    return links


@ogcApiApp.get("/collections/", response_model=Collections)
@ogcApiApp.get("/collections", response_model=Collections)
async def getCollections(req: Request, f: str = "json"):
    print("getCollections()")
    collections: List[Collection] = []
    parsed_collections = generate_collections()
    for parsed_collection in parsed_collections:
        spatial = Spatial(
            bbox=[list(get_extent(parsed_collections[parsed_collection]["dataset"]))]
        )
        extent = Extent(spatial=spatial)
        collections.append(
            Collection(
                id=parsed_collections[parsed_collection]["dataset"],
                title="title1",
                description="descr1",
                links=getCollectionLinks(
                    req.url_for(
                        "getCollection",
                        id=parsed_collections[parsed_collection]["dataset"],
                    ),
                    parsed_collections[parsed_collection]["dataset"],
                ),
                extent=extent,
                itemType="feature",
            )
        )
    links = getCollectionsLinks(req.url_for("getCollections"))
    return Collections(
        links=links,
        collections=collections,
    )


@ogcApiApp.get("/collections/{id}", response_model=Collection)
async def getCollection(id: str, req: Request, f: str = "json"):
    extent = Extent(spatial=Spatial(bbox=[get_extent(id)]))
    coll = Collection(
        id=id,
        title="title1",
        description="descr1",
        extent=extent,
        links=getCollectionLinks(req.url_for("getCollection", id=id), "coll1"),
    )
    return coll


conformanceClasses = [
    "http://www.opengis.net/spec/ogcapi-features-1/1.0/conf/core",
    "http://www.opengis.net/spec/ogcapi-features-1/1.0/conf/oas30",
    "http://www.opengis.net/spec/ogcapi-features-1/1.0/conf/html",
    "http://www.opengis.net/spec/ogcapi-features-1/1.0/conf/geojson",
    "http://www.opengis.net/spec/ogcapi-features-2/1.0/conf/crs",
    "http://www.opengis.net/spec/ogcapi-common-2/1.0/conf/collections",
    "http://www.opengis.net/spec/ogcapi-common-1/1.0/conf/core",
]


@ogcApiApp.get("/conformance", response_model=ConfClasses)
async def getConformance(f: str = "json"):
    confClasses = ConfClasses(conformsTo=conformanceClasses)
    if f == "json":
        return confClasses
    elif f == "html":
        return confClasses


def makeOpenApi():
    openapi = ogcApiApp.openapi()
    paths = openapi.get("paths", {})
    for p in paths.keys():
        params = (paths.get(p).get("get", {}).get("parameters"), [])
        for param in params[0]:
            if param["in"] == "query" and param["name"] == "bbox":
                param["style"] = "form"
                param["explode"] = False
                param["schema"] = {
                    "type": "array",
                    # "oneOf": [
                    #     {"minItems": 4, "maxItems": 4},
                    #     {"minItems": 6, "maxItems": 6},
                    # ],
                    "minItems": 4,
                    "maxItems": 6,
                    "items": {"type": "number"},
                }
            if param["in"] == "query" and param["name"] == "limit":
                param["style"] = "form"
                param["explode"] = False
                param["schema"] = {
                    "type": "integer",
                    "minimum": 1,
                    "maximum": 10000,
                    "default": 10,
                }
            if param["in"] == "query" and param["name"] == "datetime":
                param["style"] = "form"
                param["explode"] = False
                param["schema"] = {
                    "type": "string",
                }
    return openapi


@ogcApiApp.get("/api")
async def getOpenApi(req: Request, f: str = "json"):
    return makeOpenApi()


@ogcApiApp.get("/api.yaml")
async def getOpenApiYaml(req: Request, f: str = "yaml"):
    openapi = makeOpenApi()
    openapi_yaml = yaml.dump(openapi, sort_keys=False)
    print(openapi_yaml, "\nNNNN")
    return Response(
        content=openapi_yaml,
        media_type="application/vnd.oai.openapi;version=3.0",
    )


def getFeaturesForItems(
    id: str, limit: int = 10, start: int = 0, bbox=None, datetime_=None
) -> List[FeatureGeoJSON]:
    features: List[FeatureGeoJSON] = []
    for n in range(120):
        point = PointGeoJSON(
            type=Type7.Point, coordinates=[5.2 + (n - 25) / 10, 52.0 + (n - 25) / 10]
        )
        features.append(
            FeatureGeoJSON(
                type=Type1.Feature,
                geometry=point,
                properties={},
                id=str(n + 1234),
                links=None,
            )
        )
    return features


def check_bbox(bbox: Union[str, None] = Query(default=None)) -> List[float]:
    print("bbox:", bbox)
    if bbox is None:
        return None
    coords = bbox.split(",")
    print("coords:", coords)
    if len(coords) != 4 and len(coords) != 6:
        # Error
        raise HTTPException(status_code=404, detail="bbox should contain 4 or 6 floats")
    return list(map(float, coords))


@ogcApiApp.get(
    "/collections/{id}/items",
    response_model=FeatureCollectionGeoJSON,
)
async def getItemsForCollection(
    id: str,
    req: Request,
    f: str = "json",
    limit: Union[int, None] = Query(default=10),
    start: Union[int, None] = Query(default=0),
    bbox: Union[str, None] = Depends(check_bbox),
    datetime_: Union[str, None] = Query(default=None, alias="datetime"),
):
    print("getItemsForCollection")
    allowed_params = ["f", "limit", "start", "bbox", "datetime"]
    extra_params = [k for k in req.query_params if k not in allowed_params]

    if len(extra_params):
        return JSONResponse(
            status_code=status.HTTP_400_BAD_REQUEST,
            content=jsonable_encoder(
                {
                    "detail": "extra parameters",
                    "body": [extra_params],
                }
            ),
        )
    try:
        features = getFeaturesForItems(id, limit, start, bbox, datetime_)
        # print(features)
        features_to_return = features[start : start + limit]
        number_matched = len(features)
        number_returned = len(features_to_return)
        print("LEN:", len(features), len(features_to_return), limit, start)
        prev_start = None
        if start > 0:
            prev_start = max(start - limit, 0)
        next_start = None
        if (start + limit) < number_matched:
            next_start = start + limit
            print(start, limit, prev_start, next_start)
        links = getItemLinks(
            id,
            None,
            req.url_for("getItemsForCollection", id=id),
            prev_start=prev_start,
            next_start=next_start,
            limit=limit,
        )
        print("links:", links)

        featureCollection = FeatureCollectionGeoJSON(
            type=Type.FeatureCollection,
            timeStamp="2023-01-09T12:00:00Z",
            links=links,
            features=features_to_return,
            numberMatched=len(features),
            numberReturned=len(features_to_return),
        )
        print(
            "numberMatched:", len(features), "numberReturned:", len(features_to_return)
        )
        return featureCollection
    except Exception as e:
        print(
            "ERR:",
            traceback.format_exception(None, e, e.__traceback__),
            file=sys.stderr,
            flush=True,
        )

    return None


def getItemLinks(
    id: str,
    item_id: str,
    url: str,
    prev_start: int = None,
    next_start: int = None,
    limit: int = None,
) -> List[Link]:
    links: List[Link] = []
    links.append(
        Link(
            href=url,
            rel="self",
            title="Item in JSON",
            type="application/geo+json",
        )
    )
    links.append(
        Link(
            href=url + "?f=html",
            rel="alternate",
            title="Item in HTML",
            type="text/html",
        )
    )
    if prev_start is not None:
        links.append(
            Link(
                href=url + f"?start={prev_start}&limit={limit}",
                rel="prev",
                title="Item in JSON",
                type="application/geo+json",
            )
        )
    if next_start is not None:
        links.append(
            Link(
                href=url + f"?start={next_start}&limit={limit}",
                rel="next",
                title="Item in JSON",
                type="application/geo+json",
            )
        )

    collection_url = "/".join(url.split("/")[:-2])
    links.append(
        Link(
            href=collection_url,
            rel="collection",
            title="Collection",
            type="application/geo+json",
        )
    )
    return links


@ogcApiApp.get("/collections/{id}/items/{item_id}", response_model=FeatureGeoJSON)
async def getItemForCollection(
    id: str, item_id: str, req: Request, f: str = "json", crs: str = None
):
    features: List[FeatureGeoJSON] = getFeaturesForItems(id, limit=-1)
    feature: FeatureGeoJSON = None
    for f in features:
        if f.id == item_id:
            feature = f
            break
    url = (
        req.url
    )  # req.url_for("getItemForCollection", id=id, item_id=item_id, req=req)
    try:
        f.links = getItemLinks(
            id,
            item_id,
            str(url),
        )
    except ValidationError as e:
        print("ValidationError:", e)

    return feature
