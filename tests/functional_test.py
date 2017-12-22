import os
from StringIO import StringIO
from adaguc.CGIRunner import CGIRunner
import unittest
import shutil
import subprocess
from lxml import etree
from lxml import objectify
import re

FNULL = open(os.devnull, 'w')
ADAGUC_PATH = os.environ['ADAGUC_PATH']


def runADAGUCServer(url, extraenv = []):
  def getLogFile():
    ADAGUC_LOGFILE = os.environ['ADAGUC_LOGFILE']
    f=open(ADAGUC_LOGFILE)
    data=f.read()
    f.close()
    return data

  def printLogFile():
    ADAGUC_LOGFILE = os.environ['ADAGUC_LOGFILE']
    print "--------- START ADAGUC LOGS ---------"
    print getLogFile()
    print "--------- END ADAGUC LOGS ---------"

  adagucenv=os.environ.copy()
  
  adagucenv.update(extraenv)

  ADAGUC_TMP = os.environ['ADAGUC_TMP']
  ADAGUC_PATH = os.environ['ADAGUC_PATH']
 
  adagucexecutable = ADAGUC_PATH+"/bin/adagucserver";

 

  filetogenerate =  StringIO()
  status, headers = CGIRunner().run([adagucexecutable],url,output = filetogenerate,extraenv=adagucenv)


  if status != 0:
    printLogFile()
    
    print ("Adaguc-server has non zero exit status %d ", status)
    if status == -9: print("Process: Killed")
    if status == -11: print("Process: Segmentation Fault ")
    
    if len(headers)!=0: 
      print("Process: No HTTP Headers written")
      print headers
    return [status]

    
  else:  
    return [0,filetogenerate]
    
def writetofile(filename, data):
  with open(filename, 'w') as f:
    f.write(data)

def readfromfile(filename):
  ADAGUC_PATH = os.environ['ADAGUC_PATH']
  with open(ADAGUC_PATH + "/tests/" + filename, 'r') as f:
    return f.read()
  
def cleanTempDir():
  ADAGUC_TMP = os.environ['ADAGUC_TMP']
  try:
    shutil.rmtree(ADAGUC_TMP)
  except:
    pass
  return

class TestStringMethods(unittest.TestCase):
    def compareXML(self,xml,expectedxml):
        obj1 = objectify.fromstring(re.sub(' xmlns="[^"]+"', '', expectedxml, count=1))
        obj2 = objectify.fromstring(re.sub(' xmlns="[^"]+"', '', xml, count=1))
        for child in obj1.findall("Service/KeywordList")[0]:child.getparent().remove(child)
        for child in obj2.findall("Service/KeywordList")[0]:child.getparent().remove(child)
        result = etree.tostring(obj1)     
        expect = etree.tostring(obj2)     
        self.assertEquals(expect, result)
    
    def test_WMSGetCapabilities_testdatanc(self):
        cleanTempDir()
        filename="test_WMSGetCapabilities_testdatanc"
        status,data = runADAGUCServer("source=testdata.nc&SERVICE=WMS&request=getcapabilities")
        writetofile("testresults/" + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.compareXML(data.getvalue(), readfromfile("expectedoutputs/" + filename))

    def test_WMSGetMap_testdatanc(self):
        cleanTempDir()
        filename="test_WMSGetMap_testdatanc"
        status,data = runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&")
        writetofile("testresults/" + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), readfromfile("expectedoutputs/" + filename))
        
    def test_WMSGetCapabilitiesGetMap_testdatanc(self):
        cleanTempDir()
        filename="test_WMSGetCapabilities_testdatanc"
        status,data = runADAGUCServer("source=testdata.nc&SERVICE=WMS&request=getcapabilities")
        writetofile("testresults/" + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.compareXML(data.getvalue(), readfromfile("expectedoutputs/" + filename))
        filename="test_WMSGetMap_testdatanc"
        status,data = runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&")
        writetofile("testresults/" + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), readfromfile("expectedoutputs/" + filename))
        
    def test_WMSGetMapGetCapabilities_testdatanc(self):
        cleanTempDir()
        filename="test_WMSGetMap_testdatanc"
        status,data = runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&")
        writetofile("testresults/" + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), readfromfile("expectedoutputs/" + filename))
        filename="test_WMSGetCapabilities_testdatanc"
        status,data = runADAGUCServer("source=testdata.nc&SERVICE=WMS&request=getcapabilities")
        writetofile("testresults/" + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.compareXML(data.getvalue(), readfromfile("expectedoutputs/" + filename))
        
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

        cleanTempDir()
        
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
                status,data = runADAGUCServer(url)
                writetofile("testresults/" + filename,data.getvalue())
                self.assertEqual(status, 0)
                self.assertEqual(data.getvalue(), readfromfile("expectedoutputs/" + filename))
        l = []

        for i in range(len(dims)):
          l.append(0)      
        Recurse(dims,len(dims),l)

    def test_WMSCMDUpdateDBNoConfig(self):
        cleanTempDir()
        args = [ADAGUC_PATH+'/bin/adagucserver', '--updatedb']
        returnCode = subprocess.call(args, stdout=FNULL, stderr=subprocess.STDOUT) 
        self.assertEqual(returnCode, 1)
        
    def test_WMSCMDUpdateDB(self):
        cleanTempDir()
        ADAGUC_PATH = os.environ['ADAGUC_PATH']
        args = [ADAGUC_PATH+'/bin/adagucserver', '--updatedb', '--config', ADAGUC_PATH + '/data/config/adaguc.timeseries.xml']
        returnCode = subprocess.call(args, stdout=FNULL, stderr=subprocess.STDOUT) 
        self.assertEqual(returnCode, 0)
        
        filename="test_WMSGetCapabilities_timeseries_twofiles"
        status,data = runADAGUCServer("SERVICE=WMS&request=getcapabilities", {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.timeseries.xml'})
        writetofile("testresults/" + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.compareXML(data.getvalue(), readfromfile("expectedoutputs/" + filename))
    
    def test_WMSCMDUpdateDBTailPath(self):
        cleanTempDir()
        ADAGUC_PATH = os.environ['ADAGUC_PATH']
        args = [ADAGUC_PATH+'/bin/adagucserver', '--updatedb', '--config', ADAGUC_PATH + '/data/config/adaguc.timeseries.xml', '--tailpath','netcdf_5dims_seq1']
        returnCode = subprocess.call(args, stdout=FNULL, stderr=subprocess.STDOUT) 
        self.assertEqual(returnCode, 0)
        
        filename="test_WMSGetCapabilities_timeseries_tailpath_netcdf_5dims_seq1"
        status,data = runADAGUCServer("SERVICE=WMS&request=getcapabilities", {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.timeseries.xml'})
        writetofile("testresults/" + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.compareXML(data.getvalue(), readfromfile("expectedoutputs/" + filename))
        
        args = [ADAGUC_PATH+'/bin/adagucserver', '--updatedb', '--config', ADAGUC_PATH + '/data/config/adaguc.timeseries.xml', '--tailpath','netcdf_5dims_seq2']
        returnCode = subprocess.call(args, stdout=FNULL, stderr=subprocess.STDOUT) 
        self.assertEqual(returnCode, 0)

        filename="test_WMSGetCapabilities_timeseries_tailpath_netcdf_5dims_seq1_and_seq2"
        status,data = runADAGUCServer("SERVICE=WMS&request=getcapabilities", {'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.timeseries.xml'})
        writetofile("testresults/" + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.compareXML(data.getvalue(), readfromfile("expectedoutputs/" + filename))

    def test_WMSGetFeatureInfo_forecastreferencetime_texthtml(self):
        cleanTempDir()
        filename="test_WMSGetFeatureInfo_forecastreferencetime.html"
        status,data = runADAGUCServer("source=forecast_reference_time%2FHARM_N25_20171215090000_dimx16_dimy16_dimtime49_dimforecastreferencetime1_varairtemperatureat2m.nc&SERVICE=WMS&REQUEST=GetFeatureInfo&VERSION=1.3.0&LAYERS=air_temperature__at_2m&QUERY_LAYERS=air_temperature__at_2m&CRS=EPSG%3A4326&BBOX=49.55171074378079,1.4162628389784275,54.80328142582087,9.526486675156528&WIDTH=1515&HEIGHT=981&I=832&J=484&FORMAT=image/gif&INFO_FORMAT=text/html&STYLES=&&time=2017-12-17T09%3A00%3A00Z&DIM_reference_time=2017-12-15T09%3A00%3A00Z")
        writetofile("testresults/" + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), readfromfile("expectedoutputs/" + filename))
        filename="test_WMSGetCapabilities_testdatanc"
        status,data = runADAGUCServer("source=testdata.nc&SERVICE=WMS&request=getcapabilities")
        writetofile("testresults/" + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.compareXML(data.getvalue(), readfromfile("expectedoutputs/" + filename))      

    
    def test_WMSGetFeatureInfo_timeseries_forecastreferencetime_json(self):
        cleanTempDir()
        filename="test_WMSGetFeatureInfo_timeseries_forecastreferencetime.json"
        status,data = runADAGUCServer("source=forecast_reference_time%2FHARM_N25_20171215090000_dimx16_dimy16_dimtime49_dimforecastreferencetime1_varairtemperatureat2m.nc&service=WMS&request=GetFeatureInfo&version=1.3.0&layers=air_temperature__at_2m&query_layers=air_temperature__at_2m&crs=EPSG%3A4326&bbox=47.80599631376197%2C1.4162628389784275%2C56.548995855839685%2C9.526486675156528&width=910&height=981&i=502&j=481&format=image%2Fgif&info_format=application%2Fjson&time=1000-01-01T00%3A00%3A00Z%2F3000-01-01T00%3A00%3A00Z&dim_reference_time=2017-12-15T09%3A00%3A00Z")
        writetofile("testresults/" + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), readfromfile("expectedoutputs/" + filename))
        filename="test_WMSGetCapabilities_testdatanc"
        status,data = runADAGUCServer("source=testdata.nc&SERVICE=WMS&request=getcapabilities")
        writetofile("testresults/" + filename,data.getvalue())
        self.assertEqual(status, 0)
        self.compareXML(data.getvalue(), readfromfile("expectedoutputs/" + filename))      

if __name__ == '__main__':
    unittest.main()
    
#a = TestStringMethods
#a.test_WMSGetMap_getmap_3dims_member6_3000_20170101001500()  
