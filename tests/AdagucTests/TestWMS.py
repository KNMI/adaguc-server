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

class TestWMS(unittest.TestCase):
    testresultspath = "testresults/TestWMS/"
    expectedoutputsspath = "expectedoutputs/TestWMS/"
    env={'ADAGUC_CONFIG' : ADAGUC_PATH + "/data/config/adaguc.autoresource.xml"}
    
    
    AdagucTestTools().mkdir_p(testresultspath);
    
    def compareXML(self,xml,expectedxml):
        obj1 = objectify.fromstring(re.sub(' xmlns="[^"]+"', '', expectedxml, count=1))
        obj2 = objectify.fromstring(re.sub(' xmlns="[^"]+"', '', xml, count=1))

        # Remove ADAGUC build date and version from keywordlists
        for child in obj1.findall("Service/KeywordList")[0]:child.getparent().remove(child)
        for child in obj2.findall("Service/KeywordList")[0]:child.getparent().remove(child)
        
        # Boundingbox extent values are too varying by different Proj libraries
        def removeBBOX(root):
          if (root.tag.title() == "Boundingbox"):
            #root.getparent().remove(root)
            try: 
              del root.attrib["minx"]
              del root.attrib["miny"] 
              del root.attrib["maxx"] 
              del root.attrib["maxy"] 
            except: pass
          for elem in root.getchildren():
              removeBBOX(elem)
        
        removeBBOX(obj1);
        removeBBOX(obj2);  
        
        result = etree.tostring(obj1)     
        expect = etree.tostring(obj2)     

        self.assertEquals(expect, result)
    
    def test_WMSGetCapabilities_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetCapabilities_testdatanc"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&request=getcapabilities", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.compareXML(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_WMSGetMap_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetMap_testdatanc"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
        
    def test_WMSGetCapabilitiesGetMap_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetCapabilities_testdatanc"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&request=getcapabilities", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.compareXML(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
        filename="test_WMSGetMap_testdatanc"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
        
    def test_WMSGetMapGetCapabilities_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetMap_testdatanc"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
        filename="test_WMSGetCapabilities_testdatanc"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&request=getcapabilities", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.compareXML(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
        
    def test_WMSGetMap_getmap_3dims_singlefile(self):
        dims = {
          'time':{
            'vartype':'d',
            'units':"seconds since 1970-01-01 00:00:00",
            'standard_name':'time',
            'values':["2017-01-01T00:00:00Z", "2017-01-01T00:05:00Z", "2017-01-01T00:10:00Z"],
            'wmsname':'time'
          },
          'elevation':{
            'vartype':'d',
            'units':"meters",
            'standard_name':'height',    
            'values':[ 7000, 8000, 9000],
            'wmsname':'elevation'
          },
          'member':{
            'vartype':str,
            'units':"member number",
            'standard_name':'member',
            'values':['member5','member4'],
            'wmsname':'DIM_member'
          }
        }

        AdagucTestTools().cleanTempDir()
        
        def Recurse (dims, number, l):
          for value in range(len(dims[dims.keys()[number-1]]['values'])):
            l[number-1] = value
            if number > 1:
                Recurse ( dims, number - 1 ,l)
            else:
                kvps = ""
                for i in range(len(l)):
                  key = (dims[dims.keys()[i]]['wmsname'])
                  value = (dims[dims.keys()[i]]['values'])[l[i]]
                  kvps += "&" + key +'=' + str(value)
                #print "Checking dims" + kvps
                filename="test_WMSGetMap_getmap_3dims_"+kvps+".png"
                filename = filename.replace("&","_").replace(":","_").replace("=","_");
                #print filename
                url="source=netcdf_5dims%2Fnetcdf_5dims_seq1%2Fnc_5D_20170101000000-20170101001000.nc&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&"
                url+=kvps
                status,data,headers = AdagucTestTools().runADAGUCServer(url, env = self.env)
                AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
                self.assertEqual(status, 0)
                self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
        l = []

        for i in range(len(dims)):
          l.append(0)      
        Recurse(dims,len(dims),l)

    def test_WMSCMDUpdateDBNoConfig(self):
        AdagucTestTools().cleanTempDir()
        args = [ADAGUC_PATH+'/bin/adagucserver', '--updatedb']
        returnCode = subprocess.call(args, stdout=FNULL, stderr=subprocess.STDOUT) 
        self.assertEqual(returnCode, 1)
        
    def test_WMSCMDUpdateDB(self):
        AdagucTestTools().cleanTempDir()
        ADAGUC_PATH = os.environ['ADAGUC_PATH']
        args = [ADAGUC_PATH+'/bin/adagucserver', '--updatedb', '--config', ADAGUC_PATH + '/data/config/adaguc.timeseries.xml']
        returnCode = subprocess.call(args, stdout=FNULL, stderr=subprocess.STDOUT) 
        self.assertEqual(returnCode, 0)
        
        filename="test_WMSGetCapabilities_timeseries_twofiles"
        status,data,headers = AdagucTestTools().runADAGUCServer("SERVICE=WMS&request=getcapabilities", {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.timeseries.xml'})
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.compareXML(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
    
    def test_WMSCMDUpdateDBTailPath(self):
        AdagucTestTools().cleanTempDir()
        ADAGUC_PATH = os.environ['ADAGUC_PATH']
        args = [ADAGUC_PATH+'/bin/adagucserver', '--updatedb', '--config', ADAGUC_PATH + '/data/config/adaguc.timeseries.xml', '--tailpath','netcdf_5dims_seq1']
        returnCode = subprocess.call(args, stdout=FNULL, stderr=subprocess.STDOUT) 
        self.assertEqual(returnCode, 0)
        
        filename="test_WMSGetCapabilities_timeseries_tailpath_netcdf_5dims_seq1"
        status,data,headers = AdagucTestTools().runADAGUCServer("SERVICE=WMS&request=getcapabilities", {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.timeseries.xml'})
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.compareXML(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
        
        args = [ADAGUC_PATH+'/bin/adagucserver', '--updatedb', '--config', ADAGUC_PATH + '/data/config/adaguc.timeseries.xml', '--tailpath','netcdf_5dims_seq2']
        returnCode = subprocess.call(args, stdout=FNULL, stderr=subprocess.STDOUT) 
        self.assertEqual(returnCode, 0)

        filename="test_WMSGetCapabilities_timeseries_tailpath_netcdf_5dims_seq1_and_seq2"
        status,data,headers = AdagucTestTools().runADAGUCServer("SERVICE=WMS&request=getcapabilities", {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.timeseries.xml'})
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.compareXML(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_WMSGetFeatureInfo_forecastreferencetime_texthtml(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetFeatureInfo_forecastreferencetime.html"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=forecast_reference_time%2FHARM_N25_20171215090000_dimx16_dimy16_dimtime49_dimforecastreferencetime1_varairtemperatureat2m.nc&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS=air_temperature__at_2m&QUERY_LAYERS=air_temperature__at_2m&CRS=EPSG%3A4326&BBOX=49.55171074378079,1.4162628389784275,54.80328142582087,9.526486675156528&WIDTH=1515&HEIGHT=981&I=832&J=484&FORMAT=image/gif&INFO_FORMAT=text/html&STYLES=&&time=2017-12-17T09%3A00%3A00Z&DIM_reference_time=2017-12-15T09%3A00%3A00Z", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
        filename="test_WMSGetCapabilities_testdatanc"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&request=getcapabilities", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.compareXML(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))      

    
    def test_WMSGetFeatureInfo_timeseries_forecastreferencetime_json(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetFeatureInfo_timeseries_forecastreferencetime.json"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=forecast_reference_time%2FHARM_N25_20171215090000_dimx16_dimy16_dimtime49_dimforecastreferencetime1_varairtemperatureat2m.nc&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=air_temperature__at_2m&query_layers=air_temperature__at_2m&crs=EPSG%3A4326&bbox=47.80599631376197%2C1.4162628389784275%2C56.548995855839685%2C9.526486675156528&width=910&height=981&i=502&j=481&format=image%2Fgif&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_reference_time=2017-12-15T09%3A00%3A00Z", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
        filename="test_WMSGetCapabilities_testdatanc"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&request=getcapabilities", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.compareXML(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))      
