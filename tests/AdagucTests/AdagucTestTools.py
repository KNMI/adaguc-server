import os
from io import BytesIO
from adaguc.CGIRunner import CGIRunner
import unittest
import shutil
import subprocess
from lxml import etree
from lxml import objectify
import re


ADAGUC_PATH = os.environ['ADAGUC_PATH']

class AdagucTestTools:
  def getLogFile(self):
      ADAGUC_LOGFILE = os.environ['ADAGUC_LOGFILE']
      try:
        f=open(ADAGUC_LOGFILE)
        data=f.read()
        f.close()
        return data
      except:
        pass
      return ""
    
  def printLogFile(self):
      ADAGUC_LOGFILE = os.environ['ADAGUC_LOGFILE']
      print("\n=== START ADAGUC LOGS ===")
      print(self.getLogFile())
      print("=== END ADAGUC LOGS ===")
      
  def runADAGUCServer(self, url = None, env = [], path = None, args = None, isCGI = True, showLogOnError = True, showLog = False):
    

    adagucenv=os.environ.copy()
    adagucenv.update(env)
    


    ADAGUC_PATH = adagucenv['ADAGUC_PATH']
    
    ADAGUC_LOGFILE = os.environ['ADAGUC_LOGFILE']
    
    try:
      os.remove(ADAGUC_LOGFILE);
    except:
      pass
    
    adagucexecutable = ADAGUC_PATH+"/bin/adagucserver";
    
    adagucargs = [adagucexecutable]
    
    if args is not None:
      adagucargs  = adagucargs + args
    
    os.chdir(ADAGUC_PATH+"/tests");
    
    

    filetogenerate =  BytesIO()
    status, headers = CGIRunner().run(adagucargs,url=url,output = filetogenerate, env=adagucenv, path=path, isCGI= isCGI)


    
    
    if (status != 0 and showLogOnError == True) or showLog == True:
      print("\n\n--- START ADAGUC DEBUG INFO ---")
      print("Adaguc-server has non zero exit status %d " % status)
      if isCGI == False:
        print(filetogenerate.getvalue())
      else:
        self.printLogFile();
      if status == -9: print("Process: Killed")
      if status == -11: print("Process: Segmentation Fault ")
      
      if len(headers)!=0: 
        print("=== START ADAGUC HTTP HEADER ===")
        print(headers)
        print("=== END ADAGUC HTTP HEADER ===")
      else: 
        print("Process: No HTTP Headers written")

      print("--- END ADAGUC DEBUG INFO ---\n")        
      return [status, filetogenerate, headers]

      
    else:  
      return [status,filetogenerate, headers]
      
  def writetofile(self, filename, data):
    with open(filename, 'wb') as f:
      f.write(data)

  def readfromfile(self, filename):
    ADAGUC_PATH = os.environ['ADAGUC_PATH']
    with open(ADAGUC_PATH + "/tests/" + filename, 'rb') as f:
      return f.read()
    
  def cleanTempDir(self):
    ADAGUC_TMP = os.environ['ADAGUC_TMP']
    try:
      shutil.rmtree(ADAGUC_TMP)
    except:
      pass
    self.mkdir_p(os.environ['ADAGUC_TMP']);
    return
  
  def mkdir_p(self, directory):
    if not os.path.exists(directory):
      os.makedirs(directory)
      
      
  def compareGetCapabilitiesXML(self,testresultFileLocation,expectedOutputFileLocation):
    expectedxml = self.readfromfile(expectedOutputFileLocation)
    testxml = self.readfromfile(testresultFileLocation)

    obj1 = objectify.fromstring(re.sub(b' xmlns="[^"]+"', b'', expectedxml, count=1))
    obj2 = objectify.fromstring(re.sub(b' xmlns="[^"]+"', b'', testxml, count=1))

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

    if (result == expect) is False:
        print("\nExpected XML is different, file \n\"%s\"\n  should be equal to \n\"%s\"" % (testresultFileLocation, expectedOutputFileLocation));
        
        
    return result == expect
