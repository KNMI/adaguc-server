"""wmsWcsRouter"""

import asyncio
import logging
import os
from urllib.parse import parse_qsl, quote
from adaguc.CGIRunner import HTTP_STATUSCODE_400_BAD_REQUEST
from adaguc.CGIRunner import HTTP_STATUSCODE_404_NOT_FOUND
from adaguc.CGIRunner import HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY
from adaguc.CGIRunner import HTTP_STATUSCODE_500_TIMEOUT

from fastapi import APIRouter, Request, Response

from .setup_adaguc import setup_adaguc
from .utils.utils import get_base_url

wmsWcsRouter = APIRouter(responses={404: {"description": "Not found"}})

logger = logging.getLogger(__name__)


def build_query_string(params) -> str:
    """
    Builds the QUERY_STRING for adaguc-server from already decoded (key, value) pairs
    """
    query_string = ""
    for k, query_param in params:
        param_parts = k.split("=")
        # Hack for urlencode dataset= parametwer (i.e. dataset%03DHARM_N25)
        if len(param_parts) == 2 and param_parts[0].upper() == "DATASET":
            query_string += f"&{param_parts[0]}={param_parts[1]}"
        elif k.upper() == "STYLESHEET":
            # Fully encoded: may contain &, % and newlines, adaguc-server decodes it per parameter
            query_string += f"&{k}={quote(query_param, safe='')}"
        else:
            query_param = query_param.replace("+", "%2B")
            query_string += f"&{k}={query_param}"
    return query_string


@wmsWcsRouter.api_route("/wms", methods=["GET", "POST"])
@wmsWcsRouter.api_route("/wcs", methods=["GET", "POST"])
@wmsWcsRouter.api_route("/adagucserver", methods=["GET", "POST"])
@wmsWcsRouter.api_route("/adaguc-server", methods=["GET", "POST"])
async def handle_wms(req: Request):
    """
    Handle /wms endpoint. POST accepts the same parameters form url encoded in the body.
    """
    params = list(req.query_params.items())
    if req.method == "POST":
        content_type = req.headers.get("content-type", "").split(";")[0].strip().lower()
        if content_type != "application/x-www-form-urlencoded":
            return Response(content="POST requires Content-Type application/x-www-form-urlencoded", status_code=415)
        body = (await req.body()).decode("utf-8", errors="replace")
        # Last value wins for repeated keys, same as for query parameters
        params += list(dict(parse_qsl(body, keep_blank_values=True)).items())

    adaguc_instance = setup_adaguc()

    adagucenv = {}

    # Set required environment variables
    base_url = get_base_url(req)
    adagucenv["ADAGUC_ONLINERESOURCE"] = base_url + "adaguc-server?"
    adagucenv["ADAGUC_DB"] = os.getenv("ADAGUC_DB", "user=adaguc password=adaguc host=localhost dbname=adaguc")

    query_string = build_query_string(params)

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
