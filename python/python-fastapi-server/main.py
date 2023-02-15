import os

from fastapi import Depends, FastAPI, Request
import uvicorn

from fastapi.middleware.cors import CORSMiddleware
from routers.healthcheck import healthCheckRouter
from routers.wmswcs import wmsWcsRouter, testadaguc
from routers.autowms import autoWmsRouter
from routers.opendap import opendapRouter
from routers.ogcapi import ogcApiApp
from routers.edr import edrApiApp
from routers.maps import create_maps_routes

from routers.middleware import FixSchemeMiddleware
import time

import logging
from configureLogging import configureLogging

configureLogging(logging)
logger = logging.getLogger(__name__)

# from adaguc import routeAdagucServer

app = FastAPI()


@app.middleware("http")
async def add_process_time_header(request: Request, call_next):
    start_time = time.time()
    response = await call_next(request)
    process_time = time.time() - start_time
    response.headers["X-Process-Time"] = str(process_time)
    return response


app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)
if "EXTERNALADDRESS" in os.environ:
    app.add_middleware(FixSchemeMiddleware)


@app.get("/")
async def root():
    return {"message": "ADAGUC server base URL, use /wms, /wcs, /autowms or /ogcapi"}


app.mount("/ogcapi", ogcApiApp)
app.mount("/edr", edrApiApp)
# app.mount("/maps", mapsApiApp)
create_maps_routes(app, ["RADAR", "HARM_N25"])

app.include_router(healthCheckRouter)
app.include_router(wmsWcsRouter)
app.include_router(autoWmsRouter)
app.include_router(opendapRouter)

if __name__ == "__main__":
    testadaguc()
    uvicorn.run(app="main:app", host="0.0.0.0", reload=True)
