import os
from StringIO import StringIO
from adaguc.CGIRunner import CGIRunner
import unittest
import shutil
import subprocess
from lxml import etree
from lxml import objectify
import re
from AdagucTestTools import AdagucTestTools

FNULL = open(os.devnull, 'w')
ADAGUC_PATH = os.environ['ADAGUC_PATH']

class TestOpenDAP(unittest.TestCase):
    testresultspath = "testresults/TestOpenDAP/"
    expectedoutputsspath = "expectedoutputs/TestOpenDAP/"
    
    AdagucTestTools().mkdir_p(testresultspath);
    
    def test_WMSGetMap_testdatanc(self):
      AdagucTestTools().cleanTempDir()
      filename="test_WMSGetMap_testdatanc"
      status,data = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&")
      AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
      self.assertEqual(status, 0)
      #self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
