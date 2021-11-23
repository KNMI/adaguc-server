from setupAdaguc import setupAdaguc
from flask_cors import cross_origin
from flask import request, Blueprint, Response

routeHealthCheck = Blueprint('routeHealthCheck', __name__)


@routeHealthCheck.route("/healthcheck", methods=["GET"])
@cross_origin()
def handleRouteHealthCheck():
  setupAdaguc(False)
  response = Response(
      response="OK",
      mimetype='text/plain',
      status=200,
  )
  return response
