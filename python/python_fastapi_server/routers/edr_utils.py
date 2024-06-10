"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

import json
import logging
import os
from datetime import datetime, timezone

from cachetools import TTLCache, cached
from defusedxml.ElementTree import ParseError, parse
from fastapi import Request

from .ogcapi_tools import call_adaguc

logger = logging.getLogger(__name__)


def parse_config_file(
    dataset_filename: str, adaguc_dataset_dir: str = os.environ["ADAGUC_DATASET_DIR"]
):
    """Return translation names for NetCDF data"""
    translate = {}
    dim_translate = {}
    filename = dataset_filename
    _, ext = os.path.splitext(filename)
    if ext == "" or ext != ".xml":
        filename = filename + ".xml"

    if os.path.isfile(os.path.join(adaguc_dataset_dir, filename)) and filename.endswith(
        ".xml"
    ):
        tree = parse(os.path.join(adaguc_dataset_dir, filename))
        root = tree.getroot()
        for lyr in root.iter("Layer"):
            layer_variables = [l.text for l in lyr.iter("Variable")]
            if len(layer_variables) == 1:
                translate[layer_variables[0]] = lyr.find("Name").text
        for coll in root.iter("EdrCollection"):
            if "vertical_name" in coll.attrib:
                vertical_name = coll.attrib["vertical_name"]
                for lyr in root.iter("Layer"):
                    for dimension in lyr.iter("Dimension"):
                        if dimension.text == vertical_name:
                            dim_translate[dimension.attrib["name"]] = "z"
    return translate, dim_translate


parse_config_file("uwcw_ha43_nl_2km")


def parse_iso(dts: str) -> datetime:
    """
    Converts a ISO8601 string into a datetime object
    """
    parsed_dt = None
    try:
        parsed_dt = datetime.strptime(dts, "%Y-%m-%dT%H:%M:%SZ").replace(
            tzinfo=timezone.utc
        )
    except ValueError as exc:
        logger.error("err: %s %s", dts, exc)
    return parsed_dt


async def get_ref_times_for_coll(edr_collectioninfo: dict, layer: str) -> list[str]:
    """
    Returns available reference times for given collection
    """
    dataset = edr_collectioninfo["dataset"]
    url = f"?DATASET={dataset}&SERVICE=WMS&VERSION=1.3.0&request=getreferencetimes&LAYER={layer}"
    # logger.info("getreftime_url(%s,%s): %s", dataset, layer, url)
    status, response, _ = await call_adaguc(url=url.encode("UTF-8"))
    if status == 0:
        ref_times = json.loads(response.getvalue())
        instance_ids = [parse_iso(reft).strftime("%Y%m%d%H%M") for reft in ref_times]
        return instance_ids
    return []


def get_base_url(req: Request = None) -> str:
    """Returns the base url of this service"""

    base_url_from_request = (
        f"{req.url.scheme}://{req.url.hostname}{(':'+str(req.url.port)) if req.url.port else ''}"
        if req
        else None
    )
    base_url = (
        os.getenv("EXTERNALADDRESS", base_url_from_request) or "http://localhost:8080"
    )

    return base_url.strip("/")


OWSLIB_DUMMY_URL = "http://localhost:8000"


def init_edr_collections(adaguc_dataset_dir: str = os.environ["ADAGUC_DATASET_DIR"]):
    """
    Return all possible OGCAPI EDR datasets, based on the dataset directory
    """
    dataset_files = sorted(
        [
            f
            for f in os.listdir(adaguc_dataset_dir)
            if os.path.isfile(os.path.join(adaguc_dataset_dir, f))
            and f.endswith(".xml")
        ]
    )

    edr_collections = {}
    for dataset_file in dataset_files:
        dataset = dataset_file.replace(".xml", "")
        try:
            tree = parse(os.path.join(adaguc_dataset_dir, dataset_file))
            root = tree.getroot()
            for ogcapi_edr in root.iter("OgcApiEdr"):
                for edr_collection in ogcapi_edr.iter("EdrCollection"):
                    edr_params = []
                    for edr_parameter in edr_collection.iter("EdrParameter"):
                        if (
                            "name" in edr_parameter.attrib
                            and "unit" in edr_parameter.attrib
                        ):
                            name = edr_parameter.attrib.get("name")
                            edr_param = {
                                "name": name,
                                "parameter_label": name,
                                "observed_property_label": name,
                                "description": name,
                                "standard_name": None,
                                "unit": "-",
                            }
                            # Try to take the standard name from the configuration
                            if "standard_name" in edr_parameter.attrib:
                                edr_param["standard_name"] = edr_parameter.attrib.get(
                                    "standard_name"
                                )
                                # If there is a standard name, set the label to the same as fallback
                                edr_param["observed_property_label"] = (
                                    edr_parameter.attrib.get("standard_name")
                                )

                            # Try to take the observed_property_label from the configuration
                            if "observed_property_label" in edr_parameter.attrib:
                                edr_param["observed_property_label"] = (
                                    edr_parameter.attrib.get("observed_property_label")
                                )
                            # Try to take the parameter_label from the Layer configuration Title
                            if "parameter_label" in edr_parameter.attrib:
                                edr_param["parameter_label"] = edr_parameter.attrib.get(
                                    "parameter_label"
                                )
                            # Try to take the parameter_label from the Layer configuration Title
                            if "unit" in edr_parameter.attrib:
                                edr_param["unit"] = edr_parameter.attrib.get("unit")

                            edr_params.append(edr_param)

                        else:
                            logger.warning(
                                "In dataset %s, skipping parameter %s: has no name or units configured",
                                dataset_file,
                                edr_parameter,
                            )

                    if len(edr_params) == 0:
                        logger.warning(
                            "Skipping dataset %s: has no EdrParameter configured",
                            dataset_file,
                        )
                    else:
                        collection_url = (
                            get_base_url()
                            + f"/edr/collections/{edr_collection.attrib.get('name')}"
                        )
                        edr_collections[edr_collection.attrib.get("name")] = {
                            "dataset": dataset,
                            "name": edr_collection.attrib.get("name"),
                            "service": f"{OWSLIB_DUMMY_URL}/wms",
                            "parameters": edr_params,
                            "vertical_name": edr_collection.attrib.get("vertical_name"),
                            "base_url": collection_url,
                            "time_interval": edr_collection.attrib.get("time_interval"),
                        }
        except ParseError:
            pass
    return edr_collections


# The edr_collections information is cached locally for a maximum of 2 minutes
# It will be refreshed if older than 2 minutes
edr_cache = TTLCache(maxsize=100, ttl=120)  # TODO: Redis?


@cached(cache=edr_cache)
def get_edr_collections():
    """Returns all EDR collections"""
    return init_edr_collections()


def instance_to_iso(instance_dt: str):
    """
    Converts EDR instance time in format %Y%m%d%H%M into iso8601 time string.
    """
    parsed_dt = parse_instance_time(instance_dt)
    if parsed_dt:
        return parsed_dt.strftime("%Y-%m-%dT%H:%M:%SZ")
    return None


def parse_instance_time(dts: str) -> datetime:
    """
    Converts EDR instance time in format %Y%m%d%H%M into datetime.
    """
    parsed_dt = None
    try:
        parsed_dt = datetime.strptime(dts, "%Y%m%d%H%M").replace(tzinfo=timezone.utc)
    except ValueError as exc:
        logger.error("err: %s %s", dts, exc)
    return parsed_dt


def get_ttl_from_adaguc_headers(headers):
    """Derives an age value from the max-age/age cache headers that ADAGUC executable returns"""
    max_age = None
    age = None
    for hdr in headers:
        hdr_terms = hdr.split(":")
        if hdr_terms[0].lower() == "cache-control":
            for cache_control_terms in hdr_terms[1].split(","):
                terms = cache_control_terms.split("=")
                if terms[0].lower() == "max-age":
                    max_age = int(terms[1])
        elif hdr_terms[0].lower() == "age":
            age = int(hdr_terms[1])
    if max_age and age:
        return max_age - age
    elif max_age:
        return max_age
    return None


def generate_max_age(ttl):
    """
    Return max-age header for ttl value
    """
    if ttl >= 0:
        return f"max-age={ttl}"
    return "max-age=0"
