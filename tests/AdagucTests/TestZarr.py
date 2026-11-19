"""
Tests the Zarr reader: the same data from a NetCDF file and from Zarr stores with different encodings (see
data/datasets/zarr/create_zarr_test_stores.py) must give the same images and values.
"""

import os
import json

import pytest
from adaguc.AdagucTestTools import AdagucTestTools

from conftest import make_adaguc_env, update_db

ADAGUC_PATH = os.environ["ADAGUC_PATH"]

ZARR_STORES = ["zlib", "gzip", "zstd", "blosc", "uncompressed"]
VARIABLES = ["temperature", "reflectivity"]
# Every time step and level; level 0.5 km has chunks that only hold the fill value, level 1 km is only fill value
TIMES_AND_ELEVATIONS = [("2024-05-28T00:00:00Z", "0.5"), ("2024-05-28T00:05:00Z", "1"), ("2024-05-28T00:05:00Z", "1.5")]


class TestZarr:
    testresultspath = "testresults/TestZarr/"

    AdagucTestTools().mkdir_p(testresultspath)

    def get(self, layer: str, query: str):
        env = make_adaguc_env("{ADAGUC_PATH}/data/config/datasets/adaguc.tests.zarr.xml")
        update_db(env)
        status, data, _ = AdagucTestTools().runADAGUCServer(f"&SERVICE=WMS&VERSION=1.3.0&LAYERS={layer}&QUERY_LAYERS={layer}&{query}", env=env)
        assert status == 0
        return data.getvalue() if hasattr(data, "getvalue") else data

    def get_map(self, layer: str, time: str, elevation: str):
        return self.get(
            layer,
            f"REQUEST=GetMap&WIDTH=200&HEIGHT=160&CRS=EPSG%3A4326&BBOX=49.5,1.5,56.5,10.5&STYLES=&FORMAT=image/png&TRANSPARENT=TRUE&TIME={time}&ELEVATION={elevation}",
        )

    @pytest.mark.parametrize("store", ZARR_STORES)
    @pytest.mark.parametrize("variable", VARIABLES)
    @pytest.mark.parametrize(("time", "elevation"), TIMES_AND_ELEVATIONS)
    def test_zarr_getmap_is_the_same_as_netcdf(self, store: str, variable: str, time: str, elevation: str):
        expected = self.get_map(f"nc_{variable}", time, elevation)
        actual = self.get_map(f"{store}_{variable}", time, elevation)
        filename = f"{store}_{variable}_{time}_{elevation}.png".replace(":", "_")
        AdagucTestTools().writetofile(self.testresultspath + "nc_" + filename, expected)
        AdagucTestTools().writetofile(self.testresultspath + filename, actual)
        assert actual == expected

    @pytest.mark.parametrize("store", ZARR_STORES)
    @pytest.mark.parametrize("variable", VARIABLES)
    def test_zarr_getfeatureinfo_is_the_same_as_netcdf(self, store: str, variable: str):
        query = "REQUEST=GetFeatureInfo&WIDTH=200&HEIGHT=160&CRS=EPSG%3A4326&BBOX=49.5,1.5,56.5,10.5&I=150&J=40&INFO_FORMAT=application/json&TIME=*&ELEVATION=*"
        expected = json.loads(self.get(f"nc_{variable}", query))
        actual = json.loads(self.get(f"{store}_{variable}", query))
        assert len(actual) == 1 and len(expected) == 1
        assert actual[0]["data"] == expected[0]["data"]
        assert actual[0]["data"]  # There are values to compare

    def test_zarr_getcapabilities(self):
        data = self.get("nc_temperature", "REQUEST=GetCapabilities").decode()
        for store in ["nc"] + ZARR_STORES:
            for variable in VARIABLES:
                assert f"<Name>{store}_{variable}</Name>" in data
