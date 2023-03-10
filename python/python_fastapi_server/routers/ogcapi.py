"""ogcApiApp"""
import traceback
import os
import os.path
import json
from typing import Dict, List, Union, Type
from collections import OrderedDict
import logging
import yaml

from defusedxml.ElementTree import fromstring

from cachetools import TTLCache

from fastapi import (
    Depends,
    HTTPException,
    Query,
    Request,
    Response,
    FastAPI,
    status as fastapi_status,
)
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
from fastapi.encoders import jsonable_encoder
from fastapi.templating import Jinja2Templates
from fastapi.staticfiles import StaticFiles

from .models.ogcapifeatures_1_model import (
    LandingPage,
    Link,
    Collections,
    Collection,
    ConfClasses,
    FeatureCollectionGeoJSON,
    FeatureGeoJSON,
    Type,
    Extent,
    Spatial,
)
from .ogcapi_tools import (
    generate_collections,
    get_extent,
    make_bbox,
    calculate_coords,
    get_parameters,
    call_adaguc,
    feature_from_dat,
    get_items_links,
)

logger = logging.getLogger(__name__)

cache = TTLCache(maxsize=1000, ttl=30)

DEFAULT_CRS = "http://www.opengis.net/def/crs/OGC/1.3/CRS84"
SUPPORTED_CRS_LIST = [DEFAULT_CRS]

ogcApiApp = FastAPI(debug=True)

script_dir = os.path.dirname(__file__)
static_abs_file_path = os.path.join(script_dir, "static")
ogcApiApp.mount("/static", StaticFiles(directory=static_abs_file_path), name="static")
templates_abs_file_path = os.path.join(script_dir, "templates/ogcapi")
templates = Jinja2Templates(directory=templates_abs_file_path)


@ogcApiApp.exception_handler(RequestValidationError)
async def validation_exception_handler(exc: RequestValidationError):
    return JSONResponse(
        status_code=fastapi_status.HTTP_400_BAD_REQUEST,
        content=jsonable_encoder({"detail": exc.errors(), "body": exc.body}),
    )


@ogcApiApp.get("/", response_model=LandingPage, response_model_exclude_none=True)
@ogcApiApp.get("", response_model=LandingPage, response_model_exclude_none=True)
async def handle_ogc_api_root(req: Request, wanted_format: str = "json"):
    links: List[Link] = []
    links.append(
        Link(
            href=req.url_for("handle_ogc_api_root"),
            rel="self",
            title="This document in JSON",
            type="application/json",
        )
    )
    links.append(
        Link(
            href=req.url_for("handle_ogc_api_root") + "?f=html",
            rel="alternate",
            title="This document in HTML",
            type="text/html",
        )
    )
    links.append(
        Link(
            href=req.url_for("get_conformance"),
            rel="conformance",
            title="This document in HTML",
            type="application/json",
        )
    )
    links.append(
        Link(
            href=req.url_for("get_collections"),
            rel="data",
            title="Collections",
            type="application/json",
        )
    )
    links.append(
        Link(
            href=req.url_for("get_open_api"),
            rel="service-desc",
            title="The OpenAPI definition as JSON",
            type="application/vnd.oai.openapi+json;version=3.0",
        )
    )
    links.append(
        Link(
            href=req.url_for("get_open_api_yaml"),
            rel="service-desc",
            title="The OpenAPI definition as YAML",
            type="application/vnd.oai.openapi;version=3.0",
        )
    )
    landing_page = LandingPage(
        title="ogcapi", description="ADAGUC OGCAPI-Features server", links=links
    )
    if request_type(wanted_format) == "HTML":
        return templates.TemplateResponse(
            "landingpage.html", {"request": req, "landingpage": landing_page.dict()}
        )
    return landing_page


def get_collection_links(url):
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


def get_collections_links(url):
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


def request_type(wanted_format: str) -> str:
    json_types = ["application/json", "json"]
    html_types = ["html", "text/html"]
    if wanted_format in json_types:
        return "JSON"
    if wanted_format in html_types:
        return "HTML"
    return json


@ogcApiApp.get(
    "/collections/", response_model=Collections, response_model_exclude_none=True
)
@ogcApiApp.get(
    "/collections", response_model=Collections, response_model_exclude_none=True
)
async def get_collections(req: Request, wanted_format: str = "json"):
    collections: List[Collection] = []
    parsed_collections = generate_collections()
    for parsed_collection in parsed_collections.values():
        parsed_extent = get_extent(parsed_collection["dataset"])
        if parsed_extent:
            spatial = Spatial(bbox=[list(get_extent(parsed_collection["dataset"]))])
            extent = Extent(spatial=spatial)
            collections.append(
                Collection(
                    id=parsed_collection["dataset"],
                    title="title1",
                    description="descr1",
                    links=get_collection_links(
                        req.url_for(
                            "get_collection",
                            id=parsed_collection["dataset"],
                        )
                    ),
                    extent=extent,
                    itemType="feature",
                    crs=[DEFAULT_CRS],
                    storageCrs=DEFAULT_CRS,
                )
            )

    links = get_collections_links(req.url_for("get_collections"))
    if request_type(wanted_format) == "HTML":
        collections_list = [c.dict() for c in collections]
        return templates.TemplateResponse(
            "collections.html", {"request": req, "collections": collections_list}
        )

    return Collections(
        links=links,
        collections=collections,
    )


@ogcApiApp.get(
    "/collections/{id}", response_model=Collection, response_model_exclude_none=True
)
async def get_collection(collection_id: str, req: Request, wanted_format: str = "json"):
    extent = Extent(spatial=Spatial(bbox=[get_extent(collection_id)]))
    coll = Collection(
        id=collection_id,
        title="title1",
        description="descr1",
        extent=extent,
        links=get_collection_links(req.url_for("get_collection", id=collection_id)),
    )
    if request_type(wanted_format) == "HTML":
        return templates.TemplateResponse(
            "collection.html", {"request": req, "collection": coll.dict()}
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


@ogcApiApp.get(
    "/conformance", response_model=ConfClasses, response_model_exclude_none=True
)
async def get_conformance(req: Request, wanted_format: str = "json"):
    conf_classes = ConfClasses(conformsTo=conformanceClasses)
    if request_type(wanted_format) == "HTML":
        return templates.TemplateResponse(
            "conformance.html",
            {
                "request": req,
                "title": "conformance",
                "description": "D",
                "conformance": jsonable_encoder(conf_classes),
            },
        )
    return conf_classes


def make_open_api():
    openapi = ogcApiApp.openapi()
    paths = openapi.get("paths", {})
    for pth in paths.keys():
        params = (paths.get(pth).get("get", {}).get("parameters"), [])
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
async def get_open_api():
    return Response(
        content=make_open_api(),
        media_type="application/vnd.oai.openapi+json;version=3.0",
    )


@ogcApiApp.get("/api.yaml")
async def get_open_api_yaml():
    openapi = make_open_api()
    openapi_yaml = yaml.dump(openapi, sort_keys=False)
    return Response(
        content=openapi_yaml,
        media_type="application/vnd.oai.openapi;version=3.0",
    )


def get_single_item(item_id: str, url: str) -> FeatureGeoJSON:
    collection, observed_property_name, point, dims, datetime_ = item_id.split(";")
    coord = list(map(float, point.split(",")))
    dimspec = ""
    if len(dims):
        for dim in dims.split("|"):
            dimname, dimval = dim.split("=")
            dimspec += f"&{dimname}={dimval}"

    datetime_ = datetime_.replace("$", "/")
    request_url = (
        f"http://localhost:8000/wms?dataset={collection}&query_layers={observed_property_name}"
        + "&service=WMS&version=1.3.0&request=getPointValue"
        + "&FORMAT=application/json&INFO_FORMAT=application/json"
        + f"&X={coord[0]}&Y={coord[1]}&CRS=EPSG:4326"
    )
    if datetime_:
        request_url += f"&TIME={datetime_}"
    request_url += dimspec
    status, data = call_adaguc(request_url.encode("UTF-8"))
    if status == 0:
        try:
            response_data = json.loads(data.getvalue(), object_pairs_hook=OrderedDict)
        except ValueError:
            root = fromstring(data)

            retval = json.dumps(
                {"Error": {"code": root[0].attrib["code"], "message": root[0].text}}
            )
            return 400, retval
        features = []
        for data in response_data:
            data_features = feature_from_dat(data, collection, url, True)
            features.extend(data_features)
        return features[0]

    return None


def get_features_for_items(
    coll: str,
    base_url: str,
    point=None,
    bbox=None,
    datetime_=None,
    result_time=None,
    observed_property_name=None,
    dims=None,
    npoints=None,
) -> List[FeatureGeoJSON]:
    features: List[FeatureGeoJSON] = []
    if not point:
        if not bbox:
            bbox = make_bbox(get_extent(coll))

        if not npoints:
            npoints = 4
        coords = calculate_coords(bbox, npoints, npoints)
    else:
        coords = [point]
    if not observed_property_name:
        collinfo = get_parameters(coll)
        observed_property_name = [collinfo["layers"][0]["name"]]
        # Default observedPropertyName = first layername
    param_list = ",".join(observed_property_name)

    if result_time:
        collinfo = get_parameters(coll)

    param_list = ",".join(observed_property_name)

    dimspec = ""
    if dims:
        for dimname, dimval in dims.items():
            dimspec += f"&{dimname}={dimval}" % (dimname, dimval)

    features = []
    for coord in coords:
        request_url = (
            f"http://localhost:8000/wms?dataset={coll}&query_layers={param_list}"
            + "&service=WMS&version=1.3.0&request=getPointValue&"
            + "FORMAT=application/json&INFO_FORMAT=application/json"
            + f"&X={coord[0]}&Y={coord[1]}&CRS=EPSG:4326"
        )
        if datetime_:
            request_url += f"&TIME={datetime_}"
        if result_time:
            request_url += f"&DIM_REFERENCE_TIME={result_time}"
        request_url += dimspec
        status, data = call_adaguc(request_url.encode("UTF-8"))
        if status == 0:
            try:
                response_data = json.loads(
                    data.getvalue(), object_pairs_hook=OrderedDict
                )
            except ValueError:
                root = fromstring(data)

                retval = json.dumps(
                    {"Error": {"code": root[0].attrib["code"], "message": root[0].text}}
                )
                return 400, retval  # TODO
            for data in response_data:
                data_features = feature_from_dat(data, coll, base_url, False)
                features.extend(data_features)

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
    coords = bbox.split(",")
    if len(coords) != 4 and len(coords) != 6:
        # Error
        raise HTTPException(status_code=404, detail="bbox should contain 4 or 6 floats")
    return list(map(float, coords))


def check_observed_property_name(
    observed_property_name: Union[str, None] = Query(default=None)
) -> List[str]:
    if observed_property_name is None:
        return None
    names = observed_property_name.split(",")
    if len(names) < 1:
        # Error
        raise HTTPException(
            status_code=404, detail="observedPropertyName should contain > 0 names"
        )
    return names


def check_dims(dims: Union[str, None] = Query(default=None)) -> Dict[str, str]:
    if dims is None:
        return None
    dim_terms = dims.split(";")
    if len(dim_terms) < 1:
        # Error
        raise HTTPException(status_code=404, detail="dims should contain > 0 names")
    dimensions = {}
    for dim in dim_terms:
        dimname, dimval = dim.split(":")
        if dimname.upper() == "ELEVATION":
            dimensions["ELEVATION"] = dimval
        else:
            dimensions[dimname] = dimval

    return dimensions


def check_bbox_crs(
    bbox_crs: Union[str, None] = Query(default=None, alias="bbox-crs")
) -> str:
    if bbox_crs is None:
        return None

    if bbox_crs not in SUPPORTED_CRS_LIST:
        # Error
        raise HTTPException(
            status_code=400, detail=f"bbox-crs {bbox_crs} not in supported list"
        )
    return bbox_crs


def check_crs(crs: Union[str, None] = Query(default=None)) -> str:
    if crs is None:
        return None

    if crs not in SUPPORTED_CRS_LIST:
        # Error
        raise HTTPException(status_code=400, detail=f"crs {crs} not in supported list")
    return crs


@ogcApiApp.get(
    "/collections/{coll}/items",
    response_model=FeatureCollectionGeoJSON,
    response_model_exclude_none=True,
)
async def get_items_for_collection(
    coll: str,
    req: Request,
    response: Response,
    wanted_format: str = "json",
    limit: Union[int, None] = Query(default=10),
    start: Union[int, None] = Query(default=0),
    bbox: Union[str, None] = Depends(check_bbox),
    point: Union[str, None] = Depends(check_point),
    crs: Union[str, None] = Depends(check_crs),  # Query(default=None),
    result_time: Union[str, None] = Query(default=None),
    datetime_: Union[str, None] = Query(default=None, alias="datetime"),
    observed_property_name: Union[str, None] = Depends(check_observed_property_name),
    dims: Union[str, None] = Depends(check_dims),
    npoints: Union[int, None] = Query(default=4),
    bbox_crs: Union[str, None] = Depends(check_bbox_crs),
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
        "bbox-crs",
    ]
    extra_params = [k for k in req.query_params if k not in allowed_params]

    if len(extra_params):
        return JSONResponse(
            status_code=fastapi_status.HTTP_400_BAD_REQUEST,
            content=jsonable_encoder(
                {
                    "detail": "extra parameters",
                    "body": [extra_params],
                }
            ),
        )
    base_url = req.url_for("get_items_for_collection", coll=coll)
    try:
        features = get_features_for_items(
            coll=coll,
            base_url=base_url,
            point=point,
            bbox=bbox,
            datetime_=datetime_,
            result_time=result_time,
            observed_property_name=observed_property_name,
            npoints=npoints,
            dims=dims,
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
        links = get_items_links(
            req.url_for("get_items_for_collection", coll=coll),
            prev_start=prev_start,
            next_start=next_start,
            limit=limit,
        )

        feature_collection = FeatureCollectionGeoJSON(
            type=Type.FeatureCollection,
            timeStamp="2023-01-09T12:00:00Z",
            links=links,
            features=features_to_return,
            numberMatched=number_matched,
            numberReturned=number_returned,
        )
        response.headers["Content-Crs"] = f"<{DEFAULT_CRS}>"
        if request_type(wanted_format) == "HTML":
            features = [f.dict() for f in feature_collection.features]
            return templates.TemplateResponse(
                "items.html",
                {
                    "request": req,
                    "items": jsonable_encoder(feature_collection),
                    "description": "Description",
                    "collection": coll,
                    "bbox": bbox,
                },
            )
        return feature_collection
    except Exception as exc:
        logger.error(
            "ERR: %s", traceback.format_exception(None, exc, exc.__traceback__)
        )

    return None


@ogcApiApp.get(
    "/collections/{coll}/items/{item_id}",
    response_model=FeatureGeoJSON,
    response_model_exclude_none=True,
)
async def get_item_for_collection(
    coll: str,
    item_id: str,
    req: Request,
    response: Response,
    wanted_format: str = "json",
    crs: Union[str, None] = Depends(check_crs),
):
    url = req.url
    feature_to_return = get_single_item(item_id, str(url))

    if feature_to_return:
        response.headers["Content-Crs"] = f"<{DEFAULT_CRS}>"
        if request_type(wanted_format) == "HTML":
            return templates.TemplateResponse(
                "item.html",
                {
                    "request": req,
                    "collection": coll,
                    "description": "D",
                    "item": jsonable_encoder(feature_to_return),
                },
            )
        return feature_to_return
    return JSONResponse(
        status_code=fastapi_status.HTTP_400_BAD_REQUEST,
        content={
            "detail": "Unknown item_id",
            "body": item_id,
        },
    )
