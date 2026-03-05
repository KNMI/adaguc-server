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


from conftest import make_adaguc_env, update_db

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

    def test_datapostprocessor_convert_units_scaled_getmap(self):
        """
        Test for the convert_units post processor with data with scale/offset
        """
        AdagucTestTools().cleanTempDir()
        config = (
            ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.tests.datapostproc.xml"
        )
        status, data, headers = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)
        filename = "test_DataPostProcessor_convert_units_scaled_GetMap.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.datapostproc&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=scaled_fahrenheit&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=-1841640.561188397,4434884.178327009,2446462.937192397,9899900.45389999&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )

        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

        filename_unscaled = "test_DataPostProcessor_convert_units_unscaled_GetMap.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.datapostproc&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=unscaled_fahrenheit&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=-1841640.561188397,4434884.178327009,2446462.937192397,9899900.45389999&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&",
            {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"},
        )

        AdagucTestTools().writetofile(self.testresultspath + filename_unscaled, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_GFITimeseries_multiple_post_procs_on_arcus_uwcw_wind_speed_hagl_ms_member_3_backandforth_multiple(self):
        """
        Test multiple data conversions in one layer for timeseries.
        """
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("adaguc.tests.arcus_uwcw.xml")
        update_db(env)
        filename = "test_GFITimeseries_multiple_post_procs_on_arcus_uwcw_wind_speed_hagl_ms_member_3_backandforth_multiple.json"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.arcus_uwcw&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=wind_speed_hagl_ms_member_3_backandforth_multiple&query_layers=wind_speed_hagl_ms_member_3_backandforth_multiple&crs=EPSG%3A3857&bbox=128776.09414167603%2C5091803.541357795%2C1280317.2144171826%2C8160253.516394952&width=495&height=1319&i=225&j=551&format=image%2Fgif&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_reference_time=2024-06-05T03%3A00%3A00Z&",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        AdagucTestTools().compareJson(self.testresultspath + filename, self.expectedoutputsspath + filename)

    def test_GetLegendGraphic_multiple_post_procs_on_arcus_uwcw_wind_speed_hagl_ms_member_3_backandforth_multiple(self):
        """
        Test multiple data conversions in one layer for timeseries.
        """
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("adaguc.tests.arcus_uwcw.xml")
        update_db(env)
        filename = "test_GetLegendGraphic_multiple_post_procs_on_arcus_uwcw_wind_speed_hagl_ms_member_3_backandforth_multiple.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.arcus_uwcw&SERVICE=WMS&&version=1.1.1&service=WMS&request=GetLegendGraphic&layer=wind_speed_hagl_ms_member_3_backandforth_multiple&format=image/png&STYLE=auto/nearest&layers=wind_speed_hagl_ms_member_3_backandforth_multiple&&time=2024-06-06T03%3A00%3A00Z&DIM_reference_time=2024-06-05T03%3A00%3A00Z&&transparent=true&&0.5077404424685739",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        AdagucTestTools().compareImage(self.testresultspath + filename, self.expectedoutputsspath + filename)

    def test_GetMap_multiple_post_procs_on_arcus_uwcw_wind_speed_hagl_ms_member_3_backandforth_multiple(self):
        """
        Test multiple data conversions in one layer for timeseries.
        """
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("adaguc.tests.arcus_uwcw.xml")
        update_db(env)
        filename = "test_GetMap_multiple_post_procs_on_arcus_uwcw_wind_speed_hagl_ms_member_3_backandforth_multiple.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.tests.arcus_uwcw&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=wind_speed_hagl_ms_member_3_backandforth_multiple&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=229850.82433886,6364199.538544346,1015783.6170691401,7306604.405463655&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&time=2024-06-06T03%3A00%3A00Z&DIM_reference_time=2024-06-05T03%3A00%3A00Z&0.9370971599246679",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        AdagucTestTools().compareImage(self.testresultspath + filename, self.expectedoutputsspath + filename)

    def test_GetCoverage_multiple_post_procs_on_arcus_uwcw_wind_speed_hagl_ms_member_3_backandforth_multiple(self):
        """
        Test multiple data conversions in one layer for timeseries.
        """
        AdagucTestTools().cleanTempDir()
        env = make_adaguc_env("adaguc.tests.arcus_uwcw.xml")
        update_db(env)

        # Write NetCDF file using WCS
        wcs_filename = "test_GetCoverage_multiple_post_procs_on_arcus_uwcw_wind_speed_hagl_ms_member_3_backandforth_multiple.nc"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.arcus_uwcw&SERVICE=WCS&REQUEST=GetCoverage&COVERAGE=wind_speed_hagl_ms_member_3_backandforth_multiple&CRS=EPSG%3A3857&FORMAT=netcdf&BBOX=229850.82433886,6364199.538544346,1015783.6170691401,7306604.405463655&RESX=157186.558546056&RESY=188480.97338386177&TIME=2024-06-06T03:00:00Z&DIM_REFERENCE_TIME=2024-06-05T03:00:00Z",
            env=env,
        )
        AdagucTestTools().writetofile(self.testresultspath + wcs_filename, data.getvalue())
        self.assertEqual(status, 0)

        # Getmap on written NetCDF file
        wms_filename = "test_GetMapOnGetCoverage_multiple_post_procs_on_arcus_uwcw_wind_speed_hagl_ms_member_3_backandforth_multiple.png"
        status, data, _ = AdagucTestTools().runADAGUCServer(
            "source=TestDataPostProcessor%2Ftest_GetCoverage_multiple_post_procs_on_arcus_uwcw_wind_speed_hagl_ms_member_3_backandforth_multiple%2Enc&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=wind-speed-hagl&WIDTH=256&HEIGHT=256&CRS=EPSG%3A3857&BBOX=221991.4964117,6269353.652580505,1023642.9449963,7401450.291427495&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&DIM_wind_at_hagl=10&time=2024-06-06T03%3A00%3A00Z&DIM_reference_time=2024-06-05T03%3A00%3A00Z&DIM_member=3&0.47167832666058407",
            env={"ADAGUC_CONFIG": ADAGUC_PATH + "data/config/adaguc.autoresource.xml", "ADAGUC_AUTOWMS_DIR": self.testresultspath},
        )
        AdagucTestTools().writetofile(self.testresultspath + wms_filename, data.getvalue())
        self.assertEqual(status, 0)
        AdagucTestTools().compareImage(self.testresultspath + wms_filename, self.expectedoutputsspath + wms_filename)
