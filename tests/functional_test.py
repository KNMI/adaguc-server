import os
from StringIO import StringIO
from adaguc.CGIRunner import CGIRunner
import unittest
import shutil


def runADAGUCServer(url):
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
    overWriteExpectedData = False
    
    def test_WMSGetCapabilities_testdatanc(self):
        cleanTempDir()
        filename="expectedoutputs/test_WMSGetCapabilities_testdatanc"
        status,data = runADAGUCServer("source=testdata.nc&SERVICE=WMS&request=getcapabilities")
        if self.overWriteExpectedData: writetofile(filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), readfromfile(filename))

    def test_WMSGetMap_testdatanc(self):
        cleanTempDir()
        filename="expectedoutputs/test_WMSGetMap_testdatanc"
        status,data = runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&")
        if self.overWriteExpectedData: writetofile(filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), readfromfile(filename))
        
    def test_WMSGetCapabilitiesGetMap_testdatanc(self):
        cleanTempDir()
        filename="expectedoutputs/test_WMSGetCapabilities_testdatanc"
        status,data = runADAGUCServer("source=testdata.nc&SERVICE=WMS&request=getcapabilities")
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), readfromfile(filename))
        filename="expectedoutputs/test_WMSGetMap_testdatanc"
        status,data = runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&")
        if self.overWriteExpectedData: writetofile(filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), readfromfile(filename))
        
    def test_WMSGetMapGetCapabilities_testdatanc(self):
        cleanTempDir()
        filename="expectedoutputs/test_WMSGetMap_testdatanc"
        status,data = runADAGUCServer("source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&")
        if self.overWriteExpectedData: writetofile(filename,data.getvalue())
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), readfromfile(filename))
        filename="expectedoutputs/test_WMSGetCapabilities_testdatanc"
        status,data = runADAGUCServer("source=testdata.nc&SERVICE=WMS&request=getcapabilities")
        self.assertEqual(status, 0)
        self.assertEqual(data.getvalue(), readfromfile(filename))
        
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
                filename="expectedoutputs/test_WMSGetMap_getmap_3dims_"+kvps+".png"
                filename = filename.replace("&","_").replace(":","_").replace("=","_");
                #print filename
                url="source=netcdf_5dims_seq1.nc&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=data&WIDTH=360&HEIGHT=180&CRS=EPSG%3A4326&BBOX=-90,-180,90,180&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&COLORSCALERANGE=0,1&"
                url+=kvps
                status,data = runADAGUCServer(url)
                if self.overWriteExpectedData: 
                  writetofile(filename,data.getvalue())
                self.assertEqual(status, 0)
                self.assertEqual(data.getvalue(), readfromfile(filename))
        l = []

        for i in range(len(dims)):
          l.append(0)      
        Recurse(dims,len(dims),l)

       
    
   

if __name__ == '__main__':
    unittest.main()
    
#a = TestStringMethods
#a.test_WMSGetMap_getmap_3dims_member6_3000_20170101001500()  