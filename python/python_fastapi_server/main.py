import logging
import os
import time
from urllib.parse import urlsplit

import uvicorn
from brotli_asgi import BrotliMiddleware
from fastapi import FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware

from configure_logging import configure_logging
from routers.autowms import autowms_router
from routers.healthcheck import health_check_router
from routers.middleware import FixSchemeMiddleware
from routers.ogcapi import ogcApiApp
from routers.opendap import opendapRouter
from routers.wmswcs import testadaguc, wmsWcsRouter

configure_logging(logging)
logger = logging.getLogger(__name__)

app = FastAPI()


@app.middleware("http")
async def add_process_time_header(request: Request, call_next):
    start_time = time.time()
    response = await call_next(request)
    process_time = time.time() - start_time
    response.headers["X-Process-Time"] = str(process_time)
    return response


@app.middleware("http")
async def add_hsts_header(request: Request, call_next):
    response = await call_next(request)
    if "EXTERNALADDRESS" in os.environ:
        external_address = os.environ["EXTERNALADDRESS"]
        scheme = urlsplit(external_address).scheme
        if scheme == "https":
            response.headers[
                "Strict-Transport-Security"] = "max-age=31536000; includeSubDomains"
            response.headers["X-Content-Type-Options"] = "nosniff"
            response.headers["Content-Security-Policy"] = "default-src 'self'"

    return response


app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

app.add_middleware(BrotliMiddleware, gzip_fallback=True)

if "EXTERNALADDRESS" in os.environ:
    app.add_middleware(FixSchemeMiddleware)


@app.get("/")
async def root():
    return {
        "message":
        "ADAGUC server base URL, use /wms, /wcs, /autowms, /adagucopendap or /ogcapi"
    }


app.mount("/ogcapi", ogcApiApp)

app.include_router(health_check_router)
app.include_router(wmsWcsRouter)
app.include_router(autowms_router)
app.include_router(opendapRouter)

if __name__ == "__main__":
    testadaguc()
    uvicorn.run(app="main:app", host="0.0.0.0", port=8080, reload=True)