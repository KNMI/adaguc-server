# pylint: disable=invalid-name,missing-function-docstring
"""
This class contains tests to test the adaguc-server binary executable file. This is similar to black box testing, it tests the behaviour of the server software. It configures the server and checks if the response is OK.
"""
import os
import os.path
import unittest
import json
import re
import datetime
from adaguc.AdagucTestTools import AdagucTestTools
from lxml import etree, objectify

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestConvertLatLonBnds(unittest.TestCase):
    """
    TestConvertLatLonBnds class to test handling of latlonbnds
    files by ADAGUC.
    """

    testresultspath = "testresults/TestConvertLatLonBnds/"
    expectedoutputsspath = "expectedoutputs/TestConvertLatLonBnds/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_ConvertLatLonBnds_getCapabilities(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_ConvertLatLonBnds_getCapabilities.xml"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=example_file_latlonbnds.nc&SERVICE=WMS&request=getcapabilities",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareGetCapabilitiesXML(
                self.testresultspath + filename, self.expectedoutputsspath + filename
            )
        )

    def test_ConvertLatLonBnds_getMap(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_ConvertLatLonBnds_getMap.png"
        # pylint: disable=unused-variable

        AdagucTestTools().runADAGUCServer(
            "source=example_file_latlonbnds.nc&SERVICE=WMS&request=getcapabilities",
            env=self.env,
        )
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=example_file_latlonbnds.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=probability&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=50,2,57,9&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&showlegend=true&DIM_threshold=20&time=2024-06-01T00:00:00Z",
            env=self.env,
            args=["--report"],
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename, self.testresultspath + filename, 8
            )
        )

    def test_ConvertLatLonBnds_getFeatureInfo(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_ConvertLatLonBnds_getFeatureInfo.txt"
        # pylint: disable=unused-variable
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "source=example_file_latlonbnds.nc&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetPointValue&QUERY_LAYERS=probability&X=4.78&Y=52.13&CRS=EPSG:4326&INFO_FORMAT=application/json&DIM_THRESHOLD=*",
            env=self.env,
            args=["--report"],
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            json.loads(data.getvalue()),
            json.loads(
                AdagucTestTools().readfromfile(self.expectedoutputsspath + filename)
            ),
        )
