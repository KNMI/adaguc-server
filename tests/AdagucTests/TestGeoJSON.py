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
from .AdagucTestTools import AdagucTestTools

ADAGUC_PATH = os.environ['ADAGUC_PATH']

class TestGeoJSON(unittest.TestCase):
    testresultspath = "testresults/TestGeoJSON/"
    expectedoutputsspath = "expectedoutputs/TestGeoJSON/"
    env={'ADAGUC_CONFIG' : ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.testGeoJSONReader_time.xml"}
    
    AdagucTestTools().mkdir_p(testresultspath);

    def test_GeoJSON_countries_autowms(self):
        AdagucTestTools().cleanTempDir()
        filename="test_GeoJSON_countries_autowms.png"
        env={'ADAGUC_CONFIG' : ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}
        status,data,headers = AdagucTestTools().runADAGUCServer("source=countries.geojson&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=features&WIDTH=256&HEIGHT=128&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=default&FORMAT=image/png&TRANSPARENT=TRUE", env = env,showLog = False)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_GeoJSON_time_GetCapabilities(self):
        AdagucTestTools().cleanTempDir()
        config =  ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + ADAGUC_PATH + '/data/config/datasets/adaguc.testGeoJSONReader_time.xml'
        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config', config], env = {'ADAGUC_CONFIG' : config}, isCGI = False, showLog = False)
        self.assertEqual(status, 0)
          # Test GetCapabilities
        filename="test_GeoJSON_time_GetCapabilities.xml"
        status,data,headers = AdagucTestTools().runADAGUCServer("&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities", env = {'ADAGUC_CONFIG' : config})
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))


    def test_GeoJSON_3timesteps(self):
        AdagucTestTools().cleanTempDir()
        config =  ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + ADAGUC_PATH + '/data/config/datasets/adaguc.testGeoJSONReader_time.xml'
        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config', config], env = {'ADAGUC_CONFIG' : config}, isCGI = False, showLog = False)
        self.assertEqual(status, 0)
        
        dates=['2018-12-04T12:00:00Z',
               '2018-12-04T12:05:00Z',
               '2018-12-04T12:10:00Z']
               
        for date in dates:
          filename="test_GeoJSON_timesupport"+date+".png"
          status,data,headers = AdagucTestTools().runADAGUCServer("&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=features&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=40,-10,60,40&STYLES=features&FORMAT=image/png&TRANSPARENT=TRUE&TIME=" + date, env = {'ADAGUC_CONFIG' : config})
          AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
          self.assertEqual(status, 0)
          self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

