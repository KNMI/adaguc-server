import os
import os.path
from io import BytesIO
import unittest
import shutil
import subprocess
import json
from lxml import etree, objectify
import re
from adaguc.AdagucTestTools import AdagucTestTools
from lxml import etree, objectify
import datetime

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestDataPostProcessor(unittest.TestCase):
    testresultspath = "testresults/TestDataPostProcessor/"
    expectedoutputsspath = "expectedoutputs/TestDataPostProcessor/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_DataPostProcessor_SubstractLevels_GetCapabilities(self):
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.tests.datapostproc.xml"
        )
        status, data, headers = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)
        filename = "test_DataPostProcessor_SubstractLevels_GetCapabilities.xml"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.datapostproc&SERVICE=WMS&request=getcapabilities",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))

    def test_DataPostProcessor_SubstractLevels_GetMap(self):
        """
        The input data for this set is a NetCDF file where textmessages are printed in the grid.
        This test checks if the correct dimensions are chosen and verifies if the result is as expected.
        The result does not look pretty, it is purely functional.
        """
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.tests.datapostproc.xml"
        )
        status, data, headers = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)
        filename = "test_DataPostProcessor_SubstractLevels_GetMap.png"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.datapostproc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=output&WIDTH=256&CRS=EPSG:4326&HEIGHT=256&STYLES=default&FORMAT=image/png&TRANSPARENT=FALSE&showlegend=false",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)

        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_datapostprocessor_windshear_getmap(self):
        """
        Test for the windshear post processor
        """
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.datapostproc-windshear.xml"
        )
        status, data, headers = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)

        filename = "test_DataPostProcessor_WindShear_GetMap.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.datapostproc-windshear&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=output&WIDTH=256&CRS=EPSG:4326&HEIGHT=256&STYLES=default&FORMAT=image/png&TRANSPARENT=FALSE&showlegend=false",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_datapostprocessor_windshear_getmetadata(self):
        """
        Test for the windshear post processor
        """
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.datapostproc-windshear.xml"
        )
        status, data, headers = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)
        filename = "test_DataPostProcessor_WindShear_GetMetaData.txt"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.datapostproc-windshear&service=wms&request=getmetadata&format=image/png&srs=EPSG:4326&layer=output",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_datapostprocessor_windshear_getfeatureinfo(self):
        """
        Test for the windshear post processor
        """
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.datapostproc-windshear.xml"
        )
        status, data, headers = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)
        filename = "test_DataPostProcessor_WindShear_GetFeatureInfo.json"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.datapostproc-windshear&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS=output&QUERY_LAYERS=output&CRS=EPSG%3A3857&BBOX=-1084594.00339733,5299085.702520586,2054668.3733940602,8244166.9013661165&WIDTH=1358&HEIGHT=1274&I=626&J=578&INFO_FORMAT=application/json&STYLES=&&time=2019-01-01T22%3A00%3A00Z",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_datapostprocessor_windshear_gettimeseries(self):
        """
        Test for the windshear post processor
        """
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH
            + "/data/config/adaguc.tests.dataset.xml,"
            + ADAGUC_PATH
            + "/data/config/datasets/adaguc.tests.datapostproc-windshear.xml"
        )
        status, data, headers = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)
        filename = "test_DataPostProcessor_WindShear_GetTimeSeries.json"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.datapostproc-windshear&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS=output&QUERY_LAYERS=output&CRS=EPSG%3A3857&BBOX=-1084594.00339733,5299085.702520586,2054668.3733940602,8244166.9013661165&WIDTH=1358&HEIGHT=1274&I=626&J=578&INFO_FORMAT=application/json&STYLES=&&time=*",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_datapostprocessor_dbztorr_getmap(self):
        """
        Test for the dbztorr post processor
        """
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.tests.datapostproc.xml"
        )
        status, data, headers = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)
        filename = "test_DataPostProcessor_DBZ_GetMap.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.datapostproc&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=RAD_NL25_PCP_CM&WIDTH=256IGHT=256&CRS=EPSG%3A3857&BBOX=562267.2546644434,6568919.974681269,797831.9309681491,6869137.726056894&STYLES=radar%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&time=2021-06-22T20%3A00%3A00Z",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_datapostprocessor_axplusb_getmap(self):
        """
        Test for the axplusb post processor
        """
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.tests.datapostproc.xml"
        )
        status, data, headers = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)
        filename = "test_DataPostProcessor_axplusb_GetMap.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.datapostproc&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=-1841640.561188397,4434884.178327009,2446462.937192397,9899900.45389999&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
