import datetime
import netCDF4
import numpy as np
from random import randint



fileOutName="create_featuretype_point_notimedim.nc"


latvar = np.random.rand(10000)*180 - 90
lonvar = np.random.rand(10000)*360 - 180
pointvar = np.random.rand(10000)*100

numpoints=len(pointvar)


ncfile = netCDF4.Dataset(fileOutName,'w')
obs_dim = ncfile.createDimension('numpoints', numpoints)     # latitude axis

lat = ncfile.createVariable('lat', 'd', ('numpoints'))
lat.units = 'degrees_north'
lat.standard_name = 'latitude'

lon = ncfile.createVariable('lon', 'd', ('numpoints'))
lon.units = 'degrees_east'
lon.standard_name = 'longitude'

floatVar = ncfile.createVariable('percent','f4',('numpoints'))
floatVar.units = '%'
floatVar.standard_name = 'percentage'


lat[:] = [latvar]
lon[:] = [lonvar]
floatVar[:] = pointvar


ncfile.featureType = "point";
ncfile.Conventions = "CF-1.4";
ncfile.close()
