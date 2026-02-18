"""Main file where FastAPI is defined and started"""

import logging
import os
import time
from urllib.parse import urlsplit
import uvicorn
from brotli_asgi import BrotliMiddleware
from fastapi import FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware
from asgi_logger import AccessLoggerMiddleware

from routers.autowms import autowms_router
from routers.edr import edrApiApp
from routers.healthcheck import health_check_router
from routers.middleware import FixSchemeMiddleware
from routers.ogcapi import ogcApiApp
from routers.opendap import opendapRouter
from routers.wmswcs import testadaguc, wmsWcsRouter
from routers.caching_middleware import CachingMiddleware
from configure_logging import configure_logging

configure_logging(logging)

logger = logging.getLogger(__name__)


app = FastAPI(redirect_slashes=False)

# Set uvicorn access log format using middleware
ACCESS_LOG_FORMAT = 'accesslog %(h)s ; %(t)s ; %(H)s ; %(m)s ; %(U)s ; %(q)s ; %(s)s ; %(M)s"'
logging.getLogger("uvicorn.access").handlers.clear()
app.add_middleware(AccessLoggerMiddleware, format=ACCESS_LOG_FORMAT)
logging.getLogger("access").propagate = False


@app.middleware("http")
async def add_hsts_header(request: Request, call_next):
    """Middleware to HTTP Strict Transport Security (HSTS) header"""
    response = await call_next(request)
    if "EXTERNALADDRESS" in os.environ:
        external_address = os.environ["EXTERNALADDRESS"]
        scheme = urlsplit(external_address).scheme
        if scheme == "https":
            response.headers["Strict-Transport-Security"] = "max-age=31536000; includeSubDomains"
            response.headers["X-Content-Type-Options"] = "nosniff"
            response.headers["Content-Security-Policy"] = "default-src 'self'"

    return response


if "ADAGUC_REDIS" in os.environ:
    app.add_middleware(CachingMiddleware)

if "EXTERNALADDRESS" in os.environ:
    app.add_middleware(FixSchemeMiddleware)


@app.middleware("http")
async def add_process_time_header(request: Request, call_next):
    """Middle ware to at X-Process-Time to each request"""
    start_time = time.time()
    response = await call_next(request)
    # Log the X-Trace-Timings so we can monitor timings of the server
    if "X-Trace-Timings" in response.headers:
        logger.log(35, str(request.query_params) + response.headers["X-Trace-Timings"])
    process_time = time.time() - start_time
    response.headers["X-Process-Time"] = str(process_time)
    return response


app.add_middleware(BrotliMiddleware, gzip_fallback=True)

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.get("/")
async def root():
    """Root page"""
    return {"message": "ADAGUC server base URL, use /wms, /wcs, /autowms, /adagucopendap or /ogcapi"}


app.mount("/ogcapi", ogcApiApp)
app.mount("/edr", edrApiApp)

app.include_router(health_check_router)
app.include_router(wmsWcsRouter)
app.include_router(autowms_router)
app.include_router(opendapRouter)

if __name__ == "__main__":
    testadaguc()
    uvicorn.run(app="main:app", host="0.0.0.0", port=8080, reload=True)
