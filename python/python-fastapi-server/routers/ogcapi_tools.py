from .setup_adaguc import setup_adaguc
import logging
import time
import os

import json

import itertools
from typing import Any, Dict, List, Union, Sequence, Type

from owslib.wms import WebMapService
from .setup_adaguc import setup_adaguc
from defusedxml.ElementTree import fromstring, parse, ParseError

from .models.ogcapifeatures_1_model import (
    GeometryGeoJSON,
    LandingPage,
    Link,
    Collections,
    Collection,
    ConfClasses,
    FeatureCollectionGeoJSON,
    FeatureGeoJSON,
    PointGeoJSON,
    Type,
    Type1,
    Type2,
    Type3,
    Type7,
    NumberMatched,
    NumberReturned,
    Extent,
    Spatial,
    Temporal,
)

logger = logging.getLogger(__name__)


def make_bbox(extent):
    s = []
    for i in extent:
        s.append(i)
    return s


def get_extent(coll):
    """
    Get the boundingbox extent from the WMS GetCapabilities
    """
    contents = get_capabilities(coll)
    if contents and len(contents):
        return contents[next(iter(contents))].boundingBoxWGS84
    return None


def get_datasets(adagucDataSetDir):
    """
    Return all possible OGCAPI feature datasets, based on the dataset directory
    """
    logger.info("getDatasets(%s)", adagucDataSetDir)
    datasetFiles = [
        f
        for f in os.listdir(adagucDataSetDir)
        if os.path.isfile(os.path.join(adagucDataSetDir, f)) and f.endswith(".xml")
    ]
    datasets = {}
    for datasetFile in datasetFiles:
        try:
            tree = parse(os.path.join(adagucDataSetDir, datasetFile))
            root = tree.getroot()
            for ogcapi in root.iter("OgcApiFeatures"):
                """Note, service is just a placeholder because it is needed by OWSLib. Adaguc is still ran as executable, not as service"""
                dataset = {
                    "dataset": datasetFile.replace(".xml", ""),
                    "name": datasetFile.replace(".xml", ""),
                    "title": datasetFile.replace(".xml", "").lower().capitalize(),
                    "service": "http://localhost:8080/wms?DATASET="
                    + datasetFile.replace(".xml", ""),
                }
                datasets[dataset["name"]] = dataset
        except ParseError:
            pass
    return datasets


def calculate_coords(bbox, nlon, nlat):
    """calculate_coords"""
    dlon = (bbox[2] - bbox[0]) / (nlon + 1)
    dlat = (bbox[3] - bbox[1]) / (nlat + 1)
    coords = []
    for lo in range(nlon):
        lon = bbox[0] + lo * dlon + dlon / 2.0
        for la in range(nlat):
            lat = bbox[1] + la * dlat + dlat / 2
            coords.append([lon, lat])
    return coords


def callADAGUC(url):
    """Call adaguc-server"""
    adagucInstance = setup_adaguc()

    url = url.decode()
    if "?" in url:
        url = url[url.index("?") + 1 :]
    logger.info(url)
    stage1 = time.perf_counter()

    adagucenv = {}

    # Set required environment variables
    adagucenv["ADAGUC_ONLINERESOURCE"] = (
        os.getenv("EXTERNALADDRESS", "http://192.168.178.113:8080") + "/adaguc-server?"
    )
    adagucenv["ADAGUC_DB"] = os.getenv(
        "ADAGUC_DB", "user=adaguc password=adaguc host=localhost dbname=adaguc"
    )

    # Run adaguc-server
    # pylint: disable=unused-variable
    status, data, headers = adagucInstance.runADAGUCServer(
        url, env=adagucenv, showLogOnError=True
    )

    # Obtain logfile
    logfile = adagucInstance.getLogFile()
    adagucInstance.removeLogFile()

    stage2 = time.perf_counter()
    logger.info("[PERF] Adaguc execution took: %f", (stage2 - stage1))

    if len(logfile) > 0:
        logger.info(logfile)

    return status, data


# @cacher.memoize(timeout=30)
def get_capabilities(collname):
    """
    Get the collectioninfo from the WMS GetCapabilities
    """
    coll = generate_collections().get(collname)
    if "dataset" in coll:
        logger.info("callADAGUC by dataset")
        dataset = coll["dataset"]
        urlrequest = (
            f"dataset={dataset}&service=wms&version=1.3.0&request=getcapabilities"
        )
        status, response = callADAGUC(url=urlrequest.encode("UTF-8"))
        logger.info("status: %d", status)
        if status == 0:
            xml = response.getvalue()
            wms = WebMapService(coll["service"], xml=xml, version="1.3.0")
        else:
            logger.error("status: %d", status)
            return {}
    else:
        logger.info("callADAGUC by service %s", coll)
        wms = WebMapService(coll["service"], version="1.3.0")
    return wms.contents


# @cacher.cached(timeout=30, key_prefix="collections")
def generate_collections():
    """
    Generate OGC API Feature collections
    """
    collections = get_datasets(os.environ.get("ADAGUC_DATASET_DIR"))
    return collections


def get_dimensions(l, skip_dims=None):
    """
    get_dimensions
    """
    dims = []
    for s in l.dimensions:
        if not s in skip_dims:
            dim = {"name": s, "values": l.dimensions[s]["values"]}
            dims.append(dim)
    return dims


# @cacher.memoize(timeout=30)
def get_parameters(collname):
    """
    get_parameters
    """
    contents = get_capabilities(collname)
    layers = []
    for l in contents:
        logger.info("l: %s", l)
        ls = l
        dims = get_dimensions(contents[l], ["time"])
        if len(dims) > 0:
            layer = {"name": ls, "dims": dims}
        else:
            layer = {"name": ls}
        layers.append(layer)

    layers.sort(key=lambda l: l["name"])
    # logger.info("l:%s", json.dumps(layers)) # THIS CAUSES A HUGE LOGGING MESSAGE!
    return {"layers": layers}


def makedims(dims, data):
    """
    Makedims
    """
    dimlist = []
    if isinstance(dims, str) and dims == "time":
        times = list(data.keys())
        dimlist.append({"time": times})
        return dimlist

    dt = data
    d1 = list(dt.keys())
    if len(d1) == 0:
        return []
    dimlist.append({dims[0]: d1})

    if len(dims) >= 2:
        d2 = list(dt[d1[0]].keys())
        dimlist.append({dims[1]: d2})

    if len(dims) >= 3:
        d3 = list(dt[d1[0]][d2[0]].keys())
        dimlist.append({dims[2]: d3})

    if len(dims) >= 4:
        d4 = list(dt[d1[0]][d2[0]][d3[0]].keys())
        dimlist.append({dims[2]: d4})

    if len(dims) >= 5:
        d5 = list(dt[d1[0]][d2[0]][d3[0]][d4[0]].keys())
        dimlist.append({dims[2]: d5})

    return dimlist


def getdimvals(dims, name):
    """
    getdimvals
    """
    for n in dims:
        if list(n.keys())[0] == name:
            return list(n.values())[0]
    return None


def multi_get(dict_obj, attrs, default=None):
    """
    multi_get
    """
    result = dict_obj
    for attr in attrs:
        if attr not in result:
            return default
        result = result[attr]
    return result


def getCollectionLinks(
    id: str,
    item_id: str,
    url: str,
    self_url: str,
    prev_start: int = None,
    next_start: int = None,
    limit: int = None,
) -> List[Link]:
    links: List[Link] = []
    links.append(
        Link(
            href=self_url,
            rel="self",
            title="Item in JSON",
            type="application/geo+json",
        )
    )
    return links


def getItemLinks(
    id: str,
    item_id: str,
    url: str,
    self_url: str,
    prev_start: int = None,
    next_start: int = None,
    limit: int = None,
) -> List[Link]:
    links: List[Link] = []
    logger.info(f"GI:{id},{item_id}{url==self_url}")
    links.append(
        Link(
            href=f"{url}{'/'+item_id if item_id else ''}",
            rel="self",
            title="Item in JSON",
            type="application/geo+json",
        )
    )
    links.append(
        Link(
            href=f"{url}{'/'+item_id if item_id else ''}?f=html",
            rel="alternate",
            title="Item in HTML",
            type="text/html",
        )
    )
    if prev_start is not None:
        links.append(
            Link(
                href=url + f"?start={prev_start}&limit={limit}",
                rel="prev",
                title="Item in JSON",
                type="application/geo+json",
            )
        )
    if next_start is not None:
        links.append(
            Link(
                href=url + f"?start={next_start}&limit={limit}",
                rel="next",
                title="Item in JSON",
                type="application/geo+json",
            )
        )

    if item_id:
        collection_url = "/".join(url.split("/")[:-2])
    else:
        collection_url = "/".join(url.split("/")[:-1])
    links.append(
        Link(
            href=collection_url,
            rel="collection",
            title="Collection",
            type="application/geo+json",
        )
    )

    return links


def feature_from_dat(dat, observedPropertyName, name, url, self_url):
    """
    feature_from_dat
    """
    dims = makedims(dat["dims"], dat["data"])
    timeSteps = getdimvals(dims, "time")
    if not timeSteps or len(timeSteps) == 0:
        return []

    valstack = []
    dims_without_time = []
    for d in dims:
        dim_name = list(d.keys())[0]
        if dim_name != "time":
            dims_without_time.append(d)
            vals = getdimvals(dims, dim_name)
            valstack.append(vals)
    tuples = list(itertools.product(*valstack))

    features = []

    for t in tuples:
        logger.info("T:%s", t)
        result = []
        for ts in timeSteps:
            v = multi_get(dat["data"], t + (ts,))
            if v:
                try:
                    value = float(v)
                    result.append(value)
                except ValueError:
                    result.append(v)

        feature_dims = {}
        datname = dat["name"]
        datpointcoords = dat["point"]["coords"]
        feature_id = f"{name};{datname};{datpointcoords}"
        i = 0
        for dim_value in t:
            feature_dims[list(dims_without_time[i].keys())[0]] = dim_value
            # pylint: disable=consider-using-f-string
            feature_id = feature_id + ";%s=%s" % (
                list(dims_without_time[i].keys())[0],
                dim_value,
            )
            i = i + 1

        feature_id = feature_id + f";{timeSteps[0]}${timeSteps[-1]}"
        if len(feature_dims) == 0:
            properties = {
                "timestep": timeSteps,
                "observationType": "MeasureTimeseriesObservation",
                "observedPropertyName": observedPropertyName,
                "result": result,
            }
        else:
            properties = {
                "timestep": timeSteps,
                "dims": feature_dims,
                "observationType": "MeasureTimeseriesObservation",
                "observedPropertyName": observedPropertyName,
                "result": result,
            }

        coords = dat["point"]["coords"].split(",")
        coords[0] = float(coords[0])
        coords[1] = float(coords[1])
        links = getItemLinks("ID???", feature_id, str(url), str(self_url))

        point = PointGeoJSON(type=Type7.Point, coordinates=coords)
        feature = FeatureGeoJSON(
            type=Type1.Feature,
            geometry=point,
            properties=properties,
            id=feature_id,
            links=links,
        )
        features.append(feature)
    return features
