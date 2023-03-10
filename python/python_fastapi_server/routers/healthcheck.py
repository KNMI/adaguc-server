"""handleRouteHealthCheck"""
from fastapi import APIRouter

from .setup_adaguc import setup_adaguc

health_check_router = APIRouter(responses={404: {"description": "Not found"}})


@health_check_router.get("/healthcheck")
async def handle_route_healthcheck():
    """handleRouteHealthCheck"""
    setup_adaguc(False)
    response = {"response": "OK"}
    return response
