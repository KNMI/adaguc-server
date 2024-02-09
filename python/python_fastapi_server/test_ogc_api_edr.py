import json
import logging
import os

import pytest
import pytest_asyncio
from adaguc.AdagucTestTools import AdagucTestTools
from fastapi.testclient import TestClient
from httpx import AsyncClient
import asyncio

from main import app

logger = logging.getLogger(__name__)


def set_environ():
    os.environ["ADAGUC_CONFIG"] = os.path.join(
        os.environ["ADAGUC_PATH"], "data", "config", "adaguc.dataset.xml"
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

@pytest.fixture(name="client")
def fixture_client() -> TestClient:
    # Initialize adaguc-server
    set_environ()
    setup_test_data()
    yield TestClient(app)

@pytest.mark.asyncio
async def test_root(client):
    resp = client.get("/edr/")
    root_info = resp.json()
    print("resp:", resp, json.dumps(root_info, indent=2))
    assert root_info["description"] == "EDR service for ADAGUC datasets"
    assert len(root_info["links"]) >= 4

@pytest.mark.asyncio
async def test_collections(client):
    resp = client.get("/edr/collections")
    colls = resp.json()
    assert len(colls["collections"]) == 1
    print(json.dumps(colls["collections"][0], indent=2))
    coll_5d = colls["collections"][0]
    assert coll_5d.get("id") == "data_5d"
    assert all(
        ext_name in coll_5d["extent"]
        for ext_name in ("spatial", "temporal", "vertical", "custom")
    )  # TODO 'custom'
    assert [ext_name for ext_name in coll_5d["extent"]] == [
        "spatial",
        "temporal",
        "vertical",
        "custom",
    ]  # TODO 'custom'
    assert (
        coll_5d["extent"]["temporal"]["values"][0]
        == "R6/2017-01-01 00:00:00+00:00/PT5M"
    )

    assert "position" in coll_5d["data_queries"]

@pytest.mark.asyncio
async def test_coll_5d_position(client):
    resp = client.get("/edr/collections/data_5d/position?coords=POINT(5.2 50.0)&parameter-name=data")
    print(resp.json())