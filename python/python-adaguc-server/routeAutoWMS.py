import sys
import os
import json
import urllib.parse
from flask_cors import cross_origin
from flask import request,Blueprint, Response
import logging


routeAutoWMS = Blueprint('routeAutoWMS', __name__)

from setupAdaguc import setupAdaguc

def handleBaseRoute():
  datasets = []
  datasets.append({
    "path": "/adaguc::datasets",
    "name": "adaguc::datasets",
    "leaf": False
  })
  datasets.append({
    "path": "/adaguc::data",
    "name": "adaguc::data",
    "leaf": False
  })

  datasets.append({
    "path": "/adaguc::autowms",
    "name": "adaguc::autowms",
    "leaf": False
  })
      
  response = Response(
    response=json.dumps({"result":datasets}),
    mimetype='application/json',
    status=200,
  )
  return response

def handleDatasetsRoute(adagucDataSetDir, adagucOnlineResource):
  datasetFiles = [f for f in os.listdir(adagucDataSetDir) if os.path.isfile(os.path.join(adagucDataSetDir, f)) and f.endswith(".xml")]
  datasets = []
  for datasetFile in datasetFiles:
    datasets.append({
      "path": "/adaguc::datasets/" + datasetFile,
      "adaguc": adagucOnlineResource + "/adagucserver?dataset=" + datasetFile.replace(".xml","") + "&",
      "name": datasetFile.replace(".xml",""),
      "leaf": True
      })
      
  response = Response(
    response=json.dumps({"result":datasets}),
    mimetype='application/json',
    status=200,
  )
  return response

def isFileAllowed(fileName):
  if (fileName.endswith(".nc")): return True;
  if (fileName.endswith(".nc4")): return True;
  if (fileName.endswith(".hdf5")): return True;
  if (fileName.endswith(".h5")): return True;
  if (fileName.endswith(".png")): return True;
  if (fileName.endswith(".json")): return True;
  if (fileName.endswith(".geojson")): return True;
  if (fileName.endswith(".csv")): return True;

def handleDataRoute(adagucDataDir, urlParamPath, adagucOnlineResource):
  subPath = urlParamPath.replace("/adaguc::data/", "")
  subPath = subPath.replace("/adaguc::data", "")
  logging.info("adagucDataDir [%s] and subPath [%s]" % (adagucDataDir, subPath))
  localPathToBrowse = os.path.realpath(os.path.join(adagucDataDir, subPath))
  logging.info("localPathToBrowse = [%s]" % localPathToBrowse)

  if not localPathToBrowse.startswith(adagucDataDir):
    logging.error("Invalid path detected = constructed [%s] from [%s]" % (localPathToBrowse, urlParamPath))
    response = Response(
      response="Invalid path detected",
      mimetype='application/json',
      status=400,
    )
    return response

  dataDirectories = [f for f in os.listdir(localPathToBrowse) if os.path.isdir(os.path.join(localPathToBrowse, f))]
  dataFiles = [f for f in os.listdir(localPathToBrowse) if os.path.isfile(os.path.join(localPathToBrowse, f)) and isFileAllowed(f)]
  data = []

  for dataDirectory in dataDirectories:
    data.append({
      "path": os.path.join("/adaguc::data" , subPath, dataDirectory),
      "name": dataDirectory,
      "leaf": False
      })


  for dataFile in dataFiles:
    data.append({
      "path": os.path.join("/adaguc::data", subPath, dataFile),
      "adaguc": adagucOnlineResource + "/adagucserver?source=" + urllib.parse.quote_plus(subPath+"/"+dataFile) + "&",
      "name": dataFile,
      "leaf": True
      })
      
  response = Response(
    response=json.dumps({"result":data}),
    mimetype='application/json',
    status=200,
  )
  return response

def handleAutoWMSDIRRoute(adagucAutoWMSDir, urlParamPath, adagucOnlineResource):
  subPath = urlParamPath.replace("/adaguc::autowms/", "")
  subPath = subPath.replace("/adaguc::autowms", "")
  logging.info("adagucAutoWMSDir [%s] and subPath [%s]" % (adagucAutoWMSDir, subPath))
  localPathToBrowse = os.path.realpath(os.path.join(adagucAutoWMSDir, subPath))
  logging.info("localPathToBrowse = [%s]" % localPathToBrowse)

  if not localPathToBrowse.startswith(adagucAutoWMSDir):
    logging.error("Invalid path detected = constructed [%s] from [%s]" % (localPathToBrowse, urlParamPath))
    response = Response(
      response="Invalid path detected",
      mimetype='application/json',
      status=400,
    )
    return response

  dataDirectories = [f for f in os.listdir(localPathToBrowse) if os.path.isdir(os.path.join(localPathToBrowse, f))]
  dataFiles = [f for f in os.listdir(localPathToBrowse) if os.path.isfile(os.path.join(localPathToBrowse, f)) and isFileAllowed(f)]
  data = []

  for dataDirectory in dataDirectories:
    data.append({
      "path": os.path.join("/adaguc::autowms", subPath, dataDirectory),
      "name": dataDirectory,
      "leaf": False
      })


  for dataFile in dataFiles:
    data.append({
      "path": os.path.join("/adaguc::autowms",subPath, dataFile),
      "adaguc": adagucOnlineResource + "/adagucserver?source=" + urllib.parse.quote_plus(subPath+"/"+dataFile) + "&",
      "name": dataFile,
      "leaf": True
      })
      
  response = Response(
    response=json.dumps({"result":data}),
    mimetype='application/json',
    status=200,
  )
  return response  

@routeAutoWMS.route("/autowms", methods=["GET"]) 
@cross_origin()
def handleAutoWMS():
    adagucInstance = setupAdaguc()
    logging.info(request.query_string)
    adagucDataSetDir = adagucInstance.ADAGUC_DATASET_DIR
    adagucDataDir = adagucInstance.ADAGUC_DATA_DIR
    adagucAutoWMSDir = adagucInstance.ADAGUC_AUTOWMS_DIR
    baseUrl = request.base_url.replace(request.path,"");
    adagucOnlineResource=os.getenv('EXTERNALADDRESS', baseUrl)
    print('Online resource = [%s]' % adagucOnlineResource)
    urlParamRequest = request.args.get('request')
    urlParamPath = request.args.get('path')
    if urlParamRequest is None or urlParamPath is None:
      response = Response(
        response="Mandatory parameters [request] and or [path] are missing",
        status=400,
      )
      return response
    if urlParamRequest != "getfiles":
      response = Response(
        response="Only request=getfiles is supported",
        status=400,
      )
      return response
    print(urlParamPath)

    if urlParamPath == "":
      return handleBaseRoute()

    if urlParamPath.startswith("/adaguc::datasets"):
      return handleDatasetsRoute(adagucDataSetDir, adagucOnlineResource)
    
    if urlParamPath.startswith("/adaguc::data"):
      return handleDataRoute(adagucDataDir, urlParamPath, adagucOnlineResource)

    if urlParamPath.startswith("/adaguc::autowms"):
      return handleAutoWMSDIRRoute(adagucAutoWMSDir, urlParamPath, adagucOnlineResource)
    

    response = Response(
        response="Path parameter not understood..",
        mimetype='application/json',
        status=400,
    )
    return response