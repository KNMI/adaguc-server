import os, os.path
from io import BytesIO
import unittest
import shutil
import subprocess
import json
from lxml import etree
from lxml import objectify
import re
from .AdagucTestTools import AdagucTestTools

import sys
sys.path.append('../../adaguc')
from adaguc.CGIRunner import CGIRunner

ADAGUC_PATH = os.environ['ADAGUC_PATH']

class TestMetadataService(unittest.TestCase):
    testresultspath = "testresults/TestMetadataService/"
    expectedoutputsspath = "expectedoutputs/TestMetadataService/"
    env={'ADAGUC_CONFIG' : ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}
    
    
    AdagucTestTools().mkdir_p(testresultspath);
    def test_MetadataGetMetadata(self):
        AdagucTestTools().cleanTempDir()
        filename="test_MetadataGetMetadata.json"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=test/testdata_timedim.nc&SERVICE=metadata&request=getmetadata&", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

