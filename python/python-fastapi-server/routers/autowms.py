"""autoWmsRouter"""
from .setup_adaguc import setup_adaguc
from fastapi import Request, APIRouter, Response

from .setup_adaguc import setup_adaguc
import sys
import os
import logging
import json
import urllib.parse

autoWmsRouter = APIRouter(responses={404: {"description": "Not found"}})

logger = logging.getLogger(__name__)


def handleBaseRoute():
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


def handleDatasetsRoute(adagucDataSetDir, adagucOnlineResource):
    datasetFiles = [
        f
        for f in os.listdir(adagucDataSetDir)
        if os.path.isfile(os.path.join(adagucDataSetDir, f)) and f.endswith(".xml")
    ]
    datasets = []
    for datasetFile in datasetFiles:
        datasets.append(
            {
                "path": "/adaguc::datasets/" + datasetFile,
                "adaguc": adagucOnlineResource
                + "/adagucserver?dataset="
                + datasetFile.replace(".xml", "")
                + "&",
                "name": datasetFile.replace(".xml", ""),
                "leaf": True,
            }
        )

    response = Response(
        content=json.dumps({"result": datasets}),
        media_type="application/json",
        status_code=200,
    )
    return response


def isFileAllowed(fileName):
    if fileName.endswith(".nc"):
        return True
    if fileName.endswith(".nc4"):
        return True
    if fileName.endswith(".hdf5"):
        return True
    if fileName.endswith(".h5"):
        return True
    if fileName.endswith(".png"):
        return True
    if fileName.endswith(".json"):
        return True
    if fileName.endswith(".geojson"):
        return True
    if fileName.endswith(".csv"):
        return True


def handleDataRoute(adagucDataDir, urlParamPath, adagucOnlineResource):
    subPath = urlParamPath.replace("/adaguc::data/", "")
    subPath = subPath.replace("/adaguc::data", "")
    logger.info("adagucDataDir [%s] and subPath [%s]" % (adagucDataDir, subPath))
    localPathToBrowse = os.path.realpath(os.path.join(adagucDataDir, subPath))
    logger.info("localPathToBrowse = [%s]" % localPathToBrowse)

    if not localPathToBrowse.startswith(adagucDataDir):
        logger.error(
            "Invalid path detected = constructed [%s] from [%s], localPathToBrowse [%s] does not start with adagucDataDir [%s] "
            % (localPathToBrowse, urlParamPath, localPathToBrowse, adagucDataDir)
        )
        response = Response(
            contente="Invalid path detected",
            media_type="application/json",
            status_code=400,
        )
        return response

    dataDirectories = [
        f
        for f in os.listdir(localPathToBrowse)
        if os.path.isdir(os.path.join(localPathToBrowse, f))
    ]
    dataFiles = [
        f
        for f in os.listdir(localPathToBrowse)
        if os.path.isfile(os.path.join(localPathToBrowse, f)) and isFileAllowed(f)
    ]
    data = []

    for dataDirectory in sorted(dataDirectories, key=lambda f: f.upper()):
        data.append(
            {
                "path": os.path.join("/adaguc::data", subPath, dataDirectory),
                "name": dataDirectory,
                "leaf": False,
            }
        )

    for dataFile in sorted(dataFiles, key=lambda f: f.upper()):
        data.append(
            {
                "path": os.path.join("/adaguc::data", subPath, dataFile),
                "adaguc": adagucOnlineResource
                + "/adagucserver?source="
                + urllib.parse.quote_plus(subPath + "/" + dataFile)
                + "&",
                "name": dataFile,
                "leaf": True,
            }
        )

    response = Response(
        content=json.dumps({"result": data}),
        media_type="application/json",
        status_code=200,
    )
    return response


def handleAutoWMSDIRRoute(adagucAutoWMSDir, urlParamPath, adagucOnlineResource):
    subPath = urlParamPath.replace("/adaguc::autowms/", "")
    subPath = subPath.replace("/adaguc::autowms", "")
    if len(subPath) != 0:
        subPath = subPath + "/"
    logger.info("adagucAutoWMSDir [%s] and subPath [%s]" % (adagucAutoWMSDir, subPath))
    localPathToBrowse = os.path.realpath(os.path.join(adagucAutoWMSDir, subPath))
    logger.info("localPathToBrowse = [%s]" % localPathToBrowse)

    if not localPathToBrowse.startswith(adagucAutoWMSDir):
        logger.error(
            "Invalid path detected = constructed [%s] from [%s] %s"
            % (localPathToBrowse, urlParamPath, adagucAutoWMSDir)
        )
        response = Response(
            content="Invalid path detected",
            media_type="application/json",
            status_code=400,
        )
        return response

    dataDirectories = [
        f
        for f in os.listdir(localPathToBrowse)
        if os.path.isdir(os.path.join(localPathToBrowse, f))
    ]
    dataFiles = [
        f
        for f in os.listdir(localPathToBrowse)
        if os.path.isfile(os.path.join(localPathToBrowse, f)) and isFileAllowed(f)
    ]
    data = []

    for dataDirectory in sorted(dataDirectories, key=lambda f: f.upper()):
        data.append(
            {
                "path": os.path.join("/adaguc::autowms", subPath, dataDirectory),
                "name": dataDirectory,
                "leaf": False,
            }
        )

    for dataFile in sorted(dataFiles, key=lambda f: f.upper()):
        data.append(
            {
                "path": os.path.join("/adaguc::autowms", subPath, dataFile),
                "adaguc": adagucOnlineResource
                + "/adagucserver?source="
                + urllib.parse.quote_plus(subPath + dataFile)
                + "&",
                "name": dataFile,
                "leaf": True,
            }
        )

    response = Response(
        content=json.dumps({"result": data}),
        media_type="application/json",
        status_code=200,
    )
    return response


@autoWmsRouter.get("/autowms")
async def handleAutoWMS(req: Request, request: str = None, path: str = None):
    adagucInstance = setup_adaguc()
    adagucDataSetDir = adagucInstance.ADAGUC_DATASET_DIR
    adagucDataDir = adagucInstance.ADAGUC_DATA_DIR
    adagucAutoWMSDir = adagucInstance.ADAGUC_AUTOWMS_DIR
    url = req.url
    baseUrl = f"{url.scheme}://{url.hostname}:{url.port}"
    adagucOnlineResource = os.getenv("EXTERNALADDRESS", baseUrl)
    print("Online resource = [%s]" % adagucOnlineResource)
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
    print(path)

    if path == "":
        return handleBaseRoute()

    if path.startswith("/adaguc::datasets"):
        return handleDatasetsRoute(adagucDataSetDir, adagucOnlineResource)

    if path.startswith("/adaguc::data"):
        return handleDataRoute(adagucDataDir, path, adagucOnlineResource)

    if path.startswith("/adaguc::autowms"):
        return handleAutoWMSDIRRoute(adagucAutoWMSDir, path, adagucOnlineResource)

    response = Response(
        content="Path parameter not understood..",
        media_type="application/json",
        status_code=400,
    )
    return response
