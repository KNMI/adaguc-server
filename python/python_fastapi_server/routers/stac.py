"""stacRouter - exposes the autoWMS file tree (datasets/data/autowms) as a STAC catalog"""

import asyncio
import json
import logging
from functools import partial

from fastapi import APIRouter, HTTPException, Request, Response
from owslib.wms import WebMapService

from .autowms import list_data_files, list_dataset_files
from .setup_adaguc import setup_adaguc
from .utils.ogcapi_tools import call_adaguc
from .utils.utils import get_base_url

stac_router = APIRouter(responses={404: {"description": "Not found"}})

logger = logging.getLogger(__name__)

STAC_VERSION = "1.0.0"

WEB_MAP_LINKS_EXTENSION = "https://stac-extensions.github.io/web-map-links/v1.1.0/schema.json"

TOP_LEVEL_PATHS = ("adaguc::datasets", "adaguc::data", "adaguc::autowms")

MEDIA_TYPES = {
    ".nc": "application/x-netcdf",
    ".nc4": "application/x-netcdf",
    ".hdf5": "application/x-hdf5",
    ".h5": "application/x-hdf5",
    ".png": "image/png",
    ".geojson": "application/geo+json",
    ".json": "application/json",
    ".csv": "text/csv",
    ".xml": "application/xml",
}


def guess_media_type(name: str) -> str:
    """Guess the asset media type from a file name"""
    lower_name = name.lower()
    for ext, media_type in MEDIA_TYPES.items():
        if lower_name.endswith(ext):
            return media_type
    return "application/octet-stream"


def normalize_path(path: str) -> str:
    """Collapse repeated/leading/trailing slashes into a single leading-slash form, so an
    accidental double slash in the URL does not desync href matching elsewhere in this module
    from the single-slash paths list_data_files() returns"""
    return "/" + "/".join(part for part in path.split("/") if part)


def stac_link(rel: str, href: str, media_type: str = "application/json", title: str | None = None) -> dict:
    """Build a STAC link object"""
    link = {"rel": rel, "href": href, "type": media_type}
    if title:
        link["title"] = title
    return link


async def list_entries(full_path: str, adaguc_instance, adaguc_online_resource: str) -> list[dict]:
    """List the datasets/data/autowms entries at a given autoWMS path (mirrors handle_autowms dispatch)"""
    if full_path.startswith("/adaguc::datasets"):
        handler = partial(list_dataset_files, adaguc_instance.ADAGUC_DATASET_DIR, adaguc_online_resource)
    elif full_path.startswith("/adaguc::data"):
        handler = partial(
            list_data_files,
            adaguc_instance.ADAGUC_DATA_DIR,
            full_path,
            adaguc_online_resource,
            "/adaguc::data",
        )
    elif full_path.startswith("/adaguc::autowms"):
        handler = partial(
            list_data_files,
            adaguc_instance.ADAGUC_AUTOWMS_DIR,
            full_path,
            adaguc_online_resource,
            "/adaguc::autowms",
        )
    else:
        raise HTTPException(status_code=404, detail="Path parameter not understood")

    return await asyncio.to_thread(handler)


def parent_catalog_href(full_path: str, stac_base: str) -> str:
    """Return the href of the parent catalog of the given path"""
    trimmed = full_path.strip("/")
    if trimmed in TOP_LEVEL_PATHS:
        return stac_base
    parent = "/".join(trimmed.split("/")[:-1])
    return f"{stac_base}/catalog/{parent}"


@stac_router.get("/stac")
@stac_router.get("/stac/")
async def get_stac_root(req: Request) -> Response:
    """Root of the STAC catalog, linking to the datasets, data and autowms branches"""
    stac_base = f"{get_base_url(req)}stac"
    links = [
        stac_link("self", stac_base),
        stac_link("root", stac_base),
    ]
    for top in TOP_LEVEL_PATHS:
        links.append(stac_link("child", f"{stac_base}/catalog/{top}", title=top))

    catalog = {
        "stac_version": STAC_VERSION,
        "type": "Catalog",
        "id": "adaguc",
        "description": "STAC catalog exposing the datasets, data and autowms files served through the ADAGUC autoWMS API.",
        "links": links,
    }
    return Response(content=json.dumps(catalog), media_type="application/json", status_code=200)


@stac_router.get("/stac/catalog/{path:path}")
async def get_stac_catalog(path: str, req: Request) -> Response:
    """A catalog node representing a directory in the autoWMS file tree"""
    adaguc_instance = setup_adaguc()
    adaguc_online_resource = get_base_url(req)
    stac_base = f"{adaguc_online_resource}stac"

    full_path = normalize_path(path)
    entries = await list_entries(full_path, adaguc_instance, adaguc_online_resource)

    clean_path = full_path.strip("/")
    links = [
        stac_link("self", f"{stac_base}/catalog/{clean_path}"),
        stac_link("root", stac_base),
        stac_link("parent", parent_catalog_href(full_path, stac_base)),
    ]
    for entry in entries:
        entry_path = entry["path"].strip("/")
        if entry["leaf"]:
            links.append(
                stac_link(
                    "item",
                    f"{stac_base}/item/{entry_path}",
                    media_type="application/geo+json",
                    title=entry["name"],
                )
            )
        else:
            links.append(stac_link("child", f"{stac_base}/catalog/{entry_path}", title=entry["name"]))

    catalog = {
        "stac_version": STAC_VERSION,
        "type": "Catalog",
        "id": clean_path.replace("/", "_"),
        "description": f"STAC catalog for {full_path}",
        "links": links,
    }
    return Response(content=json.dumps(catalog), media_type="application/json", status_code=200)


async def build_stac_item(entry: dict, stac_base: str, parent_path: str) -> dict:
    """Build a STAC item for an autoWMS entry, enriching it with extent/time info from WMS GetCapabilities"""
    entry_path = entry["path"].strip("/")
    item_id = entry["name"]

    bbox = None
    geometry = None
    datetime_value = None
    start_datetime = None
    end_datetime = None
    capabilities_href = None
    wms_link = None
    stac_extensions = []
    thumbnail_asset = None

    wms_base = f"{entry['adaguc']}service=WMS&version=1.3.0&"
    capabilities_url = f"{wms_base}request=GetCapabilities"
    status, data, _ = await call_adaguc(capabilities_url.split("?", 1)[1].encode("UTF-8"))
    if status == 0:
        # The adaguc core was able to generate a capabilities document for this source,
        # so it's safe to advertise it as a working link even if parsing below fails.
        capabilities_href = capabilities_url
        try:
            wms = WebMapService("http://localhost/wms", xml=data, version="1.3.0")
            layers = list(wms.contents.values())
            layer_names = list(wms.contents.keys())

            bboxes = [layer.boundingBoxWGS84 for layer in layers if layer.boundingBoxWGS84]
            if bboxes:
                bbox = [
                    min(b[0] for b in bboxes),
                    min(b[1] for b in bboxes),
                    max(b[2] for b in bboxes),
                    max(b[3] for b in bboxes),
                ]
                geometry = {
                    "type": "Polygon",
                    "coordinates": [
                        [
                            [bbox[0], bbox[1]],
                            [bbox[2], bbox[1]],
                            [bbox[2], bbox[3]],
                            [bbox[0], bbox[3]],
                            [bbox[0], bbox[1]],
                        ]
                    ],
                }

            time_values = set()
            for layer in layers:
                time_dim = (layer.dimensions or {}).get("time")
                if time_dim and time_dim.get("values"):
                    time_values.update(time_dim["values"])
            if time_values:
                sorted_times = sorted(time_values)
                if len(sorted_times) == 1:
                    datetime_value = sorted_times[0]
                else:
                    start_datetime, end_datetime = sorted_times[0], sorted_times[-1]

            if layer_names:
                stac_extensions.append(WEB_MAP_LINKS_EXTENSION)
                wms_link = {
                    "rel": "wms",
                    "href": wms_base,
                    "title": f"WMS endpoint for {item_id}",
                    "type": "image/png",
                    "wms:layers": layer_names,
                }
                thumbnail_asset = {
                    "href": (
                        f"{wms_base}request=GetMap&format=image/png"
                        f"&layers={layer_names[0]}&width=400"
                        "&crs=EPSG:4326&styles=&exceptions=INIMAGE&showlegend=true"
                    ),
                    "title": f"Preview of {layer_names[0]}",
                    "type": "image/png",
                    "roles": ["thumbnail"],
                }
        except Exception:  # pylint: disable=broad-except
            logger.warning("Could not derive STAC extent for %s from WMS GetCapabilities", item_id, exc_info=True)

    properties = {"datetime": datetime_value}
    if start_datetime:
        properties["start_datetime"] = start_datetime
    if end_datetime:
        properties["end_datetime"] = end_datetime

    links = [
        stac_link("self", f"{stac_base}/item/{entry_path}", media_type="application/geo+json"),
        stac_link("root", stac_base),
        stac_link("parent", f"{stac_base}/catalog/{parent_path.strip('/')}"),
    ]
    if capabilities_href:
        links.append(stac_link("service-desc", capabilities_href, media_type="text/xml", title="WMS GetCapabilities"))
    if wms_link:
        links.append(wms_link)

    assets = {
        "data": {
            "href": entry["adaguc"],
            "title": item_id,
            "type": guess_media_type(item_id),
            "roles": ["data"],
        }
    }
    if thumbnail_asset:
        assets["thumbnail"] = thumbnail_asset

    item = {
        "stac_version": STAC_VERSION,
        "type": "Feature",
        "id": item_id,
        "geometry": geometry,
        "properties": properties,
        "links": links,
        "assets": assets,
    }
    if bbox:
        item["bbox"] = bbox
    if stac_extensions:
        item["stac_extensions"] = stac_extensions
    return item


@stac_router.get("/stac/item/{path:path}")
async def get_stac_item(path: str, req: Request) -> Response:
    """A STAC item representing a single leaf file in the autoWMS file tree"""
    adaguc_instance = setup_adaguc()
    adaguc_online_resource = get_base_url(req)
    stac_base = f"{adaguc_online_resource}stac"

    full_path = normalize_path(path)
    parent_path = "/".join(full_path.split("/")[:-1]) or "/"

    entries = await list_entries(parent_path, adaguc_instance, adaguc_online_resource)
    # Match on `path` rather than `name`: list_dataset_files strips the .xml suffix from
    # `name` but keeps it in `path`, so `path` is the only field consistent with the URL.
    entry = next((e for e in entries if e["leaf"] and e["path"].strip("/") == full_path.strip("/")), None)
    if entry is None:
        raise HTTPException(status_code=404, detail="Item not found")

    item = await build_stac_item(entry, stac_base, parent_path)
    return Response(content=json.dumps(item), media_type="application/geo+json", status_code=200)
