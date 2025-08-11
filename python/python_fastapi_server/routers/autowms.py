"""autoWmsRouter"""

import asyncio
from functools import partial
import json
import logging
import os
import urllib.parse

from fastapi import APIRouter, HTTPException, Request, Response

from .setup_adaguc import setup_adaguc

autowms_router = APIRouter(responses={404: {"description": "Not found"}})

logger = logging.getLogger(__name__)


def handle_base_route() -> Response:
    datasets = [
        {"path": "/adaguc::datasets", "name": "adaguc::datasets", "leaf": False},
        {"path": "/adaguc::data", "name": "adaguc::data", "leaf": False},
        {"path": "/adaguc::autowms", "name": "adaguc::autowms", "leaf": False},
    ]

    return Response(
        content=json.dumps({"result": datasets}),
        media_type="application/json",
        status_code=200,
    )


def list_dataset_files(
    adaguc_dataset_dir: str, adaguc_online_resource: str
) -> list[dict]:
    """Return a list of xml files (datasets) found in the adaguc dataset directory"""

    datasets = []
    with os.scandir(adaguc_dataset_dir) as entries:
        for entry in entries:
            if entry.is_file() and entry.name.lower().endswith(".xml"):
                datasets.append(
                    {
                        "path": f"/adaguc::datasets/{entry.name}",
                        "adaguc": f"{adaguc_online_resource}/adagucserver?dataset={entry.name.replace('.xml', '')}&",
                        "name": entry.name.replace(".xml", ""),
                        "leaf": True,
                    }
                )

    datasets.sort(key=lambda f: f["name"].upper())
    return datasets


def list_data_files(
    data_dir: str, url_param_path: str, adaguc_online_resource: str, autowms_prefix: str
) -> list[dict]:
    """Return a list of (allowed) files and directories found in the requested data directory"""

    ALLOWED_EXTENSIONS = (
        ".nc",
        ".nc4",
        ".hdf5",
        ".h5",
        ".png",
        ".json",
        ".geojson",
        ".csv",
    )

    sub_path = url_param_path.replace(f"{autowms_prefix}/", "")
    sub_path = sub_path.replace(autowms_prefix, "")
    if len(sub_path) != 0:
        sub_path = sub_path + "/"

    browse_path = os.path.realpath(os.path.join(data_dir, sub_path))
    logger.info(f"data_dir={data_dir}, sub_path={sub_path} browse_path={browse_path}")

    # To protect from `../../` path traversals, check if we begin with data_dir
    if not browse_path.startswith(data_dir):
        logger.error(
            f"Invalid path detected, used {url_param_path} to get {browse_path}, does not start with {data_dir}"
        )
        raise HTTPException(status_code=400, detail="Invalid path detected")

    data = []
    with os.scandir(browse_path) as entries:
        for entry in entries:
            if entry.is_file() and entry.name.lower().endswith(ALLOWED_EXTENSIONS):
                source = urllib.parse.quote_plus(f"{sub_path}{entry.name}")
                data.append(
                    {
                        "path": os.path.join(autowms_prefix, sub_path, entry.name),
                        "adaguc": f"{adaguc_online_resource}/adagucserver?source={source}&",
                        "name": entry.name,
                        "leaf": True,
                    }
                )

            elif entry.is_dir():
                data.append(
                    {
                        "path": os.path.join(autowms_prefix, sub_path, entry.name),
                        "name": entry.name,
                        "leaf": False,
                    }
                )

    data.sort(key=lambda f: (f["leaf"], f["name"].upper()))
    return data


@autowms_router.get("/autowms")
async def handle_autowms(
    req: Request, request: str | None = None, path: str | None = None
) -> Response:
    adaguc_instance = setup_adaguc()
    adaguc_data_set_dir = adaguc_instance.ADAGUC_DATASET_DIR
    adaguc_data_dir = adaguc_instance.ADAGUC_DATA_DIR
    adaguc_autowms_dir = adaguc_instance.ADAGUC_AUTOWMS_DIR
    url = req.url
    base_url = f"{url.scheme}://{url.hostname}:{url.port}"
    adaguc_online_resource = os.getenv("EXTERNALADDRESS", base_url)

    if request is None or path is None:
        raise HTTPException(
            status_code=400,
            detail="Mandatory parameters [request] and or [path] are missing",
        )
    if request != "getfiles":
        raise HTTPException(
            status_code=400, detail="Only request=getfiles is supported"
        )

    if path == "":
        return handle_base_route()

    if path.startswith("/adaguc::datasets"):
        handler = partial(
            list_dataset_files, adaguc_data_set_dir, adaguc_online_resource
        )
    elif path.startswith("/adaguc::data"):
        handler = partial(
            list_data_files,
            adaguc_data_dir,
            path,
            adaguc_online_resource,
            "/adaguc::data",
        )
    elif path.startswith("/adaguc::autowms"):
        handler = partial(
            list_data_files,
            adaguc_autowms_dir,
            path,
            adaguc_online_resource,
            "/adaguc::autowms",
        )
    else:
        raise HTTPException(status_code=400, detail="Path parameter not understood")

    data: list[dict] = await asyncio.to_thread(handler)
    return Response(
        content=json.dumps({"result": data}),
        media_type="application/json",
        status_code=200,
    )
