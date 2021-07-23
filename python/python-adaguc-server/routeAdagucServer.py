import sys
import os
from flask_cors import cross_origin
from flask import request, Blueprint, Response
import logging


routeAdagucServer = Blueprint('routeAdagucServer', __name__)

from setupAdaguc import setupAdaguc

@routeAdagucServer.route("/wms", methods=["GET"]) 
@routeAdagucServer.route("/wcs", methods=["GET"]) 
@routeAdagucServer.route("/adagucserver", methods=["GET"]) 
@routeAdagucServer.route("/adaguc-server", methods=["GET"]) 
@cross_origin()
def handleWMS():
    adagucInstance = setupAdaguc()
    url = request.query_string
    logging.info(request.query_string)
    adagucenv={}

    """ Set required environment variables """
    adagucenv['ADAGUC_CONFIG']=adagucInstance.ADAGUC_CONFIG
    adagucenv['ADAGUC_LOGFILE']=adagucInstance.ADAGUC_LOGFILE
    adagucenv['ADAGUC_PATH']=adagucInstance.ADAGUC_PATH
    adagucenv['ADAGUC_DATARESTRICTION']="FALSE"
    adagucenv['ADAGUC_ENABLELOGBUFFER']="FALSE"
    adagucenv['ADAGUC_FONT']="/adaguc/adaguc-server-master/data/fonts/FreeSans.ttf"
    adagucenv['ADAGUC_DATA_DIR']=adagucInstance.ADAGUC_DATA_DIR
    adagucenv['ADAGUC_AUTOWMS_DIR']=adagucInstance.ADAGUC_AUTOWMS_DIR
    adagucenv['ADAGUC_DATASET_DIR']=adagucInstance.ADAGUC_DATASET_DIR
    adagucenv['ADAGUC_TMP']=adagucInstance.ADAGUC_TMP
    adagucenv['ADAGUC_FONT']=adagucInstance.ADAGUC_FONT
    baseUrl = request.base_url.replace(request.path,"");
    adagucenv['ADAGUC_ONLINERESOURCE']=os.getenv('EXTERNALADDRESS', baseUrl) + "/adagucserver?"
    adagucenv['ADAGUC_DB']=os.getenv('ADAGUC_DB', "user=adaguc password=adaguc host=localhost dbname=adaguc")
    status,data,headers = adagucInstance.runADAGUCServer(url, env = adagucenv,  showLogOnError = False)
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