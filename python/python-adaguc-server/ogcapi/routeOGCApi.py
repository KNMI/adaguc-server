import os
import time
import itertools
import json
from flask import Flask, request, Response, render_template, Blueprint, current_app, url_for
from flask_cors import cross_origin
import logging
import marshmallow as ma
#from flask_smorest import Api, Blueprint, abort
from owslib.wms import WebMapService
import yaml
from datetime import datetime
import re
import requests
from .schemas.schemas import create_apispec
from setupAdaguc import setupAdaguc

from collections import OrderedDict
from defusedxml.ElementTree import fromstring, parse, ParseError

routeOGCApi = Blueprint('routeOGCApi', __name__, template_folder='templates')

logger = logging.getLogger(__name__)

TIMEOUT=20

EXTRA_SETTINGS = """
servers:
- url: http://192.168.178.113:8087/
  description: The OGCAPI development server
"""

"""
  variables:
    port:
      enum:
      - '5000'
      - '5001'
      default: '5000'

"""
settings =  yaml.safe_load(EXTRA_SETTINGS)
logger.debug("settings: %s", str(settings))

spec = create_apispec(
        title="OGCAPI_F",
        version="0.0.1",
        openapi_version="3.0.2",
        settings = settings
        )

SUPPORTED_CRS=[
    "http://www.opengis.net/def/crs/OGC/1.3/CRS84",
    "http://www.opengis.net/def/crs/EPSG/0/4326",
]

collections = [
    {
        "name": "precip",
        "title": "precipitation",
        "dataset": "RADAR",
    #    "service": "https://geoservices.knmi.nl/wms?DATASET=RADAR",
        "service": "http://192.168.178.113:8087/wms?DATASET=RADAR",
        "extent": [0.000000, 48.895303, 10.85645, 55.97360]
        #TODO Native projection?
    },
    {
        "name": "harmonie",
        "title": "Harmonie",
        "dataset": "HARM_N25",
     #   "service": "https://geoservices.knmi.nl/wms?DATASET=HARM_N25",
        "service": "http://192.168.178.113:8087/wms?DATASET=HARM_N25",
        "extent": [-0.018500, 48.988500, 11.081500, 55.888500]
        #TODO Native projection?
    }
]

def callADAGUC(url):
  start = time.perf_counter()
  adagucInstance = setupAdaguc()

  logger.info(url)
  stage1 = time.perf_counter()

  adagucenv = {}

  """ Set required environment variables """
  adagucenv['ADAGUC_ONLINERESOURCE'] = os.getenv(
      'EXTERNALADDRESS', 'http://192.168.178.113:8087') + "/adaguc-server?"
  adagucenv['ADAGUC_DB'] = os.getenv(
      'ADAGUC_DB', "user=adaguc password=adaguc host=localhost dbname=adaguc")

  """ Run adaguc-server """
  status, data, headers = adagucInstance.runADAGUCServer(
      url, env=adagucenv,  showLogOnError=True)

  """ Obtain logfile """
  logfile = adagucInstance.getLogFile()
  adagucInstance.removeLogFile()

  stage2 = time.perf_counter()
  logger.info("[PERF] Adaguc execution took: %f" % (stage2 - stage1))

  if len(logfile) > 0:
    logger.info(logfile)

  return status, data

def get_extent(coll):
    if "dataset" in coll:
        logger.info("callADAGUC by dataset")
        urlrequest="dataset=%s&service=wms&version=1.3.0&request=getcapabilities"%(coll["dataset"],)
        status, response = callADAGUC(url=urlrequest.encode('UTF-8'))
        logger.info("status: %d", status)
        if status==0:
            xml = response.getvalue()
            wms = WebMapService(coll["service"], xml=xml, version='1.3.0')
        else:
            logger.error("status: %d", status)
            return {}
    else:
        logger.info("callADAGUC by service")
        wms = WebMapService(coll["service"], version='1.3.0')
    return wms.contents[next(iter(wms.contents))].boundingBoxWGS84

def get_datasets(adagucDataSetDir):
    logger.info("getDatasets(%s)", adagucDataSetDir)
    datasetFiles = [f for f in os.listdir(adagucDataSetDir) if os.path.isfile(
        os.path.join(adagucDataSetDir, f)) and f.endswith(".xml")]
    datasets = []
    for datasetFile in datasetFiles:
        logger.info("parsing %s", os.path.join(adagucDataSetDir, datasetFile))
        try:
            tree = parse(os.path.join(adagucDataSetDir, datasetFile))

            root = tree.getroot()
            for ogcapi in root.iter('OgcApiFeatures'):
                logger.info("ogcapi: %s", ogcapi)
                dataset = {
                    "dataset": datasetFile.replace(".xml", ""),
                    "name": datasetFile.replace(".xml", ""),
                    "title": datasetFile.replace(".xml", "").lower().capitalize(),
                    "service": "http://localhost:8087/wms?DATASET="+datasetFile.replace(".xml", ""),
                }
                dataset["extent"] = get_extent(dataset)
                datasets.append(dataset)
        except ParseError:
            pass

    return datasets

collections = None
coll_by_name = None

def generate_collections():
    global collections, coll_by_name
    if collections is not None:
        return collections
    collections = get_datasets(os.environ.get("ADAGUC_DATASET_DIR"))
    coll_by_name={}
    for c in collections:
        coll_by_name[c["name"]]=c
    return collections


def makedims(dims, data):
    dimlist=[]
    if isinstance(dims, str) and dims=="time":
        times=list(data.keys())
        dimlist.append({"time": times})
        return dimlist

    dt = data
    d1=list(dt.keys())
    if len(d1)==0:
        return []
    dimlist.append({dims[0]: d1})

    if len(dims)>=2:
        d2=list(dt[d1[0]].keys())
        dimlist.append({dims[1]: d2})

    if len(dims)>=3:
        d3=list(dt[d1[0]][d2[0]].keys())
        dimlist.append({dims[2]: d3})

    if len(dims)>=4:
        d4=list(dt[d1[0]][d2[0]][d3[0]].keys())
        dimlist.append({dims[2]: d4})

    if len(dims)>=5:
        d5=list(dt[d1[0]][d2[0]][d3[0]][d4[0]].keys())
        dimlist.append({dims[2]: d5})

    return dimlist

def makelist(list):
    if isinstance(list, OrderedDict):
        result = []
        for l in list.keys():
            result.append(makelist(list[l]))
        return result
    else:
        return float(list)

def getdimvals(dims, name):
    for n in dims:
        if list(n.keys())[0]==name:
            return list(n.values())[0]
    return None

def multi_get(dict_obj, attrs, default=None):
    result = dict_obj
    for attr in attrs:
        if attr not in result:
            return default
        result = result[attr]
    return result

def get_args(request):
    args={}

    request_args = request.args.copy()
    if "bbox" in request_args:
        args["bbox"] = request_args.pop("bbox")
    if "bbox-crs" in request_args:
        args["bbox-crs"] = request_args.pop("bbox-crs")
    if "crs" in request_args:
        args["crs"] = request_args.pop("crs", None)
    if "datetime" in request_args:
        args["datetime"] = request_args.pop("datetime")
    if "resultTime" in request_args:
        args["resultTime"] = request_args.pop("resultTime", None)
    if "phenomenonTime" in request_args:
        args["phenomenonTime"] = request_args.pop("phenomenonTime", None)
    if "observedPropertyName" in request_args:
        args["observedPropertyName"] = request_args.pop("observedPropertyName").split(",")
    if "lonlat" in request_args:
        args["lonlat"] = request_args.pop("lonlat")
    if "latlon" in request_args:
        args["latlon"] = request_args.pop("latlon", None)
    args["limit"] = 10
    if "limit" in request_args:
        args["limit"] = int(request_args.pop("limit"))
    args["nextToken"]=0
    if "nextToken" in request_args:
        args["nextToken"] = int(request_args.pop("nextToken"))
    if "dims" in request_args:
        args["dims"] = request_args.pop("dims")
    args["f"] = request_args.pop("f", None)
    if "npoints" in request_args:
        args["npoints"] = int(request_args.pop("npoints"))

    return args, len(request_args)


def make_link(pth, rel, typ, title, vars=None):
    link = {
        "rel": rel,
        "type": typ,
        "title": title
    }
    if vars:
        link["href"]=url_for(pth, **vars)
    else:
        link["href"]=url_for(pth)
    return link

@routeOGCApi.route("/", methods=['GET'])
@cross_origin()
def hello():
    """Root endpoint.
    ---
    get:
        description: Get root links
        responses:
            200:
              description: returns root links
              content:
                application/json:
                  schema: RootSchema
    """
    root = {
        "title": "ADAGUC OGCAPI-Features server",
        "description": "ADAGUC OGCAPI-Features server demo",
        "links": []
    }
    root["links"].append(make_link(".hello", "self", "application/json", "ADAGUC OGCAPI_Features server"))
    root["links"].append(make_link(".api", "service-desc", "application/vnd.oai.openapi+json;version=3.0", "API definition (JSON)"))
    root["links"].append(make_link(".api_yaml", "service-desc", "application/vnd.oai.openapi;version=3.0", "API definition (YAML)"))
    root["links"].append(make_link(".getconformance", "conformance", "application/json", "OGC API Features conformance classes implemented by this server"))
    root["links"].append(make_link(".getcollections", "data", "application/json", "Metadata about the feature collections"))

    if "f" in request.args and request.args["f"]=="html":
        response = render_template("root.html", root=root)
        return response
    return root

@routeOGCApi.route("/api", methods=['GET'])
@cross_origin()
def api():
    resp=current_app.make_response(spec.to_dict())
    resp.mimetype="application/vnd.oai.openapi+json;version=3.0"
    return resp

@routeOGCApi.route("/api.yaml", methods=['GET'])
@cross_origin()
def api_yaml():
    resp=current_app.make_response(spec.to_yaml())
    resp.mimetype="application/vnd.oai.openapi;version=3.0"
    return resp

@routeOGCApi.route("/conformance", methods=["GET"])
@cross_origin()
def getconformance():
    """Conformance endpoint.
    ---
    get:
        description: Get conformance
        responses:
            200:
              description: returns list of conformance URI's
              content:
                application/json:
                  schema: ReqClassesSchema
    """
    conformance = {
        "conformsTo": [
            "http://www.opengis.net/spec/ogcapi-features-1/1.0/conf/core",
            "http://www.opengis.net/spec/ogcapi-features-1/1.0/conf/oas30",
            "http://www.opengis.net/spec/ogcapi-features-1/1.0/conf/geojson",
            "http://www.opengis.net/spec/ogcapi-features-1/1.0/conf/html",
            "http://www.opengis.net/spec/ogcapi-features-2/1.0/conf/crs"
        ]
    }
    if "f" in request.args and request.args["f"]=="html":
        response = render_template("conformance.html", title="Conformance", description="conforms to:", conformance=conformance)
        return response

    return conformance


def make_wms1_3(serv):
    return serv+"&service=WMS&version=1.3.0"

def get_dimensions(l, skip_dims=[]):
    dims=[]
    for s in l.dimensions:
        if not s in skip_dims:
            dim={"name": s, "values": l.dimensions[s]["values"]}
            dims.append(dim)
    return dims

# @routeOGCApi.route("/getparams/<collname>", methods=['GET'])
# @cross_origin()
def get_parameters(collname):
    coll=coll_by_name[collname]
    if "dataset" in coll:
        logger.info("callADAGUC by dataset")
        urlrequest="dataset=%s&service=wms&version=1.3.0&request=getcapabilities"%(coll["dataset"],)
        status, response = callADAGUC(url=urlrequest.encode('UTF-8'))
        logger.info("status: %d", status)
        if status==0:
            xml = response.getvalue()
            wms = WebMapService(coll["service"], xml=xml, version='1.3.0')
        else:
            logger.error("status: %d", status)
            return {}
    else:
        logger.info("callADAGUC by service %s", coll["service"])
        wms = WebMapService(coll["service"], version='1.3.0')
    layers=[]
    for l in wms.contents:
        ls = l
        dims = get_dimensions(wms[l], ["time"])
        if len(dims)>0:
          layer = { "name": ls, "dims": dims}
        else:
          layer = { "name": ls}
        layers.append(layer)

    layers.sort(key=lambda l: l["name"])
    logger.info("l:%s", json.dumps(layers))
    return { "layers": layers }

def getcollection_by_name(coll):
    collectiondata = coll_by_name[coll]
    obj = get_parameters(collectiondata["name"])
    params = get_parameters(collectiondata["name"])["layers"]
    param_s = ""
    for p in params:
        if len(param_s)>0:
            param_s += ', '
        param_s += p["name"]
        if "dims" in p:
            for d in p["dims"]:
                param_s += "[%s:%s]"%(d["name"],",".join(d["values"]))

    c = {
            "id": collectiondata["name"],
            "title": collectiondata["title"],
            "extent": { "spatial": { "bbox": [collectiondata["extent"]]}},
            "description": collectiondata["name"]+" with parameters: "+param_s,
            "links": [],
            "crs": ["http://www.opengis.net/def/crs/OGC/1.3/CRS84"],
            "storageCrs": "http://www.opengis.net/def/crs/OGC/1.3/CRS84"
        }

    c["links"].append(make_link(".getcollections",
            "self", "application/json", "Metadata of "+collectiondata["title"], {"coll": collectiondata["name"]}))

    c["links"].append(make_link(".getcollections",
            "self", "text/html", "Metadata of "+collectiondata["title"], {"coll": collectiondata["name"], "f": "html"}))

    c["links"].append(make_link(".getcollitems",
            "self", "application/geo+json", "Items of "+collectiondata["title"], {"coll": collectiondata["name"], "f": "html"}))

    c["links"].append(make_link(".getcollitems",
            "self", "text/html", "Items of "+collectiondata["title"], {"coll": collectiondata["name"], "f": "html"}))
    return c

@routeOGCApi.route("/collections", methods=["GET"])
@cross_origin()
def getcollections():
    """Collections endpoint.
    ---
    get:
        description: Get collections
        responses:
            200:
              description: returns list of collections
              content:
                application/json:
                  schema: ContentSchema
    """
    res={
        "crs": [
            "http://www.opengis.net/def/crs/OGC/1.3/CRS84",
            "http://www.opengis.net/def/crs/EPSG/0/4326",
        ],
        "collections":[],
        "links": []
        }
    res["links"].append(make_link(".getcollections",
        "self", "application/json", "Metadata about the feature collections"))
    res["links"].append(make_link(".getcollections",
        "alternate", "text/html", "Metadata about the feature collections (HTML)", {"f": "html"}))
    for c in generate_collections():
        res["collections"].append(getcollection_by_name(c["name"]))

    if "f" in request.args and request.args["f"]=="html":
        response = render_template("collections.html", collections=res)
        return response

    return res



@routeOGCApi.route("/collections/<coll>", methods=["GET"])
@cross_origin()
def getcollection(coll):
    """Collections endpoint.
    ---
    get:
        description: Get collection info
        parameters:
            - in: path
              schema: CollectionParameter
        responses:
            200:
              description: retu5ctionInfoSchema
    """
    collection = getcollection_by_name(coll)
    if "f" in request.args and request.args["f"]=="html":
        response = render_template("collection.html", collection=collection)
        return response

    return collection

def request_by_id(url, name, headers=None, requested_id=None):
    url = make_wms1_3(url)+"&request=getPointValue&INFO_FORMAT=application/json"

    if requested_id is not None:
        # Get feature data for this id
        terms = requested_id.split(";")
        layer_name = terms[0]
        observedPropertyName = terms[1]
        url = "%s&LAYERS=%s"%(url, observedPropertyName)
        lon, lat = terms[2].split(",")
        for term in terms[3:-1]:
            dim_name, dim_value = term.split("=")
            if dim_name.lower() == "reference_time":
                url = "%s&DIM_REFERENCE_TIME=%s"%(url, dim_value)
            elif dim_name.lower() == "elevation":
                url = "%s&ELEVATION=%s"%(url, dim_value)
            else:
                url = "%s&DIM_%s=%s"%(url, dim_name, dim_value)

        url = "%s&X=%s&Y=%s&CRS=EPSG:4326"%(url, lon, lat)
        url = "%s&TIME=%s"%(url, "/".join(terms[-1].split("$")))
        response = requests.get(url, headers=headers, timeout=TIMEOUT)
        if response.status_code == 200:
            try:
                data = json.loads(response.content.decode('utf-8'), object_pairs_hook=OrderedDict)
            except ValueError:
                root = fromstring(response.content.decode('utf-8'))

                retval =  json.dumps({"Error":  { "code": root[0].attrib["code"], "message": root[0].text}})
                return 400, root[0].text.strip(), None, None
            dat = data[0]
            item_feature = feature_from_dat(dat, observedPropertyName, name)
            feature = item_feature[0]
            feature["links"]=[
                make_link(".getcollitembyid", "self", "application/geo+json", "This document", {"coll": name, "featureid": requested_id}),
                make_link(".getcollitembyid", "alternate", "text/html", "This document in html", {"coll": name, "featureid": requested_id, "f": "html"}),
                make_link(".getcollection", "collection", "application/json", "Collection", {"coll": name})
            ]
            return 200, json.dumps(feature), {'Content-Crs': "<http://www.opengis.net/def/crs/OGC/1.3/CRS84>"}
    return 400, None, None

def feature_from_dat(dat, name, observedPropertyName):
    dims = makedims(dat["dims"], dat["data"])
    timeSteps = getdimvals(dims, "time")
    if not timeSteps or len(timeSteps)==0:
        return []

    valstack=[]
    dims_without_time=[]
    for d in dims:
        dim_name = list(d.keys())[0]
        if dim_name!="time":
            dims_without_time.append(d)
            vals=getdimvals(dims, dim_name)
            valstack.append(vals)
    tuples = list(itertools.product(*valstack))

    features=[]
    logger.info("TUPLES: %s [%d] %s",json.dumps(tuples), len(tuples), json.dumps(dims_without_time))

    for t in tuples:
        logger.info("T:%s", t)
        result=[]
        for ts in timeSteps:
            v = multi_get(dat["data"], t+(ts,))
            if v:
                result.append(float(v))

        feature_dims={}

        layer_name=dat["name"]
        if dat["standard_name"]=="x_wind":
            layer_name="x_"+dat["name"]
        if dat["standard_name"]=="y_wind":
            layer_name="y_"+dat["name"]

        feature_id = "%s;%s;%s"%(observedPropertyName, dat["name"],dat["point"]["coords"])
        i=0
        for dim_value in t:
            feature_dims[list(dims_without_time[i].keys())[0]]=dim_value
            feature_id = feature_id + ";%s=%s"%(list(dims_without_time[i].keys())[0], dim_value)
            i=i+1

        feature_id = feature_id + ";%s$%s"%(timeSteps[0], timeSteps[-1])
        if len(feature_dims)==0:
            properties={
                "timestep": timeSteps,
                "observationType": "MeasureTimeseriesObservation",
                "observedPropertyName": name,
                "result": result
            }
        else:
            properties={
                "timestep": timeSteps,
                "dims": feature_dims,
                "observationType": "MeasureTimeseriesObservation",
                "observedPropertyName": name,
                "result": result
            }

        coords = dat["point"]["coords"].split(",")
        coords[0]=float(coords[0])
        coords[1]=float(coords[1])
        feature = {
                "type": "Feature",
                "geometry": {
                    "type": "Point",
                    "coordinates":  coords
                },
                "properties": properties,
                "id": feature_id
        }
        features.append(feature)
    return features

@routeOGCApi.route("/collections/<coll>/items/<featureid>", methods=["GET"])
@cross_origin()
def getcollitembyid(coll, featureid):
    """Collection item with id endpoint.
    ---
    get:
        description: Get collection item with id featureid
        responses:
            200:
              description: returns items from a collection
              content:
                application/geo+json:
                  schema: FeatureGeoJSONSchema
    """

    params = get_parameters(coll)
    headers = {
        'Content-Type': 'application/geo+json',
    }

    coll_info = coll_by_name[coll]
    (status, feature, headers) =  request_by_id(coll_info["service"], coll_info["name"], headers, featureid)
    return Response(feature, status, headers=headers)

def calculate_coords(bbox, nlon, nlat):
    dlon = (bbox[2]-bbox[0])/(nlon+1)
    dlat = (bbox[3]-bbox[1])/(nlat+1)
    coords=[]
    for lo in range(nlon):
        lon=bbox[0]+lo*dlon+dlon/2.
        for la in range(nlat):
            lat = bbox[1]+la*dlat+dlat/2
            coords.append([lon, lat])
    return coords

def get_coords(coords, next, limit):
    if next>len(coords):
        return None
    else:
        return coords[next:next+limit]

def replaceNextToken(url, newNextToken):
    if "nextToken=" in url:
        return re.sub(r'(.*)nextToken=(\d+)(.*)', r'\1nextToken='+newNextToken+r'\3', url)
    return url+'&nextToken='+newNextToken

def replaceFormat(url, newFormat):
    if "f=" in url:
        return re.sub(r'(.*)f=([^&]*)(&.*)', r'\1&f='+newFormat+r'\3', url)
    return url+"&f="+newFormat

def get_reference_times(layers, layer, last=False):
    if "layers" in layers:
        for l in layers["layers"]:
            if l["name"]==layer and "dims" in l:
                for d in l["dims"]:
                    if d["name"]=="reference_time":
                        if last:
                            return d["values"][-1]
                        else:
                            return d["values"]

    return None

def request_(url, args, name, headers=None):
    url = make_wms1_3(url)+"&request=getPointValue&INFO_FORMAT=application/json"

    if "latlon" in args and args["latlon"]:
        x=args["latlon"].split(",")[1]
        y=args["latlon"].split(",")[0]
        url = "%s&X=%s&Y=%s&CRS=EPSG:4326"%(url, x, y)
    if "lonlat" in args and args["lonlat"]:
        x=args["lonlat"].split(",")[0]
        y=args["lonlat"].split(",")[1]
        url = "%s&X=%s&Y=%s&CRS=EPSG:4326"%(url, x, y)
    if not "CRS=" in url.upper():
        url = "%s&X=%s&Y=%s&CRS=EPSG:4326"%(url, 5.2, 52.0)
    reference_time = None
    if "resultTime" in args and args["resultTime"]:
        url = "%s&DIM_REFERENCE_TIME=%s"%(url, args["resultTime"])
        reference_time = args["resultTime"]
    if "datetime" in args and args["datetime"] is not None:
        url = "%s&TIME=%s"%(url, args["datetime"])
    else:
        url = "%s&TIME=%s"%(url, "*")

    url = "%s&LAYERS=%s&QUERY_LAYERS=%s"%(url, args["observedPropertyName"], args["observedPropertyName"])

    if "dims" in args and args["dims"]:
        for dim in args["dims"].split(";"):
            dimname,dimval=dim.split(":")
            if dimname.upper()=="ELEVATION":
                url = "%s&%s=%s"%(url, dimname, dimval)
            else:
                url = "%s&DIM_%s=%s"%(url, dimname, dimval)

    logger.info("URL:", url)
    response = requests.get(url, headers=headers, timeout=TIMEOUT)
    if response.status_code == 200:
        try:
            response_data = json.loads(response.content.decode('utf-8'), object_pairs_hook=OrderedDict)
        except ValueError:
            root = fromstring(response.content.decode('utf-8'))
            logger.info("ET:", root)

            retval =  json.dumps({"Error":  { "code": root[0].attrib["code"], "message": root[0].text}})
            logger.info("retval=", retval)
            return 400, root[0].text.strip()
        features=[]
        for data in response_data:
            data_features = feature_from_dat(data, args["observedPropertyName"], name)
            features.extend(data_features)

        return 200, features
    return 400, "Error"

@routeOGCApi.route("/collections/<coll>/items", methods=["GET"])
@cross_origin()
def getcollitems(coll):
    """Collection items endpoint.
    ---
    get:
        description: Get collection items
        parameters:
            - in: path
              schema: CollectionParameter
            - in: query
              schema: LimitParameter
            - in: query
              schema: BboxParameter
            - in: query
              schema: DatetimeParameter
            - in: query
              schema: PhenomenonTimeParameter
            - in: query
              schema: ResultTimeParameter
            - in: query
              schema: LonLatParameter
            - in: query
              schema: LatLonParameter
            - in: query
              schema: ObservedPropertyNameParameter
            - in: query
              schema: NPointsParameter
        responses:
            200:
              description: returns items from a collection
              content:
                application/json:
                  schema: FeatureCollectionGeoJSONSchema
    """
    coll_info = coll_by_name[coll]

    args, leftover_args = get_args(request)
    if not "bbox" in args or args["bbox"] is None:
        args["bbox"] = coll_info["extent"]
    if not "npoints" in args or args["npoints"] is None:
        args["npoints"] = 1
    coords = calculate_coords(args["bbox"], args["npoints"], args["npoints"])
    if "crs" in args and args.get("crs") not in SUPPORTED_CRS:
        return Response("Unsupported CRS", 400)
    if "bbox-crs" in args and args.get("bbox-crs") not in SUPPORTED_CRS:
        return Response("Unsupported BBOX CRS", 400)

    limit = args["limit"]
    nextToken = args["nextToken"]

    if leftover_args>0:
        return Response("Too many arguments", 400)

    headers = {
        'Content-Type': 'application/json'
    }

    request_path = request.full_path
    features=[]
    if "observedPropertyName" not in args or args["observedPropertyName"] is None:
        params = get_parameters(coll)
        args["observedPropertyName"]=[params["layers"][0]["name"]]
    logger.info("OBS:%s", args["observedPropertyName"])

    layers=[]
    if not "resultTime" in args:
        layers = get_parameters(coll)

    for parameter_name in args["observedPropertyName"]:
        param_args = {**args}
        param_args["observedPropertyName"]=parameter_name
        if not "resultTime" in param_args:
            latest_reference_time = get_reference_times(layers, parameter_name, True)
            if latest_reference_time:
                param_args["resultTime"]=latest_reference_time
        if "lonlat" in param_args or "latlon" in param_args:
            status, coordfeatures = request_(coll_info["service"], param_args, coll_info["name"], headers)
            if status==200:
                features.extend(coordfeatures)
        else:
            for c in coords: #get_coords(coords, int(args["nextToken"]), int(args["limit"])):
                param_args["lonlat"] = "%f,%f"%(c[0], c[1])
                status, coordfeatures = request_(coll_info["service"], param_args, coll_info["name"], headers)
                if status==200:
                    features.extend(coordfeatures)

    if "f" in request.args and request.args["f"]=="html":
        links=[
            make_link(".getcollitems", "self", "text/html", "This document", {"coll": coll, "f": "html"}),
            make_link(".getcollitems", "alternate", "application/geo+json", "This document", {"coll": coll}),
        ]
    else:
        links=[
            make_link(".getcollitems", "self", "application/geo+json", "This document", {"coll": coll}),
            make_link(".getcollitems", "alternate", "text/html", "This document", {"coll": coll, "f":"html"}),
        ]

    response_features = features[nextToken:nextToken+limit]
    if len(features)>limit and len(features)>(nextToken+limit):
        links.append(make_link(".getcollitems", "next", "application/geo+json", "Next set of elements", {"coll": coll, "limit": limit}))

    featurecollection = {
            "type": "FeatureCollection",
            "features": response_features,
            "timeStamp": datetime.now().strftime("%Y-%m-%dT%H:%M:%SZ"),
            "numberReturned": len(response_features),
            "numberMatched": len(features),
            "links": links
    }

    mime_type = "application/geo+json"
    headers = {'Content-Crs': "<http://www.opengis.net/def/crs/OGC/1.3/CRS84>"}
    if "f" in request.args and request.args["f"]=="html":
        response = render_template("items.html", collection=coll_info["name"], items=featurecollection)
        return response
    return Response(json.dumps(featurecollection), 200, mimetype=mime_type, headers=headers)

def init_views():
    with current_app.app_context():
        spec.path(view=hello, path="/")
        spec.path(view=getconformance, path="/conformance")
        spec.path(view=getcollection, path="/collections/<coll>")
        spec.path(view=getcollections, path="/collections")
        spec.path(view=getcollitems, path="/collections/<coll>/items")
        spec.path(view=getcollitembyid, path="/collections/<coll>/items/<featureid>")

