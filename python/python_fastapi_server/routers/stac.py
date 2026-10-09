"""stacRouter - exposes the autoWMS file tree (datasets/data/autowms) as a STAC catalog"""

import asyncio
import json
import logging
import os
from functools import partial

from fastapi import APIRouter, HTTPException, Request, Response
from owslib.wms import WebMapService

from .autowms import list_data_files, list_dataset_files
from .setup_adaguc import setup_adaguc
from .utils.edr_utils import get_metadata
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


async def fetch_capabilities_xml(wms_base: str) -> bytes | None:
    """Fetch the raw WMS GetCapabilities XML for an adaguc WMS base query (already ending in
    '&'), or None if the adaguc core could not produce a capabilities document for it"""
    capabilities_url = f"{wms_base}request=GetCapabilities"
    status, data, _ = await call_adaguc(capabilities_url.split("?", 1)[1].encode("UTF-8"))
    return data if status == 0 else None


def bbox_and_geometry_from_layers(layers: list) -> tuple[list | None, dict | None]:
    """Derive a WGS84 bbox and matching Polygon geometry from one or more WMS layers"""
    bboxes = [layer.boundingBoxWGS84 for layer in layers if layer.boundingBoxWGS84]
    if not bboxes:
        return None, None
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
    return bbox, geometry


def time_extent_from_layers(layers: list) -> tuple[str | None, str | None, str | None]:
    """Derive (datetime, start_datetime, end_datetime) from one or more WMS layers' time dimension"""
    time_values = set()
    for layer in layers:
        time_dim = (layer.dimensions or {}).get("time")
        if time_dim and time_dim.get("values"):
            time_values.update(time_dim["values"])
    if not time_values:
        return None, None, None
    sorted_times = sorted(time_values)
    if len(sorted_times) == 1:
        return sorted_times[0], None, None
    return None, sorted_times[0], sorted_times[-1]


def resolve_dataset_and_layer(collection_id: str, adaguc_dataset_dir: str) -> tuple[str, str] | None:
    """Split a dotted '{dataset}.{layer}' collection id into (dataset_name, layer_name).

    Dataset names may themselves contain dots, so this tries the longest dataset-name prefix
    first (i.e. assumes the layer name has no dots) and falls back to shorter prefixes, picking
    whichever prefix actually matches a configured dataset XML file."""
    parts = collection_id.split(".")
    for split_at in range(len(parts) - 1, 0, -1):
        candidate_dataset = ".".join(parts[:split_at])
        candidate_layer = ".".join(parts[split_at:])
        if os.path.isfile(os.path.join(adaguc_dataset_dir, f"{candidate_dataset}.xml")):
            return candidate_dataset, candidate_layer
    return None


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
    base_url = get_base_url(req)
    stac_base = f"{base_url}stac"
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

    if clean_path == "adaguc::datasets":
        # Additional resource: datasets may also be queryable through OGC API - EDR.
        links.append(stac_link("data", f"{adaguc_online_resource}edr/collections", title="OGC API - EDR collections"))

    catalog = {
        "stac_version": STAC_VERSION,
        "type": "Catalog",
        "id": clean_path.replace("/", "_"),
        "description": f"STAC catalog for {full_path}",
        "links": links,
    }
    return Response(content=json.dumps(catalog), media_type="application/json", status_code=200)


async def build_stac_item(entry: dict, stac_base: str, parent_path: str, base_url: str) -> dict:
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
    edr_link = None
    stac_extensions = []
    thumbnail_asset = None
    layer_links = []

    # Only dataset= entries (adaguc::datasets) can have an EDR collection or per-layer STAC
    # collections; source= entries (adaguc::data, adaguc::autowms) have no dataset identity
    # for either of those to key on.
    is_dataset_entry = entry["adaguc"].split("?", 1)[1].startswith("dataset=")
    if is_dataset_entry:
        try:
            await get_metadata(item_id)
            edr_link = stac_link(
                "edr",
                f"{base_url}edr/collections/{item_id}",
                title=f"OGC API - EDR collection for {item_id}",
            )
        except Exception:  # pylint: disable=broad-except
            logger.debug("No EDR collection available for %s", item_id, exc_info=True)

    wms_base = f"{entry['adaguc']}service=WMS&version=1.3.0&"
    capabilities_xml = await fetch_capabilities_xml(wms_base)
    if capabilities_xml is not None:
        # The adaguc core was able to generate a capabilities document for this source,
        # so it's safe to advertise it as a working link even if parsing below fails.
        capabilities_href = f"{wms_base}request=GetCapabilities"
        try:
            wms = WebMapService("http://localhost/wms", xml=capabilities_xml, version="1.3.0")
            layers = list(wms.contents.values())
            layer_names = list(wms.contents.keys())

            bbox, geometry = bbox_and_geometry_from_layers(layers)
            datetime_value, start_datetime, end_datetime = time_extent_from_layers(layers)

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

            if is_dataset_entry:
                # Each WMS layer in this dataset is also browsable as its own STAC collection,
                # dot-joined with the dataset name (e.g. "{dataset}.{layer}").
                layer_links = [
                    stac_link(
                        "child",
                        f"{base_url}stac/collections/{item_id}.{layer_name}",
                        title=layer_name,
                    )
                    for layer_name in layer_names
                ]
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
    if edr_link:
        links.append(edr_link)
    links.extend(layer_links)

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

    item = await build_stac_item(entry, stac_base, parent_path, adaguc_online_resource)
    return Response(content=json.dumps(item), media_type="application/geo+json", status_code=200)


@stac_router.get("/stac/collections/{collection_id}")
async def get_stac_collection(collection_id: str, req: Request) -> Response:
    """A STAC collection for a single WMS layer within a dataset, addressed as '{dataset}.{layer}'"""
    adaguc_instance = setup_adaguc()
    base_url = get_base_url(req)
    stac_base = f"{base_url}stac"

    resolved = resolve_dataset_and_layer(collection_id, adaguc_instance.ADAGUC_DATASET_DIR)
    if resolved is None:
        raise HTTPException(status_code=404, detail="Collection not found")
    dataset_name, layer_name = resolved

    wms_base = f"{base_url}adagucserver?dataset={dataset_name}&service=WMS&version=1.3.0&"
    capabilities_xml = await fetch_capabilities_xml(wms_base)
    if capabilities_xml is None:
        raise HTTPException(status_code=404, detail="Collection not found")

    try:
        wms = WebMapService("http://localhost/wms", xml=capabilities_xml, version="1.3.0")
        layer = wms.contents.get(layer_name)
    except Exception as e:  # pylint: disable=broad-except
        raise HTTPException(status_code=404, detail="Collection not found") from e
    if layer is None:
        raise HTTPException(status_code=404, detail="Collection not found")

    bbox, _ = bbox_and_geometry_from_layers([layer])
    datetime_value, start_datetime, end_datetime = time_extent_from_layers([layer])
    temporal_interval = [start_datetime, end_datetime] if (start_datetime or end_datetime) else [datetime_value, datetime_value]

    links = [
        stac_link("self", f"{stac_base}/collections/{collection_id}"),
        stac_link("root", stac_base),
        stac_link(
            "parent",
            f"{stac_base}/item/adaguc::datasets/{dataset_name}.xml",
            media_type="application/geo+json",
            title=dataset_name,
        ),
        stac_link(
            "service-desc",
            f"{wms_base}request=GetCapabilities",
            media_type="text/xml",
            title="WMS GetCapabilities",
        ),
        {
            "rel": "wms",
            "href": wms_base,
            "title": f"WMS endpoint for {layer_name}",
            "type": "image/png",
            "wms:layers": [layer_name],
        },
    ]

    collection = {
        "stac_version": STAC_VERSION,
        "type": "Collection",
        "id": collection_id,
        "description": f"{layer_name} from dataset {dataset_name}",
        "license": "proprietary",
        "extent": {
            "spatial": {"bbox": [bbox or [-180.0, -90.0, 180.0, 90.0]]},
            "temporal": {"interval": [temporal_interval]},
        },
        "links": links,
        "assets": {
            "thumbnail": {
                "href": (
                    f"{wms_base}request=GetMap&format=image/png"
                    f"&layers={layer_name}&width=400"
                    "&crs=EPSG:4326&styles=&exceptions=INIMAGE&showlegend=true"
                ),
                "title": f"Preview of {layer_name}",
                "type": "image/png",
                "roles": ["thumbnail"],
            }
        },
        "stac_extensions": [WEB_MAP_LINKS_EXTENSION],
    }
    return Response(content=json.dumps(collection), media_type="application/json", status_code=200)
