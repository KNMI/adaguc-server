from .setup_adaguc import setup_adaguc
import logging
import time
import os

from owslib.wms import WebMapService
from .setup_adaguc import setup_adaguc
from defusedxml.ElementTree import fromstring, parse, ParseError

logger = logging.getLogger(__name__)


def make_bbox(extent):
    s = []
    for i in extent:
        s.append("%f" % (i,))
    return ",".join(s)


def get_extent(coll):
    """
    Get the boundingbox extent from the WMS GetCapabilities
    """
    contents = get_capabilities(coll)
    if len(contents):
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
                logger.info("ogcapi: %s", ogcapi)
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


def callADAGUC(url):
    print(f"callADAGUC({url}")
    """Call adaguc-server"""
    adagucInstance = setup_adaguc()

    url = url.decode()
    logger.info(">>>>>%s", url)
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
