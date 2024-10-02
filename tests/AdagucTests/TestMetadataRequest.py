# pylint: disable=invalid-name,missing-function-docstring
"""
This class contains tests to test the adaguc-server binary executable file. This is similar to black box testing, it tests the behaviour of the server software. It configures the server and checks if the response is OK.
"""
import json
import os
import os.path
import unittest
import sys
from adaguc.AdagucTestTools import AdagucTestTools

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestMetadataRequest(unittest.TestCase):
    """
    TestMetadataRequest class to thest Web Map Service behaviour of adaguc-server.
    """

    testresultspath = "testresults/TestMetadataRequest/"
    expectedoutputsspath = "expectedoutputs/TestMetadataRequest/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_timeseries_adaguc_tests_arcus_uwcw_air_temperature_hagl(self):
        AdagucTestTools().cleanTempDir()
        if os.getenv("ADAGUC_DB").endswith(".db"):
            print("SKIP: Only PSQL")
            return
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.arcus_uwcw.xml"
        )
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        filename = "test_GetMetadataRequest_arcus_uwcw.json"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.arcus_uwcw&service=WMS&request=GetMetadata&format=application/json",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )


