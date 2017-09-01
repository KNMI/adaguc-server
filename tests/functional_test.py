import subprocess
import os
from os.path import expanduser
from PIL import Image
from StringIO import StringIO
from adaguc.CGIRunner import CGIRunner
import io
import tempfile
import shutil

url="source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&"

url="source=testdata.nc&SERVICE=WMS&request=getcapabilities"

adagucenv=os.environ.copy()

ADAGUC_TMP = os.environ['ADAGUC_TMP']
ADAGUC_PATH = os.environ['ADAGUC_PATH']
ADAGUC_LOGFILE = os.environ['ADAGUC_LOGFILE']
adagucexecutable = ADAGUC_PATH+"/bin/adagucserver";

print ("Starting %s" % adagucexecutable)

filetogenerate =  StringIO()
status, headers = CGIRunner().run([adagucexecutable],url,output = filetogenerate,extraenv=adagucenv)

def printLogFile():
  print "--------- START ADAGUC LOGS ---------"
  f=open(ADAGUC_LOGFILE)
  print f.read()
  f.close()
  print "--------- END ADAGUC LOGS ---------"


if status != 0:
  printLogFile()
  
  print ("Adaguc-server has non zero exit status %d ", status)
  if status == -9: print("Process: Killed")
  if status == -11: print("Process: Segmentation Fault ")
  
  if len(headers)!=0: 
    print("Process: No HTTP Headers written")
    print headers

  
else:  
  try:
    """ Try to show the image """
    img = Image.open(StringIO(filetogenerate.getvalue()))
    img.show()
  except Exception as e: 
    print(e)
    
    f = (StringIO(filetogenerate.getvalue()))
    print "Data: [" + f.read() + "]"
      
    """ Otherwise print the logfile, giving information on what went wrong """
    printLogFile()
    pass

