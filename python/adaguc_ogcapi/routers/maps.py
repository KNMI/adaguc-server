from fastapi import APIRouter, Request, Response, FastAPI

from ..types.styles import Styles
from ..types.modelx import modelx
from ..types.maps_service_descr import maps_service_descr
from ..types.landing_page import landing_page
from ..types.collections import collections
from ..types.collection import collection
from ..types.link import Link
from ..types.conformance import conformance
from ..types.extent import Extent
from ..types.style import Style

from ..configureLogging import configureLogging

from owslib.wms import WebMapService
from .setup_adaguc import setup_adaguc
from defusedxml.ElementTree import fromstring, parse, ParseError

from pyproj import CRS, Transformer
import logging
import os
import time

configureLogging(logging)
logger = logging.getLogger(__name__)
logger.debug("Starting")

routers = {}

router = APIRouter(responses={404: {"description": "Not found"}})


def add_router(dataset: str, app: FastAPI):
    routers[dataset] = {}
    routers[dataset]["maps_service"] = maps_service_descr.custom_init(
        service_root=f"http://localhost:8000/maps/{dataset}"
    )
    routers[dataset]["part_path"] = f"/maps/{dataset}"

    app.include_router(router, prefix=f"/maps/{dataset}")


# router = APIRouter(prefix="/maps/RADAR", responses={404: {"description": "Not found"}})

# maps_service = maps_service_descr.custom_init(
#     service_root="http://localhost:8000/maps/RADAR"
# )


def get_dataset(request: Request):
    return request.url.path.strip("/").split("/")[1]


def get_baseurl(request: Request):
    dataset = get_dataset(request)
    url = request.url
    return f"{url.scheme}://{url.hostname}:{url.port}/maps/{dataset}"


@router.get("/", response_model=landing_page, response_model_exclude_none=True)
async def get_landing_page(request: Request):
    logger.info("BASEREQUEST:%s %s", request.url, request.app)
    dataset = get_dataset(request)
    return routers[dataset]["maps_service"].landing.dict()


conformanceClasses: list[str] = [
    "http://www.opengis.net/spec/ogcapi-common-1/1.0/conf/core",
    "http://www.opengis.net/spec/ogcapi-common-2/1.0/conf/collections",
    "http://www.opengis.net/spec/ogcapi-maps-1/1.0/conf/core",
]

maps_conformance = conformance.custom_init(_conformsTo=conformanceClasses)


@router.get("/conformance", response_model=conformance)
async def get_conformance():
    return maps_conformance.dict()


@router.get("/x", response_model=modelx)
async def get_x():
    mdlx = modelx.custom_init(x=6)
    return modelx


def callADAGUC(url):
    logger.info("CALLING %s", url)
    """Call adaguc-server"""
    adagucInstance = setup_adaguc()

    url = url.decode()
    logger.info(">>>>>%s", url)
    if "?" in url:
        url = url[url.index("?") + 1 :]
    logger.info(url)
    stage1 = time.perf_counter()

    adagucenv = {}

    # Set required environment variables
    adagucenv["ADAGUC_ONLINERESOURCE"] = (
        os.getenv("EXTERNALADDRESS", "http://192.168.178.113:8080") + "/adaguc-server?"
    )
    adagucenv["ADAGUC_DB"] = os.getenv(
        "ADAGUC_DB", "user=adaguc password=adaguc host=localhost dbname=adaguc"
    )

    # Run adaguc-server
    # pylint: disable=unused-variable
    status, data, headers = adagucInstance.runADAGUCServer(
        url, env=adagucenv, showLogOnError=True
    )

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
    else:
        logger.error("status: %d", status)
        return {}
    return wms.contents


def get_extent(contents):
    """
    Get the boundingbox extent from the WMS GetCapabilities
    """
    # print("get_extent(", next(iter(contents)), ")")
    logger.info("get_extent(%s)", contents.boundingBoxWGS84)
    if len(contents.boundingBoxWGS84):
        return contents.boundingBoxWGS84
    return None


def get_styles(l: str, base_url: str, contents):
    logger.info("get_styles(%s) for %s", contents.styles, license)
    styles_list: list[Style] = []
    for styl in contents.styles.keys():
        fixed_style = styl.replace("/", "|")
        new_links: list[Link] = []
        new_links.append(
            Link.custom_init(
                href=base_url + f"/collections/{l}/styles/{fixed_style}",
                rel="self",
                type="application/json",
            )
        )
        new_links.append(
            Link.custom_init(
                href=base_url + f"/collections/{l}/styles/{fixed_style}/map",
                rel="http://www.opengis.net/def/rel/ogc/1.0/map",
                type="application/json",
            )
        )
        new_links.append(
            Link.custom_init(
                href=base_url + f"/collections/{l}/styles/{fixed_style}/legend",
                rel="legend",
            )
        )
        new_style = Style.custom_init(
            id=fixed_style, title=f"Style {fixed_style}", links=new_links
        )
        styles_list.append(new_style)

    styles = Styles.custom_init(styles=styles_list, links=[])
    return styles


def list_collections(dataset: str, base_url: str):
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


def list_collection(dataset: str, l: str, base_url: str, layer_wms_contents):
    extent = Extent.custom_init(
        get_extent(
            layer_wms_contents,
        ),
        "http://www.opengis.net/def/crs/OGC/1.3/CRS84",
    )
    styles = get_styles(l, base_url, layer_wms_contents)
    selflink = Link.custom_init(
        href=base_url + "/collections/%s" % (l,),
        rel="self",
        type="application/json",
    )
    maplink = Link.custom_init(
        href=base_url + "/collections/%s/map" % (l,),
        rel="http://www.opengis.net/def/rel/ogc/1.0/map",
        type="application/json",
    )
    styleslink = Link.custom_init(
        href=base_url + "/collections/%s/styles" % (l,),
        rel="http://www.opengis.net/def/rel/ogc/1.0/styles",
        type="application/json",
    )
    coll = collection.custom_init(
        id=l,
        links=[selflink, maplink, styleslink],
        title=f"{dataset} layer {l}",
        extent=extent,
        styles=styles,
    )
    return coll


@router.get("/collections/{id}")
async def get_collection(id: str, request: Request):
    dataset = get_dataset(request)
    wmscontent = get_capabilities(dataset)
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
    from_crs_key = bbox_crs or "crs84"
    transformer = get_transformer_from_store(from_crs_key, to_crs_key)
    bb = bbox.split(",")[0:4]
    ll = transformer.transform(bb[0], bb[1])
    ur = transformer.transform(bb[2], bb[3])

    return "%f,%f,%f,%f" % (ll[0], ll[1], ur[0], ur[1])


@router.get(
    "/collections/{id}/map",
    response_class=Response,
    responses={200: {"content": {"image/png": {}}}},
)
async def get_collection_map(
    id: str,
    request: Request,
    width: int = 600,
    height: int = 600,
    crs: str = "http://www.opengis.net/def/crs/EPSG/0/3857",
    bbox: str = "0,45,10,55",
    bbox_crs: str | None = None,
    transparent: str | None = None,
):
    logger.info("Map for %s %dx%d %s %s %s", id, width, height, crs, bbox, transparent)
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
)
async def get_collection_styles(
    id: str,
    request: Request,
):
    dataset = get_dataset(request)
    wmscontents = get_capabilities(dataset)
    base_url = get_baseurl(request)
    styles = get_styles(id, base_url, wmscontents[id])
    return styles


@router.get(
    "/collections/{id}/styles/{style}",
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
            return styl
    return {}


@router.get(
    "/collections/{id}/styles/{style}/map",
    responses={200: {"content": {"image/png": {}}}},
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
    bbox_crs: str | None = None,
    transparent: str | None = None,
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


logger.info("OK")


@router.get(
    "/collections", response_model=collections, response_model_exclude_none=True
)
async def get_collections(request: Request):
    dataset = get_dataset(request)
    return list_collections(dataset, get_baseurl(request))
