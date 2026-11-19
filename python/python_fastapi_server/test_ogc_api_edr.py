import json
import logging
import os
from pathlib import Path
from urllib.parse import urlsplit

import netCDF4
import numpy as np
import pytest
from pyproj import CRS
from adaguc.AdagucTestTools import AdagucTestTools
from fastapi.testclient import TestClient

from main import app
from routers.netcdf_to_covjson import get_epsg_axis_order
from routers.utils.native_grid import (
    cellsize_to_step,
    get_collection_native_crs_details,
    get_collection_native_crs_wkt,
    subsample_axis,
    subsample_axis_by_step,
)

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
        "adaguc.tests.rotatedpole_edr.xml",
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
        "adaguc.tests.rotatedpole_edr.rotatedpole",
        "adaguc_ewclocalclimateinfo_test",
        "adaguc_tests_solarterminator",
        "adaguc_tests_uwcwdini_windcomponents",
        "netcdf_5d.data_5d",
        "testcollection.testcollection",
    ]

    coll_5d = next(coll for coll in colls["collections"] if coll["id"] == "netcdf_5d.data_5d")
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

    filename = "cube_single_member.json"
    AdagucTestTools().writetojson(f"{testresultspath}{filename}", json.dumps(cube_json))
    assert AdagucTestTools().compareJson(expectedoutputspath + filename, testresultspath + filename)


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

    filename = "position_all_members.json"
    AdagucTestTools().writetojson(f"{testresultspath}{filename}", json.dumps(position_json))
    assert AdagucTestTools().compareJson(expectedoutputspath + filename, testresultspath + filename)


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

    filename = "cube_all_members.json"
    AdagucTestTools().writetojson(f"{testresultspath}{filename}", json.dumps(cube_json))
    assert AdagucTestTools().compareJson(expectedoutputspath + filename, testresultspath + filename)


@pytest.mark.parametrize("query_type", ["cube", "position"])
@pytest.mark.parametrize(
    "datetime, suffix",
    [
        ("2025-03-01T00:00:00Z/2025-03-01T01:00:00Z", "two_times"),
        ("2025-03-01T00:00:00Z", "single_time"),
    ],
)
def test_custom_dim_two_members_and_timesteps_selects_correct_values(client: TestClient, query_type, datetime, suffix):
    params = {"parameter-name": "mymemberdata", "datetime": datetime, "member": "1,2"}
    if query_type == "cube":
        extra_params = {"bbox": "4.5,51.5,7.5,54.5", "resolution_x": 1, "resolution_y": 1}
    else:
        extra_params = {"coords": "POINT(5 52)"}
    params |= extra_params

    resp = client.get(f"/edr/collections/adaguc.tests.members.mycollection/instances/202503010000/{query_type}", params=params)
    assert resp.status_code == 200, resp.text

    filename = f"{query_type}_two_members_{suffix}.json"
    AdagucTestTools().writetojson(f"{testresultspath}{filename}", json.dumps(resp.json()))
    assert AdagucTestTools().compareJson(expectedoutputspath + filename, testresultspath + filename)


def test_cube_packed_fill_values(client: TestClient) -> None:
    resp = client.get(
        "/edr/collections/testcollection.testcollection/cube",
        params={
            "parameter-name": "packed_fillvalue",
            "bbox": "-1,49,11,56",
            "z": "1/3",
            "resolution_x": 1,
            "resolution_y": 1,
        },
    )
    assert resp.status_code == 200, resp.text
    filename = "cube_packed_fill_values.json"
    AdagucTestTools().writetojson(f"{testresultspath}{filename}", json.dumps(resp.json()))
    assert AdagucTestTools().compareJson(expectedoutputspath + filename, testresultspath + filename)


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


def test_position_brotli_encoding(client: TestClient):
    """CoverageJSON responses are brotli encoded when the client accepts it"""
    resp = client.get(
        "/edr/collections/adaguc_tests_uwcwdini_windcomponents/position?coords=POINT(5.15 52.67)&parameter-name=wind-hagl&datetime=2024-09-06T10:00:00Z/2024-09-06T17:00:00Z",
        headers={"Accept-Encoding": "br"},
    )
    assert resp.status_code == 200
    assert resp.headers["content-type"] == "application/prs.coverage+json"
    assert resp.headers["content-encoding"] == "br"
    assert resp.json()["type"] == "CoverageCollection"


def test_adaguc_tests_uwcwdini_winddirection_position(client: TestClient):
    # Same query as above, but on a layer where filter_dataobjects only selects the direction component
    resp = client.get(
        "/edr/collections/adaguc_tests_uwcwdini_windcomponents/position?coords=POINT(5.15 52.67)&parameter-name=wind-direction-hagl&datetime=2024-09-06T10:00:00Z/2024-09-06T17:00:00Z"
    )
    assert resp.status_code == 200
    covjson = resp.json()

    filename = "test_adaguc_tests_uwcwdini_winddirection_position.json"
    AdagucTestTools().writetojson(f"{testresultspath}{filename}", json.dumps(covjson))

    assert AdagucTestTools().compareJson(expectedoutputspath + filename, testresultspath + filename)


def test_adaguc_tests_uwcwdini_windspeed_position(client: TestClient):
    # Same query as above, but on a layer where filter_dataobjects only selects the speed component
    resp = client.get(
        "/edr/collections/adaguc_tests_uwcwdini_windcomponents/position?coords=POINT(5.15 52.67)&parameter-name=wind-speed-hagl&datetime=2024-09-06T10:00:00Z/2024-09-06T17:00:00Z"
    )
    assert resp.status_code == 200
    covjson = resp.json()

    filename = "test_adaguc_tests_uwcwdini_windspeed_position.json"
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


@pytest.mark.parametrize("z,filename", [(None, "trajectory_section.json"), ("10", "trajectory_section_z10.json")])
def test_trajectory_section(client, z, filename):
    params = {
        # Exercise the 1,000-vertex boundary without changing the sampled path.
        "coords": "LINESTRING(" + ",".join(["5.1 52.1"] * 999 + ["8.1 52.1"]) + ")",
        "parameter-name": "testdata",
        "datetime": "2024-06-01T01:00:00Z/2024-06-01T02:00:00Z",
    }
    if z is not None:
        params["z"] = z
    resp = client.get("/edr/collections/testcollection.testcollection/instances/202406010000/trajectory", params=params)
    assert resp.status_code == 200, resp.text
    AdagucTestTools().writetojson(f"{testresultspath}{filename}", json.dumps(resp.json()))
    assert AdagucTestTools().compareJson(expectedoutputspath + filename, testresultspath + filename)


@pytest.mark.parametrize(
    "coords",
    [
        None,
        "POINT(5 52)",
        "LINESTRING(5.152.1,8 52)",
        pytest.param("LINESTRING(" + ",".join(["5 52"] * 1001) + ")", id="too-many-vertices"),
    ],
)
def test_trajectory_invalid_coords(client, coords):
    params = {} if coords is None else {"coords": coords}
    resp = client.get("/edr/collections/testcollection.testcollection/trajectory", params=params)
    assert resp.status_code == 400, resp.text


def test_trajectory_multiple_parameters(client, monkeypatch):
    resp = client.get(
        "/edr/collections/testcollection.testcollection/trajectory",
        params={"coords": "LINESTRING(5 52,7 53)", "parameter-name": "testdata,testdata2", "z": "*"},
    )
    assert resp.status_code == 200, resp.text
    result = resp.json()
    assert result["type"] == "CoverageCollection"
    assert len(result["coverages"]) == 2
    assert [len(c["domain"]["axes"]["z"]["values"]) for c in result["coverages"]] == [4, 3]

    from copy import deepcopy
    from routers import edr_trajectory

    get_metadata = edr_trajectory.get_metadata
    surface_layers = ["testdata2"]

    async def mixed_metadata(*args, **kwargs):
        metadata = deepcopy(await get_metadata(*args, **kwargs))
        for layer in surface_layers:
            metadata["testcollection.testcollection"][layer]["dims"].pop("time")
        return metadata

    monkeypatch.setattr(edr_trajectory, "get_metadata", mixed_metadata)
    base = "/edr/collections/testcollection.testcollection/trajectory"
    query = {"coords": "LINESTRING(5 52,7 53)"}
    resp = client.get(base, params=query)
    assert resp.status_code == 200, resp.text
    assert list(resp.json()["parameters"]) == ["testdata"]
    resp = client.get(base, params=query | {"parameter-name": "testdata2"})
    assert resp.status_code == 400, resp.text
    surface_layers.append("testdata")
    resp = client.get(base, params=query)
    assert resp.status_code == 400, resp.text


def test_trajectory_outside_grid(client):
    base = "/edr/collections/testcollection.testcollection/trajectory"
    resp = client.get(base, params={"coords": "LINESTRING(-20 -20,-19 -19)", "parameter-name": "testdata"})
    assert resp.status_code == 404, resp.text


def test_trajectory_rotated_grid(client):
    service = "adaguc.tests.trajectory"
    status, *_ = AdagucTestTools().runADAGUCServer(
        args=["--updatedb", "--config", os.environ["ADAGUC_CONFIG"] + f",{service}.xml"], isCGI=False
    )
    assert status == 0
    base = f"/edr/collections/{service}.pressure/instances/202407110500"
    query = {"coords": "LINESTRING(0 50,10 55)", "parameter-name": "temperature", "datetime": "2024-07-11T06:00:00Z", "z": "*"}
    response = client.get(base + "/trajectory", params=query)
    assert response.status_code == 200, response.text
    section = response.json()
    assert section["domain"]["axes"]["z"]["values"] == [500, 700, 850, 925]
    points = section["domain"]["axes"]["composite"]["values"]
    assert len(points) > 2


@pytest.mark.parametrize(
    "member, datetime, count, filename",
    [
        ("*", "2024-05-23T01:00:00Z/2024-05-23T02:00:00Z", 5, "trajectory_all_members.json"),
        ("1,2", "2024-05-23T01:00:00Z/2024-05-23T02:00:00Z", 2, "trajectory_two_members_two_times.json"),
        ("1,2", "2024-05-23T01:00:00Z", 2, "trajectory_two_members_single_time.json"),
    ],
)
def test_trajectory_hidden_height_and_members(client, member, datetime, count, filename):
    base = "/edr/collections/adaguc.tests.arcus_uwcw.hagl_member/instances/202405230000"
    response = client.get(
        base + "/trajectory",
        params={
            "coords": "LINESTRING(3.4 52.5,7.9 52.5)",
            "parameter-name": "air_temperature_hagl",
            "member": member,
            "datetime": datetime,
            "z": "3",
        },
    )
    assert response.status_code == 200, response.text
    coverages = response.json()["coverages"]
    assert len(coverages) == count
    assert len({coverage["domain"]["custom:member"] for coverage in coverages}) == count
    assert all(coverage["domain"]["axes"]["z"]["values"] == [2] for coverage in coverages)
    AdagucTestTools().writetojson(f"{testresultspath}{filename}", json.dumps(response.json()))
    assert AdagucTestTools().compareJson(expectedoutputspath + filename, testresultspath + filename)


def test_edr_untyped_or_named_height():
    from routers.edr_covjson import point_response_to_covjson
    from routers.trajectory_covjson import sections_from_response
    from routers.utils.edr_exception import EdrException
    from routers.utils.edr_utils import get_vertical_query, supports_section

    layer = {"dims": {"time": {}, "height": {"type": "dimtype_other", "serviceName": "height", "hidden": False}}}
    metadata = {"collection": {"data": layer}}
    for z in (None, "3"):
        assert get_vertical_query(metadata, "collection", "data", z) == ""
    assert not supports_section(layer)
    sample = {"layername": "data", "point": {"coords": "5,52"}, "dims": ["height", "time"], "data": {"surface": {"2024-01-01": "1"}}}
    layer["dims"]["height"].update({"cdfName": "height", "type": "dimtype_vertical"})
    layer["dims"]["time"]["cdfName"] = "time"
    assert supports_section(layer)
    for convert in (point_response_to_covjson, sections_from_response):
        with pytest.raises(EdrException) as exc:
            convert([sample], {"data": layer}, "collection")
        assert exc.value.status_code == 400


ROTATED_POLE_CUBE_URL = "/edr/collections/adaguc.tests.rotatedpole_edr.rotatedpole/cube?bbox=4,51,7,53&parameter-name=pr_adjust"
# With crs=native the bbox is in the coordinates of the native grid, here rotated degrees
NATIVE_BBOX = (-1.0, -1.5, 0.5, 0.2)
ROTATED_POLE_NATIVE_CUBE_URL = (
    "/edr/collections/adaguc.tests.rotatedpole_edr.rotatedpole/cube?bbox=" + ",".join(str(v) for v in NATIVE_BBOX) + "&parameter-name=pr_adjust&crs=native"
)


def test_cube_native_grid(client: TestClient):
    """crs=native returns the cells of the rotated pole grid of the data, with its WKT, instead of resampling to EPSG:4326"""
    collection = client.get("/edr/collections/adaguc.tests.rotatedpole_edr.rotatedpole").json()
    assert collection["crs"] == ["EPSG:4326", "native"]
    crs_details = collection["data_queries"]["cube"]["link"]["variables"]["crs_details"]
    assert [crs_detail["crs"] for crs_detail in crs_details] == ["EPSG:4326", "native"]
    # A CF rotated pole, not the PROJ-only ob_tran method
    wkt = crs_details[1]["wkt"]
    assert "Pole rotation (netCDF CF convention)" in wkt and "ob_tran" not in wkt
    # The same pole as the grid_mapping of the file
    cf = CRS.from_wkt(wkt).to_cf()
    assert cf["grid_north_pole_latitude"] == pytest.approx(37.25)
    assert cf["grid_north_pole_longitude"] == pytest.approx(-174.55, abs=1e-4)

    resp = client.get(ROTATED_POLE_NATIVE_CUBE_URL)
    assert resp.status_code == 200, resp.text
    coverage = resp.json()
    # Without an EPSG id the id is left out, as CoverageJSON prescribes for an undefined CRS. The WKT is only in crs_details.
    assert coverage["domain"]["referencing"][0] == {"coordinates": ["x", "y"], "system": {"type": "GeographicCRS"}}

    # The x and y values are cell centers of the source grid, and every value is the value of that source cell
    source = netCDF4.Dataset(os.path.join(os.environ["ADAGUC_PATH"], "data", "datasets", "test_rotated_pole_RACMO.nc"))
    source_x, source_y = source.variables["x"][:], source.variables["y"][:]
    x = coverage["domain"]["axes"]["x"]["values"]
    y = coverage["domain"]["axes"]["y"]["values"]
    columns = [int(np.argmin(np.abs(source_x - value))) for value in x]
    rows = [int(np.argmin(np.abs(source_y - value))) for value in y]
    assert np.allclose(source_x[columns], x) and np.allclose(source_y[rows], y)
    assert 1 < len(x) < len(source_x) and 1 < len(y) < len(source_y)
    # Exactly the cells that intersect the native bbox
    half_x, half_y = abs(source_x[1] - source_x[0]) / 2, abs(source_y[1] - source_y[0]) / 2
    intersecting_x = [v for v in source_x if v + half_x > NATIVE_BBOX[0] and v - half_x < NATIVE_BBOX[2]]
    intersecting_y = [v for v in source_y if v + half_y > NATIVE_BBOX[1] and v - half_y < NATIVE_BBOX[3]]
    assert np.allclose(sorted(x), sorted(intersecting_x)) and np.allclose(sorted(y), sorted(intersecting_y))
    expected = source.variables["pr_adjust"][0][rows][:, columns].astype(float).filled(np.nan).flatten()
    values = np.array([np.nan if value is None else value for value in coverage["ranges"]["pr_adjust"]["values"]])
    assert np.allclose(values, expected, equal_nan=True)
    assert np.count_nonzero(~np.isnan(values)) > 10


def test_cube_native_grid_extent(client: TestClient):
    """The crs_details of crs=native give the extent of the native grid, and that bbox returns the whole grid"""
    collection = client.get("/edr/collections/adaguc.tests.rotatedpole_edr.rotatedpole").json()
    native = collection["data_queries"]["cube"]["link"]["variables"]["crs_details"][1]
    source = netCDF4.Dataset(os.path.join(os.environ["ADAGUC_PATH"], "data", "datasets", "test_rotated_pole_RACMO.nc"))
    source_x, source_y = source.variables["x"][:], source.variables["y"][:]
    cellsize_x, cellsize_y = abs(source_x[1] - source_x[0]), abs(source_y[1] - source_y[0])
    # The outer cell edges of the grid, in rotated degrees
    expected_bbox = [source_x.min() - cellsize_x / 2, source_y.min() - cellsize_y / 2, source_x.max() + cellsize_x / 2, source_y.max() + cellsize_y / 2]
    assert native["bbox"] == pytest.approx(expected_bbox, abs=1e-4)
    assert native["cellsize"] == pytest.approx([cellsize_x, cellsize_y], abs=1e-6)
    assert native["size"] == [len(source_x), len(source_y)]

    resp = client.get(ROTATED_POLE_NATIVE_CUBE_URL.replace(",".join(str(v) for v in NATIVE_BBOX), ",".join(str(v) for v in native["bbox"])))
    assert resp.status_code == 200, resp.text
    axes = resp.json()["domain"]["axes"]
    assert len(axes["x"]["values"]) == len(source_x) and len(axes["y"]["values"]) == len(source_y)


def test_cube_native_grid_errors(client: TestClient):
    resp = client.get(ROTATED_POLE_CUBE_URL + "&crs=EPSG:3857")
    assert resp.status_code == 400
    # Outside the native grid, and an inverted bbox
    for bbox in ("100,0,101,1", "0.5,0.2,-1,-1.5"):
        resp = client.get(ROTATED_POLE_CUBE_URL.replace("bbox=4,51,7,53", f"bbox={bbox}") + "&crs=native")
        assert resp.status_code == 400
    # The default is still EPSG:4326
    coverage = client.get(ROTATED_POLE_CUBE_URL).json()
    assert coverage["domain"]["referencing"][0]["system"]["id"] == "http://www.opengis.net/def/crs/EPSG/0/4326"


def test_collection_with_trailing_slash(client: TestClient):
    """Slash redirects are disabled, so /collections/{id}/ has its own route"""
    resp = client.get("/edr/collections/netcdf_5d.data_5d/")
    assert resp.status_code == 200, resp.text
    assert resp.json()["id"] == "netcdf_5d.data_5d"


def native_cells_in_bbox(centers, bbox_min: float, bbox_max: float) -> list:
    """The cell centers of a regular axis whose cells intersect [bbox_min, bbox_max]"""
    half = abs(float(centers[1]) - float(centers[0])) / 2
    return [float(v) for v in centers if v + half > bbox_min and v - half < bbox_max]


def test_cube_native_grid_instance(client: TestClient):
    """crs=native on the instance route gives the same native cells as on the collection route"""
    collection_url = "/edr/collections/adaguc.tests.arcus_uwcw.hagl_member"
    bbox = (400000, 6600000, 800000, 7100000)  # metres in the Mercator grid
    query = "/cube?crs=native&parameter-name=wind_speed_hagl_kts&bbox=" + ",".join(str(v) for v in bbox)

    instance_id = client.get(collection_url + "/instances").json()["instances"][0]["id"]
    resp = client.get(f"{collection_url}/instances/{instance_id}{query}")
    assert resp.status_code == 200, resp.text
    coverage = resp.json()
    # This Mercator grid has an EPSG code, so it is referenced by its id instead of WKT
    assert coverage["domain"]["referencing"][0]["system"] == {"type": "ProjectedCRS", "id": "http://www.opengis.net/def/crs/EPSG/0/3857"}

    source = netCDF4.Dataset(
        os.path.join(os.environ["ADAGUC_PATH"], "data", "datasets", "arcustest_uwcw", "uwcw_ha43ens_nl_2km_wind-speed-hagl_geoweb_nc_20240605T03_5x5.nc")
    )
    axes = coverage["domain"]["axes"]
    assert np.allclose(sorted(axes["x"]["values"]), native_cells_in_bbox(source.variables["x"][:], bbox[0], bbox[2]))
    assert np.allclose(sorted(axes["y"]["values"]), sorted(native_cells_in_bbox(source.variables["y"][:], bbox[1], bbox[3])))

    # The collection route uses the same (only) instance
    assert client.get(collection_url + query).json() == coverage


def test_cube_native_grid_bbox_on_cell_edges(client: TestClient):
    """A native bbox exactly on cell edges selects only the cells inside it, not the neighbouring ones"""
    source = netCDF4.Dataset(os.path.join(os.environ["ADAGUC_PATH"], "data", "datasets", "test_rotated_pole_RACMO.nc"))
    source_x, source_y = source.variables["x"][:], source.variables["y"][:]
    cellsize_x = (source_x[-1] - source_x[0]) / (len(source_x) - 1)
    cellsize_y = (source_y[-1] - source_y[0]) / (len(source_y) - 1)
    # The left edge of column 2 to the right edge of column 5, and the bottom edge of row 3 to the top edge of row 9
    bbox = (
        source_x[0] + (2 - 0.5) * cellsize_x,
        source_y[0] + (3 - 0.5) * cellsize_y,
        source_x[0] + (5 + 0.5) * cellsize_x,
        source_y[0] + (9 + 0.5) * cellsize_y,
    )
    resp = client.get(
        "/edr/collections/adaguc.tests.rotatedpole_edr.rotatedpole/cube?parameter-name=pr_adjust&crs=native&bbox=" + ",".join(repr(float(v)) for v in bbox)
    )
    assert resp.status_code == 200, resp.text
    axes = resp.json()["domain"]["axes"]
    assert np.allclose(sorted(axes["x"]["values"]), source_x[2:6])
    assert np.allclose(sorted(axes["y"]["values"]), sorted(source_y[3:10]))


def test_cube_native_grid_not_available(client: TestClient):
    """A collection on a lat/lon grid does not offer crs=native, and refuses it"""
    collection = client.get("/edr/collections/netcdf_5d.data_5d").json()
    assert collection["crs"] == ["EPSG:4326"]
    assert [crs_detail["crs"] for crs_detail in collection["data_queries"]["cube"]["link"]["variables"]["crs_details"]] == ["EPSG:4326"]

    resp = client.get("/edr/collections/netcdf_5d.data_5d/cube?bbox=4,51,7,53&crs=native")
    assert resp.status_code == 400
    assert "crs=native is not available" in resp.json()["description"]


def test_cube_explicit_default_crs(client: TestClient):
    """Asking for EPSG:4326 explicitly, also spelled as CRS84, gives the same as not asking for a crs"""
    resp = client.get(ROTATED_POLE_CUBE_URL + "&crs=CRS84")
    assert resp.status_code == 200, resp.text
    assert resp.json() == client.get(ROTATED_POLE_CUBE_URL).json()


def test_native_crs_is_only_for_cube_queries(client: TestClient):
    """The collection lists crs=native, which only cube queries support. Position and trajectory coords are lon/lat, so they refuse it."""
    rotated = "/edr/collections/adaguc.tests.rotatedpole_edr.rotatedpole"
    assert client.get(rotated).json()["crs"] == ["EPSG:4326", "native"]
    resp = client.get(rotated + "/position", params={"coords": "POINT(5 52)", "parameter-name": "pr_adjust", "crs": "native"})
    assert resp.status_code == 400
    assert "position" in resp.json()["description"]

    trajectory = "/edr/collections/testcollection.testcollection/instances/202406010000/trajectory"
    params = {"coords": "LINESTRING(5.1 52.1,8.1 52.1)", "parameter-name": "testdata", "datetime": "2024-06-01T01:00:00Z"}
    assert client.get(trajectory, params=params | {"crs": "native"}).status_code == 400
    # Like the cube, position accepts every spelling of EPSG:4326
    position = {"coords": "POINT(5 52)", "parameter-name": "pr_adjust"}
    assert client.get(rotated + "/position", params=position | {"crs": "CRS84"}).json() == client.get(rotated + "/position", params=position).json()


def test_cube_native_grid_cellsize():
    """resolution_x/resolution_y with crs=native: a cell size in native units, rounded to a whole number of native cells"""
    assert cellsize_to_step(None, 0.11) is None
    assert cellsize_to_step(0.33, 0.11) == 3
    assert cellsize_to_step(0.3, 0.11) == 3  # 2.7 cells
    assert cellsize_to_step(0.05, 0.11) == 1  # never less than one cell
    # A step larger than the cells gives the middle cell
    assert subsample_axis_by_step(0, 5, 20) == (0.0, 1, 5)
    assert subsample_axis_by_step(0, 26, 3) == subsample_axis(0, 26, 9)


def test_collection_native_crs_needs_one_grid():
    """crs=native is only advertised when all parameters share one native grid"""
    rotated = {"projstring": "+proj=ob_tran +o_proj=longlat +lon_0=-8.0 +o_lat_p=35.0 +o_lon_p=0.0 +a=6367470 +e=0 +no_defs"}
    lambert = {"projstring": "+proj=lcc +lat_0=52 +lon_0=5 +lat_1=52 +lat_2=52 +a=6371229 +b=6371229 +no_defs"}
    assert get_collection_native_crs_wkt([rotated, rotated]) is not None
    assert get_collection_native_crs_wkt([rotated, lambert]) is None
    assert get_collection_native_crs_wkt([rotated, None]) is None
    assert get_collection_native_crs_wkt([]) is None


def test_collection_native_crs_details_extent():
    """The native extent covers all grids; cellsize and size are only given when all parameters have the same grid"""
    projstring = "+proj=ob_tran +o_proj=longlat +lon_0=-8.0 +o_lat_p=35.0 +o_lon_p=0.0 +a=6367470 +e=0 +no_defs"
    # A bbox ordered top to bottom, like the gridspec of a grid with descending y
    grid_a = {"projstring": projstring, "bbox": [-10, 5, 10, -5], "width": 20, "height": 10, "cellsizex": 1, "cellsizey": -1}
    grid_b = {"projstring": projstring, "bbox": [0, -8, 20, 2], "width": 40, "height": 20, "cellsizex": 0.5, "cellsizey": 0.5}
    details = get_collection_native_crs_details([grid_a, grid_a])
    assert details["crs"] == "native" and details["wkt"] == get_collection_native_crs_wkt([grid_a])
    assert details["bbox"] == [-10, -5, 10, 5]
    assert details["cellsize"] == [1, 1] and details["size"] == [20, 10]
    details = get_collection_native_crs_details([grid_a, grid_b])
    assert details["bbox"] == [-10, -8, 20, 5]
    assert "cellsize" not in details and "size" not in details
    assert get_collection_native_crs_details([grid_a, {**grid_b, "projstring": "+proj=longlat +datum=WGS84"}]) is None


def test_native_crs_rotated_pole_without_o_lon_p_or_lon_0():
    """A rotated pole proj string without +o_lon_p or +lon_0 is valid for PROJ, which uses 0, and gives the same CF rotated pole"""
    without_o_lon_p = {"projstring": "+proj=ob_tran +o_proj=longlat +o_lat_p=35.0 +lon_0=-8.0 +a=6367470.0 +e=0 +no_defs"}
    with_o_lon_p = {"projstring": "+proj=ob_tran +o_proj=longlat +lon_0=-8.0 +o_lat_p=35.0 +o_lon_p=0.0 +a=6367470 +e=0 +no_defs"}
    wkt = get_collection_native_crs_wkt([without_o_lon_p])
    assert "Pole rotation (netCDF CF convention)" in wkt
    assert wkt == get_collection_native_crs_wkt([with_o_lon_p])

    without_lon_0 = {"projstring": "+proj=ob_tran +o_proj=longlat +o_lat_p=35.0 +a=6367470.0 +e=0 +no_defs"}
    with_lon_0 = {"projstring": "+proj=ob_tran +o_proj=longlat +lon_0=0 +o_lat_p=35.0 +o_lon_p=0 +a=6367470.0 +e=0 +no_defs"}
    wkt = get_collection_native_crs_wkt([without_lon_0])
    assert "Pole rotation (netCDF CF convention)" in wkt
    assert wkt == get_collection_native_crs_wkt([with_lon_0])


def test_covjson_axis_order_follows_epsg():
    """Geographic EPSG CRSs have latitude first, like EPSG:4326; projected ones and CRSs without EPSG code have x first"""
    assert get_epsg_axis_order(4258) == ["y", "x"]
    assert get_epsg_axis_order(28992) == ["x", "y"]
    assert get_epsg_axis_order(None) == ["x", "y"]


def test_subsample_axis():
    """Every n-th cell, for at most the requested number of positions, with each output centre on a native cell centre"""
    # No subsampling: no resolution, 0, or at least as many positions as cells
    for positions in (None, 0, 26, 30):
        assert subsample_axis(0, 26, positions) == (0.0, 26, 1)
    for start, end, positions in ((0, 26, 9), (0, 26, 7), (3, 20, 4), (5, 9, 1), (0, 28, 10), (2, 101, 5)):
        first_edge, count, step = subsample_axis(start, end, positions)
        assert step > 1 and count <= positions
        middle = (step - 1) // 2
        for k in range(count):
            centre = first_edge + (k + 0.5) * step
            assert centre == pytest.approx(start + k * step + middle + 0.5)  # the centre of a native cell
            assert start <= start + k * step + middle < end  # inside the requested cells
        # No room for one more block: the next taken cell would be outside
        assert start + count * step + middle >= end
    # Odd steps start on the first cell edge, even steps half a cell before it
    assert subsample_axis(0, 26, 9) == (0.0, 9, 3)
    assert subsample_axis(0, 26, 7) == (-0.5, 7, 4)


ROTATED_POLE_COLLECTION_URL = "/edr/collections/adaguc.tests.rotatedpole_edr.rotatedpole"


@pytest.mark.parametrize(("positions", "step"), [((9, 10), (3, 3)), ((7, 7), (4, 4))])
def test_cube_native_grid_resolution(client: TestClient, positions, step):
    """resolution-x/-y with crs=native: every n-th cell of the native grid, exactly the source cells, without interpolation"""
    collection = client.get(ROTATED_POLE_COLLECTION_URL).json()
    native = next(d for d in collection["data_queries"]["cube"]["link"]["variables"]["crs_details"] if d["crs"] == "native")
    resp = client.get(
        ROTATED_POLE_COLLECTION_URL + "/cube?parameter-name=pr_adjust&crs=native&bbox=" + ",".join(repr(float(v)) for v in native["bbox"])
        + f"&resolution-x={positions[0]}&resolution-y={positions[1]}"
    )
    assert resp.status_code == 200, resp.text
    coverage = resp.json()
    source = netCDF4.Dataset(os.path.join(os.environ["ADAGUC_PATH"], "data", "datasets", "test_rotated_pole_RACMO.nc"))
    source_x, source_y = source.variables["x"][:], source.variables["y"][:]  # both ascending in this file
    # The middle cell of every block of `step` cells (the lower middle one for an even step)
    columns = list(range((step[0] - 1) // 2, len(source_x), step[0]))
    rows = list(range((step[1] - 1) // 2, len(source_y), step[1]))
    x, y = coverage["domain"]["axes"]["x"]["values"], coverage["domain"]["axes"]["y"]["values"]
    assert len(x) <= positions[0] and len(y) <= positions[1]
    assert np.allclose(sorted(x), source_x[columns]) and np.allclose(sorted(y), source_y[rows])
    # Every value is the value of its source cell
    returned_rows = [int(np.argmin(np.abs(source_y - value))) for value in y]
    returned_columns = [int(np.argmin(np.abs(source_x - value))) for value in x]
    expected = source.variables["pr_adjust"][0][returned_rows][:, returned_columns].astype(float).filled(np.nan).flatten()
    values = np.array([np.nan if value is None else value for value in coverage["ranges"]["pr_adjust"]["values"]], dtype=float)
    assert np.allclose(values, expected, equal_nan=True)


@pytest.mark.parametrize("positions", ["0", "500"])
def test_cube_native_grid_resolution_all_cells(client: TestClient, positions):
    """resolution-x/-y of 0, or more positions than cells, give every cell of the native grid"""
    plain = client.get(ROTATED_POLE_NATIVE_CUBE_URL).json()
    resp = client.get(ROTATED_POLE_NATIVE_CUBE_URL + f"&resolution-x={positions}&resolution-y={positions}")
    assert resp.status_code == 200, resp.text
    assert resp.json()["domain"]["axes"] == plain["domain"]["axes"]


def test_cube_resolution_positions_epsg4326(client: TestClient):
    """Without crs=native, resolution-x/-y are the width and height of the EPSG:4326 grid"""
    resp = client.get(ROTATED_POLE_CUBE_URL + "&resolution-x=12&resolution-y=8")
    assert resp.status_code == 200, resp.text
    axes = resp.json()["domain"]["axes"]
    assert len(axes["x"]["values"]) == 12 and len(axes["y"]["values"]) == 8


def test_cube_coordinates_have_no_storage_noise(client: TestClient):
    """The coordinates of the axes are written with the digits they have, like -13.35 instead of -13.350000000000001"""
    for url in (ROTATED_POLE_CUBE_URL, ROTATED_POLE_NATIVE_CUBE_URL):
        resp = client.get(url)
        assert resp.status_code == 200, resp.text
        axes = resp.json()["domain"]["axes"]
        for axis in ("x", "y"):
            assert len(axes[axis]["values"]) > 1
            for value in axes[axis]["values"]:
                assert float(f"{value:.12g}") == value


def test_cube_native_grid_resolution_one_position(client: TestClient):
    """resolution-x=1&resolution-y=1 with crs=native returns one native cell: the middle cell of the grid, not a resampled grid"""
    collection = client.get(ROTATED_POLE_COLLECTION_URL).json()
    native = next(d for d in collection["data_queries"]["cube"]["link"]["variables"]["crs_details"] if d["crs"] == "native")
    resp = client.get(
        ROTATED_POLE_COLLECTION_URL + "/cube?parameter-name=pr_adjust&crs=native&bbox=" + ",".join(repr(float(v)) for v in native["bbox"])
        + "&resolution-x=1&resolution-y=1"
    )
    assert resp.status_code == 200, resp.text
    coverage = resp.json()
    source = netCDF4.Dataset(os.path.join(os.environ["ADAGUC_PATH"], "data", "datasets", "test_rotated_pole_RACMO.nc"))
    source_x, source_y = source.variables["x"][:], source.variables["y"][:]
    column, row = (len(source_x) - 1) // 2, (len(source_y) - 1) // 2
    assert coverage["domain"]["axes"]["x"]["values"] == pytest.approx([source_x[column]])
    assert coverage["domain"]["axes"]["y"]["values"] == pytest.approx([source_y[row]])
    expected = source.variables["pr_adjust"][0][row][column]
    value = coverage["ranges"]["pr_adjust"]["values"][0]
    assert (value is None) if np.ma.is_masked(expected) else value == pytest.approx(float(expected))


def test_cube_resolution_positions_epsg4326_one_position(client: TestClient):
    """resolution-x=1&resolution-y=1 without crs=native is a grid of one cell, not the default grid"""
    resp = client.get(ROTATED_POLE_CUBE_URL + "&resolution-x=1&resolution-y=1")
    assert resp.status_code == 200, resp.text
    axes = resp.json()["domain"]["axes"]
    assert len(axes["x"]["values"]) == 1 and len(axes["y"]["values"]) == 1


def test_cube_native_grid_resolution_cellsize(client: TestClient):
    """resolution_x/resolution_y with crs=native give the same native cells as the number of positions with the same step"""
    collection = client.get(ROTATED_POLE_COLLECTION_URL).json()
    native = next(d for d in collection["data_queries"]["cube"]["link"]["variables"]["crs_details"] if d["crs"] == "native")
    whole_grid_url = (
        ROTATED_POLE_COLLECTION_URL + "/cube?parameter-name=pr_adjust&crs=native&bbox=" + ",".join(repr(float(v)) for v in native["bbox"])
    )
    # The whole grid has 26 x 28 cells of 0.11 rotated degrees: 9 and 10 positions are steps of 3 cells, as 0.33 degrees
    by_positions = client.get(whole_grid_url + "&resolution-x=9&resolution-y=10").json()
    by_cellsize = client.get(whole_grid_url + "&resolution_x=0.33&resolution_y=0.33")
    assert by_cellsize.status_code == 200, by_cellsize.text
    assert by_cellsize.json()["domain"]["axes"] == by_positions["domain"]["axes"]
    assert by_cellsize.json()["ranges"] == by_positions["ranges"]
    # One axis only: the other axis keeps every cell
    plain = client.get(whole_grid_url).json()["domain"]["axes"]
    one_axis = client.get(whole_grid_url + "&resolution_x=0.33").json()["domain"]["axes"]
    assert one_axis["x"] == by_positions["domain"]["axes"]["x"] and one_axis["y"] == plain["y"]
    # A cell size larger than the bbox gives one cell
    one_cell = client.get(whole_grid_url + "&resolution_x=100&resolution_y=100").json()["domain"]["axes"]
    assert len(one_cell["x"]["values"]) == 1 and len(one_cell["y"]["values"]) == 1


@pytest.mark.parametrize("cellsize", ["2.5", "100", "1e300"])
def test_cube_resolution_cellsize_epsg4326_larger_than_bbox(client: TestClient, cellsize):
    """Without crs=native, a cell size larger than the bbox (3 x 2 degrees) gives one cell, as with crs=native, not an empty grid"""
    resp = client.get(ROTATED_POLE_CUBE_URL + f"&resolution_x={cellsize}&resolution_y={cellsize}")
    assert resp.status_code == 200, resp.text
    axes = resp.json()["domain"]["axes"]
    # One cell, with its centre in the middle of the bbox
    assert axes["x"]["values"] == pytest.approx([5.5]) and axes["y"]["values"] == pytest.approx([52.0])


@pytest.mark.parametrize("resolution", ["resolution-x=5", "resolution-y=5", "resolution_x=0.5", "resolution_y=0.5"])
def test_trajectory_refuses_resolution(client: TestClient, resolution):
    """A trajectory samples the native grid cells it crosses, so both kinds of resolution parameters are refused"""
    resp = client.get(f"/edr/collections/testcollection.testcollection/trajectory?coords=LINESTRING(4 51,7 53)&{resolution}")
    assert resp.status_code == 400, resp.text
    assert "resolution" in resp.json()["description"]


def test_openapi_describes_invalid_parameters_as_400(client: TestClient):
    """The OpenAPI document describes the 400 that EDR returns for an invalid query parameter, not FastAPI's 422"""
    for url in ("/edr/api", "/edr/openapi.json"):
        api = client.get(url).json()
        cube = api["paths"]["/collections/{collection_name}/cube"]["get"]
        assert "422" not in cube["responses"]
        assert cube["responses"]["400"]["content"]["application/json"]["schema"] == {"$ref": "#/components/schemas/EdrError"}
        assert "EdrError" in api["components"]["schemas"] and "HTTPValidationError" not in api["components"]["schemas"]
        assert all("422" not in operation["responses"] for path in api["paths"].values() for operation in path.values())


@pytest.mark.parametrize(
    ("query", "same_as"),
    [
        # 0 is the same as leaving it out, also for one axis only
        (ROTATED_POLE_CUBE_URL + "&resolution-x=0", ROTATED_POLE_CUBE_URL),
        (ROTATED_POLE_CUBE_URL + "&resolution-x=0&resolution-y=0", ROTATED_POLE_CUBE_URL),
        # and can be combined with resolution_x/resolution_y
        (ROTATED_POLE_CUBE_URL + "&resolution-x=0&resolution_x=0.5&resolution_y=0.5", ROTATED_POLE_CUBE_URL + "&resolution_x=0.5&resolution_y=0.5"),
    ],
)
def test_cube_resolution_positions_zero_is_default(client: TestClient, query, same_as):
    resp = client.get(query)
    assert resp.status_code == 200, resp.text
    assert resp.json()["domain"]["axes"] == client.get(same_as).json()["domain"]["axes"]


@pytest.mark.parametrize(
    ("query", "status_code"),
    [
        (ROTATED_POLE_CUBE_URL + "&resolution-x=12&resolution-y=8&resolution_x=0.5&resolution_y=0.5", 400),  # both kinds
        (ROTATED_POLE_CUBE_URL + "&resolution-x=12", 400),  # only one, without crs=native
        (ROTATED_POLE_CUBE_URL + "&resolution-x=12&resolution-y=0", 400),  # 0 is the same as leaving it out
        (ROTATED_POLE_NATIVE_CUBE_URL + "&resolution-x=-1", 400),  # negative: invalid values return 400, as EDR expects
        (ROTATED_POLE_CUBE_URL + "&resolution-x=30000&resolution-y=2", 400),  # above the 20000 adaguc-server can write
        (ROTATED_POLE_NATIVE_CUBE_URL + "&resolution-y=20001", 400),
        (ROTATED_POLE_CUBE_URL + "&resolution-x=abc&resolution-y=2", 400),  # not a number
        (ROTATED_POLE_NATIVE_CUBE_URL + "&resolution_x=0", 400),  # a cell size must be larger than 0
        (ROTATED_POLE_NATIVE_CUBE_URL + "&resolution_x=inf", 400),  # and finite
        (ROTATED_POLE_CUBE_URL + "&resolution_x=inf&resolution_y=inf", 400),
        (ROTATED_POLE_CUBE_URL + "&resolution_x=nan&resolution_y=nan", 400),
        (ROTATED_POLE_CUBE_URL + "&resolution_x=0.5", 400),  # only one, without crs=native
        (ROTATED_POLE_NATIVE_CUBE_URL + "&resolution-x=9&resolution_y=0.33", 400),  # both kinds, on different axes
    ],
)
def test_cube_resolution_positions_errors(client: TestClient, query, status_code):
    resp = client.get(query)
    assert resp.status_code == status_code, resp.text
    assert resp.json()["code"] == str(status_code) and resp.json()["description"]
