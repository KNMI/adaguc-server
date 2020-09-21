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

    def checkReport(self, reportFilename="", expectedReportFilename=""):
        self.assertTrue(os.path.exists(reportFilename))
        self.assertEqual(AdagucTestTools().readfromfile(reportFilename),
                         AdagucTestTools().readfromfile(self.expectedoutputsspath + expectedReportFilename))
        os.remove(reportFilename)
    
    def test_WMSGetCapabilities_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetCapabilities_testdatanc"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&request=getcapabilities", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))
        self.assertFalse(os.path.exists("checker_report.txt"))

    def test_WMSGetMap_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetMap_testdatanc"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
                                                                env = self.env, args=["--report"])
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

        self.checkReport(reportFilename="checker_report.txt",
                         expectedReportFilename="checker_report_WMSGetMap_testdatanc.txt")

    def test_WMSGetMap_Report_env(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetMap_Report_env"
        reportfilename="./env_checker_report.txt"
        self.env['ADAGUC_CHECKER_FILE']=reportfilename
        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&",
                                                                env = self.env, args=["--report"])
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(os.path.exists(reportfilename))
        self.env.pop('ADAGUC_CHECKER_FILE', None)
        if(os.path.exists(reportfilename)):
            os.remove(reportfilename)

    def test_WMSGetMap_testdatanc_customprojectionstring(self):
        AdagucTestTools().cleanTempDir()
        
        #https://geoservices.knmi.nl/cgi-bin/RADNL_OPER_R___25PCPRR_L3.cgi?SERVICE=WMS&REQUEST=GETMAP&VERSION=1.1.1&SRS%3DPROJ4%3A%2Bproj%3Dstere%20%2Bx_0%3D0%20%2By_0%3D0%20%2Blat_ts%3D60%20%2Blon_0%3D0%20%2Blat_0%3D90%20%2Ba%3D6378140%20%2Bb%3D6356750%20%2Bunits%3Dm&FORMAT=image/png&TRANSPARENT=true&WIDTH=750&HEIGHT=660&BBOX=100000,-4250000,600000,-3810000&LAYERS=RADNL_OPER_R___25PCPRR_L3_KNMI&TIME=2018-03-12T12:40:00
        
        filename="test_WMSGetMap_testdatanc_customprojectionstring.png"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=%2Bproj%3Dstere%20%2Bx_0%3D0%20%2By_0%3D0%20%2Blat_ts%3D60%20%2Blon_0%3D0%20%2Blat_0%3D90%20%2Ba%3D6378140%20%2Bb%3D6356750%20%2Bunits%3Dm&BBOX=100000,-4250000,600000,-3810000&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_WMSGetMap_testdatanc_customprojectionstring_proj4namespace(self):
        AdagucTestTools().cleanTempDir()
        
        filename="test_WMSGetMap_testdatanc_customprojectionstring_proj4namespace.png"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=PROJ4%3A%2Bproj%3Dstere%20%2Bx_0%3D0%20%2By_0%3D0%20%2Blat_ts%3D60%20%2Blon_0%3D0%20%2Blat_0%3D90%20%2Ba%3D6378140%20%2Bb%3D6356750%20%2Bunits%3Dm&BBOX=100000,-4250000,600000,-3810000&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

  
    def test_WMSGetCapabilitiesGetMap_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetCapabilities_testdatanc"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&request=getcapabilities", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))
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
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))
        
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
          for value in range(len(dims[list(dims.keys())[number-1]]['values'])):
            l[number-1] = value
            if number > 1:
                Recurse ( dims, number - 1 ,l)
            else:
                kvps = ""
                for i in reversed(range(len(l))):
                  key = (dims[list(dims)[i]]['wmsname'])
                  value = (dims[list(dims)[i]]['values'])[l[i]]
                  kvps += "&" + key +'=' + str(value)
                # print("Checking dims" + kvps)
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
        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb'], env = self.env, isCGI = False, showLogOnError = False)
        self.assertEqual(status, 1)
        
    def test_WMSCMDUpdateDB(self):
        AdagucTestTools().cleanTempDir()
        ADAGUC_PATH = os.environ['ADAGUC_PATH']
        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config', ADAGUC_PATH + '/data/config/adaguc.timeseries.xml'], isCGI = False, showLogOnError = False)
        self.assertEqual(status, 0)
        
        filename="test_WMSGetCapabilities_timeseries_twofiles"
        status,data,headers = AdagucTestTools().runADAGUCServer("SERVICE=WMS&request=getcapabilities", {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.timeseries.xml'})
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))
    
    def test_WMSCMDUpdateDBTailPath(self):
        AdagucTestTools().cleanTempDir()
        ADAGUC_PATH = os.environ['ADAGUC_PATH']
        
        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config', ADAGUC_PATH + '/data/config/adaguc.timeseries.xml', '--tailpath','netcdf_5dims_seq1'], isCGI = False, showLogOnError = False)
        self.assertEqual(status, 0)
        
        filename="test_WMSGetCapabilities_timeseries_tailpath_netcdf_5dims_seq1"
        status,data,headers = AdagucTestTools().runADAGUCServer("SERVICE=WMS&request=getcapabilities", {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.timeseries.xml'})
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))
        
        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config', ADAGUC_PATH + '/data/config/adaguc.timeseries.xml', '--tailpath','netcdf_5dims_seq2'], isCGI = False, showLogOnError = False)
        self.assertEqual(status, 0)
        

        filename="test_WMSGetCapabilities_timeseries_tailpath_netcdf_5dims_seq1_and_seq2"
        status,data,headers = AdagucTestTools().runADAGUCServer("SERVICE=WMS&request=getcapabilities", {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.timeseries.xml'})
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))

    def test_WMSCMDUpdateDBPath(self):
        AdagucTestTools().cleanTempDir()
        ADAGUC_PATH = os.environ['ADAGUC_PATH']
        status,data,headers = AdagucTestTools().runADAGUCServer(
          args = ['--updatedb', '--config', 
          ADAGUC_PATH + '/data/config/adaguc.timeseries.xml', 
          '--path', ADAGUC_PATH + '/data/datasets/netcdf_5dims/netcdf_5dims_seq1/nc_5D_20170101000000-20170101001000.nc'], 
          isCGI = False, 
          showLogOnError = False,
          showLog = False)
        self.assertEqual(status, 0)
        
        filename="test_WMSGetCapabilities_timeseries_path_netcdf_5dims_seq1"
        status,data,headers = AdagucTestTools().runADAGUCServer("SERVICE=WMS&request=getcapabilities", {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.timeseries.xml'})
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))
        
        status,data,headers = AdagucTestTools().runADAGUCServer(
          args = ['--updatedb', 
          '--config', ADAGUC_PATH + '/data/config/adaguc.timeseries.xml', 
          '--path', ADAGUC_PATH + '/data/datasets/netcdf_5dims/netcdf_5dims_seq2/nc_5D_20170101001500-20170101002500.nc'], 
          isCGI = False, 
          showLogOnError = False)
        self.assertEqual(status, 0)
        
        filename="test_WMSGetCapabilities_timeseries_path_netcdf_5dims_seq2"
        status,data,headers = AdagucTestTools().runADAGUCServer("SERVICE=WMS&request=getcapabilities", {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.timeseries.xml'})
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))

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
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))

    
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
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))

    def test_WMSGetMap_Report_nounits(self):
        AdagucTestTools().cleanTempDir()
        if os.path.exists(os.environ["ADAGUC_LOGFILE"]):
            os.remove(os.environ["ADAGUC_LOGFILE"])
        filename="test_WMSGetMap_Report_nounits"
        reportfilename="./nounits_checker_report.txt"
        status,data,headers = AdagucTestTools().runADAGUCServer(
            "source=test/testdata_report_nounits.nc&service=WMS&request=GetMap&version=1.3.0&layers=sow_a1&crs=EPSG%3A4326&bbox=47.80599631376197%2C1.4162628389784275%2C56.548995855839685%2C9.526486675156528&width=863&height=981&format=image%2Fpng&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_reference_time=2017-12-15T09%3A00%3A00Z",
            env=self.env, args=["--report=%s" % reportfilename], isCGI=False, showLogOnError=False)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 1)
        self.assertTrue(os.path.exists(reportfilename))
        self.assertTrue(os.path.exists(os.environ["ADAGUC_LOGFILE"]))

        reportfile = open(reportfilename, "r")
        report = json.load(reportfile)
        reportfile.close()
        os.remove(reportfilename)
        self.assertTrue("messages" in report)
        expectedErrors = ["No time units found for variable time"] ## add more errors to this list if we expect more.
        foundErrors = []
        #self.assertIsNone("TODO: test if error messages end up in normale log file as well as report.")
        for message in report["messages"]:
            self.assertTrue("category" in message)
            self.assertTrue("documentationLink" in message)
            self.assertTrue("message" in message)
            self.assertTrue("severity" in message)
            if (message["severity"] == "ERROR"):
                foundErrors.append(message["message"])
                self.assertIn(message["message"], expectedErrors)
        self.assertEqual(len(expectedErrors), len(foundErrors))

        expectedErrors.append("WMS GetMap Request failed")
        foundErrors = []
        with open(os.environ["ADAGUC_LOGFILE"]) as logfile:
            for line in logfile.readlines():
                if "E:" in line:
                    for error in expectedErrors:
                        if error in line:
                            foundErrors.append(error)
        logfile.close()
        self.assertEqual(len(expectedErrors), len(foundErrors))

    def test_WMSGetMap_NoReport_nounits(self):
        AdagucTestTools().cleanTempDir()
        if os.path.exists(os.environ["ADAGUC_LOGFILE"]):
            os.remove(os.environ["ADAGUC_LOGFILE"])
        filename="test_WMSGetMap_Report_nounits"
        reportfilename="./checker_report.txt"
        status,data,headers = AdagucTestTools().runADAGUCServer(
            "source=test/testdata_report_nounits.nc&service=WMS&request=GetMap&version=1.3.0&layers=sow_a1&crs=EPSG%3A4326&bbox=47.80599631376197%2C1.4162628389784275%2C56.548995855839685%2C9.526486675156528&width=863&height=981&format=image%2Fpng&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_reference_time=2017-12-15T09%3A00%3A00Z",
            env=self.env, isCGI=False, showLogOnError=False)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 1)
        self.assertFalse(os.path.exists(reportfilename))
        self.assertTrue(os.path.exists(os.environ["ADAGUC_LOGFILE"]))
        expectedErrors = ["No time units found for variable time",
                          "Exception in DBLoopFiles",
                          "Invalid dimensions values: No data available for layer sow_a1",
                          "WMS GetMap Request failed"]
        foundErrors = []
        with open(os.environ["ADAGUC_LOGFILE"]) as logfile:
            for line in logfile.readlines():
                if "E:" in line:
                    for error in expectedErrors:
                        if error in line:
                            foundErrors.append(error)
        logfile.close()
        self.assertEqual(len(expectedErrors), len(foundErrors))


    def test_WMSGetMap_worldmap_latlon_PNGFile_withoutinfofile(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetMap_worldmap_latlon_PNGFile_withoutinfofile.png"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=worldmap_latlon.png&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=pngdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=rgba%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_WMSGetMap_worldmap_mercator_PNGFile_withinfofile(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetMap_worldmap_mercator_PNGFile_withinfofile.png"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=worldmap_mercator.png&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=pngdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=rgba%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&", env = self.env)
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))        

    def test_WMSGetCapabilities_testdatanc_autostyle(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetCapabilities_testdatanc_autostyle.xml"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&request=getcapabilities", {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.tests.autostyle.xml'})
        
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))

    def test_WMSGetCapabilities_multidimnc_autostyle(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetCapabilities_multidimnc_autostyle.xml"
        status,data,headers = AdagucTestTools().runADAGUCServer("source=netcdf_5dims/netcdf_5dims_seq1/nc_5D_20170101000000-20170101001000.nc&SERVICE=WMS&request=getcapabilities", {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.tests.autostyle.xml'})
        
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))

    def test_WMSGetCapabilities_multidimncdataset_autostyle(self):
        AdagucTestTools().cleanTempDir()
        ADAGUC_PATH = os.environ['ADAGUC_PATH']


        config =  ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml,' + ADAGUC_PATH + '/data/config/datasets/adaguc.testmultidimautostyle.xml'
        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config', config], env = self.env, isCGI = False)
        self.assertEqual(status, 0)

        filename="test_WMSGetCapabilities_multidimncdataset_autostyle.xml"
        status,data,headers = AdagucTestTools().runADAGUCServer("dataset=adaguc.testmultidimautostyle&SERVICE=WMS&request=getcapabilities", {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.tests.dataset.xml'})
        
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertTrue(AdagucTestTools().compareGetCapabilitiesXML(self.testresultspath + filename, self.expectedoutputsspath + filename))

    def test_WMSGetMapWithShowLegendTrue_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetMapWithShowLegendTrue_testdatanc.png"

        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config',  ADAGUC_PATH + '/data/config/adaguc.tests.autostyle.xml'], env = self.env, isCGI = False)
        self.assertEqual(status, 0)

        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=geojsonbaselayer,testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata_style_2/shadedcontour&FORMAT=image/png&TRANSPARENT=FALSE&showlegend=true"
                                                                 , {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.tests.autostyle.xml'})
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_WMSGetMapWithShowLegendFalse_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetMapWithShowLegendFalse_testdatanc.png"

        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config',  ADAGUC_PATH + '/data/config/adaguc.tests.autostyle.xml'], env = self.env, isCGI = False)
        self.assertEqual(status, 0)

        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=geojsonbaselayer,testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata_style_2/shadedcontour&FORMAT=image/png&TRANSPARENT=FALSE&showlegend=false"
                                                                 , {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.tests.autostyle.xml'})
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_WMSGetMapWithShowLegendNothing_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetMapWithShowLegendNothing_testdatanc.png"

        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config',  ADAGUC_PATH + '/data/config/adaguc.tests.autostyle.xml'], env = self.env, isCGI = False)
        self.assertEqual(status, 0)

        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=geojsonbaselayer,testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata_style_2/shadedcontour&FORMAT=image/png&TRANSPARENT=FALSE&showlegend="
                                                                 , {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.tests.autostyle.xml'})
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))


    def test_WMSGetMapWithShowLegendSecondLayer_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetMapWithShowLegendSecondLayer_testdatanc.png"

        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config',  ADAGUC_PATH + '/data/config/adaguc.tests.autostyle.xml'], env = self.env, isCGI = False)
        self.assertEqual(status, 0)

        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=geojsonbaselayer,testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata_style_2/shadedcontour&FORMAT=image/png&TRANSPARENT=FALSE&showlegend=testdata"
                                                                 , {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.tests.autostyle.xml'})
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))

    def test_WMSGetMapWithShowLegendAllLayers_testdatanc(self):
        AdagucTestTools().cleanTempDir()
        filename="test_WMSGetMapWithShowLegendAllLayers_testdatanc.png"

        status,data,headers = AdagucTestTools().runADAGUCServer(args = ['--updatedb', '--config',  ADAGUC_PATH + '/data/config/adaguc.tests.autostyle.xml'], env = self.env, isCGI = False)
        self.assertEqual(status, 0)

        status,data,headers = AdagucTestTools().runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=geojsonbaselayer,testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata_style_2/shadedcontour&FORMAT=image/png&TRANSPARENT=FALSE&showlegend=geojsonbaselayer,testdata"
                                                                 , {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.tests.autostyle.xml'})
        AdagucTestTools().writetofile(self.testresultspath + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), AdagucTestTools().readfromfile(self.expectedoutputsspath + filename))
