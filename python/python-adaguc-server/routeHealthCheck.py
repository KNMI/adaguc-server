# pylint: disable=invalid-name
"""handleRouteHealthCheck"""
from setup_adaguc import setup_adaguc
from flask_cors import cross_origin
from flask import Blueprint, Response

routeHealthCheck = Blueprint('routeHealthCheck', __name__)


@routeHealthCheck.route("/healthcheck", methods=["GET"])
@cross_origin()
def handleRouteHealthCheck():
    """handleRouteHealthCheck"""
    setup_adaguc(False)
    response = Response(
        response="OK",
        mimetype='text/plain',
        status=200,
    )
    return response
