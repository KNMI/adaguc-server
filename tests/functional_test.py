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
  shutil.rmtree(ADAGUC_TMP)
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

   

if __name__ == '__main__':
    unittest.main()