#import wcsrequest

import os
import netCDF4
import urllib2
import warnings
import numpy
from sets import Set
import logging
import ADAGUCWCS
from netCDF4 import num2date 

def defaultCallback(message,percentage):
  print "defaultCallback:: "+message+" "+str(percentage)

def printfield(featuredata):
  for y in range(0,numpy.shape(featuredata)[0]):
    mstr = ""
    for x in range(0,numpy.shape(featuredata)[1]):
      mstr = "%s%0.2d" %(mstr, (featuredata[y][x]))
    print mstr

def ADAGUCFeatureCombineNuts( featureNCFile,dataNCFile,bbox= "-40,20,60,85",variable = None, time= None,width=300,height=300,crs="EPSG:4326",outncfile="/tmp/stat.nc",outcsvfile="/tmp/stat.csv",callback=defaultCallback, tmpFolderPath = "/tmp", homeFolderPath="/tmp"):

  os.chdir(homeFolderPath)
  
  # 25 to 50
  def featureCallBack(m,p):
    callback("Retrieving features:[%s]" % m,25+(p/100.)*25);
    return
  # 50 to 75
  def dataCallBack(m,p):
    callback("Retrieving data:[%s]" % m,50+(p/100.)*25);
    return

  status = ADAGUCWCS.iteratewcs(
    TIME=time,
    BBOX=bbox,
    CRS=crs,
    WCSURL="source="+featureNCFile,
    WIDTH=width,
    HEIGHT=height,
    COVERAGE='features',
    TMP=tmpFolderPath,
    OUTFILE=tmpFolderPath+"/features.nc",
    FORMAT="netcdf",
    LOGFILE=tmpFolderPath+"/adagucerrlogfeatures.txt",
    CALLBACK=featureCallBack)

  status = ADAGUCWCS.iteratewcs(
    TIME=time,
    BBOX=bbox,
    CRS=crs,
    WCSURL="source="+dataNCFile,
    WIDTH=width,
    HEIGHT=height,
    COVERAGE=variable,
    TMP=tmpFolderPath,
    OUTFILE=tmpFolderPath+"/data.nc",
    FORMAT="netcdf",
    LOGFILE=tmpFolderPath+"/adagucerrlogfeatures.txt",
    CALLBACK=dataCallBack)
        
  callback("Starting feature overlay",75);        
  statistic_names={"mean":"Average",
                  "minimum":"Minumum",
                  "maximum":"Maximum",
                  "standarddeviation":"Standard deviation",
                  "masked":"Masked"}

  logging.debug("Reading from "+str(tmpFolderPath+"/features.nc"));
  nc_features = netCDF4.Dataset( tmpFolderPath+"/features.nc",'r')
  
  nutsidvar = None
  nutsascivar = None
  try:
    nutsidvar = nc_features.variables["features_NUTS_ID"]
  except:
    pass
  try:
    nutsascivar = nc_features.variables["features_NAME_ASCI"]
  except:
    pass
  featurevar = nc_features.variables["features"]

  varstodo=[];
  nc_data = netCDF4.Dataset( tmpFolderPath+"/data.nc",'r')
  for v in nc_data.variables:
    if len(nc_data.variables[v].dimensions)>2:
      if v!="x" and v!="y" and v!="lon" and v!="lat":
        varstodo.append(v)

  logging.debug('writing to %s' % outncfile);
  nc_out = netCDF4.Dataset( outncfile,'w',format="NETCDF4")

  for var_name, dimension in nc_data.dimensions.iteritems():
    nc_out.createDimension(var_name, len(dimension) if not dimension.isunlimited() else None)
    

  for var_name, ncvar in nc_data.variables.iteritems():
    outVar = nc_out.createVariable(var_name, ncvar.datatype, ncvar.dimensions)
    ad = dict((k , ncvar.getncattr(k) ) for k in ncvar.ncattrs() )
    outVar.setncatts(  ad  )
    try:
      outVar[:] = ncvar[:]
    except:
      try: 
        outVar = ncvar
      except:
        logging.debug("Data for variable "+str(var_name)+" could not be written")
        pass
      pass
    """ When a 2D+ var hasbeen found, copy its name and create vars for all statistics we want to calculate """
    if var_name in varstodo:
      for name in statistic_names:
        new_var_name = var_name+"_"+name
        outVar = nc_out.createVariable(new_var_name, "f4", ncvar.dimensions,fill_value=-9999.0 )
        ad={}
        for k in ["units","standard_name","long_name"]:
          try:
            ad[k]=ncvar.getncattr(k)
          except:
            ad[k]="none"
            pass
        if "long_name" in ad:
          ad["long_name"]=statistic_names[name]+' of '+ad["long_name"]
        outVar.setncatts(  ad  )



  """ Copy NutsID names to output file """
  for var_name, dimension in nc_features.dimensions.iteritems():
    if not var_name in nc_out.dimensions:
      nc_out.createDimension(var_name, len(dimension) if not dimension.isunlimited() else None)
  outVar = nc_out.createVariable("features", "i4", featurevar.dimensions)
  outVar[:] = featurevar[:]
  
  nutsiddata = None
  nutsascidata = None
  if nutsidvar is not None:
    nutsiddata = nutsidvar[:]
  if nutsascivar is not None:
    nutsascidata = nutsascivar[:]
  featureindexdata=featurevar[:]
  featureindexdata.mask=False

  featureCount = {}
  featureSum = {}
  
  CSV="time;variable;index;id;name;numsamples;min;mean;max;std;\n"

  numVariables = len(varstodo)
  numVariablesDone = -1;
  """ Iterate over all variables """
  for currentVarName in varstodo:
    numVariablesDone = numVariablesDone + 1
    nodatavalue= featurevar._FillValue
    invar_datain = nc_data.variables[currentVarName]
    outvar_min   = nc_out.variables[currentVarName+"_minimum"];
    outvar_mean  = nc_out.variables[currentVarName+"_mean"];
    outvar_max   = nc_out.variables[currentVarName+"_maximum"];
    outvar_std   = nc_out.variables[currentVarName+"_standarddeviation"];
    outvar_mask  = nc_out.variables[currentVarName+"_masked"];
    
    """ Iterate over all timesteps """
    timeValue = "None"
    timeVar = nc_data.variables["time"]
    calendarAttr = "standard"
    try:
        calendarAttr=timeVar.calendar
    except:
        pass
    
    numTimeSteps = numpy.shape(timeVar)[0]
    for currentStep in range(0,numTimeSteps):
      """ Read time value """
      timeValueDouble = timeVar[currentStep]
      timeValue = num2date(timeValueDouble, units=timeVar.units,calendar=calendarAttr).isoformat()#strftime("%Y %M %D %h %m %S")
      """ Read data from netCDF Variables """
      datainflat = invar_datain[currentStep]
      dataout_minflat = outvar_min[currentStep]
      dataout_meanflat = outvar_mean[currentStep]
      dataout_maxflat = outvar_max[currentStep]
      dataout_stdflat = outvar_std[currentStep]
      dataout_maskflat = outvar_mask[currentStep]
      totalmean = numpy.nanmean(datainflat)
      foundregionindexes=numpy.unique(featureindexdata)
      numRegionsDone = -1
      numRegionIndexes = len(foundregionindexes)
      for regionindex in foundregionindexes:
        numRegionsDone += 1
        if numRegionsDone % 10 == 0:
          fracVarsDone = numVariablesDone / float(numVariables)
          fracStepDone = currentStep / float(numTimeSteps)
          fracRegionDone = numRegionsDone / float(numRegionIndexes)
          fracRegionTimeDone = fracStepDone + (fracRegionDone/float(numTimeSteps))
          fracRegionTimeVarsDone = fracVarsDone + (fracRegionTimeDone/float(numVariables))
          callback("For var %s and time (%d/%d) working on feature index nr %d (%d/%d)" %(currentVarName,currentStep,numTimeSteps,regionindex,numRegionsDone,numRegionIndexes),(fracRegionTimeVarsDone)*24.+75.);  
        
        if not regionindex is numpy.ma.masked and nodatavalue != regionindex:
          indices = numpy.where(featureindexdata==regionindex)
          selecteddata=datainflat[indices]
          
          allmask = False
          try:
            if selecteddata.mask.all() == True:
              allmask = True
          except:
            pass
            
          if allmask == False:
            try: 
              minval =  numpy.nanmin(selecteddata)
              meanval=  numpy.nanmean(selecteddata)
              maxval =  numpy.nanmax(selecteddata)
              stdval =  numpy.nanstd(selecteddata)
              dataout_minflat[indices]=minval
              dataout_meanflat[indices]=meanval
              dataout_maxflat[indices]=maxval
              dataout_stdflat[indices]=stdval
              dataout_maskflat[indices]=selecteddata
              regid = regionindex
              reglongname=regid
              if nutsiddata is not None:
                regid = nutsiddata[regionindex]
              if nutsascidata is not None:
                reglongname = nutsascidata[regionindex]
              CSV += timeValue+";"+str(currentVarName)+";"+str(regionindex)+";"+str(regid)+";"+str(reglongname)+";"+str(len(selecteddata))+";"+str(minval)+";"+str(meanval)+";"+str(maxval)+";"+str(stdval)+"\n"
            except ValueError: 
              logging.debug('Masks all around!')
              pass 
          
      """ Assign each timestep to NetCDF variables """
      outvar_min[currentStep]=dataout_minflat;
      outvar_mean[currentStep]=dataout_meanflat;
      outvar_max[currentStep]=dataout_maxflat;
      outvar_std[currentStep]=dataout_stdflat;
      outvar_mask[currentStep]=dataout_maskflat;
  
  callback("Writing data",99);  
  nc_out.close()  

  out = open( outcsvfile , 'wb')
  out.write( CSV )
  out.close()
  return

def test():

  ADAGUCFeatureCombineNuts(
      featureNCFile = "countries.geojson",
      dataNCFile = "testdata.nc",
      bbox= "0,50,10,55",
      time= "*",
      variable="testdata", 
      width=80,
      height=80,
      outncfile="nutsstat.nc",
      outcsvfile="nutsstat.csv", 
      tmpFolderPath="/tmp")
  


#test()    
