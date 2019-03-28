import os
from StringIO import StringIO
from adaguc.CGIRunner import CGIRunner
import unittest
import shutil
import sys
import subprocess
from lxml import etree
from lxml import objectify
import re
from AdagucTestTools import AdagucTestTools

ADAGUC_PATH = os.environ['ADAGUC_PATH']

class TestCSV(unittest.TestCase):
    testresultspath = "testresults/TestCSV/"
    expectedoutputsspath = "expectedoutputs/TestCSV/"
    env={'ADAGUC_CONFIG' : ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.testCSVReader.xml"}
    
    AdagucTestTools().mkdir_p(testresultspath);


    # TODO FONTS are rendered differently accros platforms. Maybe because of Cairo/Truetypes renders. This causes this test to fail
    #def test_CSV_singlefile_autowms(self):
        #AdagucTestTools().cleanTempDir()
        #filename="test_CSV_csvexample_autowms.gif"
        #env={'ADAGUC_CONFIG' : ADAGUC_PATH + "/data/config/adaguc.autoresource.xml", 'ADAGUC_FONT': ADAGUC_PATH + "/data/fonts/FreeSans.ttf"}
        #status,data,headers = AdagucTestTools().runADAGUCServer("source=csvexample.csv&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=Index&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=-180,-90,180,90&STYLES=nearest&FORMAT=image/gif&TRANSPARENT=TRUE", env = env,showLog = False)
        #AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        #self.assertEqual(status, 0)
        #self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
  

    def test_CSV_12timesteps(self):
        AdagucTestTools().cleanTempDir()

        config =  ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + ADAGUC_PATH + '/data/config/datasets/adaguc.testCSVReader.xml'
        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config', config], env = self.env, isCGI = False, showLog = False)
        self.assertEqual(status, 0)
        
        dates=['2018-12-04T12:00:00Z',
               '2018-12-04T12:05:00Z',
               '2018-12-04T12:10:00Z',  
               '2018-12-04T12:15:00Z',  
               '2018-12-04T12:20:00Z',  
               '2018-12-04T12:25:00Z',  
               '2018-12-04T12:30:00Z',  
               '2018-12-04T12:35:00Z',  
               '2018-12-04T12:40:00Z',
               '2018-12-04T12:45:00Z',  
               '2018-12-04T12:50:00Z',
               '2018-12-04T12:55:00Z']
               
        for date in dates:
          filename="test_CSV_timesupport"+date+".png"
          status,data,headers = AdagucTestTools().runADAGUCServer("&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=wind&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=-180,-90,180,90&STYLES=windbarb&FORMAT=image/png&TRANSPARENT=TRUE&TIME=" + date, env = self.env)
          AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
          self.assertEqual(status, 0)
          self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))


    def test_CSV_reference_time(self):
        AdagucTestTools().cleanTempDir()
        env={'ADAGUC_CONFIG' : ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.testCSVReader_reference_time.xml"}
        config =  ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + ADAGUC_PATH + '/data/config/datasets/adaguc.testCSVReader_reference_time.xml'
        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config', config], env = env, isCGI = False, showLog = False)
        self.assertEqual(status, 0)

    def test_CSV_reference_time_GetCapabilities(self):
        AdagucTestTools().cleanTempDir()  
        env={'ADAGUC_CONFIG' : ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.testCSVReader_reference_time.xml"}
        config =  ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + ADAGUC_PATH + '/data/config/datasets/adaguc.testCSVReader_reference_time.xml'
        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config', config], env = env, isCGI = False, showLog = False)
        
        # Test GetCapabilities
        filename="test_CSV_reference_timesupport_GetCapabilities.xml"
        status,data,headers = AdagucTestTools().runADAGUCServer("&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities", env = env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))

    def test_CSV_reference_time_GetMap(self):
        AdagucTestTools().cleanTempDir()   
        env={'ADAGUC_CONFIG' : ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml," + ADAGUC_PATH + "/data/config/datasets/adaguc.testCSVReader_reference_time.xml"}
        config =  ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + ADAGUC_PATH + '/data/config/datasets/adaguc.testCSVReader_reference_time.xml'
        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config', config], env = env, isCGI = False, showLog = False)
        
        # Test WMS GetMap
        
        testcases=[
          {'time':'2018-12-04T12:00:00Z', 'reference_time': '2018-12-04T12:00:00Z'},
          {'time':'2018-12-04T12:05:00Z', 'reference_time': '2018-12-04T12:00:00Z'},
          {'time':'2018-12-04T12:05:00Z', 'reference_time': '2018-12-04T12:05:00Z'},
          {'time':'2018-12-04T12:10:00Z', 'reference_time': '2018-12-04T12:05:00Z'}                   
        ]
        
        for testcase in testcases:
          date = testcase['time']
          DIM_reference_time = testcase['reference_time']
          filename="test_CSV_reference_timesupport"+date+"_"+DIM_reference_time+".png"
          status,data,headers = AdagucTestTools().runADAGUCServer("&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=wind&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=-180,-90,180,90&STYLES=windbarb&FORMAT=image/png&TRANSPARENT=TRUE&TIME=" + date + "&DIM_reference_time=" + DIM_reference_time, env = env)
          AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
          self.assertEqual(status, 0)
          self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
