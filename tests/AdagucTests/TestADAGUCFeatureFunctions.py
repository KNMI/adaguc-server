import os
from io import BytesIO
from adaguc.CGIRunner import CGIRunner
from adaguc.ADAGUCFeatureFunctions import ADAGUCFeatureCombineNuts
import unittest
import shutil
import subprocess
from lxml import etree
from lxml import objectify
import re
from .AdagucTestTools import AdagucTestTools

ADAGUC_PATH = os.environ['ADAGUC_PATH']

class TestADAGUCFeatureFunctions(unittest.TestCase):
    testresultspath = "testresults/TestADAGUCFeatureFunctions/"
    expectedoutputsspath = "expectedoutputs/TestADAGUCFeatureFunctions/"
    env={'ADAGUC_CONFIG' : ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}
    
    AdagucTestTools().mkdir_p(testresultspath);
   
    def test_ADAGUCFeatureFunctions_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filenamencraster="test_ADAGUCFeatureFunctions_testdata_raster.nc"
        filenamencpoint="test_ADAGUCFeatureFunctions_testdata_point.nc"
        filenamecsv="test_ADAGUCFeatureFunctions_testdata.csv"
        def progressCallback(message,percentage):
            #print "testCallback:: "+message+" "+str(percentage)
            return
       
        ADAGUCFeatureCombineNuts(
            featureNCFile = "countries.geojson",
            #dataNCFile = "myfile.nc",
            dataNCFile = "testdata.nc",
            bbox= "0,50,10,55",
            time= None,
            variable="testdata",
            #variable="index", 
            width=800,
            height=800,
            outncrasterfile=os.getcwd() + "/"+ self.testresultspath + filenamencraster,
            outncpointfile=os.getcwd() + "/"+ self.testresultspath + filenamencpoint,
            outcsvfile=os.getcwd() + "/"+ self.testresultspath + filenamecsv, 
            tmpFolderPath="/tmp",
            callback=progressCallback)
        #AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        # Comparing binary NetCDF is difficult 
        #self.assertEqual( 
            #AdagucTestTools().readfromfile(self.testresultspath + filenamencraster), AdagucTestTools().readfromfile(self.expectedoutputsspath + filenamencraster))
        self.assertEqual( 
            AdagucTestTools().readfromfile(self.testresultspath + filenamecsv), AdagucTestTools().readfromfile(self.expectedoutputsspath + filenamecsv))

  