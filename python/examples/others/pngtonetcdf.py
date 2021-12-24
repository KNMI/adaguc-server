from PIL import Image
import numpy as np
import netCDF4
import dateutil.parser
from os import listdir
from os.path import isfile, join
import os.path
#Maarten Plieger, KNMI (2014)

inputname = '../datasets/radarmask.png'
imgextent = [-50,-4465,750,-3600]
imgproj = "+proj=stere +lat_0=90 +lon_0=0 +lat_ts=60 +a=6378.14 +b=6356.75 +x_0=0 y_0=0"
product = "mask"
outputname = '../datasets/radarmask.nc'


inputname = '../datasets/alpha-test.png'
imgextent = [-90,-90,90,90]
imgproj = "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs"
product = "alpha-test"
outputname = '../datasets/alpha-test.nc'

#inputname = '/nobackup/users/plieger/data/himawari/HIMAWARI_8_AHI_GLOBERGBDUST_201610270700.png'
inputname = '/data/data/himawaripng/full_disk_ahi_true_color_20161115190000.jpg'
imgextent = [-5567.2481376,-5567.2481376,5567.2481376,5567.2481376]


#imgproj = "+proj=geos +lon_0=145.000000 +lat_0=0 +h=35807.414063 +a=6378.169 +b=6356.5838"




# Himawari 8
inputname="/home/c3smagic/data/full_disk_ahi_rgb_airmass_20170516105000.jpg" 
imgextent = [-5500000,-5500000,5500000,5500000]
imgproj = "+proj=geos +h=35785863 +a=6378137.0 +b=6356752.3 +lon_0=140.7 +no_defs"
isodate="20161114062000"
#directorytoscan = "/nobackup/users/plieger/data/himawari/rammdb/"
product = "himawari"
outputname = '/home/c3smagic/data/full_disk_ahi_rgb_airmass_20170516105000.nc'


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

def convertonc(inputname,imgextent,imgproj,product,outputname):
  print("Converting file [%s]"%inputname)
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


  lat = ncfile.createVariable('y', 'd', ('y'))
  lat.units = 'degrees_north'
  lat.standard_name = 'latitude'

  lon = ncfile.createVariable('x', 'd', ('x'))
  lon.units = 'degrees_east'
  lon.standard_name = 'longitude'



  projection = ncfile.createVariable('projection','byte')
  projection.proj4 = imgproj

  cellsizex=((imgextent[2]-imgextent[0])/float(imgwidth))
  cellsizey=((imgextent[1]-imgextent[3])/float(imgheight))
  print len(list(drange(imgextent[0]+cellsizex/2,imgextent[2],cellsizex)))
  print len(list(drange(imgextent[3]+cellsizey/2,imgextent[1],cellsizey)))
  lon[:] = list(drange(imgextent[0]+cellsizex/2,imgextent[2],cellsizex))
  lat[:] = list(drange(imgextent[3]+cellsizey/2,imgextent[1],cellsizey))

  if len(isodate)>0:
    time_dim=ncfile.createDimension('time', 1)
    timevar =  ncfile.createVariable('time', 'd', ('time'))
    timevar.units="seconds since 1970-01-01 00:00:00"
    timevar.standard_name='time'
    timevar[:] = epochtime = int(dateutil.parser.parse(isodate).strftime("%s")) ;
    rgbdata = ncfile.createVariable(product,'u4',('time','y','x'))
  else:
    rgbdata = ncfile.createVariable(product,'u4',('y','x'))
  rgbdata.units = 'rgba'
  rgbdata.standard_name = 'rgba'
  rgbdata.long_name = product
  rgbdata.grid_mapping= 'projection'
  """ Convert to uint32 and assign to nc variable """
  rgbdata[:]= rgbArray.view(np.uint32)

  ncfile.close()
  print "Ok!"
  

convertonc(inputname,imgextent,imgproj,product,outputname)  

#directorytoscan = "/nobackup/users/plieger/data/himawari/rammdb/"
#onlyfiles = [f for f in listdir(directorytoscan) if isfile(join(directorytoscan, f))]
#filenr=0
   
#filteredlist = [mfile for mfile in onlyfiles if "jpg" in mfile]    

#for mfile in filteredlist:
  
  #filenr+=1
  #print("File %d/%d"%(filenr,len(filteredlist)))
    
  #inputname="/nobackup/users/plieger/data/himawari/rammdb/"+mfile
  #imgextent = [-5500000,-5500000,5500000,5500000]
  #imgproj = "+proj=geos +h=35785863 +a=6378137.0 +b=6356752.3 +lon_0=140.7 +no_defs"
  #isodate=mfile[25:-4]
  #product = "himawari"
  #outputname = "/nobackup/users/plieger/data/himawari/loop/"+mfile+".nc"
  #if os.path.isfile(outputname) == False:
    #convertonc(inputname,imgextent,imgproj,product,outputname)  


