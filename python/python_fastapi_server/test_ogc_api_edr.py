import json
import logging
import os

import pytest
from adaguc.AdagucTestTools import AdagucTestTools
from fastapi.testclient import TestClient

from python_fastapi_server.main import app

logger = logging.getLogger(__name__)


def set_environ():
    os.environ["ADAGUC_CONFIG"] = os.path.join(
        os.environ["ADAGUC_PATH"], "data", "config", "adaguc.dataset.xml"
    )


def setup_test_data():
    print("About to ingest data")
    AdagucTestTools().cleanTempDir()
    AdagucTestTools().cleanPostgres()
    for service in (
        "netcdf_5d.xml",
        "dataset_a.xml",
        "adaguc.tests.arcus_uwcw.xml",
        "testcollection.xml",
    ):
        status, _, _ = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                os.environ["ADAGUC_CONFIG"] + "," + service,
            ],
            isCGI=False,
            showLogOnError=False,
            showLog=False,
        )
        print(
            "ingesting: ", os.environ["ADAGUC_CONFIG"] + "," + service + " =>", status
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
    print("resp:", resp, json.dumps(root_info, indent=2))
    print()
    assert root_info["description"] == "EDR service for ADAGUC datasets"
    assert len(root_info["links"]) >= 4


def test_collections(client: TestClient):
    resp = client.get("/edr/collections")
    print(
        ">>>> testcollections",
        [coll["id"] for coll in resp.json()["collections"]],
        resp.status_code,
    )
    colls = resp.json()
    print("IDS:", [c["id"] for c in colls["collections"]])
    assert len(colls["collections"]) == 3

    first_collection = colls["collections"][0]
    assert first_collection.get("id") == "adaguc.tests.arcus_uwcw.hagl_member"

    coll_5d = colls["collections"][1]
    assert coll_5d.get("id") == "netcdf_5d.data_5d"
    assert all(
        ext_name in coll_5d["extent"]
        for ext_name in ("spatial", "temporal", "vertical", "custom")
    )  # TODO 'custom'
    assert list(coll_5d["extent"]) == [
        "spatial",
        "temporal",
        "vertical",
        "custom",
    ]  # TODO 'custom'
    assert coll_5d["extent"]["temporal"]["values"][0] == "R6/2017-01-01T00:00Z/PT5M"

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
            "symbol": {"value": "km", "type": "http://www.opengis.net/def/uom/UCUM"}
        },
        "observedProperty": {
            "id": "https://vocab.nerc.ac.uk/standard_name/data",
            "label": "data",
        },
    }

    assert "data_extra_metadata" in parameter_names

    data_extra_metadata = parameter_names["data_extra_metadata"]

    assert data_extra_metadata == {
        "type": "Parameter",
        "id": "data_extra_metadata",
        "label": "data",
        "unit": {
            "symbol": {
                "value": "km",
                "type": "http://www.opengis.net/def/uom/UCUM",
            }
        },
        "observedProperty": {
            "id": "https://vocab.nerc.ac.uk/standard_name/data",
            "label": "data",
        },
    }


def qest_coll_5d_position(client: TestClient):
    resp = client.get(
        "/edr/collections/data_5d/position?coords=POINT(5.2 50.0)&parameter-name=data"
    )
    print(resp.json())


def qest_coll_multi_dim_position(client: TestClient):
    resp = client.get(
        "/edr/collections/testcollection/instances/2024060100/position?coords=POINT(5.2 52.0)&datetime=2024-06-01T01:00:00Z&parameter-name=testdata"
    )
    assert resp.status_code, 200
    covjson = resp.json()

    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["axes"]["z"]["values"] == [40]
    assert covjson["domain"]["axes"]["t"]["values"] == ["2024-06-01T01:00:00Z"]
    assert covjson["ranges"]["testdata"]["values"] == [300000]

    resp = client.get(
        "/edr/collections/testcollection/instances/2024060100/position?coords=POINT(5.2 52.0)&datetime=2024-06-01T01:00:00Z&parameter-name=testdata&z=10,20,30,40"
    )
    assert resp.status_code, 200
    covjson = resp.json()
    assert covjson["type"] == "Coverage"
    print()
    assert covjson["domain"]["axes"]["z"]["values"] == [10, 20, 30, 40]
    assert covjson["domain"]["axes"]["t"]["values"] == ["2024-06-01T01:00:00Z"]
    assert covjson["ranges"]["testdata"]["values"] == [0, 100000, 200000, 300000]

    resp = client.get(
        "/edr/collections/testcollection/instances/2024060100/position?coords=POINT(5.2 52.0)&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata&z=*"
    )
    assert resp.status_code, 200
    covjson = resp.json()

    assert covjson["domain"]["axes"]["z"]["values"] == [10, 20, 30, 40]
    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["axes"]["t"]["values"] == [
        "2024-06-01T01:00:00Z",
        "2024-06-01T02:00:00Z",
        "2024-06-01T03:00:00Z",
        "2024-06-01T04:00:00Z",
    ]
    assert covjson["ranges"]["testdata"]["values"] == [
        0,
        10000,
        20000,
        30000,
        100000,
        110000,
        120000,
        130000,
        200000,
        210000,
        220000,
        230000,
        300000,
        310000,
        320000,
        330000,
    ]

    # Should handle querying multiple heights separated by comma
    resp = client.get(
        "/edr/collections/testcollection/instances/2024060100/position?coords=POINT(5.2 52.0)&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata&z=10,20"
    )
    assert resp.status_code, 200
    covjson = resp.json()

    print("COVJSON:", covjson)
    print("AXES:", covjson["domain"]["axes"].keys())
    assert covjson["domain"]["axes"]["z"]["values"] == [10, 20]
    assert covjson["ranges"]["testdata"]["shape"] == [2, 4]
    assert covjson["ranges"]["testdata"]["values"] == [
        0.0,
        10000.0,
        20000.0,
        30000.0,
        100000.0,
        110000.0,
        120000.0,
        130000.0,
    ]

    # Should handle querying multiple heights separated by comma, combined with querying ranges
    resp = client.get(
        "/edr/collections/testcollection/instances/2024060100/position?coords=POINT(5.2 52.0)&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata&z=10,30/40"
    )
    assert resp.status_code, 200
    covjson = resp.json()

    assert covjson["domain"]["axes"]["z"]["values"] == [10, 30, 40]
    assert covjson["ranges"]["testdata"]["shape"] == [3, 4]
    assert covjson["ranges"]["testdata"]["values"] == [
        0.0,
        10000.0,
        20000.0,
        30000.0,
        200000.0,
        210000.0,
        220000.0,
        230000.0,
        300000.0,
        310000.0,
        320000.0,
        330000.0,
    ]


def qest_coll_multi_dim_cube(client: TestClient):
    resp = client.get("/edr/collections")
    assert resp.status_code, 200
    covjson = resp.json()
    print("RESP1:", covjson)
    resp = client.get(
        "/edr/collections/testcollection/instances/2024060100/cube?bbox=5.5,52.5,6.5,53.5&datetime=2024-06-01T01:00:00Z&parameter-name=testdata"
    )
    assert resp.status_code, 200
    covjson = resp.json()
    print("RESP:", covjson)

    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["axes"]["z"]["values"] == [40]
    assert covjson["domain"]["axes"]["t"]["values"] == ["2024-06-01T01:00:00Z"]
    assert covjson["ranges"]["testdata"]["values"] == [300101]

    # Without instance, should use the latest instance (same as above)
    resp = client.get(
        "/edr/collections/testcollection/cube?bbox=5.5,52.5,6.5,53.5&datetime=2024-06-01T01:00:00Z&parameter-name=testdata"
    )
    assert resp.status_code, 200
    covjson = resp.json()
    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["axes"]["z"]["values"] == [40]
    assert covjson["domain"]["axes"]["t"]["values"] == ["2024-06-01T01:00:00Z"]
    assert covjson["ranges"]["testdata"]["values"] == [300101]

    # Without instance multiple timesteps, should use the latest instance
    resp = client.get(
        "/edr/collections/testcollection/cube?bbox=5.5,52.5,6.5,53.5&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata"
    )
    assert resp.status_code, 200
    covjson = resp.json()
    print("C:", covjson)
    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["axes"]["z"]["values"] == [40]
    assert covjson["domain"]["axes"]["t"]["values"] == [
        "2024-06-01T01:00:00Z",
        "2024-06-01T02:00:00Z",
        "2024-06-01T03:00:00Z",
        "2024-06-01T04:00:00Z",
    ]
    assert covjson["ranges"]["testdata"]["values"] == [300101, 310101, 320101, 330101]

    # Layer testdata2
    # Without instance multiple timesteps, should use the latest instance
    resp = client.get(
        "/edr/collections/testcollection/cube?bbox=5.5,52.5,6.5,53.5&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata2"
    )
    assert resp.status_code, 200
    covjson = resp.json()
    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["axes"]["z"]["values"] == [30]
    assert covjson["domain"]["axes"]["t"]["values"] == [
        "2024-06-01T01:00:00Z",
        "2024-06-01T02:00:00Z",
        "2024-06-01T03:00:00Z",
        "2024-06-01T04:00:00Z",
    ]
    assert covjson["ranges"]["testdata2"]["values"] == [
        -200101,
        -210101,
        -220101,
        -230101,
    ]

    # Layers testdata,testdata2
    # Without instance multiple timesteps, should use the latest instance
    resp = client.get(
        "/edr/collections/testcollection/cube?bbox=5.5,52.5,7.5,53.5&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata,testdata2"
    )
    assert resp.status_code, 200
    covjson = resp.json()
    assert covjson["type"] == "CoverageCollection"

    assert covjson["coverages"][0]["domain"]["axes"]["z"]["values"] == [40]
    assert covjson["coverages"][0]["domain"]["axes"]["t"]["values"] == [
        "2024-06-01T01:00:00Z",
        "2024-06-01T02:00:00Z",
        "2024-06-01T03:00:00Z",
        "2024-06-01T04:00:00Z",
    ]
    assert covjson["coverages"][0]["ranges"]["testdata"]["values"] == [
        300101,
        300201,
        310101,
        310201,
        320101,
        320201,
        330101,
        330201,
    ]

    # Layers testdata,testdata2
    # Without instance multiple timesteps, should use the latest instance
    resp = client.get(
        "/edr/collections/testcollection/cube?bbox=5.5,52.5,7.5,53.5&datetime=2024-06-01T01:00:00Z/2024-06-01T02:00:00Z&parameter-name=testdata,testdata2&z=*"
    )
    assert resp.status_code, 200
    covjson = resp.json()
    assert covjson["type"] == "CoverageCollection"

    assert covjson["coverages"][0]["domain"]["axes"]["z"]["values"] == [10, 20, 30, 40]
    assert covjson["coverages"][0]["domain"]["axes"]["t"]["values"] == [
        "2024-06-01T01:00:00Z",
        "2024-06-01T02:00:00Z",
    ]
    assert covjson["coverages"][0]["ranges"]["testdata"]["values"] == [
        101.0,
        201.0,
        100101.0,
        100201.0,
        200101.0,
        200201.0,
        300101.0,
        300201.0,
        10101.0,
        10201.0,
        110101.0,
        110201.0,
        210101.0,
        210201.0,
        310101.0,
        310201.0,
    ]

    assert covjson["coverages"][1]["domain"]["axes"]["z"]["values"] == [10, 20, 30]
    assert covjson["coverages"][1]["ranges"]["testdata2"]["values"] == [
        -101.0,
        -201.0,
        -100101.0,
        -100201.0,
        -200101.0,
        -200201.0,
        -10101.0,
        -10201.0,
        -110101.0,
        -110201.0,
        -210101.0,
        -210201.0,
    ]
