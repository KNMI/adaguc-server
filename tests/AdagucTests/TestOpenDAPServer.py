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

class TestOpenDAPServer(unittest.TestCase):
    testresultspath = "testresults/TestOpenDAP/"
    expectedoutputsspath = "expectedoutputs/TestOpenDAP/"
    env={'ADAGUC_CONFIG' : ADAGUC_PATH + "/data/config/adaguc.opendapserver.xml"}
    
    AdagucTestTools().mkdir_p(testresultspath);
    
    def test_OpenDAPServer_testdatanc_DDS(self):
      AdagucTestTools().cleanTempDir()
      filename="test_OpenDAPServer_testdatanc_DDS.txt"
      status,data,headers = AdagucTestTools().runADAGUCServer(path="opendap/testdata.nc.dds", env = self.env)
      AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
      self.assertEqual(status, 0)
      # print ("\nHEADERS\n" + headers +"\n")
      self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_OpenDAPServer_testdatanc_DDS_headers(self):
      AdagucTestTools().cleanTempDir()
      filename="test_OpenDAPServer_testdatanc_DDS_headers.txt"
      status,data,headers = AdagucTestTools().runADAGUCServer(path="opendap/testdata.nc.dds", env = self.env)
      AdagucTestTools().writetofile(self.testresultspath + filename,headers.encode())
      self.assertEqual(status, 0)
      # print ("\nHEADERS\n" + headers +"\n")
      self.assertEqual(headers.encode(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_OpenDAPServer_testdatanc_DAS(self):
      AdagucTestTools().cleanTempDir()
      filename="test_OpenDAPServer_testdatanc_DAS.txt"
      status,data,headers = AdagucTestTools().runADAGUCServer(path="opendap/testdata.nc.das", env = self.env)
      AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
      self.assertEqual(status, 0)
      self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
      
    def test_OpenDAPServer_testdatanc_DODS(self):
      AdagucTestTools().cleanTempDir()
      filename="test_OpenDAPServer_testdatanc_DODS.txt"
      status,data,headers = AdagucTestTools().runADAGUCServer(path="opendap/testdata.nc.dods", env = self.env)
      AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
      self.assertEqual(status, 0)
      self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))


    def test_OpenDAPServer_testdatanc_DODS_Query_testdatayxtimeprojection(self):
      AdagucTestTools().cleanTempDir()
      filename="test_OpenDAPServer_testdatanc_DODS_Query_testdatayxtimeprojection.txt"
      status,data,headers = AdagucTestTools().runADAGUCServer(path="opendap/testdata.nc.dods", url="testdata,y,x,time,projection", env = self.env)
      AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
      self.assertEqual(status, 0)
      self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))


    def test_OpenDAPServer_testdatanc_DODS_Query_testdatay(self):
      AdagucTestTools().cleanTempDir()
      filename="test_OpenDAPServer_testdatanc_DODS_Query_testdatay.txt"
      status,data,headers = AdagucTestTools().runADAGUCServer(path="opendap/testdata.nc.dods", url="testdata,y", env = self.env)
      AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
      self.assertEqual(status, 0)
      self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_OpenDAPServer_testdatanc_DODS_Query_projection(self):
      AdagucTestTools().cleanTempDir()
      filename="test_OpenDAPServer_testdatanc_DODS_Query_projection.txt"
      status,data,headers = AdagucTestTools().runADAGUCServer(path="opendap/testdata.nc.dods", url="projection", env = self.env)
      AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
      self.assertEqual(status, 0)
      self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_OpenDAPServer_testdatanc_DODS_Query_testdata_subset(self):
      AdagucTestTools().cleanTempDir()
      filename="test_OpenDAPServer_testdatanc_DODS_Query_testdata_subset.txt"
      status,data,headers = AdagucTestTools().runADAGUCServer(path="opendap/testdata.nc.dods", url="testdata[1:5][8:17]", env = self.env)
      AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
      self.assertEqual(status, 0)
      self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
