"""wmsWcsRouter"""

import asyncio
import logging
import os
from urllib.parse import quote, urlencode
from adaguc.CGIRunner import HTTP_STATUSCODE_400_BAD_REQUEST
from adaguc.CGIRunner import HTTP_STATUSCODE_404_NOT_FOUND
from adaguc.CGIRunner import HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY
from adaguc.CGIRunner import HTTP_STATUSCODE_500_TIMEOUT
from adaguc.fork_settings import MAX_FORK_REQUEST_BYTES

from fastapi import APIRouter, Request, Response

from .setup_adaguc import setup_adaguc
from .utils.utils import get_base_url

wmsWcsRouter = APIRouter(responses={404: {"description": "Not found"}})

logger = logging.getLogger(__name__)

# Maximum size of the query string passed to adaguc-server. The query string must fit in one fork
# server request, and without fork server in one environment variable of the subprocess (128 KiB on Linux).
MAX_REQUEST_BYTES = MAX_FORK_REQUEST_BYTES

# Largest request line plus headers that uvicorn accepts, set as h11_max_incomplete_event_size. Its HTTP parser h11
# refuses a request with 400 when more than 16 KB of it has arrived without the end of the headers. Whether that
# happens depends on how the request is split over network reads, so a long STYLESHEET would fail at random.
# This leaves room for a query string of MAX_REQUEST_BYTES plus the headers.
MAX_REQUEST_HEAD_BYTES = MAX_REQUEST_BYTES + 16384


def build_query_string(params) -> str:
    """
    Builds the QUERY_STRING for adaguc-server from already decoded (key, value) pairs.
    Keys and values are encoded again: they may contain &, =, %, + and newlines, adaguc-server decodes them per parameter.
    """
    pairs = []
    for k, query_param in params:
        key, separator, value = k.partition("=")
        # Hack for an url encoded dataset= parameter (i.e. dataset%3DHARM_N25): the key is then "dataset=HARM_N25"
        if separator and key.upper() == "DATASET" and "=" not in value:
            pairs.append((key, value))
        else:
            pairs.append((k, query_param))
    return "&" + urlencode(pairs, quote_via=quote) if pairs else ""


@wmsWcsRouter.get("/wms")
@wmsWcsRouter.get("/wcs")
@wmsWcsRouter.get("/adagucserver")
@wmsWcsRouter.get("/adaguc-server")
async def handle_wms(req: Request):
    """
    Handle /wms endpoint
    """
    params = list(req.query_params.items())

    adaguc_instance = setup_adaguc()

    adagucenv = {}

    # Set required environment variables
    base_url = get_base_url(req)
    adagucenv["ADAGUC_ONLINERESOURCE"] = base_url + "adaguc-server?"
    adagucenv["ADAGUC_DB"] = os.getenv("ADAGUC_DB", "user=adaguc password=adaguc host=localhost dbname=adaguc")

    query_string = build_query_string(params)
    # Encoding again can make the query string longer than the request url
    if len(query_string) > MAX_REQUEST_BYTES:
        return Response(content=f"Request parameters too large, maximum is {MAX_REQUEST_BYTES} bytes encoded", status_code=413)

    # Run adaguc-server
    status, data, headers = await adaguc_instance.runADAGUCServer(query_string, env=adagucenv, showLogOnError=False, showLog=False)

    # Obtain logfile
    logfile = adaguc_instance.getLogFile()
    adaguc_instance.removeLogFile()

    if len(logfile) > 0:
        logger.info(":\n%s", logfile)

    response_code = 200
    # Note: The Adaguc implementation requires non-zero status codes to correspond to the
    # desired response codes. Otherwise, a 500 status will be returned on exiting with errors.
    if status != 0:
        logger.info("Adaguc status code was %d", status)
        if status == HTTP_STATUSCODE_400_BAD_REQUEST:
            response_code = 400
        elif status == HTTP_STATUSCODE_404_NOT_FOUND:
            response_code = 404  # Not Found
        elif status == HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY:
            response_code = 422  # Unprocessable Entity
        elif status == HTTP_STATUSCODE_500_TIMEOUT:
            response_code = 500  # Timeout
        else:
            response_code = 500
    response = Response(content=data, status_code=response_code)

    # Append the headers from adaguc-server to the headers from fastapi.
    for header in headers:
        key, value = header.split(":", 1)
        response.headers[key] = value.strip()

    return response


def testadaguc():
    """Test adaguc is setup correctly"""
    logger.info("Checking adaguc-server.")
    adaguc_instance = setup_adaguc()
    url = "SERVICE=WMS&REQUEST=GETCAPABILITIES"
    adagucenv = {}

    #  Set required environment variables
    adagucenv["ADAGUC_ONLINERESOURCE"] = "https://example.com/adaguc-server?"
    adagucenv["ADAGUC_DB"] = os.getenv("ADAGUC_DB", "user=adaguc password=adaguc host=localhost dbname=adaguc")

    # Run adaguc-server as a subprocess. This check runs before the fork server is started (main.py, autosync.py),
    # so fork mode is disabled for the duration of this call.
    # pylint: disable=unused-variable
    fork_enable = os.environ.pop("ADAGUC_FORK_ENABLE", None)
    try:
        status, _data, headers = asyncio.run(adaguc_instance.runADAGUCServer(url, env=adagucenv, showLogOnError=False))
    finally:
        if fork_enable is not None:
            os.environ["ADAGUC_FORK_ENABLE"] = fork_enable
    assert status == 0
    assert "Content-Type:text/xml" in headers
    logger.info("adaguc-server seems [OK]")
