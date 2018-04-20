import os
from StringIO import StringIO
from adaguc.CGIRunner import CGIRunner
from adaguc.ADAGUCFeatureFunctions import ADAGUCFeatureCombineNuts
import unittest
import shutil
import subprocess
from lxml import etree
from lxml import objectify
import re
from AdagucTestTools import AdagucTestTools

ADAGUC_PATH = os.environ['ADAGUC_PATH']

class TestADAGUCFeatureFunctions(unittest.TestCase):
    testresultspath = "testresults/TestADAGUCFeatureFunctions/"
    expectedoutputsspath = "expectedoutputs/TestADAGUCFeatureFunctions/"
    env={'ADAGUC_CONFIG' : ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}
    
    AdagucTestTools().mkdir_p(testresultspath);
   
    def test_ADAGUCFeatureFunctions_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filenamenc="test_ADAGUCFeatureFunctions_testdata.nc"
        filenamecsv="test_ADAGUCFeatureFunctions_testdata.csv"
       
        print "Writing to " +self.testresultspath + filenamenc
       
        ADAGUCFeatureCombineNuts(
            featureNCFile = "countries.geojson",
            dataNCFile = "testdata.nc",
            bbox= "0,50,10,55",
            time= "*",
            variable="testdata", 
            width=80,
            height=80,
            outncfile=os.getcwd() + "/"+ self.testresultspath + filenamenc,
            outcsvfile=os.getcwd() + "/"+ self.testresultspath + filenamecsv, 
            tmpFolderPath="/tmp")
        # AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual( 
            AdagucTestTools().readfromfile(self.testresultspath + filenamenc), AdagucTestTools().readfromfile(self.expectedoutputsspath + filenamenc))
        self.assertEqual( 
            AdagucTestTools().readfromfile(self.testresultspath + filenamecsv), AdagucTestTools().readfromfile(self.expectedoutputsspath + filenamecsv))

   
