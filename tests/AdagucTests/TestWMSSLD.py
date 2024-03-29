import os
import os.path
from io import BytesIO
from adaguc.CGIRunner import CGIRunner
import unittest
import shutil
import subprocess
import json
from lxml import etree
from lxml import objectify
import re
from adaguc.AdagucTestTools import AdagucTestTools

# import sys
# sys.path.insert(0, '/nobackup/users/bennekom/adaguc-webmapjs-sld-server/env/lib/python3.6/site-packages/flask/__init__.py')
# from flask import Flask

ADAGUC_PATH = os.environ["ADAGUC_PATH"]
DEFAULT_REQUEST_PARAMS = "SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=radar&WIDTH=512&HEIGHT=512&CRS=EPSG%3A4326&BBOX=30,-30,75,30&FORMAT=image/png&TRANSPARENT=TRUE"


class TestWMSSLD(unittest.TestCase):
    testresultspath = "testresults/TestWMSSLD/"
    expectedoutputsspath = "expectedoutputs/TestWMSSLD/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.sld.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def checkReport(self, reportFilename="", expectedReportFilename=""):
        self.assertTrue(os.path.exists(reportFilename))
        self.assertEqual(
            AdagucTestTools().readfromfile(reportFilename),
            AdagucTestTools().readfromfile(
                self.expectedoutputsspath + expectedReportFilename
            ),
        )
        os.remove(reportFilename)

    def test_WMSGetMap_testdatanc_NOSLD(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMap_testdatanc_NOSLD.png"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=[
                "--updatedb",
                "--config",
                ADAGUC_PATH + "/data/config/adaguc.sld.xml",
            ],
            env=self.env,
            isCGI=False,
            showLogOnError=True,
        )
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            DEFAULT_REQUEST_PARAMS, env=self.env
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath + filename),
        )

    def test_WMSGetMap_testdatanc_NOSLDURL(self):
        AdagucTestTools().cleanTempDir()
        filename = "test_WMSGetMap_testdatanc_NOSLDURL.xml"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            DEFAULT_REQUEST_PARAMS + "&SLD=", env=self.env, showLogOnError=False
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 404)

    # def test_WMSGetMap_WITHSLD_testdatanc(self):
    #     AdagucTestTools().cleanTempDir()
    #     Start flask server
    #     filename="test_WMSGetMap_testdatanc_WITHSLD.png"
    #
    #     status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config', ADAGUC_PATH + '/data/config/adaguc.sld.xml'], env = self.env, isCGI = False, showLogOnError = True)
    #     self.assertEqual(status, 0)
    #
    #     status,data,headers = AdagucTestTools().runADAGUCServer("SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=radar&WIDTH=512&HEIGHT=512&CRS=EPSG%3A4326&BBOX=30,-30,75,30&FORMAT=image/png&TRANSPARENT=FALSE&SHOWLEGEND=TRUE&SLD=http://0.0.0.0:5000/sldFiles/radar.xml",
    #                                                             env = self.env)
    #     AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
    #     self.assertEqual(status, 0)
    #     self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
