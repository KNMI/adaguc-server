import os
from fastapi import APIRouter, Request, Response
import logging

logger = logging.getLogger(__name__)


def get_externaladdress() -> str | None:
    if (external_address := os.environ.get("EXTERNALADDRESS")) is not None:
        if len(external_address) > 5:
            if not external_address.endswith("/"):
                external_address = external_address + "/"
            return external_address
    return None


def get_base_url(req: Request) -> str:
    if (external_address := get_externaladdress()) is not None:
        return external_address

    base_url = str(req.base_url)
    return base_url
