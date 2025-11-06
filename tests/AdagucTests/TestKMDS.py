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

ADAGUC_PATH = os.environ['ADAGUC_PATH']


class TestKMDS(unittest.TestCase):
    testresultspath = "testresults/TestKMDS/"
    expectedoutputsspath = "expectedoutputs/TestKMDS/"
    env = {'ADAGUC_CONFIG': ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," +
            ADAGUC_PATH + "/data/config/datasets/adaguc.testGeoJSONReader_time.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

    def test_kmds_alle_stations_10001_10mwind_barball(self):
        AdagucTestTools().cleanTempDir()
        
        filename = "test_kmds_alle_stations_10001_10mwind_barball.png"
        
        config = (ADAGUC_PATH+ "/data/config/adaguc.tests.dataset.xml,adaguc.test.kmds_alle_stations_10001.xml")
        env = {"ADAGUC_CONFIG": config}
        status, data, _ = AdagucTestTools().runADAGUCServer(
            args=["--updatedb", "--config", config], env=self.env, isCGI=False
        )
        self.assertEqual(status, 0)

        status, data, headers = AdagucTestTools().runADAGUCServer(
            "DATASET=adaguc.test.kmds_alle_stations_10001&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=10M%2Fwind&WIDTH=300&HEIGHT=500&CRS=EPSG%3A3857&BBOX=269422.313123934,6357145.5563671775,939865.5563671777,7457638.879961043&STYLES=barball%2Fpoint&FORMAT=image/png&TRANSPARENT=FALSE&&time=2025-11-06T09%3A20%3A00Z&0.2925757777437007", env=env, showLog=False)
        AdagucTestTools().writetofile(self.testresultspath + filename, data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(
            AdagucTestTools().compareImage(
                self.expectedoutputsspath + filename,
                self.testresultspath + filename,
                10,
            )
        )
