import datetime
import netCDF4
import numpy as np
from random import randint



fileOutName="../datasets/createncpoint.nc"


latvar = []
lonvar = []
pointvar = [];



#latvar.append( 51.913368);
#lonvar.append( 6.294848);
#pointvar.append(1);
latvar.append( 52.090825);
lonvar.append( 5.122388);
pointvar.append(1);
numpoints=len(pointvar)


ncfile = netCDF4.Dataset(fileOutName,'w')
obs_dim = ncfile.createDimension('obs', numpoints)     # latitude axis
time_dim=ncfile.createDimension('time', 1)

lat = ncfile.createVariable('lat', 'd', ('obs'))
lat.units = 'degrees_north'
lat.standard_name = 'latitude'
lon = ncfile.createVariable('lon', 'd', ('obs'))
lon.units = 'degrees_east'
lon.standard_name = 'longitude'

timevar =  ncfile.createVariable('time', 'd', ('time'))
timevar.units="seconds since 1970-01-01 00:00:00"
timevar.standard_name='time'

floatVar = ncfile.createVariable('Vethuizen','f4',('obs'))
#floatVar.units = 'km'
#floatVar.standard_name = 'distance'


lat[:] = [latvar]
lon[:] = [lonvar]
timevar[:] = netCDF4.date2num(datetime.datetime.now(), "seconds since 1970-01-01 00:00:00")

floatVar[:] = pointvar


ncfile.featureType = "timeSeries";
ncfile.Conventions = "CF-1.4";
ncfile.close()
