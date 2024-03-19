"""opendapRouter"""

import logging
import os

from fastapi import APIRouter, Request, Response

from .setup_adaguc import setup_adaguc

opendapRouter = APIRouter(responses={404: {"description": "Not found"}})

logger = logging.getLogger(__name__)


@opendapRouter.get("/adagucopendap/{opendappath:path}")
async def handle_opendap(req: Request, opendappath: str):
    logger.info(opendappath)
    adaguc_instance = setup_adaguc()
    url = req.url
    adagucenv = {}

    query_string = ""
    for k in req.query_params:
        query_string += f"&{k}={req.query_params[k]}"

    # Set required environment variables
    base_url = f"{url.scheme}://{url.hostname}:{url.port}"
    adagucenv["ADAGUC_ONLINERESOURCE"] = (
        os.getenv("EXTERNALADDRESS", base_url) + "/adagucopendap?"
    )
    adagucenv["ADAGUC_DB"] = os.getenv(
        "ADAGUC_DB", "user=adaguc password=adaguc host=localhost dbname=adaguc"
    )

    logger.info("Setting request_uri to %s", base_url)
    adagucenv["REQUEST_URI"] = url.path
    adagucenv["SCRIPT_NAME"] = ""

    status, data, headers = await adaguc_instance.runADAGUCServer(
        query_string, env=adagucenv, showLogOnError=False
    )

    # Obtain logfile
    logfile = adaguc_instance.getLogFile()
    adaguc_instance.removeLogFile()

    logger.info(logfile)

    response_code = 200
    if status != 0:
        logger.info("Adaguc status code was %d", status)
        response_code = 500
    response = Response(content=data.getvalue(), status_code=response_code)

    # Append the headers from adaguc-server to the headers from fastapi.
    for header in headers:
        key = header.split(":")[0]
        value = header.split(":")[1].strip()
        response.headers[key] = value
    return response
