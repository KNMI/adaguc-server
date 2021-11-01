import sys
import os
from flask_cors import cross_origin
from flask import request, Blueprint, Response
import logging


routeAdagucOpenDAP = Blueprint('routeAdagucOpenDAP', __name__)

from setupAdaguc import setupAdaguc
@routeAdagucOpenDAP.route("/adagucopendap/<string:text>", methods=["GET"]) 
@cross_origin()
def handleWMS(text):
    logging.info(text)
    adagucInstance = setupAdaguc()
    url = request.query_string
    logging.info(request.query_string)
    adagucenv={}

    """ Set required environment variables """
    baseUrl = request.base_url.replace(request.path,"");
    adagucenv['ADAGUC_ONLINERESOURCE']=os.getenv('EXTERNALADDRESS', baseUrl) + "/adagucopendap?"
    adagucenv['ADAGUC_DB']=os.getenv('ADAGUC_DB', "user=adaguc password=adaguc host=localhost dbname=adaguc")

    logging.info('Setting request_uri to %s' % request.base_url)
    adagucenv['REQUEST_URI']=request.path
    adagucenv['SCRIPT_NAME']=""

    status,data,headers = adagucInstance.runADAGUCServer(url, env = adagucenv,  showLogOnError = False)

    """ Obtain logfile """
    logfile = adagucInstance.getLogFile()
    adagucInstance.removeLogFile()

    logging.info(logfile)
    
    response = Response( response=data.getvalue(), status=200)

    # Append the headers from adaguc-server to the headers from flask.
    for header in headers:
        key = header.split(":")[0]
        value = header.split(":")[1]
        response.headers[key] = value
    return response