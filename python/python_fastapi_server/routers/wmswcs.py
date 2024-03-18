"""wmsWcsRouter"""

import asyncio
import logging
import os
from adaguc.CGIRunner import HTTP_STATUSCODE_404_NOT_FOUND
from adaguc.CGIRunner import HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY
from adaguc.CGIRunner import HTTP_STATUSCODE_500_TIMEOUT

from fastapi import APIRouter, Request, Response

from .setup_adaguc import setup_adaguc

wmsWcsRouter = APIRouter(responses={404: {"description": "Not found"}})

logger = logging.getLogger(__name__)


@wmsWcsRouter.get("/wms")
@wmsWcsRouter.get("/wcs")
@wmsWcsRouter.get("/adagucserver")
@wmsWcsRouter.get("/adaguc-server")
async def handle_wms(
    req: Request,
):
    adaguc_instance = setup_adaguc()
    # logger.info("instance: %s", str(adaguc_instance))
    url = req.url

    logger.info(req.url)

    adagucenv = {}

    # Set required environment variables
    base_url = f"{url.scheme}://{url.hostname}:{url.port}"
    adagucenv["ADAGUC_ONLINERESOURCE"] = (
        os.getenv("EXTERNALADDRESS", base_url) + "/adaguc-server?"
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
            query_param = req.query_params[k]
            query_param = query_param.replace("+", "%2B")
            query_string += f"&{k}={query_param}"

    # Run adaguc-server
    status, data, headers = await adaguc_instance.runADAGUCServer(
        query_string, env=adagucenv, showLogOnError=False, showLog=False
    )

    # Obtain logfile
    logfile = adaguc_instance.getLogFile()
    adaguc_instance.removeLogFile()

    if len(logfile) > 0:
        logger.info(logfile)

    response_code = 200
    # Note: The Adaguc implementation requires non-zero status codes to correspond to the
    # desired response codes. Otherwise, a 500 status will be returned on exiting with errors.
    if status != 0:
        logger.info("Adaguc status code was %d", status)
        if status == HTTP_STATUSCODE_404_NOT_FOUND:
            response_code = 404  # Not Found
        elif status == HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY:
            response_code = 422  # Unprocessable Entity
        elif status == HTTP_STATUSCODE_500_TIMEOUT:
            response_code = 500  # Timeout
        else:
            response_code = 500
    response = Response(content=data.getvalue(), status_code=response_code)

    # Append the headers from adaguc-server to the headers from fastapi.
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
    status, _data, headers = asyncio.run(
        adaguc_instance.runADAGUCServer(url, env=adagucenv, showLogOnError=False)
    )
    assert status == 0
    assert "Content-Type:text/xml" in headers
    logger.info("adaguc-server seems [OK]")
