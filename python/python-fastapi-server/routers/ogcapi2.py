"""ogcApiApp"""
import itertools
import traceback
import types
from typing import Any, Dict, List, Union, Sequence, Type
from enum import Enum
import yaml
from defusedxml.ElementTree import fromstring, parse, ParseError

import pydantic

from collections import OrderedDict

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
from .ogcapi_tools import generate_collections, get_capabilities, get_extent, make_bbox, calculate_coords, get_parameters, callADAGUC, feature_from_dat, getItemLinks

DEFAULT_CRS = "http://www.opengis.net/def/crs/OGC/1.3/CRS84"
SUPPORTED_CRS_LIST = [DEFAULT_CRS]

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
    landingPage = LandingPage(title="ogcapi2", description="ADAGUC OGCAPI-Features server demo", links=links)
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
                crs=[DEFAULT_CRS],
                storageCrs=DEFAULT_CRS
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
        links=getCollectionLinks(req.url_for("getCollection", id=id)),
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
    return Response(
        content=openapi_yaml,
        media_type="application/vnd.oai.openapi;version=3.0",
    )


def request_(call_adaguc, url, args, name, headers=None):
    # pylint: disable=consider-using-f-string
    """request_"""
    url = (
        url
        + "service=WMS&version=1.3.0&request=getPointValue&INFO_FORMAT=application/json"
    )

    if "latlon" in args and args["latlon"]:
        x = args["latlon"].split(",")[1]
        y = args["latlon"].split(",")[0]
        url = "%s&X=%s&Y=%s&CRS=EPSG:4326" % (url, x, y)
    if "lonlat" in args and args["lonlat"]:
        x = args["lonlat"].split(",")[0]
        y = args["lonlat"].split(",")[1]
        url = "%s&X=%s&Y=%s&CRS=EPSG:4326" % (url, x, y)
    if not "CRS=" in url.upper():
        url = "%s&X=%s&Y=%s&CRS=EPSG:4326" % (url, 5.2, 52.0)

    if "resultTime" in args and args["resultTime"]:
        url = "%s&DIM_REFERENCE_TIME=%s" % (url, args["resultTime"])

    if "datetime" in args and args["datetime"] is not None:
        url = "%s&TIME=%s" % (url, args["datetime"])
    else:
        url = "%s&TIME=%s" % (url, "*")

    url = "%s&LAYERS=%s&QUERY_LAYERS=%s" % (
        url,
        args["observedPropertyName"],
        args["observedPropertyName"],
    )

    if "dims" in args and args["dims"]:
        for dim in args["dims"].split(";"):
            dimname, dimval = dim.split(":")
            if dimname.upper() == "ELEVATION":
                url = "%s&%s=%s" % (url, dimname, dimval)
            else:
                url = "%s&DIM_%s=%s" % (url, dimname, dimval)

def getSingleItem(id: str, coll_id:str, url: str)->FeatureGeoJSON:
    collection, observed_property_name, point, dims, datetime_=id.split(";")
    coord=list(map(float,point.split(",")))
    dimspec=""
    for dim in dims.split(";"):
        dimname, dimval = dim.split("=")
        dimspec+=f"&{dimname}={dimval}"
    datetime_=datetime_.replace("$", "/")
    request_url = (
            f"http://localhost:8000/wms?dataset={coll_id}&query_layers={observed_property_name}"
            + "&service=WMS&version=1.3.0&request=getPointValue&FORMAT=application/json&INFO_FORMAT=application/json"
            + f"&X={coord[0]}&Y={coord[1]}&CRS=EPSG:4326"
        )
    if datetime_:
        request_url+=f"&TIME={datetime_}"
    request_url+=dimspec
    status, data = callADAGUC(request_url.encode("UTF-8"))
    if status == 0:
        try:
            response_data = json.loads(
                data.getvalue(), object_pairs_hook=OrderedDict
            )
        except ValueError:
            root = fromstring(data)
            logger.info("ET:%s", root)

            retval = json.dumps(
                {"Error": {"code": root[0].attrib["code"], "message": root[0].text}}
            )
            logger.info("retval=%s", retval)
            return 400, root[0].text.strip() #TODO
        features = []
        for data in response_data:
            data_features = feature_from_dat(
                data, observed_property_name, id, url, url
            )
            features.extend(data_features)
        return features[0]

    return None

def getFeaturesForItems(
    id: str,
    base_url: str,
    limit: int = 10,
    start: int = 0,
    point=None,
    bbox=None,
    datetime_=None,
    resultTime=None,
    observedPropertyName=None,
    dims=None,
    npoints=None,
    crs=None,
    bbox_crs=None
) -> List[FeatureGeoJSON]:
    features: List[FeatureGeoJSON] = []
    if not point:
        if not bbox :
            bbox =  make_bbox(get_extent(id))

        if not npoints:
            npoints = 4
        coords = calculate_coords(bbox, npoints, npoints)
    else:
        coords = [point]
    if not observedPropertyName:
        collinfo = get_parameters(id)
        observedPropertyName = [collinfo["layers"][0]["name"]]
         # Default observedPropertyName = first layername
    paramList = ",".join(observedPropertyName)

    if resultTime:
       collinfo = get_parameters(id)


    paramList = ",".join(observedPropertyName)

    dimspec=""
    if dims:
        for dimname,dimval in dims.items():
            dimspec += "&%s=%s" % (dimname, dimval)

    for coord in coords:
        request_url = (
            f"http://localhost:8000/wms?dataset={id}&query_layers={paramList}"
            + "&service=WMS&version=1.3.0&request=getPointValue&FORMAT=application/json&INFO_FORMAT=application/json"
            + f"&X={coord[0]}&Y={coord[1]}&CRS=EPSG:4326"
        )
        if datetime_:
            request_url+=f"&TIME={datetime_}"
        if resultTime:
            request_url+=f"&DIM_REFERENCE_TIME={resultTime}"
        request_url+=dimspec
        status, data = callADAGUC(request_url.encode("UTF-8"))
        if status == 0:
            try:
                response_data = json.loads(
                    data.getvalue(), object_pairs_hook=OrderedDict
                )
            except ValueError:
                root = fromstring(data)
                logger.info("ET:%s", root)

                retval = json.dumps(
                    {"Error": {"code": root[0].attrib["code"], "message": root[0].text}}
                )
                logger.info("retval=%s", retval)
                return 400, root[0].text.strip() #TODO
            features = []
            for data in response_data:
                data_features = feature_from_dat(
                    data, observedPropertyName, id, base_url, base_url
                )
                features.extend(data_features)
            return features

    return features


def getFeaturesForItems2(
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


def check_point(point: Union[str, None] = Query(default=None)) -> List[float]:
    if point is None:
        return None
    coords = point.split(",")
    if len(coords) != 2:
        # Error
        raise HTTPException(status_code=404, detail="point should contain 2 floats")
    return list(map(float, coords))


def check_bbox(bbox: Union[str, None] = Query(default=None)) -> List[float]:
    if bbox is None:
        return None
    print("BBOX:", bbox)
    coords = bbox.split(",")
    if len(coords) != 4 and len(coords) != 6:
        # Error
        raise HTTPException(status_code=404, detail="bbox should contain 4 or 6 floats")
    return list(map(float, coords))


def check_observed_property_name(observedPropertyName: Union[str, None] = Query(default=None)) -> List[str]:
    if observedPropertyName is None:
        return None
    names = observedPropertyName.split(",")
    if len(names) < 1:
        # Error
        raise HTTPException(status_code=404, detail="observedPropertyName should contain > 0 names")
    return names


def check_dims(dims: Union[str, None] = Query(default=None)) -> Dict[str,str]:
    if dims is None:
        return None
    dim_terms = dims.split(";")
    if len(dim_terms) < 1:
        # Error
        raise HTTPException(status_code=404, detail="dims should contain > 0 names")
    dimensions = dict()
    for dim in dim_terms:
        dimname, dimval = dim.split(":")
        if dimname.upper() == "ELEVATION":
            dimensions["ELEVATION"]=dimval
        else:
            dimensions[dimname]=dimval

    return dimensions

def check_bbox_crs(bbox_crs: Union[str, None] = Query(default=None, alias="bbox-crs")) -> str:
    if bbox_crs is None:
        return None

    if bbox_crs not in SUPPORTED_CRS_LIST:
        # Error
        raise HTTPException(status_code=400, detail=f"bbox-crs {bbox_crs} not in supported list")
    return bbox_crs

def check_crs(crs: Union[str, None] = Query(default=None)) -> str:
    if crs is None:
        return None

    if crs not in SUPPORTED_CRS_LIST:
        # Error
        raise HTTPException(status_code=400, detail=f"crs {crs} not in supported list")
    return crs

@ogcApiApp.get(
    "/collections/{id}/items",
    response_model=FeatureCollectionGeoJSON,
)
async def getItemsForCollection(
    id: str,
    req: Request,
    response: Response,
    f: str = "json",
    limit: Union[int, None] = Query(default=10),
    start: Union[int, None] = Query(default=0),
    bbox: Union[str, None] = Depends(check_bbox),
    point: Union[str, None] = Depends(check_point),
    crs: Union[str, None] = Depends(check_crs), #Query(default=None),
    resultTime: Union[str, None] = Query(default=None),
    datetime_: Union[str, None] = Query(default=None, alias="datetime"),
    observedPropertyName: Union[str, None] = Depends(check_observed_property_name),
    dims: Union[str, None] = Depends(check_dims),
    npoints: Union[int, None] = Query(default=4),
    bbox_crs: Union[str, None] = Depends(check_bbox_crs)
):
    allowed_params = [
        "f",
        "limit",
        "start",
        "point",
        "bbox",
        "datetime",
        "resultTime",
        "observedPropertyName",
        "dims",
        "npoints",
        "crs",
        "bbox-crs"
    ]
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
    base_url = req.url_for("getItemsForCollection", id=id)
    try:
        features = getFeaturesForItems(
            id=id, base_url=base_url, limit=limit, start=start, point=point, bbox=bbox, datetime_=datetime_, resultTime=resultTime, crs=crs, observedPropertyName=observedPropertyName, npoints=npoints, dims=dims
        )
        features_to_return = features[start : start + limit]
        number_matched = len(features)
        number_returned = len(features_to_return)
        prev_start = None
        if start > 0:
            prev_start = max(start - limit, 0)
        next_start = None
        if (start + limit) < number_matched:
            next_start = start + limit
        links = getItemLinks(
            id,
            None,
            req.url_for("getItemsForCollection", id=id),
            str(req.url),
            prev_start=prev_start,
            next_start=next_start,
            limit=limit,
        )

        featureCollection = FeatureCollectionGeoJSON(
            type=Type.FeatureCollection,
            timeStamp="2023-01-09T12:00:00Z",
            links=links,
            features=features_to_return,
            numberMatched=len(features),
            numberReturned=len(features_to_return),
        )
        response.headers["Content-Crs"]=f"<{DEFAULT_CRS}>"
        return featureCollection
    except Exception as e:
        print(
            "ERR:",
            traceback.format_exception(None, e, e.__traceback__),
            file=sys.stderr,
            flush=True,
        )

    return None


def getItemLinks2(
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
    id: str, item_id: str, req: Request,  response: Response, f: str = "json", crs: Union[str, None] = Depends(check_crs),
):
    url = req.url
    feature_to_return = getSingleItem(item_id, id, str(url))

    links = getItemLinks(
            id,
            item_id,
            str(url),
            str(req.url)
        )
    response.headers["Content-Crs"]=f"<{DEFAULT_CRS}>"
    return feature_to_return
