from fastapi import Request, Response, FastAPI, APIRouter

from typing import Optional

from .edr_types.styles import Styles
from .edr_types.maps_service_descr import maps_service_descr
from .edr_types.landing_page import landing_page
from .edr_types.collections import collections
from .edr_types.collection import collection
from .edr_types.link import Link
from .edr_types.conformance import conformance
from .edr_types.extent import Extent
from .edr_types.style import Style
import json

from owslib.wms import WebMapService
from .setup_adaguc import setup_adaguc
from defusedxml.ElementTree import fromstring, parse, ParseError

from pyproj import CRS, Transformer
import logging
import os
import time

logger = logging.getLogger(__name__)
logger.debug("Starting OGCAPI Maps")

# mapsApiApp = FastAPI(debug=True)

routers = {}

router = APIRouter()


def create_maps_routes(mainApp: FastAPI, datasets: list[str]):
    for dataset in datasets:
        newapi = FastAPI(openapi_url=f"/api",
                         root_path=f"/maps/{dataset}",
                         servers=[{
                             "url": f"/maps/{dataset}"
                         }],
                         debug=True)
        new_router = create_router_endpoint(dataset)
        newapi.include_router(new_router, )
        if "servers" in newapi.openapi():
            print("servers:", newapi.openapi()["servers"])
        mainApp.mount(f"/maps/{dataset}", newapi)
    for r in mainApp.routes:
        print(r)


def create_router_endpoint(dataset: str):
    routers[dataset] = {}
    routers[dataset]["maps_service"] = maps_service_descr.custom_init(
        service_root=f"http://localhost:8000/maps/{dataset}")
    return router


def get_dataset(request: Request):
    return request.url.path.strip("/").split("/")[1]


def get_baseurl(request: Request):
    dataset = get_dataset(request)
    url = request.url
    return f"{url.scheme}://{url.hostname}:{url.port}/maps/{dataset}"


@router.get("/", response_model=landing_page, response_model_exclude_none=True)
async def get_landing_page(request: Request):
    logger.info("BASEREQUEST:%s %s", request.url, request.app)
    print("ROOT:", request.scope.get("root_path"))
    dataset = get_dataset(request)
    return maps_service_descr.get_landing_page(
        service_root=request.scope.get("root_path"))
    # routers[dataset]["maps_service"].landing.dict()


conformanceClasses: list[str] = [
    "http://www.opengis.net/spec/ogcapi-common-1/1.0/conf/core",
    "http://www.opengis.net/spec/ogcapi-common-2/1.0/conf/collections",
    "http://www.opengis.net/spec/ogcapi-maps-1/1.0/conf/core",
]

maps_conformance = conformance.custom_init(_conformsTo=conformanceClasses)


@router.get("/conformance", response_model=conformance)
async def get_conformance():
    return maps_conformance.dict()


def callADAGUC(url):
    logger.info("CALLING %s", url)
    """Call adaguc-server"""
    adagucInstance = setup_adaguc()

    url = url.decode()
    logger.info(">>>>>%s", url)
    if "?" in url:
        url = url[url.index("?") + 1:]
    logger.info(url)
    stage1 = time.perf_counter()

    adagucenv = {}

    # Set required environment variables
    adagucenv["ADAGUC_ONLINERESOURCE"] = (
        os.getenv("EXTERNALADDRESS", "http://192.168.178.113:8080") +
        "/adaguc-server?")
    adagucenv["ADAGUC_DB"] = os.getenv(
        "ADAGUC_DB",
        "user=adaguc password=adaguc host=localhost dbname=adaguc")

    # Run adaguc-server
    # pylint: disable=unused-variable
    status, data, headers = adagucInstance.runADAGUCServer(url,
                                                           env=adagucenv,
                                                           showLogOnError=True)

    # Obtain logfile
    logfile = adagucInstance.getLogFile()
    adagucInstance.removeLogFile()

    stage2 = time.perf_counter()
    logger.info("[PERF] Adaguc execution took: %f", (stage2 - stage1))

    if len(logfile) > 0:
        logger.info(logfile)

    return status, data


def get_capabilities(dataset: str):
    """
    Get the collectioninfo from the WMS GetCapabilities
    """
    logger.info("callADAGUC by dataset")
    urlrequest = f"dataset={dataset}&service=wms&version=1.3.0&request=getcapabilities"
    status, response = callADAGUC(url=urlrequest.encode("UTF-8"))
    logger.info("status: %d", status)
    if status == 0:
        xml = response.getvalue()
        wms = WebMapService(dataset, xml=xml, version="1.3.0")
        return wms.contents
    else:
        logger.error("status: %d", status)
        return {}


def get_extent(contents):
    """
    Get the boundingbox extent from the WMS GetCapabilities
    """
    # print("get_extent(", next(iter(contents)), ")")
    logger.info("get_extent(%s)", contents.boundingBoxWGS84)
    if len(contents.boundingBoxWGS84):
        return contents.boundingBoxWGS84
    return None


def get_time_extent(contents):
    """
    Get the temporal extent from the WMS GetCapabilities
    """
    logger.info("get_time_extent()")
    dims = get_dimensions(contents)
    for dim in dims:
        if dim == "time":
            return dims[dim]
    return []


def get_other_extents(contents):
    """
    Get the "other" extents from the WMS GetCapabilities
    """
    dims = get_dimensions(contents)
    newdims = {}
    for dim in dims:
        if dim != "time":
            newdims[dim] = dims[dim]
    if len(newdims) == 0:
        return None
    return newdims


def get_dimensions(contents):
    dims = {}
    for dim in contents.dimensions:
        dims[dim] = contents.dimensions[dim]["values"]
    return dims


def get_styles(l: str, base_url: str, contents):
    # logger.info("get_styles() for %s: %s", l, json.dumps(contents.styles, indent=2))
    styles_list: list[Style] = []
    for styl in contents.styles.keys():
        fixed_style = styl.replace("/", "|")
        new_links: list[Link] = []
        new_links.append(
            Link.custom_init(
                href=base_url + f"/collections/{l}/styles/{fixed_style}",
                rel="self",
                type="application/json",
            ))
        new_links.append(
            Link.custom_init(
                href=base_url + f"/collections/{l}/styles/{fixed_style}/map",
                rel="http://www.opengis.net/def/rel/ogc/1.0/map",
                type="application/json",
            ))
        new_links.append(
            Link.custom_init(
                href=base_url +
                f"/collections/{l}/styles/{fixed_style}/legend",
                rel="legend",
            ))
        new_style = Style.custom_init(id=fixed_style,
                                      title=f"Style {fixed_style}",
                                      links=new_links)
        styles_list.append(new_style)

    styles = Styles.custom_init(styles=styles_list, links=[])
    return styles


def get_crs(wmscontents: str) -> list[str]:
    logger.info("CRS: %s", wmscontents.crsOptions)
    crs = []
    for _crs in wmscontents.crsOptions:
        if (_crs.startswith("EPSG:") or _crs.startswith("AUTO:")
                or _crs.startswith("CRS")):
            crs.append(_crs)
    return crs


def list_collections(dataset: str, base_url: str) -> collections:
    collection_list: list[collection] = []
    links = []
    self_link = Link.custom_init(
        href=base_url + "/collections",
        rel="self",
        type="application/json",
    )
    links.append(self_link)
    wmscontents = get_capabilities(dataset)
    for l in wmscontents.keys():
        logger.info("l: %s", l)
        coll = list_collection(dataset, l, base_url, wmscontents[l])
        collection_list.append(coll)
    return collections.custom_init(colls=collection_list, links=links)


def list_collection(dataset: str, l: str, base_url: str,
                    layer_wms_contents) -> collection:
    logger.info("list_collection")
    extent = Extent.custom_init(
        get_extent(layer_wms_contents, ),
        "http://www.opengis.net/def/crs/OGC/1.3/CRS84",
        get_time_extent(layer_wms_contents, ),
        "ISO8601",
        get_other_extents(layer_wms_contents, ),
    )
    logger.info("EXTENT READY")

    selflink = Link.custom_init(
        href=base_url + "/collections/%s" % (l, ),
        rel="self",
        type="application/json",
    )
    maplink = Link.custom_init(
        href=base_url + "/collections/%s/map" % (l, ),
        rel="http://www.opengis.net/def/rel/ogc/1.0/map",
        type="application/json",
    )
    styleslink = Link.custom_init(
        href=base_url + "/collections/%s/styles" % (l, ),
        rel="http://www.opengis.net/def/rel/ogc/1.0/styles",
        type="application/json",
    )
    # dims = get_dimensions(l, base_url, layer_wms_contents)
    # logger.info("DIMS(%s): %s", l, dims)

    styles = get_styles(l, base_url, layer_wms_contents)

    crs = get_crs(layer_wms_contents)
    coll = collection.custom_init(
        id=l,
        links=[selflink, maplink, styleslink],
        title=f"{dataset} layer {l}",
        extent=extent,
        styles=styles,
        crs=crs,
    )
    return coll


@router.get("/collections/{id}",
            response_model=collection,
            response_model_exclude_none=True)
async def get_collection(id: str, request: Request):
    dataset = get_dataset(request)
    wmscontent = get_capabilities(dataset)
    if id not in wmscontent:
        return {"message": f"incorrect parameter {id}"}
    return list_collection(dataset, id, get_baseurl(request), wmscontent[id])


crs_store = {}
transformer_store = {}


def get_crs_from_store(key: str):
    if key in crs_store:
        return crs_store[key]
    else:
        crs_store[key] = CRS.from_string(key)
        return crs_store[key]


def get_transformer_from_store(from_crs_key: str, to_crs_key: str):
    key = from_crs_key + to_crs_key
    if key in transformer_store:
        return transformer_store[key]
    else:
        from_crs = get_crs_from_store(from_crs_key)
        to_crs = get_crs_from_store(to_crs_key)
        new_transformer = Transformer.from_crs(from_crs, to_crs)
        transformer_store[key] = new_transformer
        return new_transformer


def get_bbox(bbox: str, crs: str, bbox_crs: str = None):
    logger.info("get_bbox(%s,%s)", bbox, crs)
    to_crs_key = crs
    from_crs_key = bbox_crs or "OGC:CRS84"  #urn:ogc:def:crs:OGC:1.3:CRS84"
    transformer = get_transformer_from_store(from_crs_key, to_crs_key)
    bb = bbox.split(",")[0:4]
    ll = transformer.transform(bb[0], bb[1])
    ur = transformer.transform(bb[2], bb[3])

    return "%f,%f,%f,%f" % (ll[0], ll[1], ur[0], ur[1])


@router.get(
    "/collections/{id}/map",
    response_class=Response,
    responses={200: {
        "content": {
            "image/png": {}
        }
    }},
)
async def get_collection_map(
    id: str,
    request: Request,
    width: int = 600,
    height: int = 600,
    crs: str = "http://www.opengis.net/def/crs/EPSG/0/3857",
    bbox: str = "0,45,10,55",
    bbox_crs: Optional[str] = None,
    transparent: Optional[str] = None,
):
    logger.info("Map for %s %dx%d %s %s %s", id, width, height, crs, bbox,
                transparent)
    dataset = get_dataset(request)
    bbox_crs = None
    bbox_fixed = get_bbox(bbox, crs, bbox_crs)
    adaguc_crs = "EPSG:3857"
    urlrequest = f"DATASET={dataset}&service=wms&version=1.3.0&request=getMap&"
    urlrequest += f"width={width}&height={height}&bbox={bbox_fixed}&layers={id}&"
    urlrequest += "format=image/png&"
    urlrequest += f"crs={adaguc_crs}&"
    if transparent and transparent.upper() == "TRUE":
        urlrequest += "transparent=true&"
    logger.info("URL: %s", urlrequest)
    status, response = callADAGUC(url=urlrequest.encode("UTF-8"))
    logger.info("status: %d", status)
    if status == 0:
        return Response(content=response.getvalue(), media_type="image/png")

    return {}


@router.get(
    "/collections/{id}/styles",
    response_model=Styles,
    response_model_exclude_unset=True,
    response_model_exclude_none=True,
)
async def get_collection_styles(
    id: str,
    request: Request,
):
    dataset = get_dataset(request)
    wmscontents = get_capabilities(dataset)
    base_url = get_baseurl(request)
    styles = get_styles(id, base_url, wmscontents[id])
    return styles.dict()


@router.get(
    "/collections/{id}/styles/{style}",
    response_model=Style,
    response_model_exclude_none=True,
)
async def get_collection_styles(
    id: str,
    style: str,
    request: Request,
):
    dataset = get_dataset(request)
    wmscontents = get_capabilities(dataset)
    base_url = get_baseurl(request)
    styles = get_styles(id, get_baseurl(request), wmscontents[id])
    if style == "default":
        return styles.styles[0]
    for styl in styles.styles:
        if styl.id == style:
            return styl.dict()
    return {}


@router.get(
    "/collections/{id}/styles/{style}/map",
    responses={200: {
        "content": {
            "image/png": {}
        }
    }},
    response_class=Response,
)
async def get_collection_map_styled(
    id: str,
    style: str,
    request: Request,
    width: int = 600,
    height: int = 600,
    crs: str = "http://www.opengis.net/def/crs/EPSG/0/3857",
    bbox: str = "0,45,10,55",
    bbox_crs: Optional[str] = None,
    transparent: Optional[str] = None,
):
    fixed_style = style.replace("|", "/")
    logger.info(
        "Styled Map for %s %dx%d %s %s %s %s(%s)",
        id,
        width,
        height,
        crs,
        bbox,
        transparent,
        fixed_style,
        style,
    )
    time1 = time.time()
    dataset = get_dataset(request)

    bbox_crs = None
    bbox_fixed = get_bbox(bbox, crs, bbox_crs)
    adaguc_crs = "EPSG:3857"
    urlrequest = f"DATASET={dataset}&service=wms&version=1.3.0&request=getMap&"
    urlrequest += f"width={width}&height={height}&bbox={bbox_fixed}&layers={id}&"
    urlrequest += "format=image/png&"
    urlrequest += f"crs={adaguc_crs}&"
    urlrequest += f"STYLES={fixed_style}&"

    if transparent and transparent.upper() == "TRUE":
        urlrequest += "transparent=true&"
    logger.info("URL: %s", urlrequest)

    status, response = callADAGUC(url=urlrequest.encode("UTF-8"))
    logger.info("status: %d", status)
    dt = time.time() - time1
    logger.info("Took %f", dt)
    if status == 0:
        return Response(content=response.getvalue(), media_type="image/png")

    return {}


@router.get("/collections",
            response_model=collections,
            response_model_exclude_none=True)
async def get_collections(request: Request):
    dataset = get_dataset(request)
    return list_collections(dataset, get_baseurl(request))
