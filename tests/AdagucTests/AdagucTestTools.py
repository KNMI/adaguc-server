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

  def runADAGUCServer(self, url, extraenv = []):
    def getLogFile():
      ADAGUC_LOGFILE = os.environ['ADAGUC_LOGFILE']
      f=open(ADAGUC_LOGFILE)
      data=f.read()
      f.close()
      return data

    def printLogFile(self):
      ADAGUC_LOGFILE = os.environ['ADAGUC_LOGFILE']
      print "--------- START ADAGUC LOGS ---------"
      print getLogFile()
      print "--------- END ADAGUC LOGS ---------"

    adagucenv=os.environ.copy()
    
    adagucenv.update(extraenv)

    ADAGUC_TMP = os.environ['ADAGUC_TMP']
    ADAGUC_PATH = os.environ['ADAGUC_PATH']
  
    adagucexecutable = ADAGUC_PATH+"/bin/adagucserver";
    
    os.chdir(ADAGUC_PATH+"/tests");

  

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
    return
