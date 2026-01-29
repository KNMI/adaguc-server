# pylint: disable=invalid-name,missing-function-docstring
"""
This class contains tests to test the adaguc-server binary executable file. This is similar to black box testing, it tests the behaviour of the server software. It configures the server and checks if the response is OK.
"""
import os
import json
import unittest
from adaguc.AdagucTestTools import AdagucTestTools


class TestEWCLocalClimateInfo(unittest.TestCase):
    """
    TestEWCLocalClimateInfo class to thest Web Map Service behaviour of adaguc-server.
    """

    testresultspath = "testresults/TestEWCLocalClimateInfo/"
    expectedoutputsspath = "expectedoutputs/TestEWCLocalClimateInfo/"
    AdagucTestTools().mkdir_p(testresultspath)
    adaguc_path = os.environ["ADAGUC_PATH"]
    env = {"ADAGUC_CONFIG": adaguc_path + "/data/config/adaguc.tests.dataset.xml"}

    def scan_this_dataset(self):
        AdagucTestTools().cleanTempDir()
        status, _, _ = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                self.adaguc_path
                + "/data/config/adaguc.tests.dataset.xml,adaguc_ewclocalclimateinfo_test.xml",
            ],
            env=self.env,
            isCGI=False,
        )
        self.assertEqual(status, 0)

    def test_EWCLocalClimateInfo_GetMap_Preview(self):
        self.scan_this_dataset()
        filename = "test_EWCLocalClimateInfo_GetMap_Preview.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc_ewclocalclimateinfo_test&&service=WMS&request=getmap&format=image/png&layers=jaarlijks/tas_mean_2050&width=600&CRS=EPSG:4326&STYLES=&EXCEPTIONS=INIMAGE&showlegend=true",
            env=self.env,
        )
        self.assertEqual(status, 0)

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename,
                self.testresultspath + filename,
                28,
                0.6,
            )
        )

    def test_EWCLocalClimateInfo_GetFeatureInfo_Two_Scenarios(self):
        self.scan_this_dataset()
        filename = "test_EWCLocalClimateInfo_GetFeatureInfo_Two_Scenarios.json"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc_ewclocalclimateinfo_test&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=jaarlijks%2Ftas_mean_2050&query_layers=jaarlijks%2Ftas_mean_2050&crs=EPSG%3A3857&bbox=468851.3452508886%2C6508030.983054844%2C778490.0636045887%2C7004884.352286632&width=822&height=1319&i=409&j=500&info_format=application%2Fjson&time=*&dim_scenario=*",
            env=self.env,
        )
        self.assertEqual(status, 0)
        AdagucTestTools().writetofile(self.testresultspath + filename, json.dumps( json.loads(data.getvalue().decode("utf-8")), indent=2).encode('utf-8'))
        self.assertEqual(
            AdagucTestTools().readfromfile(self.testresultspath + filename),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_EWCLocalClimateInfo_GetPointValue_Two_Scenarios(self):
        self.scan_this_dataset()
        filename = "test_EWCLocalClimateInfo_GetPointValue_Two_Scenarios.json"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc_ewclocalclimateinfo_test&service=WMS&request=GetPointValue&version=1.3.0&layers=jaarlijks%2Ftas_mean_2050&query_layers=jaarlijks%2Ftas_mean_2050&info_format=application%2Fjson&time=*&dim_scenario=*&CRS=EPSG:4326&X=5&Y=52",
            env=self.env,
        )
        self.assertEqual(status, 0)
        AdagucTestTools().writetofile(self.testresultspath + filename, json.dumps( json.loads(data.getvalue().decode("utf-8")), indent=2).encode('utf-8'))
        self.assertEqual(
            AdagucTestTools().readfromfile(self.testresultspath + filename),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    # def test_EWCLocalClimateInfo_GetWCSCoverage_Two_Scenarios(self):
    #     self.scan_this_dataset()
    #     filename = "test_EWCLocalClimateInfo_GetWCSCoverage_Two_Scenarios.nc"
    #     status, data, _ = AdagucTestTools().runADAGUCServer(
    #         "dataset=adaguc_ewclocalclimateinfo_test&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=jaarlijks/tas_mean_2050&CRS=EPSG%3A28992&FORMAT=netcdf&BBOX=0,290000,300000,640000&RESX=50000&RESY=50000&TIME=*&DIM_SCENARIO=*",
    #         env=self.env,
    #     )
    #     self.assertEqual(status, 0)
