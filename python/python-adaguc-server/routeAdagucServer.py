from setup_adaguc import setup_adaguc
import sys
import os
from flask_cors import cross_origin
from flask import request, Blueprint, Response
import logging
import time


routeAdagucServer = Blueprint('routeAdagucServer', __name__)


logger = logging.getLogger(__name__)


@routeAdagucServer.route("/wms", methods=["GET"])
@routeAdagucServer.route("/wcs", methods=["GET"])
@routeAdagucServer.route("/adagucserver", methods=["GET"])
@routeAdagucServer.route("/adaguc-server", methods=["GET"])
@cross_origin()
def handleWMS():
    start = time.perf_counter()
    adagucInstance = setup_adaguc()
    url = request.query_string

    logger.info(request.query_string)
    stage1 = time.perf_counter()

    adagucenv = {}

    """ Set required environment variables """
    baseUrl = request.base_url.replace(request.path, "")
    adagucenv['ADAGUC_ONLINERESOURCE'] = os.getenv(
        'EXTERNALADDRESS', baseUrl) + "/adaguc-server?"
    adagucenv['ADAGUC_DB'] = os.getenv(
        'ADAGUC_DB', "user=adaguc password=adaguc host=localhost dbname=adaguc")

    """ Run adaguc-server """
    status, data, headers = adagucInstance.runADAGUCServer(
        url, env=adagucenv,  showLogOnError=False)

    """ Obtain logfile """
    logfile = adagucInstance.getLogFile()
    adagucInstance.removeLogFile()

    stage2 = time.perf_counter()
    logger.info("[PERF] Adaguc executation took: %f" % (stage2 - stage1))

    if len(logfile) > 0:
        logger.info(logfile)

    response = Response(response=data.getvalue(), status=200)

    stage3 = time.perf_counter()

    # Append the headers from adaguc-server to the headers from flask.
    for header in headers:
        key = header.split(":")[0]
        value = header.split(":")[1]
        response.headers[key] = value
    stage4 = time.perf_counter()

    return response
