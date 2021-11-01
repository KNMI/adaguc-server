import sys
import os
import json
import urllib.parse
from flask_cors import cross_origin
from flask import request,Blueprint, Response
import logging

routeHealthCheck = Blueprint('routeHealthCheck', __name__)

from setupAdaguc import setupAdaguc

@routeHealthCheck.route("/healthcheck", methods=["GET"]) 
@cross_origin()
def handleRouteHealthCheck():
    adagucInstance = setupAdaguc()
    response = Response(
        response="OK",
        mimetype='text/plain',
        status=200,
    )
    return response