import itertools
import logging
import os
import time
from typing import List

from defusedxml.ElementTree import ParseError, parse

from owslib.wms import WebMapService

from .models.ogcapifeatures_1_model import (
    FeatureGeoJSON,
    Link,
    PointGeoJSON,
    Type1,
    Type7,
)
from .setup_adaguc import setup_adaguc

logger = logging.getLogger(__name__)


def make_bbox(extent):
    s_extent = []
    for i in extent:
        s_extent.append(i)
    return s_extent


async def get_extent(coll):
    """
    Get the boundingbox extent from the WMS GetCapabilities
    """
    contents = await get_capabilities(coll)
    if contents and len(contents):
        return contents[next(iter(contents))].boundingBoxWGS84
    return None


def get_datasets(adaguc_data_set_dir):
    """
    Return all possible OGCAPI feature datasets, based on the dataset directory
    """
    dataset_files = [
        f
        for f in os.listdir(adaguc_data_set_dir)
        if os.path.isfile(os.path.join(adaguc_data_set_dir, f)) and f.endswith(".xml")
    ]
    datasets = {}
    for dataset_file in dataset_files:
        try:
            tree = parse(os.path.join(adaguc_data_set_dir, dataset_file))
            root = tree.getroot()
            for _ogcapi in root.iter("OgcApiFeatures"):
                # Note, service is just a placeholder because it is needed by OWSLib.
                # Adaguc is still run as executable, not as service"""
                dataset = {
                    "dataset": dataset_file.replace(".xml", ""),
                    "name": dataset_file.replace(".xml", ""),
                    "title": dataset_file.replace(".xml", "").lower().capitalize(),
                    "service": "http://localhost:8080/wms?DATASET="
                    + dataset_file.replace(".xml", ""),
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
    for lonval in range(nlon):
        lon = bbox[0] + lonval * dlon + dlon / 2.0
        for latval in range(nlat):
            lat = bbox[1] + latval * dlat + dlat / 2
            coords.append([lon, lat])
    return coords


async def call_adaguc(url):
    """Call adaguc-server"""
    adaguc_instance = setup_adaguc()

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
    status, data, headers = await adaguc_instance.runADAGUCServer(
        url, env=adagucenv, showLogOnError=True
    )

    # Obtain logfile
    logfile = adaguc_instance.getLogFile()
    adaguc_instance.removeLogFile()

    stage2 = time.perf_counter()
    logger.info("[PERF] Adaguc execution took: %f", (stage2 - stage1))

    if len(logfile) > 0:
        logger.info(logfile)

    return status, data, headers


async def get_capabilities(collname):
    """
    Get the collectioninfo from the WMS GetCapabilities
    """
    coll = generate_collections().get(collname)
    dataset = coll["dataset"]
    urlrequest = f"dataset={dataset}&service=wms&version=1.3.0&request=getcapabilities"
    status, response, _ = await call_adaguc(url=urlrequest.encode("UTF-8"))
    if status == 0:
        xml = response.getvalue()
        wms = WebMapService(coll["service"], xml=xml, version="1.3.0")
    else:
        logger.error("status: %d", status)
        return {}
    return wms.contents


def generate_collections():
    """
    Generate OGC API Feature collections
    """
    collections = get_datasets(os.environ.get("ADAGUC_DATASET_DIR"))
    return collections


def get_dimensions(layer, skip_dims=None):
    """
    Gets the dimensions from a layer definition, skipping the dimensions
    in skip_dims
    """
    dims = []
    if skip_dims is None:
        skip_dims = []
    for dim_name in layer.dimensions:
        if not dim_name in skip_dims:
            new_dim = {
                "name": dim_name,
                "values": layer.dimensions[dim_name]["values"],
            }
            dims.append(new_dim)
    return dims


async def get_parameters(collname):
    """
    get_parameters
    """
    contents = await get_capabilities(collname)
    layers = []
    for layer in contents:
        dims = get_dimensions(contents[layer], ["time"])
        if len(dims) > 0:
            layer = {"name": layer, "dims": dims}
        else:
            layer = {"name": layer}
        layers.append(layer)

    layers.sort(key=lambda l: l["name"])
    return layers


def make_dims(dims, data):
    """
    Makedims
    """
    dimlist = []
    if isinstance(dims, str) and dims == "time":
        times = list(data.keys())
        dimlist.append({"time": times})
        return dimlist

    dim1 = list(data.keys())
    if len(dim1) == 0:
        return []
    dimlist.append({dims[0]: dim1})

    if len(dims) >= 2:
        dim2 = list(data[dim1[0]].keys())
        dimlist.append({dims[1]: dim2})

    if len(dims) >= 3:
        dim3 = list(data[dim1[0]][dim2[0]].keys())
        dimlist.append({dims[2]: dim3})

    if len(dims) >= 4:
        dim4 = list(data[dim1[0]][dim2[0]][dim3[0]].keys())
        dimlist.append({dims[3]: dim4})

    if len(dims) >= 5:
        dim5 = list(data[dim1[0]][dim2[0]][dim3[0]][dim4[0]].keys())
        dimlist.append({dims[4]: dim5})

    return dimlist


def getdimvals(dims, name):
    """
    getdimvals
    """
    for nlist in dims:
        if list(nlist.keys())[0] == name:
            return list(nlist.values())[0]
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


def get_items_links(
    url: str,
    prev_start: int = None,
    next_start: int = None,
    limit: int = None,
) -> List[Link]:
    links: List[Link] = []
    links.append(
        Link(
            href=f"{url}",
            rel="self",
            title="Item in JSON",
            type="application/geo+json",
        )
    )
    links.append(
        Link(
            href=f"{url}?f=html",
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


def get_single_item_links(
    item_id: str,
    url: str,
    prev_start: int = None,
    next_start: int = None,
    limit: int = None,
) -> List[Link]:
    links: List[Link] = []
    links.append(
        Link(
            href=f"{url}",
            rel="self",
            title="Item in JSON",
            type="application/geo+json",
        )
    )
    links.append(
        Link(
            href=f"{url}?f=html",
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


def feature_from_dat(dat, coll, url, add_links: bool = False):
    """
    feature_from_dat
    """
    dims = make_dims(dat["dims"], dat["data"])
    time_steps = getdimvals(dims, "time")
    if not time_steps or len(time_steps) == 0:
        return []

    valstack = []
    dims_without_time = []
    for dim in dims:
        dim_name = list(dim.keys())[0]
        if dim_name != "time":
            dims_without_time.append(dim)
            vals = getdimvals(dims, dim_name)
            valstack.append(vals)
    tuples = list(itertools.product(*valstack))

    features = []

    for tupl in tuples:
        result = []
        for time_step in time_steps:
            val = multi_get(dat["data"], tupl + (time_step,))
            if val:
                try:
                    value = float(val)
                    result.append(value)
                except ValueError:
                    result.append(val)

        feature_dims = {}
        datname = dat["name"]
        datpointcoords = dat["point"]["coords"]
        feature_id = f"{coll};{datname};{datpointcoords};"
        cnt = 0
        result_time = None
        for dim_value in tupl:
            dim_name = list(dims_without_time[cnt].keys())[0]
            if dim_name.lower() == "reference_time":
                result_time = dim_value
            feature_dims[dim_name] = dim_value
            if cnt > 0:
                feature_id += "|"
            # pylint: disable=consider-using-f-string
            feature_id = feature_id + "%s=%s" % (
                dim_name,
                dim_value,
            )
            cnt = cnt + 1

        feature_id = feature_id + f";{time_steps[0]}${time_steps[-1]}"
        if len(feature_dims) == 0:
            properties = {
                "timestep": time_steps,
                "observationType": "MeasureTimeseriesObservation",
                "observedPropertyName": datname,
                "result": result,
                "resultTime": result_time,
            }
        else:
            properties = {
                "timestep": time_steps,
                "dims": feature_dims,
                "observationType": "MeasureTimeseriesObservation",
                "observedPropertyName": datname,
                "result": result,
                "resultTime": result_time,
            }

        coords = dat["point"]["coords"].split(",")
        coords[0] = float(coords[0])
        coords[1] = float(coords[1])
        if add_links:
            links = get_single_item_links(feature_id, str(url))
        else:
            links = None

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
