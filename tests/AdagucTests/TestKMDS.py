import json
import os
from io import BytesIO
from adaguc.CGIRunner import CGIRunner
import unittest
import shutil
import sys
import subprocess
from lxml import etree
from lxml import objectify
import re
from adaguc.AdagucTestTools import AdagucTestTools

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestKMDS(unittest.TestCase):
    testresultspath = "testresults/TestKMDS/"
    expectedoutputsspath = "expectedoutputs/TestKMDS/"
    env = {
        "ADAGUC_CONFIG": ADAGUC_PATH
        + "/data/config/adaguc.tests.dataset.xml,"
        + ADAGUC_PATH
        + "/data/config/datasets/adaguc.testGeoJSONReader_time.xml"
    }

    AdagucTestTools().mkdir_p(testresultspath)

    def test_kmds_alle_stations_10001_getmap_requests(self):
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,adaguc.test.kmds_alle_stations_10001.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        test_cases = [
            {
                "filetocheck": "test_kmds_alle_stations_10001_10Mwind_barballpoint.png",
                "layers": "10M%2Fwind",
                "styles": "barball%2Fpoint",
            },
            {
                "filetocheck": "test_kmds_alle_stations_10001_10Mwind_barbthinnedpoint.png",
                "layers": "10M%2Fwind",
                "styles": "barbthinned%2Fpoint",
            },
            {
                "filetocheck": "test_kmds_alle_stations_10001_10Mwind_barbpoint.png",
                "layers": "10M%2Fwind",
                "styles": "barb%2Fpoint",
            },
            {
                "filetocheck": "test_kmds_alle_stations_10001_10Mwind_barbdisc.png",
                "layers": "10M%2Fwind",
                "styles": "barbdisc%2Fpoint",
            },
            {
                "filetocheck": "test_kmds_alle_stations_10001_10Mwind_barbvector.png",
                "layers": "10M%2Fwind",
                "styles": "barbvector%2Fpoint",
            },
            {
                "filetocheck": "test_kmds_alle_stations_10001_10Mwind_barballvector.png",
                "layers": "10M%2Fwind",
                "styles": "barballvector%2Fpoint",
            },
            {
                "filetocheck": "test_kmds_alle_stations_10001_10Mwindbft_bftalldisc.png",
                "layers": "10M%2Fwind_bft",
                "styles": "bftalldisc%2Fpoint",
            },
            {
                "filetocheck": "test_kmds_alle_stations_10001_10Mwindms_barballpoint.png",
                "layers": "10M%2Fwind_mps",
                "styles": "barballpoint%2Fpoint",
            },
            {
                "filetocheck": "test_kmds_alle_stations_10001_10Mta_observation-temperature.png",
                "layers": "10M%2Fta",
                "styles": "observation.temperature%2Fpoint",
            },
            {
                "filetocheck": "test_kmds_alle_stations_10001_10M_animgif_ta_temperaturedisc.png",
                "layers": "10M%2Fta",
                "styles": "temperaturedisc%2Fpoint",
            },  # Animated gif KDP_WWWRADARTEMP_loop
            {
                "filetocheck": "test_kmds_alle_stations_10001_10M_animgif_windbft_bftdiscbarb.png",
                "layers": "10M%2Fwind_bft",
                "styles": "bftdisc%2Fbarb",
            },  # Animated gif KDP_WWWRADARBFT_loop, still uses rendermethod barb
            {
                "filetocheck": "test_kmds_alle_stations_10001_10M_animgif_windmps_barbdiscbarb.png",
                "layers": "10M%2Fwind_mps",
                "styles": "barbdisc%2Fbarb",
            },  # Animated gif KDP_WWWRADARWIND_loop, still uses rendermethod barb
            {
                "filetocheck": "test_kmds_alle_stations_10001_10M_nc_symbolinterval_okta.png",
                "layers": "10M%2Fnc",
                "styles": "observation.okta",
            },  # <SymbolInterval
        ]
        for test_case in test_cases:
            filename = test_case["filetocheck"]
            status, data, headers = AdagucTestTools().runADAGUCServer(
                f"LAYERS=baselayer,{test_case['layers']},overlay&STYLES=default,{test_case['styles']},default&DATASET=adaguc.test.kmds_alle_stations_10001&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&WIDTH=400&HEIGHT=600&CRS=EPSG%3A3857&BBOX=269422.313123934,6357145.5563671775,939865.5563671777,7457638.879961043&FORMAT=image/png32&TRANSPARENT=FALSE&&time=2025-11-06T09%3A20%3A00Z",
                env=env,
                showLog=False,
            )
            AdagucTestTools().writetofile(
                self.testresultspath + filename, data.getvalue()
            )

            self.assertEqual(status, 0)
            self.assertTrue(
                AdagucTestTools().compareImage(
                    self.expectedoutputsspath + filename,
                    self.testresultspath + filename,
                    30,
                )
            )

    def test_kmds_alle_stations_10001_getfeatureinfo_requests(self):
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,adaguc.test.kmds_alle_stations_10001.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        test_cases = [
            {
                "filetocheck": "test_kmds_alle_stations_10001_10M_animgif_wind.json",
                "layers": "10M%2Fwind",
                "units": "kts",
            },  # Should be kts
            {
                "filetocheck": "test_kmds_alle_stations_10001_10M_animgif_windbft.json",
                "layers": "10M%2Fwind_bft",
                "units": "bft",
            },  # Should be bft
            {
                "filetocheck": "test_kmds_alle_stations_10001_10M_animgif_windmps.json",
                "layers": "10M%2Fwind_mps",
                "units": "m s-1",
            },  # Should be m/s
        ]
        for test_case in test_cases:
            filename = test_case["filetocheck"]
            status, data, headers = AdagucTestTools().runADAGUCServer(
                f"dataset=adaguc.test.kmds_alle_stations_10001&&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS={test_case['layers']}&QUERY_LAYERS={test_case['layers']}&CRS=EPSG%3A3857&BBOX=-141702.05839427316,6126251.383284947,1317478.0003938763,7367966.542989185&WIDTH=1550&HEIGHT=1319&I=765&J=585&FORMAT=image/gif&INFO_FORMAT=application/json&STYLES=&&time=2025-11-06T09%3A20%3A00Z",
                env=env,
                showLog=False,
            )
            AdagucTestTools().writetojson(
                self.testresultspath + filename, data.getvalue()
            )
            self.assertEqual(status, 0)
            self.assertEqual(
                json.loads(data.getvalue())[0]["units"], test_case["units"]
            )
            # TODO: Value does not seem to be stored in the json.
            self.assertTrue(
                AdagucTestTools().compareFile(
                    self.expectedoutputsspath + filename,
                    self.testresultspath + filename,
                )
            )

    def test_kmds_alle_stations_10001_getmetadata_adjusted_units(self):
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,adaguc.test.kmds_alle_stations_10001.xml"
        )
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.test.kmds_alle_stations_10001&&service=WMS&request=getmetadata&format=application/json&layers=10M/wind_adjust",
            env=env,
            showLog=False,
        )
        self.assertEqual(status, 0)
        layer = json.loads(data.getvalue())["adaguc.test.kmds_alle_stations_10001"][
            "10M/wind_adjust"
        ]["layer"]
        self.assertEqual(layer["variables"][0]["units"], "mym s -1")
        self.assertEqual(layer["variables"][1]["units"], "mydegrees")
