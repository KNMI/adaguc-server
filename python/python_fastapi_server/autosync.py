"""Main file where FastAPI is defined and started"""

import asyncio
import logging
from apscheduler.schedulers.blocking import BlockingScheduler

from routers.wmswcs import testadaguc
from routers.setup_adaguc import setup_adaguc
from configure_logging import configure_logging

configure_logging(logging)

logger = logging.getLogger(__name__)


def async_autosync_layermetadata():
    """Update layermetadata table in adaguc for GetCapabilities caching"""
    adaguc = setup_adaguc(False)
    logger.info("Calling updateLayerMetadata")
    status, log = asyncio.run(adaguc.updateLayerMetadata())
    if adaguc.isLoggingEnabled():
        logger.info(log)
    else:
        logger.info("Logging for updateLayerMetadata is disabled, status was %d", status)


if __name__ == "__main__":
    """Starts scheduler for updating the metadata table as a service"""
    testadaguc()
    logger.info("=== Starting AsyncIO Blocking Scheduler ===")
    # start scheduler to refresh collections & docs every minute
    scheduler = BlockingScheduler()
    scheduler.add_job(
        async_autosync_layermetadata,
        "cron",
        [],
        minute="*",
        jitter=0,
        max_instances=1,
        coalesce=True,
    )
    try:
        scheduler.start()
    except (KeyboardInterrupt, SystemExit):
        pass
