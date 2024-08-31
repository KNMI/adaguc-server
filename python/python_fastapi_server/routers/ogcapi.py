"""ogcApiApp"""

import json
import logging
import os
import os.path
from collections import OrderedDict
from typing import Dict, List, Type, Union

import yaml
from defusedxml.ElementTree import fromstring
from fastapi import Depends, FastAPI, HTTPException, Query, Request, Response
from fastapi import status as fastapi_status
from fastapi.encoders import jsonable_encoder
from fastapi.exceptions import RequestValidationError
from fastapi.openapi.utils import get_openapi
from fastapi.responses import JSONResponse
from fastapi.staticfiles import StaticFiles
from fastapi.templating import Jinja2Templates

from .models.ogcapifeatures_1_model import (
    Collection,
    Collections,
    ConfClasses,
    Extent,
    FeatureCollectionGeoJSON,
    FeatureGeoJSON,
    LandingPage,
    Link,
    Spatial,
    Type,
)
from .ogcapi_tools import (
    calculate_coords,
    call_adaguc,
    feature_from_dat,
    generate_collections,
    get_extent,
    get_items_links,
    get_parameters,
    make_bbox,
)

logger = logging.getLogger(__name__)

DEFAULT_CRS = "http://www.opengis.net/def/crs/OGC/1.3/CRS84"
SUPPORTED_CRS_LIST = [DEFAULT_CRS]

ogcApiApp = FastAPI(debug=False, openapi_url="/api")


def custom_openapi():
    """Returns fixed openapi schema"""
    if ogcApiApp.openapi_schema:
        return ogcApiApp.openapi_schema
    openapi_schema = make_open_api()
    ogcApiApp.openapi_schema = openapi_schema
    return ogcApiApp.openapi_schema


ogcApiApp.openapi = custom_openapi

script_dir = os.path.dirname(__file__)
static_abs_file_path = os.path.join(script_dir, "static")
ogcApiApp.mount("/static", StaticFiles(directory=static_abs_file_path), name="static")
templates_abs_file_path = os.path.join(script_dir, "templates/ogcapi")
templates = Jinja2Templates(directory=templates_abs_file_path)


@ogcApiApp.exception_handler(RequestValidationError)
async def validation_exception_handler(exc: RequestValidationError):
    """Exception handler"""
    return JSONResponse(
        status_code=fastapi_status.HTTP_400_BAD_REQUEST,
        content=jsonable_encoder({"detail": exc.errors(), "body": exc.body}),
    )


@ogcApiApp.get("/", response_model=LandingPage, response_model_exclude_none=True)
@ogcApiApp.get("", response_model=LandingPage, response_model_exclude_none=True)
async def handle_ogc_api_root(
    req: Request,
    response: Response,
    responsetype: str = Query(default="json", alias="f"),
):
    """
    Landing page of OGCAPI Features endpoint
    """
    links: List[Link] = []
    links.append(
        Link(
            href=str(req.url_for("handle_ogc_api_root")),
            rel="self",
            title="This document in JSON",
            type="application/json",
        )
    )
    links.append(
        Link(
            href=str(req.url_for("handle_ogc_api_root")) + "?f=html",
            rel="alternate",
            title="This document in HTML",
            type="text/html",
        )
    )
    links.append(
        Link(
            href=str(req.url_for("get_conformance")),
            rel="conformance",
            title="Conformance document",
            type="application/json",
        )
    )
    links.append(
        Link(
            href=str(req.url_for("get_collections")),
            rel="data",
            title="Collections",
            type="application/json",
        )
    )
    links.append(
        Link(
            href=str(req.url_for("openapi")),
            rel="service-desc",
            title="The OpenAPI definition as JSON",
            type="application/vnd.oai.openapi+json;version=3.0",
        )
    )
    links.append(
        Link(
            href=str(req.url_for("get_open_api_yaml")),
            rel="service-desc",
            title="The OpenAPI definition as YAML",
            type="application/vnd.oai.openapi;version=3.0",
        )
    )
    landing_page = LandingPage(
        title="ogcapi", description="ADAGUC OGCAPI-Features server", links=links
    )

    response.headers["cache-control"] = "max-age=60"

    if request_type(responsetype) == "HTML":
        return templates.TemplateResponse(
            "landingpage.html",
            {"request": req, "landingpage": landing_page.model_dump()},
        )
    return landing_page


def get_collection_links(url):
    """
    Create the list of links for a collection url
    """
    links: List[Link] = []
    url = str(url)
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
    """
    Create the list of links for a collection url
    """
    links: List[Link] = []
    url = str(url)
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
    """
    Determine requested responsetype
    """
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
async def get_collections(
    req: Request,
    response: Response,
    responsetype: str = Query(default="json", alias="f"),
):
    """
    Handle the /collections call
    """
    collections: List[Collection] = []
    parsed_collections = generate_collections()
    for parsed_collection in parsed_collections.values():
        parsed_extent = await get_extent(parsed_collection["dataset"])
        if parsed_extent:
            spatial = Spatial(bbox=[list(parsed_extent)])
            extent = Extent(spatial=spatial)
            collections.append(
                Collection(
                    id=parsed_collection["dataset"],
                    title="title1",
                    description="descr1",
                    links=get_collection_links(
                        str(
                            req.url_for(
                                "get_collection",
                                coll=parsed_collection["dataset"],
                            )
                        )
                    ),
                    extent=extent,
                    itemType="feature",
                    crs=[DEFAULT_CRS],
                    storageCrs=DEFAULT_CRS,
                )
            )

    links = get_collections_links(req.url_for("get_collections"))

    response.headers["cache-control"] = "max-age=60"

    if request_type(responsetype) == "HTML":
        collections_list = [c.model_dump() for c in collections]
        return templates.TemplateResponse(
            "collections.html", {"request": req, "collections": collections_list}
        )

    return Collections(
        links=links,
        collections=collections,
    )


@ogcApiApp.get(
    "/collections/{coll}", response_model=Collection, response_model_exclude_none=True
)
async def get_collection(
    coll: str, req: Request, responsetype: str = Query(default="json", alias="f")
):
    """
    Return the data for a named collection
    """
    extent = Extent(spatial=Spatial(bbox=[await get_extent(coll)]))
    collection = Collection(
        id=coll,
        title="title1",
        description="descr1",
        extent=extent,
        links=get_collection_links(str(req.url_for("get_collection", coll=coll))),
    )
    if request_type(responsetype) == "HTML":
        return templates.TemplateResponse(
            "collection.html", {"request": req, "collection": collection.model_dump()}
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
async def get_conformance(
    req: Request, responsetype: str = Query(default="json", alias="f")
):
    """
    Handle the /conformance call
    """
    conf_classes = ConfClasses(conformsTo=conformanceClasses)
    if request_type(responsetype) == "HTML":
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
    """Adjusts openapi object"""
    openapi = get_openapi(
        title=ogcApiApp.title,
        version=ogcApiApp.version,
        routes=ogcApiApp.routes,
        servers=ogcApiApp.servers,
    )
    paths = openapi.get("paths", {})
    for pth in paths.keys():
        params = paths.get(pth).get("get", {}).get("parameters", [])
        for param in params:
            if param["in"] == "query" and param["name"] == "bbox":
                param["style"] = "form"
                param["explode"] = False
                param["schema"] = {
                    "type": "array",
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


@ogcApiApp.get("/api.yaml")
async def get_open_api_yaml():
    openapi = custom_openapi()
    openapi_yaml = yaml.dump(openapi, sort_keys=False)
    return Response(
        content=openapi_yaml,
        media_type="application/vnd.oai.openapi;version=3.0",
    )


async def get_single_item(item_id: str, url: str) -> FeatureGeoJSON:
    collection, observed_property_name, point, dims, datetime_ = item_id.split(";")
    coord = [float(c) for c in point.split(",")]
    dimspec = ""
    if len(dims):
        for dim in dims.split("|"):
            dimname, dimval = dim.split("=")
            dimspec += f"&{dimname}={dimval}"

    datetime_ = datetime_.replace("$", "/")
    request_url = (
        f"http://localhost:8080/wms?dataset={collection}&query_layers={observed_property_name}"
        + "&service=WMS&version=1.3.0&request=getPointValue"
        + "&FORMAT=application/json&INFO_FORMAT=application/json"
        + f"&X={coord[0]}&Y={coord[1]}&CRS=EPSG:4326"
    )
    if datetime_:
        request_url += f"&TIME={datetime_}"
    request_url += dimspec
    status, data, _ = await call_adaguc(request_url.encode("UTF-8"))
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


async def get_features_for_items(
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
            bbox = make_bbox(await get_extent(coll))

        if not npoints:
            npoints = 4
        coords = calculate_coords(bbox, npoints, npoints)
    else:
        coords = [point]
    if not observed_property_name:
        collinfo = await get_parameters(coll)
        first_param = next(iter(collinfo))
        observed_property_name = [first_param["name"]]
        # Default observedPropertyName = first layername

    param_list = ",".join(observed_property_name)

    dimspec = ""
    if dims:
        for dimname, dimval in dims.items():
            dimspec += f"&{dimname}={dimval}" % (dimname, dimval)

    for coord in coords:
        request_url = (
            f"http://localhost:8080/wms?dataset={coll}&query_layers={param_list}"
            + "&service=WMS&version=1.3.0&request=getPointValue&"
            + "FORMAT=application/json&INFO_FORMAT=application/json"
            + f"&X={coord[0]}&Y={coord[1]}&CRS=EPSG:4326"
        )
        if datetime_:
            request_url += f"&TIME={datetime_}"
        if result_time:
            request_url += f"&DIM_REFERENCE_TIME={result_time}"
        request_url += dimspec
        status, data, _ = await call_adaguc(request_url.encode("UTF-8"))
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
                return 400, retval
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
    return [float(c) for c in coords]


def check_bbox(bbox: Union[str, None] = Query(default=None)) -> List[float]:
    if bbox is None:
        return None
    coords = bbox.split(",")
    if len(coords) != 4 and len(coords) != 6:
        # Error
        raise HTTPException(status_code=404, detail="bbox should contain 4 or 6 floats")
    return [float(c) for c in coords]


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
    responsetype: str = Query(default="json", alias="f"),
    limit: Union[int, None] = Query(default=10),
    start: Union[int, None] = Query(default=0),
    bbox: Union[str, None] = Depends(check_bbox),
    point: Union[str, None] = Depends(check_point),
    result_time: Union[str, None] = Query(default=None),
    datetime_: Union[str, None] = Query(default=None, alias="datetime"),
    observed_property_name: Union[str, None] = Depends(check_observed_property_name),
    dims: Union[str, None] = Depends(check_dims),
    npoints: Union[int, None] = Query(default=4),
):
    allowed_params = [
        "responsetype",
        "limit",
        "start",
        "point",
        "bbox",
        "datetime",
        "resultTime",
        "observed_property_name",
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
    base_url = str(req.url_for("get_items_for_collection", coll=coll))
    features = await get_features_for_items(
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
        str(req.url_for("get_items_for_collection", coll=coll)),
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
    if request_type(responsetype) == "HTML":
        features = [f.model_dump() for f in feature_collection.features]
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
    responsetype: str = Query(defailt="json", alias="f"),
):
    url = req.url
    feature_to_return = await get_single_item(item_id, str(url))

    if feature_to_return:
        response.headers["Content-Crs"] = f"<{DEFAULT_CRS}>"
        if request_type(responsetype) == "HTML":
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
