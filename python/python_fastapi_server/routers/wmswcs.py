"""wmsWcsRouter"""

import asyncio
import logging
import os
from urllib.parse import parse_qsl, quote
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


def build_query_string(params) -> str:
    """
    Builds the QUERY_STRING for adaguc-server from already decoded (key, value) pairs.
    Keys and values are encoded again: they may contain &, =, %, + and newlines, adaguc-server decodes them per parameter.
    """
    query_string = ""
    for k, query_param in params:
        param_parts = k.split("=")
        # Hack for urlencode dataset= parametwer (i.e. dataset%03DHARM_N25)
        if len(param_parts) == 2 and param_parts[0].upper() == "DATASET":
            query_string += f"&{quote(param_parts[0], safe='')}={quote(param_parts[1], safe='')}"
        else:
            query_string += f"&{quote(k, safe='')}={quote(query_param, safe='')}"
    return query_string


def merge_params(query_params, body_params):
    """
    Merges POST body parameters over the query parameters. Keys are case insensitive for adaguc-server,
    so a key in the body replaces the same key in any case from the query.
    """
    merged = {}
    for k, v in list(query_params) + list(body_params):
        merged[k.upper()] = (k, v)
    return list(merged.values())


async def read_limited_body(req: Request, limit: int) -> bytes | None:
    """
    Returns the request body, or None when it is larger than limit. Stops reading as soon as the limit is exceeded,
    so a huge body is never held in memory.
    """
    content_length = req.headers.get("content-length", "")
    if content_length.isdigit() and int(content_length) > limit:
        return None
    body = bytearray()
    async for chunk in req.stream():
        body += chunk
        if len(body) > limit:
            return None
    return bytes(body)


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
        # The parameters must fit in a fork server request anyway
        body = await read_limited_body(req, MAX_FORK_REQUEST_BYTES)
        if body is None:
            return Response(content=f"Request body too large, maximum is {MAX_FORK_REQUEST_BYTES} bytes", status_code=413)
        body = body.decode("utf-8", errors="replace")
        # Last value wins for repeated keys, also across query and body
        params = merge_params(params, parse_qsl(body, keep_blank_values=True))

    adaguc_instance = setup_adaguc()

    adagucenv = {}

    # Set required environment variables
    base_url = get_base_url(req)
    adagucenv["ADAGUC_ONLINERESOURCE"] = base_url + "adaguc-server?"
    adagucenv["ADAGUC_DB"] = os.getenv("ADAGUC_DB", "user=adaguc password=adaguc host=localhost dbname=adaguc")

    query_string = build_query_string(params)
    # Encoding can make the query string up to three times as long as the body. It must fit in a fork server request,
    # and without fork server in one environment variable of the subprocess (128 KiB on Linux).
    if len(query_string) > MAX_FORK_REQUEST_BYTES:
        return Response(content=f"Request parameters too large, maximum is {MAX_FORK_REQUEST_BYTES} bytes encoded", status_code=413)

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
