"""
Tests point data split over several files with the same time: GetFeatureInfo must find the point in whichever file it is.
The files contain the stations of kmds_alle_stations_10001_202511060920.nc, see data/config/datasets/adaguc.tests.multifilepoints.xml.
"""

import json
import pytest
from adaguc.AdagucTestTools import AdagucTestTools

from conftest import make_adaguc_env, update_db

GET_FEATURE_INFO_QUERY = (
    "dataset=adaguc.tests.multifilepoints&SERVICE=WMS&VERSION=1.1.1&REQUEST=GetFeatureInfo&LAYERS=ta&QUERY_LAYERS=ta&SRS=EPSG:4326"
    "&BBOX=2,50,8,56&WIDTH=600&HEIGHT=600&STYLES=&time=2025-11-06T09:20:00Z&INFO_FORMAT=application/json"
)


class TestMultiFilePoints:
    testresultspath = "testresults/TestMultiFilePoints/"
    expectedoutputsspath = "expectedoutputs/TestMultiFilePoints/"

    AdagucTestTools().mkdir_p(testresultspath)

    def test_multifilepoints_getmap(self):
        """The stations of all files are drawn"""
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("adaguc.tests.multifilepoints.xml")
        update_db(env)

        filename = "test_multifilepoints_getmap.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.multifilepoints&SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&LAYERS=ta&STYLES=observation.temperature%2Fpoint"
            "&SRS=EPSG:4326&BBOX=2,50,8,56&WIDTH=256&HEIGHT=256&FORMAT=image/png&TRANSPARENT=FALSE&time=2025-11-06T09:20:00Z",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data)
        assert status == 0
        assert AdagucTestTools().compareImage(
            self.expectedoutputsspath + filename, self.testresultspath + filename, tolerance="antialiasing"
        )

    @pytest.mark.parametrize(
        ("station", "x", "y", "expected"),
        [
            ("VOORSCHOTEN AWS (first file)", 244, 386, "10.300000"),
            ("AMSTERDAM/SCHIPHOL AP (second file)", 279, 368, "11.000000"),
            ("MARKNESSE AWS (third file)", 389, 330, "10.500000"),
            ("HOEK VAN HOLLAND AWS (last file)", 212, 401, "12.800000"),
            ("No station nearby", 590, 590, "nodata"),
        ],
    )
    def test_multifilepoints_getfeatureinfo(self, station: str, x: int, y: int, expected: str):
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("adaguc.tests.multifilepoints.xml")
        update_db(env)

        status, data, _ = AdagucTestTools().runADAGUCServer(f"{GET_FEATURE_INFO_QUERY}&X={x}&Y={y}", env=env)
        assert status == 0, station
        assert json.loads(data)[0]["data"] == {"2025-11-06T09:20:00Z": expected}, station
