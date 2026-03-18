# pylint: disable=unused-variable
# pylint: disable=invalid-name

"""
Run test for tiling system of adaguc-server
"""

import os
import unittest
from adaguc.AdagucTestTools import AdagucTestTools

from conftest import (
    clean_temp_dir,
    make_adaguc_env,
    run_adaguc_and_compare_image,
    update_db,
)

ADAGUC_PATH = os.environ["ADAGUC_PATH"]


class TestWMSTiling(unittest.TestCase):
    """
    The class for testing tiling
    """

    testresultspath = "testresults/TestWMSTiling/"
    expectedoutputsspath = "expectedoutputs/TestWMSTiling/"
    env = {"ADAGUC_CONFIG": ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"}
    AdagucTestTools().mkdir_p(testresultspath)

    def test_WMSGetMap_testdatanc_notiling(self):
        """
        Testing standard functionality without tiling
        """
        AdagucTestTools().cleanTempDir()

        config = ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.testtiling.xml"

        status, data, headers = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)

        filename = "test_TestWMSTilingWMSGetMap_testdatanc-notiling.png"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.testtiling&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdatant&WIDTH=256&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            env=self.env,
            showLogOnError=True,
            showLog=False,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_WMSGetMap_testdatanc_tiling(self):
        """
        Testing tiling using the createtiles command
        """
        AdagucTestTools().cleanTempDir()

        config = ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.testtiling.xml"

        status, data, headers = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)

        filename = "test_TestWMSTilingWMSGetMap_testdatanc-notiling.png"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.testtiling&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdatant&WIDTH=256&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            env=self.env,
            showLogOnError=True,
            showLog=False,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

        AdagucTestTools().mkdir_p(os.environ["ADAGUC_TMP"] + "/tiling/")
        config = ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.testtiling.xml"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=["--createtiles", "--config", config], env=self.env, isCGI=False, showLogOnError=True, showLog=False
        )
        self.assertEqual(status, 0)

        filename = "test_TestWMSTilingWMSGetMap_testdatanc.png"
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.testtiling&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            env=self.env,
            showLogOnError=True,
            showLog=False,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_WMSGetMap_testdatanc_autotiling(self):
        """
        Testing auto tiling, tiling done during --updatedb
        """
        AdagucTestTools().cleanTempDir()

        config = ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.testautotiling.xml"

        status, data, headers = AdagucTestTools().runADAGUCServer(args=["--updatedb", "--config", config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)

        filename = "test_TestWMSTilingWMSGetMap_testdatanc-autotiling.png"

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.testautotiling&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
            env=self.env,
            showLogOnError=True,
            showLog=False,
        )
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_tiling_satcomp_png(self):
        """
        Test tile for satcomp png's
        """
        clean_temp_dir()
        env = make_adaguc_env("adaguc.tests.tiling_satcomppng.xml", self.testresultspath, self.expectedoutputsspath)
        # Check if there are no tiles
        files = [f for f in os.listdir(os.getenv("ADAGUC_TMP")) if f.endswith("tile.nc")]
        assert len(files) == 0
        update_db(env)
        # Check if there are three tiles
        files = [f for f in os.listdir(os.getenv("ADAGUC_TMP")) if f.endswith("tile.nc")]
        assert len(files) == 3
        run_adaguc_and_compare_image(
            env,
            "test_tiling_satcomp_png.png",
            "DATASET=adaguc.tests.tiling_satcomppng&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=MTG-FCI-FD_eur_atlantic_1km_true_color&WIDTH=688&HEIGHT=959&CRS=EPSG%3A3857&BBOX=-1649522.0372212431,3614598.9628729257,2283086.7198879756,9096244.018203944&STYLES=default&FORMAT=image/png&TRANSPARENT=TRUE&&time=2026-03-17T12%3A40%3A00Z&0.5279499342913613",
        )
