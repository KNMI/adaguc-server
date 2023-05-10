"""autoWmsRouter"""
import json
import logging
import os
import urllib.parse

from fastapi import APIRouter, Request, Response

from .setup_adaguc import setup_adaguc

autowms_router = APIRouter(responses={404: {"description": "Not found"}})

logger = logging.getLogger(__name__)


def handle_base_route():
    datasets = []
    datasets.append(
        {"path": "/adaguc::datasets", "name": "adaguc::datasets", "leaf": False}
    )
    datasets.append({"path": "/adaguc::data", "name": "adaguc::data", "leaf": False})

    datasets.append(
        {"path": "/adaguc::autowms", "name": "adaguc::autowms", "leaf": False}
    )

    response = Response(
        content=json.dumps({"result": datasets}),
        media_type="application/json",
        status_code=200,
    )
    return response


def handle_datasets_route(adaguc_dataset_dir, adaguc_online_resource):
    dataset_files = [
        f
        for f in os.listdir(adaguc_dataset_dir)
        if os.path.isfile(os.path.join(adaguc_dataset_dir, f)) and f.endswith(".xml")
    ]
    datasets = []
    for dataset_file in sorted(dataset_files, key=lambda f: f.upper()):
        datasets.append(
            {
                "path": "/adaguc::datasets/" + dataset_file,
                "adaguc": adaguc_online_resource
                + "/adagucserver?dataset="
                + dataset_file.replace(".xml", "")
                + "&",
                "name": dataset_file.replace(".xml", ""),
                "leaf": True,
            }
        )

    response = Response(
        content=json.dumps({"result": datasets}),
        media_type="application/json",
        status_code=200,
    )
    return response


def is_file_allowed(file_name):
    if file_name.endswith(".nc"):
        return True
    if file_name.endswith(".nc4"):
        return True
    if file_name.endswith(".hdf5"):
        return True
    if file_name.endswith(".h5"):
        return True
    if file_name.endswith(".png"):
        return True
    if file_name.endswith(".json"):
        return True
    if file_name.endswith(".geojson"):
        return True
    if file_name.endswith(".csv"):
        return True
    return False


def handle_data_route(adaguc_data_dir, url_param_path, adaguc_online_resource):
    sub_path = url_param_path.replace("/adaguc::data/", "")
    sub_path = sub_path.replace("/adaguc::data", "")
    logger.info("adagucDataDir [%s] and subPath [%s]", adaguc_data_dir, sub_path)
    local_path_to_browse = os.path.realpath(os.path.join(adaguc_data_dir, sub_path))
    logger.info("localPathToBrowse = [%s]", local_path_to_browse)

    if not local_path_to_browse.startswith(adaguc_data_dir):
        logger.error(
            "Invalid path detected = constructed [%s] from [%s], "
            "localPathToBrowse [%s] does not start with adagucDataDir [%s] ",
            local_path_to_browse,
            url_param_path,
            local_path_to_browse,
            adaguc_data_dir,
        )
        response = Response(
            content="Invalid path detected",
            media_type="application/json",
            status_code=400,
        )
        return response

    data_directories = [
        f
        for f in os.listdir(local_path_to_browse)
        if os.path.isdir(os.path.join(local_path_to_browse, f))
    ]
    data_files = [
        f
        for f in os.listdir(local_path_to_browse)
        if os.path.isfile(os.path.join(local_path_to_browse, f)) and is_file_allowed(f)
    ]
    data = []

    for data_directory in sorted(data_directories, key=lambda f: f.upper()):
        data.append(
            {
                "path": os.path.join("/adaguc::data", sub_path, data_directory),
                "name": data_directory,
                "leaf": False,
            }
        )

    for data_file in sorted(data_files, key=lambda f: f.upper()):
        data.append(
            {
                "path": os.path.join("/adaguc::data", sub_path, data_file),
                "adaguc": adaguc_online_resource
                + "/adagucserver?source="
                + urllib.parse.quote_plus(sub_path + "/" + data_file)
                + "&",
                "name": data_file,
                "leaf": True,
            }
        )

    response = Response(
        content=json.dumps({"result": data}),
        media_type="application/json",
        status_code=200,
    )
    return response


def handle_autowmsdir_route(adaguc_autowms_dir, url_param_path, adaguc_online_resource):
    sub_path = url_param_path.replace("/adaguc::autowms/", "")
    sub_path = sub_path.replace("/adaguc::autowms", "")
    if len(sub_path) != 0:
        sub_path = sub_path + "/"
    logger.info("adagucAutoWMSDir [%s] and subPath [%s]", adaguc_autowms_dir, sub_path)
    local_path_to_browse = os.path.realpath(os.path.join(adaguc_autowms_dir, sub_path))
    logger.info("localPathToBrowse = [%s]", local_path_to_browse)

    if not local_path_to_browse.startswith(adaguc_autowms_dir):
        logger.error(
            "Invalid path detected = constructed [%s] from [%s] %s",
            local_path_to_browse,
            url_param_path,
            adaguc_autowms_dir,
        )
        response = Response(
            content="Invalid path detected",
            media_type="application/json",
            status_code=400,
        )
        return response

    data_directories = [
        f
        for f in os.listdir(local_path_to_browse)
        if os.path.isdir(os.path.join(local_path_to_browse, f))
    ]
    data_files = [
        f
        for f in os.listdir(local_path_to_browse)
        if os.path.isfile(os.path.join(local_path_to_browse, f)) and is_file_allowed(f)
    ]
    data = []

    for data_directory in sorted(data_directories, key=lambda f: f.upper()):
        data.append(
            {
                "path": os.path.join("/adaguc::autowms", sub_path, data_directory),
                "name": data_directory,
                "leaf": False,
            }
        )

    for data_file in sorted(data_files, key=lambda f: f.upper()):
        data.append(
            {
                "path": os.path.join("/adaguc::autowms", sub_path, data_file),
                "adaguc": adaguc_online_resource
                + "/adagucserver?source="
                + urllib.parse.quote_plus(sub_path + data_file)
                + "&",
                "name": data_file,
                "leaf": True,
            }
        )

    response = Response(
        content=json.dumps({"result": data}),
        media_type="application/json",
        status_code=200,
    )
    return response


@autowms_router.get("/autowms")
async def handle_autowms(req: Request, request: str = None, path: str = None):
    adaguc_instance = setup_adaguc()
    adaguc_data_set_dir = adaguc_instance.ADAGUC_DATASET_DIR
    adaguc_data_dir = adaguc_instance.ADAGUC_DATA_DIR
    adaguc_autowms_dir = adaguc_instance.ADAGUC_AUTOWMS_DIR
    url = req.url
    base_url = f"{url.scheme}://{url.hostname}:{url.port}"
    adaguc_online_resource = os.getenv("EXTERNALADDRESS", base_url)
    if request is None or path is None:
        response = Response(
            content="Mandatory parameters [request] and or [path] are missing",
            status_code=400,
        )
        return response
    if request != "getfiles":
        response = Response(
            content="Only request=getfiles is supported",
            status_code=400,
        )
        return response

    if path == "":
        return handle_base_route()

    if path.startswith("/adaguc::datasets"):
        return handle_datasets_route(adaguc_data_set_dir, adaguc_online_resource)

    if path.startswith("/adaguc::data"):
        return handle_data_route(adaguc_data_dir, path, adaguc_online_resource)

    if path.startswith("/adaguc::autowms"):
        return handle_autowmsdir_route(adaguc_autowms_dir, path, adaguc_online_resource)

    response = Response(
        content="Path parameter not understood..",
        media_type="application/json",
        status_code=400,
    )
    return response
