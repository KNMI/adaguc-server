import os
from io import BytesIO
from adaguc.CGIRunner import CGIRunner
import unittest
import shutil
import subprocess
from lxml import etree
from lxml import objectify
import re
from .AdagucTestTools import AdagucTestTools


ADAGUC_PATH = os.environ['ADAGUC_PATH']


class TestWMSTiling(unittest.TestCase):
    testresultspath = "testresults/TestWMSTiling/"
    expectedoutputsspath = "expectedoutputs/TestWMSTiling/"
    env = {'ADAGUC_CONFIG': ADAGUC_PATH +
           "/data/config/adaguc.tests.dataset.xml"}
    AdagucTestTools().mkdir_p(testresultspath)

    def test_WMSGetMap_testdatanc(self):
        AdagucTestTools().cleanTempDir()

        config = ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + \
            ADAGUC_PATH + '/data/config/datasets/adaguc.testtiling.xml'
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--updatedb', '--config', config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)

        filename = "test_TestWMSTilingWMSGetMap_testdatanc-notiling.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.testtiling&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdatant&WIDTH=256&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&", env=self.env, showLogOnError=True, showLog=False)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools(
        ).readfromfile(self.expectedoutputsspath + filename))

        ADAGUC_TMP = os.environ['ADAGUC_TMP']
        AdagucTestTools().mkdir_p(os.environ['ADAGUC_TMP']+"/tiling/")
        print("TILING  NOW")
        config = ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + \
            ADAGUC_PATH + '/data/config/datasets/adaguc.testtiling.xml'
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--createtiles', '--config', config], env=self.env, isCGI=False, showLogOnError=True, showLog=True)
        self.assertEqual(status, 0)

        # # AdagucTestTools().cleanTempDir()
        # filename = "test_TestWMSTilingWMSGetMap_testdatanc.png"
        # status, data, headers = AdagucTestTools().runADAGUCServer(
        #     "dataset=adaguc.testtiling&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&", env=self.env, showLogOnError=True, showLog=True)
        # AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        # self.assertEqual(status, 0)
        # self.assertEqual(data.getvalue(), AdagucTestTools(
        # ).readfromfile(self.expectedoutputsspath + filename))
