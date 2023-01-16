"""opendapRouter"""
from .setup_adaguc import setup_adaguc
from fastapi import Request, APIRouter, Response

from .setup_adaguc import setup_adaguc
import sys
import os
import logging
import json
import urllib.parse

opendapRouter = APIRouter(responses={404: {"description": "Not found"}})

logger = logging.getLogger(__name__)


@opendapRouter.get("/adagucopendap/{opendappath:path}")
def handleOpendap(req: Request, opendappath: str):
    logger.info(opendappath)
    adagucInstance = setup_adaguc()
    url = req.url
    adagucenv = {}

    query_string = ""
    for k in req.query_params:
        query_string += f"&{k}={req.query_params[k]}"

    """ Set required environment variables """
    baseUrl = f"{url.scheme}://{url.hostname}:{url.port}"
    adagucenv["ADAGUC_ONLINERESOURCE"] = (
        os.getenv("EXTERNALADDRESS", baseUrl) + "/adagucopendap?"
    )
    adagucenv["ADAGUC_DB"] = os.getenv(
        "ADAGUC_DB", "user=adaguc password=adaguc host=localhost dbname=adaguc"
    )

    logger.info("Setting request_uri to %s" % baseUrl)
    adagucenv["REQUEST_URI"] = url.path
    adagucenv["SCRIPT_NAME"] = ""

    status, data, headers = adagucInstance.runADAGUCServer(
        query_string, env=adagucenv, showLogOnError=False
    )

    """ Obtain logfile """
    logfile = adagucInstance.getLogFile()
    adagucInstance.removeLogFile()

    logger.info(logfile)

    responseCode = 200
    if status != 0:
        logger.info("Adaguc status code was %d" % status)
        responseCode = 500
    response = Response(content=data.getvalue(), status_code=responseCode)

    # Append the headers from adaguc-server to the headers from flask.
    for header in headers:
        key = header.split(":")[0]
        value = header.split(":")[1].strip()
        response.headers[key] = value
    return response
