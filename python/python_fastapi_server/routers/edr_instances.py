"""python/python_fastapi_server/routers/edr.py
Adaguc-Server OGC EDR implementation

This code uses Adaguc's OGC WMS and WCS endpoints to convert into an EDR service.

Author: Ernst de Vreede, 2023-11-23

KNMI
"""

from edr_pydantic.collections import Collection, Instance, Instances
from edr_pydantic.link import Link
from fastapi import APIRouter, Request, Response

from .edr_utils import (
    generate_max_age,
    get_base_url,
    get_collectioninfo_for_id,
    get_edr_collections,
    get_ref_times_for_coll,
)

router = APIRouter()


@router.get(
    "/collections/{collection_name}/instances",
    response_model=Instances,
    response_model_exclude_none=True,
)
async def rest_get_edr_inst_for_coll(
    collection_name: str, request: Request, response: Response
):
    """
    GET: Returns all available instances for the collection
    """
    instances_url = (
        get_base_url(request) + f"/edr/collections/{collection_name}/instances"
    )

    instances: list[Instance] = []
    edr_collections = get_edr_collections()

    ref_times = await get_ref_times_for_coll(
        edr_collections[collection_name]["dataset"],
        edr_collections[collection_name]["parameters"][0]["name"],
    )
    print("REF:", ref_times)
    links: list[Link] = []
    links.append(Link(href=instances_url, rel="collection"))
    ttl_set = set()
    for instance in list(ref_times):
        instance_links: list[Link] = []
        instance_link = Link(href=f"{instances_url}/{instance}", rel="collection")
        instance_links.append(instance_link)
        instance_info, ttl = await get_collectioninfo_for_id(collection_name, instance)
        if ttl is not None:
            ttl_set.add(ttl)
        instances.append(instance_info)

    instances_data = Instances(instances=instances, links=links)
    if ttl_set:
        response.headers["cache-control"] = generate_max_age(min(ttl_set))
    return instances_data


@router.get(
    "/collections/{collection_name}/instances/{instance}",
    response_model=Collection,
    response_model_exclude_none=True,
)
async def rest_get_collection_info(collection_name: str, instance, response: Response):
    """
    GET  "/collections/{collection_name}/instances/{instance}"
    """
    coll, ttl = await get_collectioninfo_for_id(collection_name, instance)
    if ttl is not None:
        response.headers["cache-control"] = generate_max_age(ttl)
    return coll
