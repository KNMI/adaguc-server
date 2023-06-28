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

ADAGUC_PATH = os.environ['ADAGUC_PATH']


class TestDataPostProcessor(unittest.TestCase):
    testresultspath = "testresults/TestDataPostProcessor/"
    expectedoutputsspath = "expectedoutputs/TestDataPostProcessor/"
    env = {'ADAGUC_CONFIG': ADAGUC_PATH +
            "/data/config/adaguc.tests.dataset.xml"}

    AdagucTestTools().mkdir_p(testresultspath)

  

    def test_DataPostProcessor_SubstractLevels_GetCapabilities(self):
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + \
            ADAGUC_PATH + '/data/config/datasets/adaguc.tests.datapostproc.xml'
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--updatedb', '--config', config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)
        filename = "test_DataPostProcessor_SubstractLevels_GetCapabilities.xml"
        
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.datapostproc&SERVICE=WMS&request=getcapabilities",
            {
                'ADAGUC_CONFIG':
                ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml'
            })

        AdagucTestTools().writetofile(self.testresultspath + filename,
                                      data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(
            self.testresultspath + filename,
            self.expectedoutputsspath + filename))



    def test_DataPostProcessor_SubstractLevels_GetMap(self):
        AdagucTestTools().cleanTempDir()
        config = ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + \
            ADAGUC_PATH + '/data/config/datasets/adaguc.tests.datapostproc.xml'
        status, data, headers = AdagucTestTools().runADAGUCServer(
            args=['--updatedb', '--config', config], env=self.env, isCGI=False)
        self.assertEqual(status, 0)
        filename = "test_DataPostProcessor_SubstractLevels_GetMap.png"
        
        status, data, headers = AdagucTestTools().runADAGUCServer(
            "dataset=adaguc.tests.datapostproc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=output&WIDTH=256&CRS=EPSG:4326&HEIGHT=256&STYLES=default&FORMAT=image/png&TRANSPARENT=FALSE&showlegend=false",
            {
                'ADAGUC_CONFIG':
                ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml'
            })

        AdagucTestTools().writetofile(self.testresultspath + filename,
                                      data.getvalue())
        self.assertEqual(status, 0)

        self.assertEqual(
            data.getvalue(),
            AdagucTestTools().readfromfile(self.expectedoutputsspath +
                                           filename))
