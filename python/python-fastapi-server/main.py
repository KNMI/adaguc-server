from wsgiref.simple_server import WSGIRequestHandler
from fastapi import Depends, FastAPI, Request
from fastapi.responses import RedirectResponse
from flask import Flask
import uvicorn

from fastapi.middleware.cors import CORSMiddleware
from fastapi.middleware.wsgi import WSGIMiddleware
from routers.healthcheck import healthCheckRouter
from routers.wmswcs import wmsWcsRouter
from routers.autowms import autoWmsRouter
from routers.opendap import opendapRouter
from routers.ogcapi.routeOGCApi import routeOGCApi, init_views
from routers.cacher import cacher, init_cache
from routers.ogcapi2 import ogcApiApp

import time

# from adaguc import routeAdagucServer

app = FastAPI()


# def create_app():
#     """Create the Flask/Gunicorn applicaiton"""
#     _app = Flask(__name__)

#     init_cache(_app)

#     _app.register_blueprint(routeOGCApi, url_prefix="/ogcapi")
#     with _app.app_context():
#         init_views()
#     return _app


# flask_app = create_app()
# WSGIRequestHandler.protocol_version = "HTTP/1.1"


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
# app.include_router(ogcApiRouter)

# app.mount("", WSGIMiddleware(flask_app))

if __name__ == "__main__":
    uvicorn.run(app="main:app", host="0.0.0.0", reload=True)
