"""Main file where FastAPI is defined and started"""
import asyncio
import logging
import os
import time
from urllib.parse import urlsplit
from apscheduler.schedulers.asyncio import AsyncIOScheduler
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
from routers.setup_adaguc import setup_adaguc
from configure_logging import configure_logging

configure_logging(logging)

logger = logging.getLogger(__name__)

def update_layermetadatatable():
    """Update layermetadata table in adaguc for GetCapabilities caching"""
    adaguc = setup_adaguc(False)
    logger.info("Calling updateLayerMetadata")
    status, log = adaguc.updateLayerMetadata()
    if adaguc.isLoggingEnabled():
        logger.info(log)
    else:
        logger.info("Logging for updateLayerMetadata is disabled, status was %d", status)


async def lifespan(_fastapiapp: FastAPI):
    """Captures FASTAPI Lifespan events to start the AsyncIOScheduler"""

    logger.info("=== Starting AsyncIO Scheduler ===")
    # start scheduler to refresh collections & docs every minute
    scheduler = AsyncIOScheduler(event_loop=asyncio.get_running_loop())
    scheduler.add_job(update_layermetadatatable, "cron", [], second="*", jitter=0, max_instances=1, coalesce=True)
    scheduler.start()

    yield

    logger.info("=== Stopping AsyncIO Scheduler ===")
    scheduler.shutdown()


app = FastAPI(lifespan=lifespan)

# Set uvicorn access log format using middleware
ACCESS_LOG_FORMAT = (
    'accesslog %(h)s ; %(t)s ; %(H)s ; %(m)s ; %(U)s ; %(q)s ; %(s)s ; %(M)s ; "%(a)s"'
)
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
            response.headers["Strict-Transport-Security"] = (
                "max-age=31536000; includeSubDomains")
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
    return {
        "message":
        "ADAGUC server base URL, use /wms, /wcs, /autowms, /adagucopendap or /ogcapi"
    }


app.mount("/ogcapi", ogcApiApp)
app.mount("/edr", edrApiApp)

app.include_router(health_check_router)
app.include_router(wmsWcsRouter)
app.include_router(autowms_router)
app.include_router(opendapRouter)

if __name__ == "__main__":
    testadaguc()
    uvicorn.run(app="main:app", host="0.0.0.0", port=8080, reload=True)
