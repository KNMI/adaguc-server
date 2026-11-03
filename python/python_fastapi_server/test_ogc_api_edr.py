import json
import logging
import os
from pathlib import Path
from urllib.parse import urlsplit

import pytest
from adaguc.AdagucTestTools import AdagucTestTools
from fastapi.testclient import TestClient

from main import app

logger = logging.getLogger(__name__)

testresultspath = f"testresults/{Path(__file__).stem}/"
expectedoutputspath = f"expectedoutputs/{Path(__file__).stem}/"


def set_environ():
    os.environ["ADAGUC_CONFIG"] = os.path.join(os.environ["ADAGUC_PATH"], "data", "config", "adaguc.dataset.xml")


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
        "adaguc_ewclocalclimateinfo_test.xml",
        "adaguc_tests_uwcwdini_windcomponents.xml",
        "adaguc_tests_solarterminator.xml",
        "adaguc.tests.multi_reftime_temporal_extent.xml",
        "adaguc.tests.arcus_uwcw_manymodelruns.xml",
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
        print("ingesting: ", os.environ["ADAGUC_CONFIG"] + "," + service + " =>", status)

    AdagucTestTools().mkdir_p(testresultspath)


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

    collection_ids = [coll["id"] for coll in colls["collections"]]
    assert collection_ids == [
        "adaguc.tests.arcus_uwcw.hagl_member",
        "adaguc.tests.arcus_uwcw_manymodelruns.hagl",
        "adaguc.tests.members.mycollection",
        "adaguc.tests.multi_reftime_temporal_extent.gl",
        "adaguc_ewclocalclimateinfo_test",
        "adaguc_tests_solarterminator",
        "adaguc_tests_uwcwdini_windcomponents",
        "netcdf_5d.data_5d",
        "testcollection.testcollection",
    ]

    coll_5d = colls["collections"][7]
    assert coll_5d.get("id") == "netcdf_5d.data_5d"
    assert all(ext_name in coll_5d["extent"] for ext_name in ("spatial", "temporal", "vertical", "custom"))
    assert list(coll_5d["extent"]) == [
        "spatial",
        "temporal",
        "vertical",
        "custom",
    ]
    assert coll_5d["extent"]["temporal"]["values"][0] == "R6/2017-01-01T00:00:00Z/PT5M"

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

    assert "data_extra_metadata" in parameter_names

    data_extra_metadata = parameter_names["data_extra_metadata"]

    assert data_extra_metadata == {
        "type": "Parameter",
        "id": "data_extra_metadata",
        "label": "data extra metadata",
        "description": "data extra metadata",
        "unit": {
            "symbol": {
                "value": "new_units",
                "type": "http://www.opengis.net/def/uom/UCUM",
            }
        },
        "observedProperty": {
            "id": "https://vocab.nerc.ac.uk/standard_name/extra_metadata",
            "label": "extra_metadata",
        },
    }


def test_hrefs(client: TestClient):
    """Test if all hrefs from a collection don't return 404"""
    resp = client.get("/edr/collections")
    collection = next((c for c in resp.json()["collections"] if c["id"] == "adaguc.tests.arcus_uwcw.hagl_member"))

    # collect all urls
    urls = [links["href"] for links in collection["links"]]
    urls += [query["link"]["href"] for query in collection["data_queries"].values()]

    # make unique, and remove base urls.
    urls = sorted(set(urls))
    urls = [urlsplit(url).path for url in urls]

    assert urls == [
        "/edr/collections/adaguc.tests.arcus_uwcw.hagl_member",
        "/edr/collections/adaguc.tests.arcus_uwcw.hagl_member/cube",
        "/edr/collections/adaguc.tests.arcus_uwcw.hagl_member/instances",
        "/edr/collections/adaguc.tests.arcus_uwcw.hagl_member/locations",
        "/edr/collections/adaguc.tests.arcus_uwcw.hagl_member/position",
        "/edr/collections/adaguc.tests.arcus_uwcw.hagl_member/trajectory",
    ]

    for url in urls:
        resp = client.get(url)
        assert resp.status_code != 404


def test_coll_multi_dim_position_single_coverage(client: TestClient):
    # Querying a single datetime and single Z results in a Coverage
    resp = client.get(
        "/edr/collections/testcollection.testcollection/instances/202406010000/position?coords=POINT(5.2 52.0)&datetime=2024-06-01T01:00:00Z&parameter-name=testdata&z=40"
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
    resp = client.get(position_url.format(datetime="2024-06-01T01:00:00Z/2024-06-01T04:00:00Z", z="10"))
    assert resp.status_code, 200
    covjson = resp.json()
    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["domainType"] == "PointSeries"

    # Multiple datetime, multiple z = CoverageCollection with VerticalProfile
    resp = client.get(position_url.format(datetime="2024-06-01T01:00:00Z/2024-06-01T04:00:00Z", z="10,20"))
    assert resp.status_code, 200
    covjson = resp.json()
    assert covjson["type"] == "CoverageCollection"
    assert len(covjson["coverages"]) == 4
    assert all([c["domain"]["domainType"] == "VerticalProfile" for c in covjson["coverages"]])


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
    resp = client.get(cube_url.format(datetime="2024-06-01T01:00:00Z/2024-06-01T04:00:00Z", z="10,20"))
    assert resp.status_code, 200
    covjson = resp.json()
    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["domainType"] == "Grid"


def test_coll_multi_dim_position_coverage_collection_all_z_star(client: TestClient):
    # Querying a multiple datetime and all z results in a CoverageCollection
    resp = client.get(
        "/edr/collections/testcollection.testcollection/instances/202406010000/position?coords=POINT(5.2 52.0)&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata&z=*"
    )
    assert resp.status_code, 200
    covjson = resp.json()

    assert covjson["type"] == "CoverageCollection"
    assert len(covjson["coverages"]) == 4

    # All coverages should have same z
    assert all([c["domain"]["axes"]["z"]["values"] == [10, 20, 30, 40] for c in covjson["coverages"]])

    # All coverages should have same shape
    assert all([c["ranges"]["testdata"]["shape"] == [4, 1] for c in covjson["coverages"]])

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


def test_coll_multi_dim_position_coverage_collection_all_z_undefined(
    client: TestClient,
):
    # Querying a multiple datetime without a z results in a CoverageCollection
    resp = client.get(
        "/edr/collections/testcollection.testcollection/instances/202406010000/position?coords=POINT(5.2 52.0)&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata"
    )
    assert resp.status_code, 200
    covjson = resp.json()

    assert covjson["type"] == "CoverageCollection"
    assert len(covjson["coverages"]) == 4

    # All coverages should have same z
    assert all([c["domain"]["axes"]["z"]["values"] == [10, 20, 30, 40] for c in covjson["coverages"]])

    # All coverages should have same shape
    assert all([c["ranges"]["testdata"]["shape"] == [4, 1] for c in covjson["coverages"]])

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
    assert all([c["domain"]["axes"]["z"]["values"] == [10, 20] for c in covjson["coverages"]])

    # All coverages should have same shape
    assert all([c["ranges"]["testdata"]["shape"] == [2, 1] for c in covjson["coverages"]])

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

    assert covjson["coverages"][0]["ranges"]["jaarlijks/tas_mean_2050"]["values"] == [
        10.90503,
        12.007289,
        11.059411,
        12.062785,
        11.947086,
        10.081615,
        11.765973,
        11.881777,
        12.438473,
        12.412801,
        11.976863,
        12.304342,
        12.01673,
        11.909211,
        12.190491,
        12.779091,
        12.678858,
        12.103762,
        12.038959,
        10.637293,
        12.453781,
        11.861727,
        11.348157,
        13.191739,
        12.320773,
        12.319611,
        12.470861,
        12.96588,
        12.703981,
        13.242649,
    ]

    assert covjson["coverages"][1]["ranges"]["jaarlijks/tas_mean_2050"]["values"] == [
        10.82554,
        11.873678,
        11.01875,
        11.902385,
        11.815706,
        10.020253,
        11.643116,
        11.819782,
        12.285396,
        12.28452,
        11.84561,
        12.195546,
        11.844776,
        11.799691,
        12.05139,
        12.585763,
        12.544561,
        11.989633,
        11.920113,
        10.537866,
        12.319339,
        11.716161,
        11.255101,
        12.989518,
        12.206244,
        12.159081,
        12.357863,
        12.784953,
        12.555624,
        13.065644,
    ]


def test_coll_multi_dim_position_coverage_collection_z_range(client: TestClient):
    # Should handle querying multiple heights separated by comma, combined with querying ranges
    resp = client.get(
        "/edr/collections/testcollection.testcollection/instances/202406010000/position?coords=POINT(5.2 52.0)&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata&z=10,30/40"
    )
    assert resp.status_code, 200
    covjson = resp.json()

    # All coverages should have same z
    assert all([c["domain"]["axes"]["z"]["values"] == [10, 30, 40] for c in covjson["coverages"]])
    # All coverages should have same shape
    assert all([c["ranges"]["testdata"]["shape"] == [3, 1] for c in covjson["coverages"]])

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
        "/edr/collections/testcollection.testcollection/instances/202406010000/cube?bbox=5.5,52.5,6.5,53.5&datetime=2024-06-01T01:00:00Z&parameter-name=testdata&z=40"
    )
    assert resp.status_code, 200
    covjson = resp.json()

    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["axes"]["z"]["values"] == [40]
    assert covjson["domain"]["axes"]["t"]["values"] == ["2024-06-01T01:00:00Z"]
    assert covjson["ranges"]["testdata"]["values"] == [300101]

    # Without instance, should use the latest instance (same as above)
    resp = client.get(
        "/edr/collections/testcollection.testcollection/cube?bbox=5.5,52.5,6.5,53.5&datetime=2024-06-01T01:00:00Z&parameter-name=testdata&z=40"
    )
    assert resp.status_code, 200
    covjson = resp.json()
    print("RESP:", json.dumps(covjson, indent=2))
    assert covjson["type"] == "Coverage"
    assert covjson["domain"]["axes"]["z"]["values"] == [40]
    assert covjson["domain"]["axes"]["t"]["values"] == ["2024-06-01T01:00:00Z"]
    assert covjson["ranges"]["testdata"]["values"] == [300101]

    # Without instance multiple timesteps, should use the latest instance
    resp = client.get(
        "/edr/collections/testcollection.testcollection/cube?bbox=5.5,52.5,6.5,53.5&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata&z=40"
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
        "/edr/collections/testcollection.testcollection/cube?bbox=5.5,52.5,6.5,53.5&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata2&z=30"
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
        "/edr/collections/testcollection.testcollection/cube?bbox=5.5,52.5,7.5,53.5&datetime=2024-06-01T01:00:00Z/2024-06-01T04:00:00Z&parameter-name=testdata,testdata2&z=40"
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
    assert covjson["ranges"]["testdata"]["values"] == [
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


def test_trace_timings_header(client: TestClient):
    """The adaguc executable emits an X-Trace-Timings header when ADAGUC_TRACE_TIMINGS is
    enabled. EDR endpoints can invoke adaguc multiple times per request (e.g. /cube does
    one call per requested parameter, plus a separate getmetadata call), so every internal
    call's header must be propagated to the response, not just the last one. The getmetadata
    call's trace timings are propagated under their own X-Trace-Timings-Metadata header, kept
    distinct from the data call(s)' X-Trace-Timings header."""
    old_trace_timings = os.environ.get("ADAGUC_TRACE_TIMINGS")
    try:
        # Tracing disabled (the default for this test suite): no header should be present.
        os.environ["ADAGUC_TRACE_TIMINGS"] = "FALSE"
        resp = client.get(
            "/edr/collections/testcollection.testcollection/instances/202406010000/position?coords=POINT(5.2 52.0)&datetime=2024-06-01T01:00:00Z&parameter-name=testdata&z=40"
        )
        assert resp.status_code == 200
        assert resp.headers.get_list("x-trace-timings") == []
        assert resp.headers.get_list("x-trace-timings-metadata") == []

        os.environ["ADAGUC_TRACE_TIMINGS"] = "TRUE"

        # Position makes a single internal adaguc data call and a single getmetadata call,
        # so a single header of each kind is expected.
        resp = client.get(
            "/edr/collections/testcollection.testcollection/instances/202406010000/position?coords=POINT(5.2 52.0)&datetime=2024-06-01T01:00:00Z&parameter-name=testdata&z=40"
        )
        assert resp.status_code == 200
        trace_headers = resp.headers.get_list("x-trace-timings")
        assert len(trace_headers) == 1
        assert trace_headers[0] != ""
        metadata_trace_headers = resp.headers.get_list("x-trace-timings-metadata")
        assert len(metadata_trace_headers) == 1
        assert metadata_trace_headers[0] != ""

        # Cube with two parameters makes two internal adaguc data calls, one per parameter,
        # so both of their headers are expected to be propagated, alongside the single
        # getmetadata call cube makes regardless of the number of parameters.
        resp = client.get(
            "/edr/collections/testcollection.testcollection/cube?bbox=5.5,52.5,7.5,53.5&datetime=2024-06-01T01:00:00Z&parameter-name=testdata,testdata2&z=30"
        )
        assert resp.status_code == 200
        trace_headers = resp.headers.get_list("x-trace-timings")
        assert len(trace_headers) == 2
        assert all(h != "" for h in trace_headers)
        metadata_trace_headers = resp.headers.get_list("x-trace-timings-metadata")
        assert len(metadata_trace_headers) == 1
        assert metadata_trace_headers[0] != ""

        # Collection-by-id makes two getmetadata calls in a row (once to find the latest
        # instance, once more with that instance), so two metadata headers are expected.
        resp = client.get("/edr/collections/testcollection.testcollection")
        assert resp.status_code == 200
        metadata_trace_headers = resp.headers.get_list("x-trace-timings-metadata")
        assert len(metadata_trace_headers) == 2
        assert all(h != "" for h in metadata_trace_headers)
    finally:
        if old_trace_timings is None:
            os.environ.pop("ADAGUC_TRACE_TIMINGS", None)
        else:
            os.environ["ADAGUC_TRACE_TIMINGS"] = old_trace_timings


def test_point_custom_dim(client: TestClient):
    # position call on data which includes a custom dimension, should mention dimension even if we don't query it specifically
    resp = client.get(
        "/edr/collections/adaguc.tests.members.mycollection/instances/202503010000/"
        + "position?coords=POINT(5.0 52.0)&parameter-name=mymemberdata"
    )
    assert resp.status_code, 200
    position_json = resp.json()

    # Should mention the custom dimension inside the "domain" section
    custom_dims = [custom_dim for custom_dim in position_json["domain"] if custom_dim.startswith("custom:")]
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
    custom_dims = [custom_dim for custom_dim in cube_json["domain"] if custom_dim.startswith("custom:")]
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
            404,
            "Collection my_unknown_collection unknown",
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
            "/edr/collections/adaguc.tests.members.mycollection/instances/202503010000/position?coords=POINT()&parameter-name=mymemberdata",
            400,
            "Could not parse WKT POINT, received coords=POINT()",
        ),
    ],
    ids=[
        "unknown_collection",
        "incorrect_instance",
        "incorrect_parameter",
        "invalid_point",
    ],
)
def test_edr_exceptions(url, status_code, description, client: TestClient):
    resp = client.get(url)
    print(json.dumps(resp.json(), indent=2))
    assert resp.status_code == status_code
    assert resp.json()["description"] == description


def test_adaguc_tests_uwcwdini_windcomponents_position(client: TestClient):
    # Querying from dini with x-wind and y wind. Should become grid relative
    resp = client.get(
        "/edr/collections/adaguc_tests_uwcwdini_windcomponents/position?coords=POINT(5.15 52.67)&parameter-name=wind-hagl&datetime=2024-09-06T10:00:00Z/2024-09-06T17:00:00Z"
    )
    assert resp.status_code, 200
    covjson = resp.json()

    filename = "test_adaguc_tests_uwcwdini_windcomponents_position.json"
    AdagucTestTools().writetojson(f"{testresultspath}{filename}", json.dumps(covjson))

    assert AdagucTestTools().compareJson(expectedoutputspath + filename, testresultspath + filename)


def test_no_parameters(client: TestClient):
    url = "/edr/collections/adaguc.tests.members.mycollection/instances/202503010000/position?coords=POINT(5.0 52.0)"
    resp = client.get(url)
    print(json.dumps(resp.json(), indent=2))
    assert resp.status_code == 200
    assert list(resp.json()["parameters"].keys()) == ["mymemberdata"]


def test_locations(client: TestClient):
    """
    This should filter away all locations outside of [4.5,51.5,7.5,54.5]
    """
    url = "/edr/collections/adaguc.tests.members.mycollection/instances/202503010000/locations"
    resp = client.get(url)
    assert resp.status_code == 200
    stations = [f["id"] for f in resp.json()["features"]]
    assert stations == ["EHAM", "EHTW", "EHGG", "EHKD", "EHDB", "EHLW"]


def test_position(client: TestClient):
    """test a position call with the coordinates of EHAM"""
    url = "/edr/collections/adaguc.tests.members.mycollection/instances/202503010000/position?coords=POINT(4.76389 52.308601)"
    resp = client.get(url)
    assert resp.status_code == 200
    print("/position?coords=POINT(4.76389,52.308601):", json.dumps(resp.json(), indent=2))
    covjson = resp.json()
    assert covjson["ranges"]["mymemberdata"]["values"] == [152055950.0]


def test_location(client: TestClient):
    url = "/edr/collections/adaguc.tests.members.mycollection/instances/202503010000/locations/EHAM"
    resp = client.get(url)
    assert resp.status_code == 200
    print("/location/EHAM:", json.dumps(resp.json(), indent=2))
    covjson = resp.json()
    assert covjson["ranges"]["mymemberdata"]["values"] == [152055950.0]


def test_unknown_location(client: TestClient):
    url = "/edr/collections/adaguc.tests.members.mycollection/instances/202503010000/locations/KJFK"
    resp = client.get(url)
    assert resp.status_code == 404
    print("/location/KJFK:", json.dumps(resp.json(), indent=2))
    covjson = resp.json()
    assert covjson["detail"] == "location KJFK not found"


def test_solt_instanceless_position(client: TestClient):
    url = "/edr/collections/adaguc_tests_solarterminator/position?coords=POINT(5.0 52.0)&datetime=2026-06-01T01:00:00Z/2026-06-01T02:00:00Z&parameter-name=solarterminator"
    resp = client.get(url)
    assert resp.status_code == 200
    covjson = resp.json()
    filename = "test_solt_instanceless_position.json"
    AdagucTestTools().writetojson(f"{testresultspath}{filename}", json.dumps(covjson))
    assert AdagucTestTools().compareJson(expectedoutputspath + filename, testresultspath + filename)


def test_multi_reftime_temporal_extent(client: TestClient):
    """
    This dataset contains two instances, with reftime 2026-04-01T12 and 2026-04-07T00.
    There is an hourly and 24-hourly parameter in this dataset.

    The temporal extent of a specific instance must be calculated over _all_ the parameters.
    This means that even though the first (configured) parameter in the dataset is a 24-hourly
    parameter (starts at T+24) the other parameter in the dataset is an hourly parameter
    (starts at T+1). This means the start of the temporal extent must begin at T+1.

    Temporal extents in the /instances call will be wrong, we cannot fix this.
    """

    base_url = "/edr/collections/adaguc.tests.multi_reftime_temporal_extent.gl"

    # The temporal extent in the /instances call will be wrong, as they are the same for _all_ instances.
    resp = client.get(f"{base_url}/instances")
    assert resp.status_code == 200
    covjson = resp.json()

    instances = covjson.get("instances")
    assert instances[0]["id"] == "202604070000"
    assert instances[1]["id"] == "202604011200"
    assert (
        instances[0]["extent"]["temporal"]["interval"]
        == instances[1]["extent"]["temporal"]["interval"]
        == [["2026-04-01T13:00:00Z", "2026-04-09T12:00:00Z"]]
    )

    # Temporal extent must be correct when requesting a specific instance
    resp = client.get(f"{base_url}/instances/202604011200")
    assert resp.status_code == 200
    covjson = resp.json()

    assert covjson["extent"]["temporal"]["interval"] == [["2026-04-01T13:00:00Z", "2026-04-04T00:00:00Z"]]
    assert covjson["extent"]["temporal"]["values"] == ["R60/2026-04-01T13:00:00Z/PT1H"]

    # Temporal extent must be correct when requesting a specific instance
    resp = client.get(f"{base_url}/instances/202604070000")
    assert resp.status_code == 200
    covjson = resp.json()

    assert covjson["extent"]["temporal"]["interval"] == [["2026-04-07T01:00:00Z", "2026-04-09T12:00:00Z"]]
    assert covjson["extent"]["temporal"]["values"] == ["R60/2026-04-07T01:00:00Z/PT1H"]

    # If you omit the instance, EDR must return the most recent instance
    resp = client.get(f"{base_url}")
    assert resp.status_code == 200
    most_recent_instance = resp.json()

    # ids and urls will be different, but the temporal extent must be the same since it refers to the same specific instance
    assert most_recent_instance["extent"] == covjson["extent"]


def test_many_reference_times(client: TestClient):
    """
    The reference_time dimension of this dataset is advertised by GetMetadata as an interval
    (start/end/resolution) instead of a list. EDR must expand it into separate instances.
    """
    base_url = "/edr/collections/adaguc.tests.arcus_uwcw_manymodelruns.hagl"

    resp = client.get(base_url)
    assert resp.status_code == 200
    assert resp.json()["id"] == "adaguc.tests.arcus_uwcw_manymodelruns.hagl"

    resp = client.get(f"{base_url}/instances")
    assert resp.status_code == 200
    instance_ids = [instance["id"] for instance in resp.json()["instances"]]
    assert len(instance_ids) == 22
    assert instance_ids[0] == "202610010900"
    assert instance_ids[-1] == "202609301200"


def test_multi_reftime_instanceless_query(client: TestClient):
    """Querying position and cube without selecting an instance should return results for the most recent instance"""

    base_url = "/edr/collections/adaguc.tests.multi_reftime_temporal_extent.gl"

    # Position query
    resp = client.get(f"{base_url}/position?coords=POINT(5.0 52.0)&datetime=1000-01-01T00:00:00Z/3000-01-01T00:00:00Z")
    assert resp.status_code == 200
    covjson = resp.json()

    filename = "multi_reftime_instanceless_position.json"
    AdagucTestTools().writetojson(f"{testresultspath}{filename}", json.dumps(covjson))
    assert AdagucTestTools().compareJson(expectedoutputspath + filename, testresultspath + filename)

    # Cube query
    resp = client.get(f"{base_url}/cube?bbox=2.0,50.5,9.0,54.5&datetime=1000-01-01T00:00:00Z/3000-01-01T00:00:00Z")
    assert resp.status_code == 200
    covjson = resp.json()

    filename = "multi_reftime_instanceless_cube.json"
    AdagucTestTools().writetojson(f"{testresultspath}{filename}", json.dumps(covjson))
    assert AdagucTestTools().compareJson(expectedoutputspath + filename, testresultspath + filename)


def test_wildcard_query(client: TestClient):
    """Querying position and cube without selecting an instance should return results for the most recent instance"""

    base_url = "/edr/collections/adaguc.tests.multi_reftime_temporal_extent.gl"

    # Position query
    resp = client.get(f"{base_url}/position?coords=POINT(5.0 52.0)&datetime=*")
    assert resp.status_code == 200
    covjson = resp.json()

    filename = "multi_reftime_instanceless_position_wildcard.json"
    AdagucTestTools().writetojson(f"{testresultspath}{filename}", json.dumps(covjson))
    assert AdagucTestTools().compareJson(expectedoutputspath + filename, testresultspath + filename)

    # Cube query
    resp = client.get(f"{base_url}/cube?bbox=2.0,50.5,9.0,54.5&datetime=*")
    assert resp.status_code == 200
    covjson = resp.json()

    filename = "multi_reftime_instanceless_cube_wildcard.json"
    AdagucTestTools().writetojson(f"{testresultspath}{filename}", json.dumps(covjson))
    assert AdagucTestTools().compareJson(expectedoutputspath + filename, testresultspath + filename)


@pytest.mark.parametrize("z,levels", [(None, [10, 20, 30, 40]), ("*", [10, 20, 30, 40]), ("10", [10]), ("20,40", [20, 40])])
def test_trajectory_section(client, z, levels, monkeypatch):
    monkeypatch.setenv("ADAGUC_TRACE_TIMINGS", "TRUE")
    params = {
        "coords": "LINESTRING(5.1 52.1,8.1 52.1)",
        "parameter-name": "testdata",
        "datetime": "2024-06-01T01:00:00Z/2024-06-01T02:00:00Z",
    }
    if z is not None:
        params["z"] = z
    resp = client.get("/edr/collections/testcollection.testcollection/instances/202406010000/trajectory", params=params)
    assert resp.status_code == 200, resp.text
    result = resp.json()
    assert result["domain"]["domainType"] == "Section"
    axes = result["domain"]["axes"]
    assert axes["z"]["values"] == levels
    assert axes["composite"]["coordinates"] == ["t", "x", "y"]
    assert axes["composite"]["dataType"] == "tuple"
    expected_coords = [[lon, 52] for lon in range(5, 9)]
    assert axes["composite"]["values"] == [
        [time, *coord] for time in ["2024-06-01T01:00:00Z", "2024-06-01T02:00:00Z"] for coord in expected_coords
    ]
    values = result["ranges"]["testdata"]
    assert values["axisNames"] == ["z", "composite"]
    assert values["shape"] == [len(levels), 8]
    assert values["values"] == [10000 * (level - 10) + time * 10000 + x * 100 for level in levels for time in range(2) for x in range(4)]
    assert len(resp.headers.get_list("x-trace-timings-metadata")) == 1
    assert len(resp.headers.get_list("x-trace-timings")) == 1
    import re

    assert re.search(r"\[FSNCOPEN [^/]+/1/0\]", resp.headers["x-trace-timings"])


def test_trajectory_revisits_and_defaults(client):
    base = "/edr/collections/testcollection.testcollection"
    resp = client.get(base + "/trajectory", params={"coords": "LINESTRING(5 52,8 52,5 52)", "parameter-name": "testdata", "z": "10"})
    assert resp.status_code == 200, resp.text
    section = resp.json()
    tuples = section["domain"]["axes"]["composite"]["values"]
    assert [point[1] for point in tuples] == [5, 6, 7, 8, 7, 6, 5]
    point = client.get(base + "/position", params={"coords": "POINT(5 52)", "parameter-name": "testdata", "z": "10"})
    assert point.status_code == 200
    assert {v[0] for v in tuples} == set(point.json()["domain"]["axes"]["t"]["values"])
    assert section["ranges"]["testdata"]["values"][0] == point.json()["ranges"]["testdata"]["values"][0]


@pytest.mark.parametrize(
    "coords",
    [None, "POINT(5 52)", "LINESTRING(5 52)", "LINESTRING(5 52 10,6 53 10)", "LINESTRING(nan 52,6 53)", "LINESTRING(5.152.1,8 52)"],
)
def test_trajectory_invalid_coords(client, coords):
    params = {} if coords is None else {"coords": coords}
    resp = client.get("/edr/collections/testcollection.testcollection/trajectory", params=params)
    assert resp.status_code == 400, resp.text


def test_trajectory_multiple_parameters_one_file(client, monkeypatch):
    monkeypatch.setenv("ADAGUC_TRACE_TIMINGS", "TRUE")
    resp = client.get(
        "/edr/collections/testcollection.testcollection/trajectory",
        params={"coords": "LINESTRING(5 52,7 53)", "parameter-name": "testdata,testdata2", "z": "*"},
    )
    assert resp.status_code == 200, resp.text
    result = resp.json()
    assert result["type"] == "CoverageCollection"
    assert len(result["coverages"]) == 2
    assert [len(c["domain"]["axes"]["z"]["values"]) for c in result["coverages"]] == [4, 3]
    import re

    assert re.search(r"\[FSNCOPEN [^/]+/1/0\]", resp.headers["x-trace-timings"])


@pytest.mark.parametrize("z", [None, "2", "3"])
def test_trajectory_hidden_default_height(client, z):
    resp = client.get(
        "/edr/collections/adaguc.tests.arcus_uwcw.hagl_member/instances/202405230000/trajectory",
        params={"coords": "LINESTRING(3.4 52.5,7.9 52.5)", "parameter-name": "air_temperature_hagl", **({"z": z} if z else {})},
    )
    assert resp.status_code == 200, resp.text
    assert resp.json()["domain"]["axes"]["z"]["values"] == [2]


def test_trajectory_outside_and_resolution(client):
    base = "/edr/collections/testcollection.testcollection/trajectory"
    resp = client.get(base, params={"coords": "LINESTRING(-20 -20,-19 -19)", "parameter-name": "testdata"})
    assert resp.status_code == 404, resp.text
    resp = client.get(base, params={"coords": "LINESTRING(5 52,7 53)", "resolution_x": "1"})
    assert resp.status_code == 400


def test_trajectory_rotated_grid_and_multiple_files(client, monkeypatch):
    import re
    from routers.trajectory_covjson import sections_from_response
    from routers.utils.edr_utils import handle_metadata

    service = "adaguc.tests.trajectory"
    status, *_ = AdagucTestTools().runADAGUCServer(
        args=["--updatedb", "--config", os.environ["ADAGUC_CONFIG"] + f",{service}.xml"], isCGI=False
    )
    assert status == 0
    monkeypatch.setenv("ADAGUC_TRACE_TIMINGS", "TRUE")
    base = f"/edr/collections/{service}.pressure/instances/202407110500"
    query = {"coords": "LINESTRING(0 50,10 55)", "parameter-name": "temperature", "datetime": "2024-07-11T06:00:00Z", "z": "*"}
    response = client.get(base + "/trajectory", params=query)
    assert response.status_code == 200, response.text
    section = response.json()
    assert section["domain"]["axes"]["z"]["values"] == [500, 700, 850, 925]
    points = section["domain"]["axes"]["composite"]["values"]
    assert len(points) > 2
    assert re.search(r"\[FSNCOPEN [^/]+/1/0\]", response.headers["x-trace-timings"])
    strided = client.get(base + "/trajectory", params={**query, "parameter-name": "temperature_strided"})
    assert strided.status_code == 200, strided.text
    assert strided.json()["domain"] == section["domain"]
    assert strided.json()["ranges"]["temperature_strided"] == section["ranges"]["temperature"]
    for i, (time, lon, lat) in enumerate(points):
        position = client.get(
            base + "/position", params={"coords": f"POINT({lon} {lat})", "parameter-name": "temperature", "datetime": time, "z": "500"}
        )
        assert position.status_code == 200, position.text
        assert section["ranges"]["temperature"]["values"][i] == position.json()["ranges"]["temperature"]["values"][0]

    # All four model runs are separate files. Test the native operation directly
    # because EDR's instance selection intentionally selects a single model run.
    native_query = (
        f"dataset={service}&service=wms&version=1.3.0&request=GetTrajectory&info_format=application/json&crs=EPSG:4326"
        "&query_layers=temperature&coords=LINESTRING(0 50,10 55)&time=2024-07-11T06:00:00Z&dim_reference_time=*&dim_pressure_level_in_hpa=*"
    )
    status, body, headers = AdagucTestTools().runADAGUCServer(native_query)
    assert status == 0
    assert any(re.search(r"\[FSNCOPEN [^/]+/4/0\]", header) for header in headers)
    status, md, _ = AdagucTestTools().runADAGUCServer(
        f"dataset={service}&service=wms&version=1.3.0&request=GetMetadata&format=application/json"
    )
    assert status == 0
    metadata = handle_metadata(json.loads(md))[f"{service}.pressure"]
    result = sections_from_response(json.loads(body), metadata, f"{service}.pressure").model_dump(mode="json", exclude_none=True)
    assert len(result["coverages"]) == 4
    assert len({c["domain"]["custom:reference_time"] for c in result["coverages"]}) == 4
    latest = next(c for c in result["coverages"] if c["domain"]["custom:reference_time"] == "2024-07-11T05:00:00Z")
    assert latest["ranges"] == section["ranges"]


def test_trajectory_zero_missing_and_ordering():
    from routers.trajectory_covjson import sections_from_response

    metadata = {
        "p": {
            "dims": {"height": {"type": "dimtype_vertical", "serviceName": "height"}},
            "layer": {"layername": "p", "title": "Temperature", "abstract": "", "variables": [{"units": "K", "label": "Temperature"}]},
        }
    }
    t1, t2 = "2024-01-01T00:00:00Z", "2024-01-01T01:00:00Z"
    samples = [
        {"layername": "p", "point": {"coords": "5,52"}, "dims": ["height", "time"], "data": {"10": {t2: "0", t1: "nodata"}}},
        {"layername": "p", "point": {"coords": "6,52"}, "dims": ["height", "time"], "data": {"10": {t2: "nan", t1: "2"}}},
    ]
    result = sections_from_response(samples, metadata, "dataset.collection").model_dump(mode="json", exclude_none=True)
    assert result["domain"]["axes"]["composite"]["values"] == [[t1, 5, 52], [t1, 6, 52], [t2, 5, 52], [t2, 6, 52]]
    assert result["ranges"]["p"]["values"] == [None, 2, 0, None]


def test_trajectory_custom_dimension(client):
    base = "/edr/collections/adaguc.tests.arcus_uwcw.hagl_member/instances/202405230000"
    response = client.get(
        base + "/trajectory",
        params={
            "coords": "LINESTRING(3.4 52.5,7.9 52.5)",
            "parameter-name": "air_temperature_hagl",
            "member": "*",
        },
    )
    assert response.status_code == 200, response.text
    coverages = response.json()["coverages"]
    assert len(coverages) == 5
    assert len({coverage["domain"]["custom:member"] for coverage in coverages}) == 5
    for coverage in coverages:
        time, lon, lat = coverage["domain"]["axes"]["composite"]["values"][0]
        point = client.get(
            base + "/position",
            params={
                "coords": f"POINT({lon} {lat})",
                "parameter-name": "air_temperature_hagl",
                "datetime": time,
                "member": f"{coverage['domain']['custom:member']:g}",
            },
        )
        assert point.status_code == 200, point.text
        assert coverage["ranges"]["air_temperature_hagl"]["values"][0] == point.json()["ranges"]["air_temperature_hagl"]["values"][0]


def test_trajectory_scalar_postprocessing(client):
    base = "/edr/collections/adaguc.tests.arcus_uwcw.hagl_member/instances/202406050300"
    response = client.get(
        base + "/trajectory",
        params={
            "coords": "LINESTRING(3.4 52.5,7.9 53.9)",
            "parameter-name": "wind_speed_hagl_ms",
        },
    )
    assert response.status_code == 200, response.text
    coverage = response.json()
    for i, (time, lon, lat) in enumerate(coverage["domain"]["axes"]["composite"]["values"]):
        point = client.get(
            base + "/position",
            params={
                "coords": f"POINT({lon} {lat})",
                "parameter-name": "wind_speed_hagl_ms",
                "datetime": time,
            },
        )
        assert point.status_code == 200, point.text
        assert coverage["ranges"]["wind_speed_hagl_ms"]["values"][i] == point.json()["ranges"]["wind_speed_hagl_ms"]["values"][0]


@pytest.mark.parametrize(
    "status,body",
    [
        (0, b""),
        (0, b"<ServiceExceptionReport>Native failure</ServiceExceptionReport>"),
        (0, b'{"error":"UnprocessableEntity"}'),
        (0, b"\xff"),
        (1, b"Native failure"),
    ],
)
def test_trajectory_invalid_native_response(client, monkeypatch, status, body):
    from unittest.mock import AsyncMock
    from routers import edr_trajectory

    monkeypatch.setattr(edr_trajectory, "call_adaguc", AsyncMock(return_value=(status, body, [])))
    response = client.get(
        "/edr/collections/testcollection.testcollection/trajectory",
        params={
            "coords": "LINESTRING(5 52,7 53)",
            "parameter-name": "testdata",
        },
    )
    assert response.status_code == 500, response.text
    assert response.json()["description"].startswith("Trajectory data request")


@pytest.mark.parametrize("dimension_order", [("time", "ml", "y", "x"), ("ml", "time", "x", "y"), ("y", "time", "x", "ml")])
def test_trajectory_float32_axes_and_native_failure(client, tmp_path, monkeypatch, dimension_order):
    import numpy as np
    from netCDF4 import Dataset

    # Match the soundings grid's float32 rounding over 676 x 564 coordinates.
    path = tmp_path / "rounded.nc"
    with Dataset(path, "w") as nc:
        for name, size in [("x", 676), ("y", 564), ("time", 1), ("ml", 2), ("forecast_reference_time", 1)]:
            nc.createDimension(name, size)
        nc.createVariable("x", "f4", ("x",))[:] = -13.5 + np.arange(676) * 0.05
        nc.createVariable("y", "f4", ("y",))[:] = -13.6 + np.arange(564) * 0.05
        for name, value in [("time", 1), ("forecast_reference_time", 0)]:
            variable = nc.createVariable(name, "f8", (name,))
            variable[:] = [value]
            variable.standard_name = name
            variable.units = "hours since 2026-09-28 00:00:00"
        level = nc.createVariable("ml", "f4", ("ml",))
        level[:] = [1, 2]
        level.standard_name = "model_level"
        crs = nc.createVariable("crs", "i4")
        crs.setncatts(
            {"grid_mapping_name": "rotated_latitude_longitude", "grid_north_pole_latitude": 35.0, "grid_north_pole_longitude": 172.0}
        )
        temperature = nc.createVariable("temperature", "f4", dimension_order)
        temperature.grid_mapping = "crs"
        temperature.standard_name = "air_temperature"
        temperature.units = "K"
        values = np.arange(564 * 676, dtype=np.float32).reshape(564, 676)
        source = np.stack([values, values + 1000000])[np.newaxis, ...]
        temperature[:] = source.transpose([("time", "ml", "y", "x").index(dim) for dim in dimension_order])
    config = tmp_path / "rounded.xml"
    config.write_text(f"""<Configuration>
      <Layer type="database">
        <Group collection="ml"/><Name>air_temperature_ml</Name><Title>Float32 grid regression</Title>
        <Variable>temperature</Variable><FilePath>{path}</FilePath><Styles>auto</Styles>
        <Dimension name="forecast_reference_time" units="ISO8601" type="reference_time">reference_time</Dimension>
        <Dimension name="time" units="ISO8601" default="forecast_reference_time+PT1H" type="time">time</Dimension>
        <Dimension name="ml" units=" " type="vertical">model_level</Dimension>
      </Layer>
    </Configuration>""")
    monkeypatch.setenv("ADAGUC_DATASET_DIR", str(tmp_path))
    monkeypatch.setenv("ADAGUC_TRACE_TIMINGS", "TRUE")
    status, *_ = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", os.environ["ADAGUC_CONFIG"] + f",{config}"], isCGI=False)
    assert status == 0
    base = "/edr/collections/rounded.ml/instances/202609280000"
    coords = "LINESTRING(4.219 51.534,6.532 52.902)"
    response = client.get(base + "/trajectory", params={"coords": coords, "parameter-name": "air_temperature_ml"})
    if dimension_order[-1] == "ml":
        # The legacy header reader cannot identify this layout; do not report empty data.
        assert response.status_code == 422, response.text
        return
    assert response.status_code == 200, response.text
    section = response.json()
    assert section["domain"]["domainType"] == "Section"
    assert section["domain"]["axes"]["z"]["values"] == [1, 2]
    points = section["domain"]["axes"]["composite"]["values"]
    assert len(points) > 2
    for index in (0, len(points) - 1):
        time, lon, lat = points[index]
        position = client.get(
            base + "/position",
            params={
                "coords": f"POINT({lon} {lat})",
                "parameter-name": "air_temperature_ml",
                "datetime": time,
                "z": "1",
            },
        )
        assert position.status_code == 200, position.text
        assert section["ranges"]["air_temperature_ml"]["values"][index] == position.json()["ranges"]["air_temperature_ml"]["values"][0]
    import re

    assert re.search(r"\[FSNCOPEN [^/]+/1/0\]", response.headers["x-trace-timings"])

    # A genuinely irregular axis must still fail, with a native failure status
    # and a handled EDR response rather than a JSONDecodeError.
    with Dataset(path, "a") as nc:
        nc["x"][100] += 0.01
    response = client.get(base + "/trajectory", params={"coords": coords, "parameter-name": "air_temperature_ml"})
    assert response.status_code == 422, response.text
    assert "unsupported for the selected data" in response.json()["description"]
    status, body, _ = AdagucTestTools().runADAGUCServer(
        f"dataset=rounded&service=wms&version=1.3.0&request=GetTrajectory&info_format=application/json&crs=EPSG:4326"
        f"&query_layers=air_temperature_ml&coords={coords}&dim_reference_time=2026-09-28T00:00:00Z&dim_model_level=*"
    )
    assert status == 422  # AdagucTestTools translates the native exit code 33.
    assert b"Trajectory requires regularly spaced grid axes" in body


@pytest.mark.parametrize("status,expected", [(32, 404), (33, 422), (35, 400), (34, 500), (1, 500)])
def test_trajectory_native_failure_categories(client, monkeypatch, status, expected):
    from unittest.mock import AsyncMock
    from routers import edr_trajectory

    monkeypatch.setattr(edr_trajectory, "call_adaguc", AsyncMock(return_value=(status, b"native failure", [])))
    response = client.get(
        "/edr/collections/testcollection.testcollection/trajectory",
        params={
            "coords": "LINESTRING(5 52,7 53)",
            "parameter-name": "testdata",
        },
    )
    assert response.status_code == expected


def test_trajectory_discovery_requires_section_dimensions(client, monkeypatch):
    from copy import deepcopy
    from unittest.mock import AsyncMock
    from routers import edr
    from routers.utils.edr_utils import get_metadata
    import asyncio

    collection = "testcollection.testcollection"
    metadata = asyncio.run(get_metadata(collection))
    without_vertical = deepcopy(metadata)
    for layer in without_vertical[collection].values():
        layer["dims"] = {name: dim for name, dim in layer["dims"].items() if dim["type"] != "dimtype_vertical"}
    monkeypatch.setattr(edr, "get_metadata", AsyncMock(return_value=without_vertical))
    response = client.get(f"/edr/collections/{collection}")
    assert response.status_code == 200, response.text
    assert "trajectory" not in response.json()["data_queries"]
    monkeypatch.setattr(edr, "get_metadata", AsyncMock(return_value=metadata))
    response = client.get(f"/edr/collections/{collection}")
    assert "trajectory" in response.json()["data_queries"]


def test_shared_vertical_selection_and_query_encoding():
    from urllib.parse import parse_qs
    from routers.utils.edr_utils import get_vertical, native_query, vertical_dimension

    layer = {
        "dims": {
            "surface": {"serviceName": "height", "hidden": True, "type": "dimtype_none"},
            "levels": {"serviceName": "model_level", "hidden": False, "type": "dimtype_vertical"},
        }
    }
    assert vertical_dimension(layer) == "levels"
    metadata = {"c": {"p": layer}}
    assert get_vertical(metadata, "c", "p", None) == ("model_level", "DIM_model_level=*")
    assert get_vertical(metadata, "c", "p", "3") == ("model_level", "DIM_model_level=3")
    del layer["dims"]["levels"]
    assert vertical_dimension(layer) == "surface"
    assert get_vertical(metadata, "c", "p", "3") == (None, "")
    query = native_query({"dataset": "a&b", "coords": "LINESTRING(+5 52,6 53)"}, ["DIM_member=a+b"])
    assert parse_qs(query.decode()) == {"dataset": ["a&b"], "coords": ["LINESTRING(+5 52,6 53)"], "DIM_member": ["a+b"]}
