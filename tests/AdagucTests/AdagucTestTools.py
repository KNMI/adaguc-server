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
      print "\n=== START ADAGUC LOGS ==="
      print self.getLogFile()
      print "=== END ADAGUC LOGS ==="
      
  def runADAGUCServer(self, url = None, env = [], path = None, args = None, isCGI = True):
    

    adagucenv=os.environ.copy()
    adagucenv.update(env)
    


    ADAGUC_PATH = adagucenv['ADAGUC_PATH']
    
    adagucexecutable = ADAGUC_PATH+"/bin/adagucserver";
    
    adagucargs = [adagucexecutable]
    
    if args is not None:
      adagucargs  = adagucargs + args
    
    os.chdir(ADAGUC_PATH+"/tests");

    filetogenerate =  StringIO()
    status, headers = CGIRunner().run(adagucargs,url=url,output = filetogenerate, env=adagucenv, path=path, isCGI= isCGI)


    if status != 0:
      print("\n\n--- START ADAGUC DEBUG INFO ---")
      print("Adaguc-server has non zero exit status %d " % status)
      self.printLogFile();
      if status == -9: print("Process: Killed")
      if status == -11: print("Process: Segmentation Fault ")
      
      if len(headers)!=0: 
        print "=== START ADAGUC HTTP HEADER ==="
        print headers
        print "=== END ADAGUC HTTP HEADER ==="
      else: 
        print("Process: No HTTP Headers written")

      print("--- END ADAGUC DEBUG INFO ---\n")        
      return [status, filetogenerate, headers]

      
    else:  
      return [0,filetogenerate, headers]
      
  def writetofile(self, filename, data):
    with open(filename, 'w') as f:
      f.write(data)

  def readfromfile(self, filename):
    ADAGUC_PATH = os.environ['ADAGUC_PATH']
    with open(ADAGUC_PATH + "/tests/" + filename, 'r') as f:
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
