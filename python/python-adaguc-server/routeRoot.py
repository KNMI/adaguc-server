# pylint: disable=invalid-name
"""Root route"""
from setup_adaguc import setup_adaguc
from flask_cors import cross_origin
from flask import Blueprint, Response


routeRoot = Blueprint('routeRoot', __name__)


@routeRoot.route("/", methods=["GET"])
@cross_origin()
def handlerouteRoot():
    """Root route"""
    setup_adaguc()
    response = Response(
        response="Welcome to adaguc-server! Please go to /wms or /autowms.",
        mimetype='text/plain',
        status=200,
    )
    return response
