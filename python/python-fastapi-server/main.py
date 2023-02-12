from wsgiref.simple_server import WSGIRequestHandler
from fastapi import Depends, FastAPI, Request
from fastapi.responses import RedirectResponse
import uvicorn

from fastapi.middleware.cors import CORSMiddleware
from fastapi.middleware.wsgi import WSGIMiddleware
from routers.healthcheck import healthCheckRouter
from routers.wmswcs import wmsWcsRouter, testadaguc
from routers.autowms import autoWmsRouter
from routers.opendap import opendapRouter
from routers.ogcapi import ogcApiApp

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


@app.get("/")
async def root():
    return {"message": "Hello Bigger Applications!"}


app.mount("/ogcapi", ogcApiApp)

app.include_router(healthCheckRouter)
app.include_router(wmsWcsRouter)
app.include_router(autoWmsRouter)
app.include_router(opendapRouter)

if __name__ == "__main__":
    testadaguc()
    uvicorn.run(app="main:app", host="0.0.0.0", reload=True)
