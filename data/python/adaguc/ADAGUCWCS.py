import urllib
from StringIO import StringIO
import isodate 
import time
import netCDF4
from netCDF4 import MFDataset
import sys
from subprocess import PIPE, Popen, STDOUT
from threading  import Thread
import json
import os
import shutil
from mkdir_p import *
import zipfile
from xml.sax.saxutils import escape
from xml.dom import minidom
import CGIRunner
import re
import logging;

  
    
def daterange(start_date, end_date, delta):
  d = start_date
  while d <= end_date:
    yield d
    d += delta
    

  
def which(program):
    import os
    def is_exe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK)

    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file

    return None



def cleanlog(tmpdir):
  try:
    os.remove(tmpdir+"/iteratewcs.log")
  except:
    pass
    

def dolog(tmpdir, data):
  with open(tmpdir+"/iteratewcs.log", "a") as myfile:
      myfile.write(str(data)+"\n")
      myfile.flush();

def openfile(file):
  with open (file, "r") as myfile:
    data = myfile.read()
    return data
  
def getlog(tmpdir):
  return openfile(tmpdir+"/iteratewcs.log")
  
  
def makezip(tmpdir,OUTFILE):
  currentpath=os.getcwd()
  os.chdir(tmpdir)
  """ Otherwise, make a zipfile of all files """
  zipf = zipfile.ZipFile(OUTFILE, 'w',zipfile.ZIP_DEFLATED)
  for root, dirs, files in os.walk("."):
    for file in files:
      zipf.write(os.path.join(root, file))
  zipf.close()
  os.chdir(currentpath)
  
def callADAGUC(adagucexecutable,tmpdir,LOGFILE,url,filetogenerate, env = {}):

  if(LOGFILE != None):
    env["ADAGUC_ERRORFILE"]=LOGFILE
  try:
    os.remove(tmpdir+"/adaguclog.log")
  except:
    pass
  env["ADAGUC_LOGFILE"]=tmpdir+"/adaguclog.log"
  
  
  
  #  status, headers = CGIRunner().run(adagucargs,url=url,output = filetogenerate, env=adagucenv, path=path, isCGI= isCGI)

  
  status, headers = CGIRunner.CGIRunner().run([adagucexecutable],url,output = filetogenerate,env=env)  
  return status


"""
  Describecoverage is done in order to retrieve the avaible dates in the coverage
"""
def describeCoverage(adagucexecutable,tmpdir,LOGFILE,WCSURL,COVERAGE, env = None):
  
  filetogenerate =  StringIO();
  #filetogenerate = tmpdir+"/describecoverage.xml"
  url = WCSURL + "&SERVICE=WCS&REQUEST=DescribeCoverage&";
  url = url + "COVERAGE="+COVERAGE+"&";
  status = callADAGUC(adagucexecutable,tmpdir,LOGFILE,url,filetogenerate, env = env);
  
  if(status != 0):
    adaguclog = openfile(tmpdir+"/adaguclog.log");
    raise ValueError( "Unable to retrieve "+url+"\n"+adaguclog+"\n");
  
  filetogenerate.seek(0, os.SEEK_END)
  filesize = filetogenerate.tell()

  if(filesize == 0):
    adaguclog = openfile(tmpdir+"/adaguclog.log");
    raise ValueError ("Succesfully completed WCS DescribeCoverage, but no data found. Log is: "+url+"\n"+adaguclog+"\n");
  
  #print filetogenerate.getvalue()
  try:
    xmldoc = minidom.parseString(filetogenerate.getvalue())
  except:
    adaguclog = openfile(tmpdir+"/adaguclog.log");
    raise ValueError ("Succesfully completed WCS DescribeCoverage, but unable to parse file for "+url+"\n"+adaguclog+"\n");
  try:
    itemlist = xmldoc.getElementsByTagName('gml:timePosition')
    if(len(itemlist)!=0):

      listtoreturn  = [];
      for s in itemlist:
        listtoreturn.append(isodate.parse_datetime(s.childNodes[0].nodeValue))
      return listtoreturn
    else:
      start_date = xmldoc.getElementsByTagName('gml:begin')[0].childNodes[0].nodeValue
      end_date = xmldoc.getElementsByTagName('gml:end')[0].childNodes[0].nodeValue
      res_date  = xmldoc.getElementsByTagName('gml:duration')[0].childNodes[0].nodeValue
      logging.debug(start_date);
      logging.debug(end_date);
      logging.debug(res_date);
      return list(daterange(isodate.parse_datetime(start_date),isodate.parse_datetime(end_date),isodate.parse_duration(res_date)));
  except:
    pass
  
  return []


def defaultCallback(message,percentage):
  return
  
"""
This requires a working ADAGUC server in the PATH environment, ADAGUC_CONFIG environment variable must point to ADAGUC's config file.
"""
def iteratewcs(TIME = "",BBOX = "-180,-90,180,90",CRS = "EPSG:4326",RESX=None,RESY=None,WIDTH=None, HEIGHT= None,WCSURL="",TMP=".",COVERAGE="pr",LOGFILE=None,OUTFILE="out.nc",FORMAT="netcdf",CALLBACK=defaultCallback):
  maxRequestsAtOnce = 1
  adagucenv=os.environ.copy()
  #adagucenv.update(env)
  ADAGUC_PATH = adagucenv['ADAGUC_PATH']
  adagucenv.update({'ADAGUC_CONFIG': ADAGUC_PATH + '/data/config/adaguc.autoresource.xml'});
  adagucenv.update({'ADAGUC_TMP': TMP});
  adagucexecutable = ADAGUC_PATH+"/bin/adagucserver";
  """ Check if adagucserver is in the path """
  if(which(adagucexecutable) == None):
    raise ValueError("ADAGUC Executable '"+adagucexecutable+"' not found in PATH");
  
  CALLBACK("Starting iterateWCS",1)
  tmpdir = TMP+"/iteratewcstmp";
  shutil.rmtree(tmpdir, ignore_errors=True)
  mkdir_p(tmpdir);
  
  """ Determine which dates to do based on describe coverage call"""
  CALLBACK("Starting WCS DescribeCoverage request",1)
  
  founddates = describeCoverage(adagucexecutable,tmpdir,LOGFILE,WCSURL,COVERAGE, env = adagucenv);
  
  start_date=""
  end_date=""
  
  if TIME and len(TIME) > 0 :
    if len(TIME.split("/")) >= 2:
      start_date = isodate.parse_datetime(TIME.split("/")[0]);
      end_date = isodate.parse_datetime(TIME.split("/")[1]);
    else:
      if TIME == None or TIME == "*" or TIME == "None":
        start_date = isodate.parse_datetime("0001-01-01T00:00:00Z");
        end_date = isodate.parse_datetime("9999-01-01T00:00:00Z");
      else:
        start_date = isodate.parse_datetime(TIME);
        end_date = isodate.parse_datetime(TIME);
  
  
  CALLBACK("File has "+str(len(founddates))+" dates",1)
  datestodo = []
  
  
  if len(founddates) > 0:
        for date in founddates:
            if(date>=start_date and date<=end_date):
                datestodo.append(date)
  else:
    datestodo.append("*");
  
  CALLBACK("Found "+str(len(datestodo))+" dates",1)
  
  if(len(datestodo) == 0):
    raise ValueError("No data found in resource for given dates. Possible date range should be within "+str(founddates[0])+" and "+str(founddates[-1]))
  
  
  numdatestodo=len(datestodo);
  datesdone = 0;
  filetogenerate = ""
  CALLBACK("Starting Iterating WCS GetCoverage",1)
  
  def makeGetCoverage(single_date, env = {}):
    filetime=""
    wcstime=""
    messagetime = ""
    
    url = WCSURL + "&SERVICE=WCS&REQUEST=GetCoverage&";
    url = url + "FORMAT="+urllib.quote_plus(FORMAT)+"&";
    url = url + "COVERAGE="+urllib.quote_plus(COVERAGE)+"&";
    
    logging.debug("WCS GetCoverage URL: "+str(url));
    
    for j in range(0,len(single_date)):
      wcsdate = single_date[j]
      if wcsdate != "*":
        if(j>0):
          wcstime=wcstime + ","
        wcstime=wcstime + time.strftime("%Y-%m-%dT%H:%M:%SZ", wcsdate.timetuple())
        filetime=time.strftime("%Y%m%dT%H%M%SZ", single_date[0].timetuple()) + '-'  + time.strftime("%Y%m%dT%H%M%SZ", single_date[-1].timetuple())
        messagetime=time.strftime("%Y%m%dT%H%M%SZ", single_date[0].timetuple()) 
        url = url + "TIME="+urllib.quote_plus(wcstime)+"&";
        
        
    url = url + "BBOX="+BBOX+"&";
    if RESX != None:
      url = url + "RESX="+str(RESX)+"&";
    if RESY != None:
      url = url + "RESY="+str(RESY)+"&";
    if WIDTH != None:
      url = url + "WIDTH="+str(WIDTH)+"&";
    if HEIGHT != None:
      url = url + "HEIGHT="+str(HEIGHT)+"&";
      
    url = url + "CRS="+urllib.quote_plus(CRS)+"&";
    logging.debug(url);
    filetogenerate = tmpdir+"/file"+filetime
    
    if(FORMAT == "netcdf"):
      filetogenerate = filetogenerate  + ".nc"
    if(FORMAT == "geotiff"):
      filetogenerate = filetogenerate  + ".tiff"
    if(FORMAT == "aaigrid"):
      filetogenerate = filetogenerate  + ".grd"
    

    stringioobject = StringIO();
    
    status = callADAGUC(adagucexecutable,tmpdir,LOGFILE,url,stringioobject, env = env);
    with open (filetogenerate, 'w') as fd:
        stringioobject.seek (0)
        shutil.copyfileobj (stringioobject, fd)
    if(status != 0):
      adaguclog = openfile(tmpdir+"/adaguclog.log");
      raise ValueError( "Unable to retrieve "+url+"\n"+adaguclog+"\n");
    
    if(os.path.isfile(filetogenerate) != True):
      adaguclog = openfile(tmpdir+"/adaguclog.log");
      raise ValueError ("Succesfully completed WCS GetCoverage, but no data found for "+url+"\n"+adaguclog+"\n");
   
    if(CALLBACK==None):
      print str(int((float(datesdone)/numdatestodo)*90.))
    else:
      CALLBACK(messagetime,((float(datesdone)/float(numdatestodo))*90.))
    return filetogenerate
  
  """ Make the WCS GetCoverage calls """
  grouped_dates = []

  for single_date in datestodo:
    grouped_dates.append(single_date) 
    datesdone=datesdone+1;
    if (len(grouped_dates) >= maxRequestsAtOnce):
      filetogenerate = makeGetCoverage(grouped_dates, env = adagucenv)    
      grouped_dates = []
    
  if len(grouped_dates) > 0:
    filetogenerate = makeGetCoverage(grouped_dates, env = adagucenv)
 
  def monitor2(line):
    dolog(tmpdir,line);
    try:
      data = json.loads(line)
      if(CALLBACK == None):
        print float(data["percentage"])*(1./10)+90
      else:
        CALLBACK(data["message"],float(data["percentage"])*(1./10)+90)
    except:
      CALLBACK(line,50)
  
  
  """ If it is netcdf, make a new big netcdf file """
  if(FORMAT == "netcdf"):
    if datesdone == 1:
      shutil.copyfile(filetogenerate ,OUTFILE)
    if datesdone > 1:
      cleanlog(tmpdir);
      dolog(tmpdir,tmpdir)
      dolog(tmpdir,OUTFILE)
      aggregate_timeexecutable = ADAGUC_PATH+"/bin/aggregate_time";
      cmds=[aggregate_timeexecutable,tmpdir,OUTFILE]
      dolog(tmpdir,cmds)
      status = CGIRunner.CGIRunner().startProcess(cmds,monitor2)
      
      if(status != 0):
        dolog(tmpdir,"statuscode: "+str(status))
        
        data = getlog(tmpdir)
        
        raise ValueError('Unable to aggregate: statuscode='+str(status)+"\n"+data)
      
  else:
    makezip(tmpdir,OUTFILE)
    
    
  logging.debug("Writing to "+str(OUTFILE));  
  shutil.rmtree(tmpdir, ignore_errors=True)  
  return 0
  

