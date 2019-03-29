import os
from StringIO import StringIO
from adaguc.CGIRunner import CGIRunner
from adaguc.ADAGUCFeatureFunctions import ADAGUCFeatureCombineNuts
import shutil
import subprocess
from lxml import etree
from lxml import objectify
import re
import logging
#logging.basicConfig(level=logging.DEBUG)

ADAGUC_PATH = os.environ['ADAGUC_PATH']

class PolygonOverlay():
    testresultspath = "./"
    env={'ADAGUC_CONFIG' : ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}
    
    
    def test_ADAGUCFeatureFunctions_testdatanc(self):
        filenamencraster="test_ADAGUCFeatureFunctions_testdata_raster.nc"
        filenamencpoint="test_ADAGUCFeatureFunctions_testdata_point.nc"
        filenamecsv="test_ADAGUCFeatureFunctions_testdata.csv"
        def progressCallback(message,percentage):
            print "Progress:: "+message+" "+str(percentage)
            return
       
        # data files need to reside in adaguc-server/data/datasets
        ADAGUCFeatureCombineNuts(
            featureNCFile = "countries.geojson",
            dataNCFile = "testdata.nc", #data files need to reside in adaguc-server/data/datasets
            variable="testdata", #Variable to use should be given
            #dataNCFile="out_icclim.nc",
            #variable="SU",
            bbox= "0,50,10,55",
            time= "*",            
            width=800,
            height=800,
            outncrasterfile=os.getcwd() + "/"+ self.testresultspath + filenamencraster,
            outncpointfile=os.getcwd() + "/"+ self.testresultspath + filenamencpoint,
            outcsvfile=os.getcwd() + "/"+ self.testresultspath + filenamecsv, 
            tmpFolderPath="/tmp",
            callback=progressCallback)
        

PolygonOverlay().test_ADAGUCFeatureFunctions_testdatanc();  