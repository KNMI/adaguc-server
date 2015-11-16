#Maarten Plieger, KNMI (2014)

inputname = '../datasets/radarmask.png'
imgextent = [-50,-4465,750,-3600]
imgproj = "+proj=stere +lat_0=90 +lon_0=0 +lat_ts=60 +a=6378.14 +b=6356.75 +x_0=0 y_0=0"
product = "mask"
outputname = '../datasets/radarmask.nc'

import Image
import numpy as np
import netCDF4
import dateutil.parser

def drange(start, stop, step):
  r = start
  if(step > 0):
    while r < stop:
      yield r
      r += step
  if(step < 0):
    while r > stop:
      yield r
      r += step


image = Image.open(inputname)

imgwidth = image.size[0]
imgheight = image.size[1]
rgbaimage=image.convert("RGBA")
d=rgbaimage.getdata()

rgbArray = np.zeros((len(d),4), 'uint8')
rgbArray[:] = d

print "Writing to "+outputname
ncfile = netCDF4.Dataset(outputname,'w')
  
ncfile.Conventions = "CF-1.4";

lat_dim = ncfile.createDimension('y', imgheight)     # latitude axis
lon_dim = ncfile.createDimension('x', imgwidth)     # longitude axis
#time_dim=ncfile.createDimension('time', 1)

lat = ncfile.createVariable('y', 'd', ('y'))
lat.units = 'degrees_north'
lat.standard_name = 'latitude'

lon = ncfile.createVariable('x', 'd', ('x'))
lon.units = 'degrees_east'
lon.standard_name = 'longitude'

#timevar =  ncfile.createVariable('time', 'd', ('time'))
#timevar.units="seconds since 1970-01-01 00:00:00"
#timevar.standard_name='time'

projection = ncfile.createVariable('projection','byte')
projection.proj4 = imgproj

cellsizex=((imgextent[2]-imgextent[0])/float(imgwidth))
cellsizey=((imgextent[1]-imgextent[3])/float(imgheight))
print len(list(drange(imgextent[0]+cellsizex/2,imgextent[2],cellsizex)))
print len(list(drange(imgextent[3]+cellsizey/2,imgextent[1],cellsizey)))
lon[:] = list(drange(imgextent[0]+cellsizex/2,imgextent[2],cellsizex))
lat[:] = list(drange(imgextent[3]+cellsizey/2,imgextent[1],cellsizey))
#timevar[:] = epochtime = int(dateutil.parser.parse("2015-05-10T092812").strftime("%s")) ;
#rgbdata = ncfile.createVariable(product,'u4',('time','y','x'))
rgbdata = ncfile.createVariable(product,'u4',('y','x'))
rgbdata.units = 'rgba'
rgbdata.standard_name = 'rgba'
rgbdata.long_name = product
rgbdata.grid_mapping= 'projection'
""" Convert to uint32 and assign to nc variable """
rgbdata[:]= rgbArray.view(np.uint32)

ncfile.close()
print "Ok!"