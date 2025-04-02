import logging
import os

import pytest
from adaguc.AdagucTestTools import AdagucTestTools
from fastapi.testclient import TestClient

from main import app

logger = logging.getLogger(__name__)


def set_environ():
    os.environ["ADAGUC_CONFIG"] = os.path.join(
        os.environ["ADAGUC_PATH"], "data", "config", "adaguc.ogcfeatures.xml"
    )


def setup_test_data():
    print("About to ingest data")
    AdagucTestTools().cleanTempDir()
    AdagucTestTools().cleanPostgres()
    for service in ("netcdf_5d.xml", "dataset_a.xml"):
        _, _, _ = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                os.environ["ADAGUC_CONFIG"] + "," + service,
            ],
            isCGI=False,
            showLogOnError=False,
            showLog=False,
        )


# Call this once during the whole pytest session
@pytest.fixture(scope="session", autouse=True)
def ingest_data():
    set_environ()
    setup_test_data()


@pytest.fixture(name="client")
def fixture_client():
    # Initialize adaguc-server
    yield TestClient(app)


def test_root(client: TestClient):
    resp = client.get(
        "/adaguc-server?dataset=netcdf_5d&request=getcapabilities&service=wms&version=1.3.0"
    )
    # print("getcap:", resp.text)

    resp = client.get("/ogcapi/")
    # print("resp:", resp, resp.json())
    assert resp.json()["description"] == "ADAGUC OGCAPI-Features server"


def test_collections(client: TestClient):
    resp = client.get("/ogcapi/collections")
    colls = resp.json()
    # print(json.dumps(colls["collections"][1], indent=2))
    assert len(colls["collections"]) == 2
