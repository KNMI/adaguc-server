import json
import logging
import os

import pytest
from adaguc.AdagucTestTools import AdagucTestTools
from fastapi.testclient import TestClient

from main import app

logger = logging.getLogger(__name__)


def set_environ():
    os.environ["ADAGUC_CONFIG"] = os.path.join(os.environ["ADAGUC_PATH"],
                                               "data", "config",
                                               "adaguc.dataset.xml")


def setup_test_data():
    print("About to ingest data")
    AdagucTestTools().cleanTempDir()
    AdagucTestTools().cleanPostgres()
    for service in ["netcdf_5d.xml", "dataset_a.xml", "adaguc.tests.arcus_uwcw.xml"]:
        _status, _data, _headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                os.environ["ADAGUC_CONFIG"] + "," + service,
            ],
            isCGI=False,
            showLogOnError=False,
            showLog=False,
        )


@pytest.fixture(name="client")
def fixture_client():
    # Initialize adaguc-server
    set_environ()
    setup_test_data()
    yield TestClient(app)


def test_root(client: TestClient):
    resp = client.get("/edr/")
    root_info = resp.json()
    #print("resp:", resp, json.dumps(root_info, indent=2))
    #print()
    assert root_info["description"] == "EDR service for ADAGUC datasets"
    assert len(root_info["links"]) >= 4


def test_collections(client: TestClient):
    resp = client.get("/edr/collections")
    colls = resp.json()
    assert len(colls["collections"]) == 2
    #print(json.dumps(colls["collections"][0], indent=2))


    uwcw_ha43ens_nl_2km_hagl = colls["collections"][0]
    assert uwcw_ha43ens_nl_2km_hagl.get("id") == "uwcw_ha43ens_nl_2km_hagl"

    coll_5d = colls["collections"][1]
    assert coll_5d.get("id") == "data_5d"
    assert all(ext_name in coll_5d["extent"]
               for ext_name in ("spatial", "temporal", "vertical",
                                "custom"))  # TODO 'custom'
    assert [ext_name for ext_name in coll_5d["extent"]] == [
        "spatial",
        "temporal",
        "vertical",
        "custom",
    ]  # TODO 'custom'
    assert (coll_5d["extent"]["temporal"]["values"][0] ==
            "R6/2017-01-01T00:00:00Z/PT5M")

    assert "position" in coll_5d["data_queries"]

    assert "parameter_names" in coll_5d

    parameter_names = coll_5d["parameter_names"]

    assert "data" in parameter_names

    data = parameter_names["data"]

    assert data == {
      "type": "Parameter",
      "id": "data",
      "label": "data",
      "unit": {
        "symbol": {
          "value": "unit",
          "type": "http://www.opengis.net/def/uom/UCUM"
        }
      },
      "observedProperty": {
        "id": "data",
        "label": "data"
      }
    }


    assert "data_extra_metadata" in parameter_names

    data_extra_metadata = parameter_names["data_extra_metadata"]

    assert data_extra_metadata == {
      "type": "Parameter",
      "id": "data_extra_metadata",
      "label": "Air temperature, 2 metre",
      "unit": {
        "symbol": {
          "value": "\u00b0C",
          "type": "http://www.opengis.net/def/uom/UCUM"
        }
      },
      "observedProperty": {
        "id": "https://vocab.nerc.ac.uk/standard_name/air_temperature",
        "label": "Air temperature"
      }
    }


def test_coll_5d_position(client: TestClient):
    resp = client.get(
        "/edr/collections/data_5d/position?coords=POINT(5.2 50.0)&parameter-name=data"
    )
    print(resp.json())
