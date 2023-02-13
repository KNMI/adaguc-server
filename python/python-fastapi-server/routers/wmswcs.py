"""wmsWcsRouter"""
from .setup_adaguc import setup_adaguc
from fastapi import Request, APIRouter, Response

from .setup_adaguc import setup_adaguc
import sys
import os
import logging
import time

wmsWcsRouter = APIRouter(responses={404: {"description": "Not found"}})

logger = logging.getLogger(__name__)


@wmsWcsRouter.get("/wms")
@wmsWcsRouter.get("/wcs")
@wmsWcsRouter.get("/adagucserver")
@wmsWcsRouter.get("/adaguc-server")
async def handleWMS(
    req: Request,
    response: Response,
):
    start = time.perf_counter()
    adagucInstance = setup_adaguc()
    logger.info("instance:" + str(adagucInstance))
    url = req.url

    logger.info(req.url)
    stage1 = time.perf_counter()

    adagucenv = {}

    """ Set required environment variables """
    ## baseUrl = req.base_url.replace(req.path, "")
    baseUrl = f"{url.scheme}://{url.hostname}:{url.port}"
    adagucenv["ADAGUC_ONLINERESOURCE"] = (
        os.getenv("EXTERNALADDRESS", baseUrl) + "/adaguc-server?"
    )
    adagucenv["ADAGUC_DB"] = os.getenv(
        "ADAGUC_DB", "user=adaguc password=adaguc host=localhost dbname=adaguc"
    )

    query_string = ""
    for k in req.query_params:
        param_parts = k.split("=")
        # Hack for urlencode dataset= parametwer (i.e. dataset%03DHARM_N25)
        if len(param_parts) == 2 and param_parts[0].upper() == "DATASET":
            query_string += f"&{param_parts[0]}={param_parts[1]}"
        else:
            query_string += f"&{k}={req.query_params[k]}"

    """ Run adaguc-server """
    status, data, headers = adagucInstance.runADAGUCServer(
        query_string, env=adagucenv, showLog=False
    )

    """ Obtain logfile """
    logfile = adagucInstance.getLogFile()
    adagucInstance.removeLogFile()

    stage2 = time.perf_counter()
    logger.info("[PERF] Adaguc executation took: %f" % (stage2 - stage1))

    if len(logfile) > 0:
        logger.info(logfile)

    responseCode = 200
    if status != 0:
        logger.info("Adaguc status code was %d" % status)
        responseCode = 500
    response = Response(content=data.getvalue(), status_code=responseCode)

    stage3 = time.perf_counter()

    # Append the headers from adaguc-server to the headers from flask.
    for header in headers:
        key = header.split(":")[0]
        value = header.split(":")[1].strip()
        response.headers[key] = value

    return response


def testadaguc():
    """Test adaguc is setup correctly"""
    logger.info("Checking adaguc-server.")
    adaguc_instance = setup_adaguc()
    url = "SERVICE=WMS&REQUEST=GETCAPABILITIES"
    adagucenv = {}

    #  Set required environment variables
    baseurl = "---"
    adagucenv["ADAGUC_ONLINERESOURCE"] = (
        os.getenv("EXTERNALADDRESS", baseurl) + "/adaguc-server?"
    )
    adagucenv["ADAGUC_DB"] = os.getenv(
        "ADAGUC_DB", "user=adaguc password=adaguc host=localhost dbname=adaguc"
    )

    # Run adaguc-server
    # pylint: disable=unused-variable
    status, data, headers = adaguc_instance.runADAGUCServer(
        url, env=adagucenv, showLogOnError=False
    )
    assert status == 0
    assert headers == ["Content-Type:text/xml"]
    logger.info("adaguc-server seems [OK]")
