import json
import logging
import os

import pytest
from adaguc.AdagucTestTools import AdagucTestTools
from fastapi.testclient import TestClient

from main import app

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
        "adaguc.tests.members.xml",
        "adaguc_ewclocalclimateinfo_test.xml"
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
    resp = client.get("/edr/")
    root_info = resp.json()
    assert root_info["description"] == "EDR service for ADAGUC datasets"
    assert len(root_info["links"]) >= 4


def test_collections(client: TestClient):
    resp = client.get("/edr/collections")
    colls = resp.json()
    assert len(colls["collections"]) == 5
    print(colls["collections"])
    first_collection = colls["collections"][0]
    assert first_collection.get("id") == "adaguc.tests.arcus_uwcw.hagl_member"

    coll_5d = colls["collections"][3]
    assert coll_5d.get("id") == "netcdf_5d.data_5d"
    assert all(
        ext_name in coll_5d["extent"]
        for ext_name in ("spatial", "temporal", "vertical", "custom")
    )
    assert list(coll_5d["extent"]) == [
        "spatial",
        "temporal",
        "vertical",
        "custom",
    ]
    assert coll_5d["extent"]["temporal"]["values"][0] == "R6/2017-01-01T00:00Z/PT5M"

    assert "position" in coll_5d["data_queries"]

    assert "parameter_names" in coll_5d

    parameter_names = coll_5d["parameter_names"]

    assert "data" in parameter_names

    data = parameter_names["data"]

    assert data == {
        "type": "Parameter",
        "id": "data",
        "label": "data (data)",
        "description": "data (data)",
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
        "label": "data extra metadata",
        "description": "data extra metadata",
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


def test_coll_multi_dim_position_single_coverage(client: TestClient):
    # Querying a single datetime and single Z results in a Coverage
    resp = client.get(
        "/edr/collections/testcollection.testcollection/instances/202406010000/position?coords=POINT(5.2 52.0)&datetime=2024-06-01T01:00:00Z&parameter-name=testdata"
    )
    assert resp.status_code, 200
    covjson = resp.json()

    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["axes"]["z"]["values"] == [40]
    assert covjson["domain"]["axes"]["t"]["values"] == ["2024-06-01T01:00:00Z"]
    assert covjson["ranges"]["testdata"]["values"] == [300000]

    # Querying a single datetime and multiple Z results in a Coverage
    resp = client.get(
        "/edr/collections/testcollection.testcollection/instances/202406010000/position?coords=POINT(5.2 52.0)&datetime=2024-06-01T01:00:00Z&parameter-name=testdata&z=10,20,30,40"
    )
    assert resp.status_code, 200
    covjson = resp.json()
    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["axes"]["z"]["values"] == [10, 20, 30, 40]
    assert covjson["domain"]["axes"]["t"]["values"] == ["2024-06-01T01:00:00Z"]
    assert covjson["ranges"]["testdata"]["values"] == [0, 100000, 200000, 300000]


def test_position_domain_types(client: TestClient):
    position_url = "/edr/collections/testcollection.testcollection/instances/202406010000/position?coords=POINT(5.2 52.0)&datetime={datetime}&parameter-name=testdata&z={z}"

    # Single datetime, single z = Coverage with Point
    resp = client.get(position_url.format(datetime="2024-06-01T01:00:00Z", z=10))
    assert resp.status_code, 200
    covjson = resp.json()
    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["domainType"] == "Point"

    # Single datetime, multiple z = Coverage with VerticleProfile
    resp = client.get(position_url.format(datetime="2024-06-01T01:00:00Z", z="10,20"))
    assert resp.status_code, 200
    covjson = resp.json()
    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["domainType"] == "VerticalProfile"

    # Multiple datetime, single z = Coverage with PointSeries
    resp = client.get(
        position_url.format(
            datetime="2024-06-01T01:00:00Z/2024-06-01T04:00:00Z", z="10"
        )
    )
    assert resp.status_code, 200
    covjson = resp.json()
    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["domainType"] == "PointSeries"

    # Multiple datetime, multiple z = CoverageCollection with VerticalProfile
    resp = client.get(
        position_url.format(
            datetime="2024-06-01T01:00:00Z/2024-06-01T04:00:00Z", z="10,20"
        )
    )
    assert resp.status_code, 200
    covjson = resp.json()
    assert covjson["type"] == "CoverageCollection"
    assert len(covjson["coverages"]) == 4
    assert all(
        [c["domain"]["domainType"] == "VerticalProfile" for c in covjson["coverages"]]
    )


def test_cube_domain_types(client: TestClient):
    # Multiple points (through cube) will always result in a Coverage with Grid
    cube_url = "/edr/collections/testcollection.testcollection/instances/202406010000/cube?bbox=5.5,52.5,6.5,53.5&datetime={datetime}&parameter-name=testdata&z={z}"

    # Single datetime and single z
    resp = client.get(cube_url.format(datetime="2024-06-01T01:00:00Z", z="10"))
    assert resp.status_code, 200
    covjson = resp.json()
    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["domainType"] == "Grid"

    # Single datetime and multiple z
    resp = client.get(cube_url.format(datetime="2024-06-01T01:00:00Z", z="10,20"))
    assert resp.status_code, 200
    covjson = resp.json()
    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["domainType"] == "Grid"

    # Multiple datetimes and multiple z
    resp = client.get(
        cube_url.format(datetime="2024-06-01T01:00:00Z/2024-06-01T04:00:00Z", z="10,20")
    )
    assert resp.status_code, 200
    covjson = resp.json()
    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["domainType"] == "Grid"


def test_coll_multi_dim_position_coverage_collection_all_z(client: TestClient):
    # Querying a multiple datetime and all z results in a CoverageCollection
    resp = client.get(
        "/edr/collections/testcollection.testcollection/instances/202406010000/position?coords=POINT(5.2 52.0)&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata&z=*"
    )
    assert resp.status_code, 200
    covjson = resp.json()

    assert covjson["type"] == "CoverageCollection"
    assert len(covjson["coverages"]) == 4

    # All coverages should have same z
    assert all(
        [
            c["domain"]["axes"]["z"]["values"] == [10, 20, 30, 40]
            for c in covjson["coverages"]
        ]
    )

    # All coverages should have same shape
    assert all(
        [c["ranges"]["testdata"]["shape"] == [4, 1] for c in covjson["coverages"]]
    )

    assert [c["domain"]["axes"]["t"]["values"] for c in covjson["coverages"]] == [
        ["2024-06-01T01:00:00Z"],
        ["2024-06-01T02:00:00Z"],
        ["2024-06-01T03:00:00Z"],
        ["2024-06-01T04:00:00Z"],
    ]

    assert covjson["coverages"][0]["ranges"]["testdata"]["values"] == [
        0,
        100000,
        200000,
        300000,
    ]
    assert covjson["coverages"][1]["ranges"]["testdata"]["values"] == [
        10000,
        110000,
        210000,
        310000,
    ]
    assert covjson["coverages"][2]["ranges"]["testdata"]["values"] == [
        20000,
        120000,
        220000,
        320000,
    ]
    assert covjson["coverages"][3]["ranges"]["testdata"]["values"] == [
        30000,
        130000,
        230000,
        330000,
    ]


def test_coll_multi_dim_position_coverage_collection_multiple_z(client: TestClient):
    # Should handle querying multiple heights separated by comma
    resp = client.get(
        "/edr/collections/testcollection.testcollection/instances/202406010000/position?coords=POINT(5.2 52.0)&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata&z=10,20"
    )
    assert resp.status_code, 200
    covjson = resp.json()

    assert covjson["type"] == "CoverageCollection"
    assert len(covjson["coverages"]) == 4

    # All coverages should have same z
    assert all(
        [c["domain"]["axes"]["z"]["values"] == [10, 20] for c in covjson["coverages"]]
    )

    # All coverages should have same shape
    assert all(
        [c["ranges"]["testdata"]["shape"] == [2, 1] for c in covjson["coverages"]]
    )

    assert covjson["coverages"][0]["ranges"]["testdata"]["values"] == [
        0,
        100000,
    ]
    assert covjson["coverages"][1]["ranges"]["testdata"]["values"] == [
        10000,
        110000,
    ]

def test_coll_multi_dim_position_auto_ewc_local_climate_info(client: TestClient):
    # Querying a CoverageCollection with two scenarios (2050Hd and 2050Hn) for ewc local climate info
    resp = client.get(
        "/edr/collections/adaguc_ewclocalclimateinfo_test/position?coords=POINT(5 52)&parameter-name=jaarlijks/tas_mean_2050&datetime=2036-01-01T00:00:00Z/2065-01-01T00:00:00Z&scenario=*"
    )
    assert resp.status_code, 200
    covjson = resp.json()

    assert covjson["type"] == "CoverageCollection"
    assert len(covjson["coverages"]) == 2
    assert covjson["coverages"][0]["type"] == "Coverage"
    assert covjson["coverages"][0]["domain"]["domainType"] == "PointSeries"
    assert len(covjson["coverages"][0]["domain"]["axes"]["t"]["values"]) == 30
    assert covjson["coverages"][0]["domain"]["axes"]["t"]["values"][0] == "2036-01-01T00:00:00Z"
    assert covjson["coverages"][0]["domain"]["axes"]["t"]["values"][29] == "2065-01-01T00:00:00Z"
    assert covjson["coverages"][0]["domain"]["custom:scenario"] == "2050Hd"

    assert len(covjson["coverages"][1]["domain"]["axes"]["t"]["values"]) == 30
    assert covjson["coverages"][1]["domain"]["axes"]["t"]["values"][0] == "2036-01-01T00:00:00Z"
    assert covjson["coverages"][1]["domain"]["axes"]["t"]["values"][29] == "2065-01-01T00:00:00Z"
    assert covjson["coverages"][1]["domain"]["custom:scenario"] == "2050Hn"

    assert covjson["coverages"][0]["ranges"]["jaarlijks/tas_mean_2050"]["shape"] == [30]
    assert covjson["coverages"][1]["ranges"]["jaarlijks/tas_mean_2050"]["shape"] == [30]
    
    assert covjson["coverages"][0]["ranges"]["jaarlijks/tas_mean_2050"]["values"] == [10.90503,12.007289,11.059411,12.062785,11.947086,10.081615,11.765973,11.881777,12.438473,12.412801,11.976863,12.304342,12.01673,11.909211,12.190491,12.779091,12.678858,12.103762,12.038959,10.637293,12.453781,11.861727,11.348157,13.191739,12.320773,12.319611,12.470861,12.96588,12.703981,13.242649]
    
    assert covjson["coverages"][1]["ranges"]["jaarlijks/tas_mean_2050"]["values"] == [10.82554,11.873678,11.01875,11.902385,11.815706,10.020253,11.643116,11.819782,12.285396,12.28452,11.84561,12.195546,11.844776,11.799691,12.05139,12.585763,12.544561,11.989633,11.920113,10.537866,12.319339,11.716161,11.255101,12.989518,12.206244,12.159081,12.357863,12.784953,12.555624,13.065644]



def test_coll_multi_dim_position_coverage_collection_z_range(client: TestClient):
    # Should handle querying multiple heights separated by comma, combined with querying ranges
    resp = client.get(
        "/edr/collections/testcollection.testcollection/instances/202406010000/position?coords=POINT(5.2 52.0)&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata&z=10,30/40"
    )
    assert resp.status_code, 200
    covjson = resp.json()

    # All coverages should have same z
    assert all(
        [
            c["domain"]["axes"]["z"]["values"] == [10, 30, 40]
            for c in covjson["coverages"]
        ]
    )
    # All coverages should have same shape
    assert all(
        [c["ranges"]["testdata"]["shape"] == [3, 1] for c in covjson["coverages"]]
    )

    assert covjson["coverages"][0]["ranges"]["testdata"]["values"] == [
        0,
        200000,
        300000,
    ]
    assert covjson["coverages"][2]["ranges"]["testdata"]["values"] == [
        20000,
        220000,
        320000,
    ]
    assert covjson["coverages"][3]["ranges"]["testdata"]["values"] == [
        30000,
        230000,
        330000,
    ]


def test_coll_multi_dim_cube(client: TestClient):
    resp = client.get("/edr/collections")
    assert resp.status_code, 200
    covjson = resp.json()
    resp = client.get(
        "/edr/collections/testcollection.testcollection/instances/202406010000/cube?bbox=5.5,52.5,6.5,53.5&datetime=2024-06-01T01:00:00Z&parameter-name=testdata"
    )
    assert resp.status_code, 200
    covjson = resp.json()

    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["axes"]["z"]["values"] == [40]
    assert covjson["domain"]["axes"]["t"]["values"] == ["2024-06-01T01:00:00Z"]
    assert covjson["ranges"]["testdata"]["values"] == [300101]

    # Without instance, should use the latest instance (same as above)
    resp = client.get(
        "/edr/collections/testcollection.testcollection/cube?bbox=5.5,52.5,6.5,53.5&datetime=2024-06-01T01:00:00Z&parameter-name=testdata"
    )
    assert resp.status_code, 200
    covjson = resp.json()
    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["axes"]["z"]["values"] == [40]
    assert covjson["domain"]["axes"]["t"]["values"] == ["2024-06-01T01:00:00Z"]
    assert covjson["ranges"]["testdata"]["values"] == [300101]

    # Without instance multiple timesteps, should use the latest instance
    resp = client.get(
        "/edr/collections/testcollection.testcollection/cube?bbox=5.5,52.5,6.5,53.5&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata"
    )
    assert resp.status_code, 200
    covjson = resp.json()
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
        "/edr/collections/testcollection.testcollection/cube?bbox=5.5,52.5,6.5,53.5&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata2"
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
        "/edr/collections/testcollection.testcollection/cube?bbox=5.5,52.5,7.5,53.5&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata,testdata2"
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
        "/edr/collections/testcollection.testcollection/cube?bbox=5.5,52.5,7.5,53.5&datetime=2024-06-01T01:00:00Z/2024-06-01T02:00:00Z&parameter-name=testdata,testdata2&z=*"
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


def test_point_custom_dim(client: TestClient):
    # position call on data which includes a custom dimension, should mention dimension even if we don't query it specifically
    resp = client.get(
        "/edr/collections/adaguc.tests.members.mycollection/instances/202503010000/"
        + "position?coords=POINT(5.0 52.0)&parameter-name=mymemberdata"
    )
    assert resp.status_code, 200
    position_json = resp.json()

    # Should mention the custom dimension inside the "domain" section
    custom_dims = [
        custom_dim
        for custom_dim in position_json["domain"]
        if custom_dim.startswith("custom:")
    ]
    # TODO: point includes custom:reference_time, is bad?
    assert custom_dims == ["custom:member", "custom:reference_time"]

    # We did not query a specific member, it should show the highest
    assert position_json["domain"]["custom:member"] == 50.0


def test_cube_custom_dim(client: TestClient):
    # cube call on data which includes a custom dimension, should mention dimension even if we don't query it specifically
    resp = client.get(
        "/edr/collections/adaguc.tests.members.mycollection/instances/202503010000/"
        + "cube?bbox=4.5,51.5,7.5,54.5&resolution_x=1&resolution_y=1&parameter-name=mymemberdata"
    )
    assert resp.status_code, 200
    cube_json = resp.json()

    # Should mention the custom dimension inside the "domain" section
    custom_dims = [
        custom_dim
        for custom_dim in cube_json["domain"]
        if custom_dim.startswith("custom:")
    ]
    # TODO: cube does not include custom:reference_time, good?
    assert custom_dims == ["custom:member"]

    # We did not query a specific member, it should show the highest
    assert cube_json["domain"]["custom:member"] == 50.0


def test_point_custom_dim_request_all_members(client: TestClient):
    # position call on data which includes a custom dimension, should mention dimension even if we don't query it specifically
    resp = client.get(
        "/edr/collections/adaguc.tests.members.mycollection/instances/202503010000/"
        + "position?coords=POINT(5.0 52.0)&parameter-name=mymemberdata"
        + "&datetime=2025-03-01T00:00:00Z/2025-03-01T03:00:00Z&member=*"
    )
    assert resp.status_code, 200
    position_json = resp.json()

    # We're requesting all members at once, this should be a CoverageCollection
    assert position_json["type"] == "CoverageCollection"
    assert len(position_json["coverages"]) == 50
    assert position_json["coverages"][0]["domain"]["custom:member"] == 1.0
    assert position_json["coverages"][-1]["domain"]["custom:member"] == 50.0

    # We requested 4 times, but since it's a custom dimension, a coverage will have a single member which won't get mentioned explicitly
    data = position_json["coverages"][0]["ranges"]["mymemberdata"]
    assert data["type"] == "NdArray"
    assert data["axisNames"] == ["t"]
    assert data["shape"] == [4]

    data = position_json["coverages"][-1]["ranges"]["mymemberdata"]
    assert data["type"] == "NdArray"
    assert data["axisNames"] == ["t"]
    assert data["shape"] == [4]


def test_cube_custom_dim_request_all_members(client: TestClient):
    # position call on data which includes a custom dimension, should mention dimension even if we don't query it specifically
    resp = client.get(
        "/edr/collections/adaguc.tests.members.mycollection/instances/202503010000/"
        + "cube?bbox=4.5,51.5,7.5,54.5&resolution_x=1&resolution_y=1&parameter-name=mymemberdata"
        + "&datetime=2025-03-01T00:00:00Z/2025-03-01T03:00:00Z&member=*"
    )
    assert resp.status_code, 200
    cube_json = resp.json()

    # We're requesting all members at once, this should be a CoverageCollection
    assert cube_json["type"] == "CoverageCollection"
    assert len(cube_json["coverages"]) == 50
    assert cube_json["coverages"][0]["domain"]["custom:member"] == 1.0
    assert cube_json["coverages"][-1]["domain"]["custom:member"] == 50.0

    # We requested 4 times, and it's a cube, but since it's a custom dimension, a coverage will have a single member which won't get mentioned explicitly
    data = cube_json["coverages"][0]["ranges"]["mymemberdata"]
    assert data["type"] == "NdArray"
    assert data["axisNames"] == ["t", "y", "x"]
    assert data["shape"] == [4, 3, 3]

    data = cube_json["coverages"][-1]["ranges"]["mymemberdata"]
    assert data["type"] == "NdArray"
    assert data["axisNames"] == ["t", "y", "x"]
    assert data["shape"] == [4, 3, 3]


@pytest.mark.parametrize(
    "url, status_code, description",
    [
        (
            "/edr/collections/my_unknown_collection",
            400,
            "Unknown or unconfigured collection my_unknown_collection",
        ),
        (
            "/edr/collections/adaguc.tests.members.mycollection/instances/12345",
            404,
            "Incorrect instance 12345 for collection adaguc.tests.members.mycollection",
        ),
        (
            "/edr/collections/adaguc.tests.members.mycollection/instances/202503010000/position?coords=POINT(5.0 52.0)&parameter-name=myunknown-parameter",
            404,
            "Incorrect parameter myunknown-parameter requested for collection adaguc.tests.members.mycollection",
        ),
        (
            "/edr/collections/adaguc.tests.members.mycollection/instances/202503010000/position?coords=POINT(5.0 52.0)",
            404,
            "Incorrect parameter  requested for collection adaguc.tests.members.mycollection",
        ),
        (
            "/edr/collections/adaguc.tests.members.mycollection/instances/202503010000/position?coords=POINT()&parameter-name=mymemberdata",
            400,
            "Could not parse WKT POINT, received coords=POINT()",
        ),
    ],
    ids=[
        "unknown_collection",
        "incorrect_instance",
        "incorrect_parameter",
        "no_parameter",
        "invalid_point",
    ],
)
def test_edr_exceptions(url, status_code, description, client: TestClient):
    resp = client.get(url)
    assert resp.status_code == status_code
    assert resp.json()["description"] == description
