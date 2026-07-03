"""
Range headers are currently not supported.
"""

import os
import re
from importlib import reload
from xml.etree import ElementTree
from fastapi.testclient import TestClient
from fastapi import Response
import main


def get_testclient(environment=None):
    """Return a fresh testclient with specific environment settings"""

    # Default environment
    os.environ["EXTERNALADDRESS"] = ""
    os.environ["ADAGUC_TRUSTED_HOSTS"] = ""
    os.environ["ADAGUC_TRUSTED_PROXIES"] = ""
    os.environ["ADAGUC_CONFIG"] = os.path.join(os.environ["ADAGUC_PATH"], "data", "config", "adaguc.dataset.xml")

    # Configured environment
    if environment is not None:
        for item, value in environment.items():
            os.environ[item] = value

    # Force reload of the app, so environment variables are propagated to the middlewares
    reload(main)
    client = TestClient(
        main.app,
        client=("adaguc-testclient", 50000),
    )
    client.base_url = "http://default-test-server"
    client.client = {}
    client.headers = {}
    return client


def test_range_requests_blocked():
    """
    Test that range header request are really blocked
    """
    client = get_testclient(
        environment={
            "EXTERNALADDRESS": "",
            "ADAGUC_TRUSTED_HOSTS": "",
            "ADAGUC_TRUSTED_PROXIES": "*",  # default
        }
    )
    response = client.get(
        "/adaguc-server?service=WMS&request=getCapabilities",
        headers={
            "Host": "my-host-to-be-ignored",
            "Range": "bytes=0-0",
        },
    )
    assert response.status_code == 400
