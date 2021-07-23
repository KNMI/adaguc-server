import datetime
import netCDF4
import numpy as np
from random import randint


inputFileName="../datasets/volcano_list.txt"
fileOutName="../datasets//volcano_list.nc"

latvar = []
lonvar = []
volcanovar = np.array([], dtype='object')

f = open(inputFileName, 'r')  
for _line in f:
    line=(_line.replace('\n','').replace('\r',''));
    items = line.split()
    name = items[2]
    lon = items[0]
    lat = items[1]
    latvar.append(lat);
    lonvar.append(lon);
    volcanovar = np.append(volcanovar,name)

f.close()

numpoints=len(volcanovar)


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

floatVar = ncfile.createVariable('volcanoes',str,('obs'))
floatVar.units = '-'
floatVar.standard_name = 'volcanoes'


lat[:] = [latvar]
lon[:] = [lonvar]
timevar[:] = netCDF4.date2num(datetime.datetime.now(), "seconds since 1970-01-01 00:00:00")

floatVar[:] = volcanovar


ncfile.featureType = "timeSeries";
ncfile.Conventions = "CF-1.4";
ncfile.close()
