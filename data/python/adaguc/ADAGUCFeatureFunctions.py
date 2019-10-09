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
from datetime import date

#logging.basicConfig(level=logging.DEBUG)

def defaultCallback(message,percentage):
  print "defaultCallback:: "+message+" "+str(percentage)

def printfield(featuredata):
  for y in range(0,numpy.shape(featuredata)[0]):
    mstr = ""
    for x in range(0,numpy.shape(featuredata)[1]):
      mstr = "%s%0.2d" %(mstr, (featuredata[y][x]))
    print mstr

def ADAGUCFeatureCombineNuts( featureNCFile,dataNCFile,bbox= "-40,20,60,85",variable = None, time= None,width=300,height=300,crs="EPSG:4326",outncrasterfile="/tmp/stat.nc",outncpointfile="/tmp/statpoints.nc", outcsvfile="/tmp/stat.csv",callback=defaultCallback, tmpFolderPath = "/tmp", homeFolderPath="/tmp"):

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
    try:
      nutsidvar = nc_features.variables["features_name"]
    except:
      pass
    pass
  try:
    nutsascivar = nc_features.variables["features_NAME_ASCI"]
  except:
    pass
  featurevar = nc_features.variables["features"]
  featureindexdata=featurevar[:]
  featureindexdata.mask=False
  foundregionindexes=numpy.unique(featureindexdata)
  
  varstodo=[];
  nc_data = netCDF4.Dataset( tmpFolderPath+"/data.nc",'r')
  for v in nc_data.variables:
    if len(nc_data.variables[v].dimensions)>=2:
      if v!="x" and v!="y" and v!="lon" and v!="lat":
        varstodo.append(v)

  # Create raster NETCDF
  logging.debug('writing outncrasterfile to %s' % outncrasterfile);
  nc_raster_out = netCDF4.Dataset( outncrasterfile,'w',format="NETCDF4")

  for var_name, dimension in reversed(list(nc_data.dimensions.iteritems())):
    nc_raster_out.createDimension(var_name, len(dimension if not dimension.isunlimited() else None))
    
  for var_name, ncvar in nc_data.variables.iteritems():
    outVar = nc_raster_out.createVariable(var_name, ncvar.datatype, ncvar.dimensions)
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
        outVar = nc_raster_out.createVariable(new_var_name, "f4", ncvar.dimensions, fill_value=-9999.0 )
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

  timeValue = "None"
  timeVar = None
  calendarAttr = "standard"
  try:
      timeVar = nc_data.variables["time"]
      calendarAttr=timeVar.calendar
  except:
      pass

  
  numTimeSteps = 1
  if timeVar is not None:
      numTimeSteps = numpy.shape(timeVar)[0]
  logging.debug('numTimeSteps %d ' % numTimeSteps);

  
  ''' ####################### Create point NetCDF file###################################### '''
  nc_point_out = netCDF4.Dataset( outncpointfile,'w',format="NETCDF4")
  
  nc_point_out.featureType = "timeSeries"
  nc_point_out.Conventions = "CF-1.6"
  
  logging.debug(featurevar);


  nc_point_out.createDimension('time', numTimeSteps)
  nc_point_out.createDimension('station', len(foundregionindexes))

  nc_point_out.createVariable('lat', 'f4', ('station'))
  nc_point_out.createVariable('lon', 'f4', ('station'))
  nc_point_out.createVariable('station', str, ('station'))
  
  if timeVar is not None:
    nc_point_timevar = nc_point_out.createVariable('time', timeVar.datatype, timeVar.dimensions)
    ad = dict((k , timeVar.getncattr(k) ) for k in timeVar.ncattrs() )
    nc_point_timevar.setncatts(  ad  )
    try:
      nc_point_timevar[:] = timeVar[:]
    except:
      try: 
        nc_point_timevar = timeVar
      except:
        logging.debug("Data for variable "+str(var_name)+" could not be written")
        pass
      pass
  else:
    nc_point_timevar = nc_point_out.createVariable('time', 'f8', ('time'))
    nc_point_timevar.standard_name = "time" ;
    nc_point_timevar.units = "seconds since 1970-1-1"
    nc_point_timevar[0] = 0
  
  for var_name, ncvar in nc_data.variables.iteritems():
      if var_name in varstodo:
          for name in statistic_names:
              new_var_name = var_name+"_"+name
              outVar = nc_point_out.createVariable(new_var_name, "f4", ('time', 'station'),fill_value=-9999.0 )
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
    if not var_name in nc_raster_out.dimensions:
      nc_raster_out.createDimension(var_name, len(dimension) if not dimension.isunlimited() else None)
  outVar = nc_raster_out.createVariable("features", "i4", featurevar.dimensions)
  outVar[:] = featurevar[:]
  
  nutsiddata = None
  nutsascidata = None
  if nutsidvar is not None:
    nutsiddata = nutsidvar[:]
  if nutsascivar is not None:
    nutsascidata = nutsascivar[:]

  
  logging.debug('starting');
  CSV="time;variable;index;id;name;numsamples;min;mean;max;std\n"

  numVariables = len(varstodo)
  numVariablesDone = -1;
  logging.debug('numVariables: %d'% numVariables);
  """ Iterate over all variables """
  for currentVarName in varstodo:
    numVariablesDone = numVariablesDone + 1
    nodatavalue= featurevar._FillValue
    invar_datain = nc_data.variables[currentVarName]
    outvar_min   = nc_raster_out.variables[currentVarName+"_minimum"];
    outvar_mean  = nc_raster_out.variables[currentVarName+"_mean"];
    outvar_max   = nc_raster_out.variables[currentVarName+"_maximum"];
    outvar_std   = nc_raster_out.variables[currentVarName+"_standarddeviation"];
    outvar_mask  = nc_raster_out.variables[currentVarName+"_masked"];

    outvar_point_min   = nc_point_out.variables[currentVarName+"_minimum"];
    outvar_point_mean  = nc_point_out.variables[currentVarName+"_mean"];
    outvar_point_max   = nc_point_out.variables[currentVarName+"_maximum"];
    outvar_point_std   = nc_point_out.variables[currentVarName+"_standarddeviation"];
    outvar_point_mask  = nc_point_out.variables[currentVarName+"_masked"];
    
    #outvar_point_lon = nc_point_out.variables["lon"];
    #outvar_point_lat = nc_point_out.variables["lat"];
    
    #logging.debug(outvar_mask.dimensions[0])
    
    xdim = outvar_mask.dimensions[len(outvar_mask.dimensions)- 1];
    ydim = outvar_mask.dimensions[len(outvar_mask.dimensions)- 2];
    logging.debug("dims: %s %s" % (xdim, ydim));
    ysize = (nc_raster_out.dimensions[xdim].size)
    xsize = (nc_raster_out.dimensions[ydim].size)
    
    logging.debug("size: %d %d" % (xsize, ysize));
      
    origLonRaster = numpy.repeat(((nc_raster_out.variables[xdim])[:]), xsize).reshape((ysize,xsize)).transpose();
    origLatRaster = numpy.repeat(((nc_raster_out.variables[ydim])[:]), ysize).reshape((xsize,ysize));
    
    logging.debug(numpy.shape(origLonRaster));
    logging.debug(origLonRaster)
    
    
    
    
    """ Iterate over all timesteps """
    for currentStep in range(0,numTimeSteps):
      logging.debug("iterateing timestep %d" % currentStep);
      if timeVar is not None:
        logging.debug("Has timeVar");
        """ Read time value """
        timeValueDouble = timeVar[currentStep]
        #logging.debug("timeValueDouble:" + str(timeValueDouble))
        #logging.debug("units:" + str(timeVar.units))
        #logging.debug("calendar:" + str(calendarAttr))
        num2dateValue = num2date(timeValueDouble, units=timeVar.units,calendar=calendarAttr);
        #logging.debug("num2dateValue:" + str(num2dateValue))
        timeValue = str(num2dateValue).replace(" ","T") + "Z"
        logging.debug("timeValue:" + str(timeValue))
        """ Read data from netCDF Variables """
        datainflat = invar_datain[currentStep]
        dataout_minflat = outvar_min[currentStep]
        dataout_meanflat = outvar_mean[currentStep]
        dataout_maxflat = outvar_max[currentStep]
        dataout_stdflat = outvar_std[currentStep]
        dataout_maskflat = outvar_mask[currentStep]
        
        dataout_point_minflat = outvar_point_min[currentStep]
        dataout_point_meanflat = outvar_point_mean[currentStep]
        dataout_point_maxflat = outvar_point_max[currentStep]
        dataout_point_stdflat = outvar_point_std[currentStep]
        dataout_point_maskflat = outvar_point_mask[currentStep]
      else:
        logging.debug("Has NO timeVar");
        timeValue = "None"
        """ Read data from netCDF Variables """
        datainflat = invar_datain[:]
        dataout_minflat = outvar_min[:]
        dataout_meanflat = outvar_mean[:]
        dataout_maxflat = outvar_max[:]
        dataout_stdflat = outvar_std[:]
        dataout_maskflat = outvar_mask[:]
        
        dataout_point_minflat = outvar_point_min[0]
        dataout_point_meanflat = outvar_point_mean[0]
        dataout_point_maxflat = outvar_point_max[0]
        dataout_point_stdflat = outvar_point_std[0]
        dataout_point_maskflat = outvar_point_mask[0]
        
      # Numpy 1.7.1 has no numpy.nanmean
      totalmean = numpy.nanmean(datainflat)
      
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
              # Get statistics from selected datasets
              minval =  numpy.nanmin(selecteddata)
              meanval=  numpy.nanmean(selecteddata)
              maxval =  numpy.nanmax(selecteddata)
              stdval =  numpy.nanstd(selecteddata)
              
              
              # Assign to raster version of NetCDF
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
                
              # Assign to CSV file
              CSV += "%s;%s;%d;%s;%s;%d;%f;%f;%f;%f\n" % (timeValue,currentVarName,regionindex,regid,reglongname,len(selecteddata),minval,meanval,maxval,stdval);
              
              # Assign to point version of NetCDF
              logging.debug("numRegionsDone %d, regionindex %d" % (numRegionsDone, regionindex));
              dataout_point_minflat[numRegionsDone] = minval
              dataout_point_meanflat[numRegionsDone] = meanval
              dataout_point_maxflat[numRegionsDone] = maxval
              dataout_point_stdflat[numRegionsDone] = stdval
              dataout_point_maskflat[numRegionsDone] = regionindex
              lon = numpy.mean(origLonRaster[indices])
              lat = numpy.mean(origLatRaster[indices])
              nc_point_out.variables['lon'][numRegionsDone] = lon
              nc_point_out.variables['lat'][numRegionsDone] = lat
              nc_point_out.variables['station'][numRegionsDone] = str(regid)
              
          

              
            except ValueError: 
              logging.debug('Masks all around!')
              pass 
          
      """ Assign each timestep to NetCDF variables """
      if timeVar is not None:
        outvar_min[currentStep]=dataout_minflat;
        outvar_mean[currentStep]=dataout_meanflat;
        outvar_max[currentStep]=dataout_maxflat;
        outvar_std[currentStep]=dataout_stdflat;
        outvar_mask[currentStep]=dataout_maskflat;
        
        outvar_point_min[currentStep] = dataout_point_minflat
        outvar_point_mean[currentStep] = dataout_point_meanflat
        outvar_point_max[currentStep] = dataout_point_maxflat
        outvar_point_std[currentStep] = dataout_point_stdflat
        outvar_point_mask[currentStep] = dataout_point_maskflat
      else:
        outvar_min[:]=dataout_minflat;
        outvar_mean[:]=dataout_meanflat;
        outvar_max[:]=dataout_maxflat;
        outvar_std[:]=dataout_stdflat;
        outvar_mask[:]=dataout_maskflat;
        
        outvar_point_min[:] = dataout_point_minflat
        outvar_point_mean[:] = dataout_point_meanflat
        outvar_point_max[:] = dataout_point_maxflat
        outvar_point_std[:] = dataout_point_stdflat
        outvar_point_mask[:] = dataout_point_maskflat
  
  callback("Writing data",99);  
  nc_raster_out.close()
  
  nc_point_out.close()  
  
  

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
      outncrasterfile="nutsstat.nc",
      outcsvfile="nutsstat.csv", 
      tmpFolderPath="/tmp")
  


#test()    
