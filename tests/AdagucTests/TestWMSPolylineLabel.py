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


class TestWMSPolylineLabel(unittest.TestCase):
    testresultspath = "testresults/TestWMSPolylineLabel/"
    expectedoutputsspath = "expectedoutputs/TestWMSPolylineLabel/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def dotest(self, stylename):
        AdagucTestTools().cleanTempDir()

        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.testwmspolylinelabels.xml"
        )
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        sys.stdout.write("\ntest style %s " % stylename)
        sys.stdout.flush()
        filename = "test_WMSPolylineLabel_" + stylename + ".png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.testwmspolylinelabels&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=areas&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=49,1.5,55,7.5&STYLES="
            + stylename
            + "%2Fpolyline&FORMAT=image/png&TRANSPARENT=TRUE&",
            env=self.env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSPolyLineLabel_borderwidth_0_5px(self):
        self.dotest("polyline_black_0.5px")

    def test_WMSPolyLineLabel(self):
        self.dotest("polyline_with_label")

    def test_WMSPolyLineLabelOverlap2(self):
        self.dotest("polyline_with_label_overlap")

    def test_WMSPolyLineLabelAngle(self):
        self.dotest("polyline_with_label_angle")

    def test_WMSPolyLineLabelRoboto(self):
        self.dotest("polyline_with_label_roboto")

    def test_WMSPolyLineLabelColor(self):
        self.dotest("polyline_with_label_color")
