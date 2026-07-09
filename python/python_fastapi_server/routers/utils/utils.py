import os
from fastapi import APIRouter, Request, Response
import logging

logger = logging.getLogger(__name__)


def get_base_url(req: Request) -> str:
    if (external_address := os.environ.get("EXTERNALADDRESS")) is not None:
        if not external_address.endswith("/"):
            external_address = external_address + "/"
        logger.info("URL (EXTERNALADDRESS): %s", external_address)
        return external_address

    base_url = str(req.base_url)
    logger.info("URL (CALCULATED): %s", base_url)
    return base_url
