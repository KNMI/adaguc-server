import json
import logging
import os
from httpx import AsyncClient

import pytest
from adaguc.AdagucTestTools import AdagucTestTools
from fastapi.testclient import TestClient

import pytest_asyncio
from main import app

logger = logging.getLogger(__name__)


def set_environ():
    os.environ["ADAGUC_CONFIG"] = os.path.join(
        os.environ["ADAGUC_PATH"], "data", "config", "adaguc.ogcfeatures.xml"
    )


def setup_test_data():
    AdagucTestTools().cleanTempDir()
    for service in ["netcdf_5d.xml", "dataset_a.xml"]:
        _status, _data, _headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                os.environ["ADAGUC_CONFIG"] + "," + service,
            ],
            isCGI=False,
            showLogOnError=False,
            showLog=True,
        )


@pytest_asyncio.fixture(name="clientdata")
async def clientdata():
    set_environ()
    setup_test_data()

@pytest.mark.asyncio()
async def test_root(clientdata):
    async with AsyncClient(app=app, base_url="http://test") as client:
        resp = await client.get(
            "/adaguc-server?dataset=netcdf_5d&request=getcapabilities&service=wms&version=1.3.0"
        )

        resp = await client.get("/ogcapi/")
        assert resp.json()["description"] == "ADAGUC OGCAPI-Features server"

@pytest.mark.asyncio()
async def test_collections(clientdata):
    async with AsyncClient(app=app, base_url="http://test") as client:
        resp = await client.get("/ogcapi/collections")
        colls = resp.json()
        print(json.dumps(colls["collections"][1], indent=2))
        assert len(colls["collections"]) == 2
