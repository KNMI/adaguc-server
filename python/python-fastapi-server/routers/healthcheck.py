"""handleRouteHealthCheck"""
from .setup_adaguc import setup_adaguc
from fastapi import Request, APIRouter

healthCheckRouter = APIRouter(responses={404: {"description": "Not found"}})


@healthCheckRouter.get("/healthcheck")
async def handleRouteHealthCheck(request: Request):
    """handleRouteHealthCheck"""
    setup_adaguc(False)
    response = {"response": "OK"}
    return response
