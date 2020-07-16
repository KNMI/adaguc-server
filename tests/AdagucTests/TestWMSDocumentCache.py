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

class TestWMSDocumentCache(unittest.TestCase):
    testresultspath = "testresults/TestWMSDocumentCache/"
    expectedoutputsspath = "expectedoutputs/TestWMSDocumentCache/"
    
    env={'ADAGUC_CONFIG' : ADAGUC_PATH + "/data/config/adaguc.tests.dataset.xml"}
    
    AdagucTestTools().mkdir_p(testresultspath);
    
    def test_WMSCMDUpdateDB(self):
        AdagucTestTools().cleanTempDir()
        ADAGUC_PATH = os.environ['ADAGUC_PATH']

        config =  ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + ADAGUC_PATH + '/data/config/datasets/adaguc.testtimeseriescached.xml'
        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config', config], env = self.env, isCGI = False)
        self.assertEqual(status, 0)
        
        filename="test_WMSGetCapabilities_timeseries_twofiles"
        status,data,headers = AdagucTestTools().runADAGUCServer("DATASET=adaguc.testtimeseriescached&SERVICE=WMS&request=getcapabilities", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))
    
    def test_WMSCMDUpdateDBTailPath(self):
        AdagucTestTools().cleanTempDir()
        ADAGUC_PATH = os.environ['ADAGUC_PATH']
        config =  ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + ADAGUC_PATH + '/data/config/datasets/adaguc.testtimeseriescached.xml'

        """ First insert one file """
        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config', config, '--tailpath','netcdf_5dims_seq1'], env = self.env, isCGI = False)
        self.assertEqual(status, 0)

        """ Check getcap """
        filename="test_WMSGetCapabilities_timeseries_tailpath_netcdf_5dims_seq1"
        status,data,headers = AdagucTestTools().runADAGUCServer("DATASET=adaguc.testtimeseriescached&SERVICE=WMS&request=getcapabilities", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))
        
        """ Insert second file"""
        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config', config, '--tailpath','netcdf_5dims_seq2'], env = self.env, isCGI = False)
        self.assertEqual(status, 0)
        
        """ And check if getcap was indeed updated """
        filename="test_WMSGetCapabilities_timeseries_tailpath_netcdf_5dims_seq1_and_seq2"
        status,data,headers = AdagucTestTools().runADAGUCServer("DATASET=adaguc.testtimeseriescached&SERVICE=WMS&request=getcapabilities", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))

    
