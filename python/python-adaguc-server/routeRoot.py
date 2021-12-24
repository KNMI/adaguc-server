import sys
import os
import json
import urllib.parse
from flask_cors import cross_origin
from flask import request,Blueprint, Response
import logging

routeRoot = Blueprint('routeRoot', __name__)

from setupAdaguc import setupAdaguc

@routeRoot.route("/", methods=["GET"]) 
@cross_origin()
def handlerouteRoot():
    adagucInstance = setupAdaguc()
    response = Response(
        response="Welcome to adaguc-server! Please go to /wms or /autowms.",
        mimetype='text/plain',
        status=200,
    )
    return response