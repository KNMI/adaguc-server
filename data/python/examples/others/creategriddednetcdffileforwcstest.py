import datetime
import netCDF4
import numpy as np
from random import randint
from PIL import Image
from PIL import ImageDraw, ImageFont
from random import randint

font = ImageFont.truetype("../fonts/FreeSans.ttf", 40)

fileOutDir="../datasets"

latvar = -np.array(range(180)) + 90 -.5
lonvar = np.array(range(360)) -180 + 0.5

print (len(lonvar),len(latvar))

im2 = Image.new("L", (len(lonvar),len(latvar)))


draw = ImageDraw.Draw(im2) 

filename = "griddedmetcdffileforwcstest_lonlat.nc"

draw.fontmode = "1"

ncfile = netCDF4.Dataset("../datasets/wcstest_lonlat.nc",'w')

lon_dim = ncfile.createDimension('lon', len(lonvar))
lat_dim = ncfile.createDimension('lat', len(latvar))
lat = ncfile.createVariable('lat', 'f4', ('lat'),)
lat.units = 'degrees_north'
lat.standard_name = 'latitude'
lon = ncfile.createVariable('lon', 'f4', ('lon'))
lon.units = 'degrees_east'
lon.standard_name = 'longitude'



dataVar = ncfile.createVariable('testdata','B',['lat', 'lon'],zlib=True,least_significant_digit=3)
dataVar.units = 'km'
dataVar.standard_name = 'data'

for i in range(128):
  draw.line((104 + 128 + i,0,128 - i,128), fill = (i*2) % 256);
  draw.line((104 + 128 - i,0,128 + i,128), fill = (i*2) % 256);
  
for i in range(2048):  
  draw.point((randint(90,270),randint(45,135)), fill = randint(0,255))

draw.text((5, 25), "LONLAT TEST", font=font,  fill=255)
draw.line((0,0,360,180), fill = 128);
draw.line((0,180,360,0), fill = 255);

dataVar[:]=np.reshape(list(im2.getdata(0)), (180,360))

lat[:] = [latvar]
lon[:] = [lonvar]

ncfile.Conventions = "CF-1.4";
ncfile.close()
