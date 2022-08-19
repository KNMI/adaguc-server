import datetime
import netCDF4
import numpy as np
from random import randint
from PIL import Image
from PIL import ImageDraw, ImageFont


font = ImageFont.truetype("../fonts/FreeSans.ttf", 12)

fileOutDir="../../../data/datasets"
dimsset = {
  'netcdf_5dims/netcdf_5dims_seq1/nc_5D_20170101000000-20170101001000.nc':{
    'member':{
      'vartype':str,
      'units':"member number",
      'standard_name':'member',
      'values':['member6','member5','member4','member3','member2','member1']
     },
    'height':{
      'vartype':'d',
      'units':"meters",
      'standard_name':'height',    
      'values':[1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000]
      },
    'time':{
      'vartype':'d',
      'units':"seconds since 1970-01-01 00:00:00",
      'standard_name':'time',
      'values':[netCDF4.date2num(datetime.datetime(2017,1,1,0,0,0), "seconds since 1970-01-01 00:00:00"),
                netCDF4.date2num(datetime.datetime(2017,1,1,0,5,0), "seconds since 1970-01-01 00:00:00"),
                netCDF4.date2num(datetime.datetime(2017,1,1,0,10,0), "seconds since 1970-01-01 00:00:00")]
      }
    },
    'netcdf_5dims/netcdf_5dims_seq2/nc_5D_20170101001500-20170101002500.nc':{
    'member':{
      'vartype':str,
      'units':"member number",
      'standard_name':'member',
      'values':['member6','member5','member4','member3','member2','member1']
      },
    'height':{
      'vartype':'d',
      'units':"meters",
      'standard_name':'height',    
      'values':[1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000]
      },
    'time':{
      'vartype':'d',
      'units':"seconds since 1970-01-01 00:00:00",
      'standard_name':'time',
      'values':[netCDF4.date2num(datetime.datetime(2017,1,1,0,15,0), "seconds since 1970-01-01 00:00:00"),
                netCDF4.date2num(datetime.datetime(2017,1,1,0,20,0), "seconds since 1970-01-01 00:00:00"),
                netCDF4.date2num(datetime.datetime(2017,1,1,0,25,0), "seconds since 1970-01-01 00:00:00")]
      },
    }    
}
    
fileNumber = -1
for filename in dimsset:
  fileNumber = fileNumber + 1
  dims = dimsset[filename]
  dimstring = []
  for a in dims:
    dimstring.append(a);
    print (dims[a]['values'])
    
  dimstring.append('lat')  
  dimstring.append('lon')
  print(dimstring)

  latvar = []
  lonvar = []
  latvar = list(reversed(np.arange(-90. + (2./180.), 90 + (2./180.), 1.0)))
  lonvar = np.arange(-180. + (2./360.),180 + (2./360.),1.0)

  print (len(lonvar),len(latvar))

  im2 = Image.new("L", (len(lonvar),len(latvar)))


  draw = ImageDraw.Draw(im2) 

  draw.fontmode = "1"
  filepath = fileOutDir +"/"+filename
  print ("writing to " + filepath)
  ncfile = netCDF4.Dataset(filepath,'w')
  lon_dim = ncfile.createDimension('lon', len(lonvar))
  lat_dim = ncfile.createDimension('lat', len(latvar))

  lat = ncfile.createVariable('lat', 'f4', ('lat'),)
  lat.units = 'degrees_north'
  lat.standard_name = 'latitude'
  lon = ncfile.createVariable('lon', 'f4', ('lon'))
  lon.units = 'degrees_east'
  lon.standard_name = 'longitude'

  for dim in dims:
    d=ncfile.createDimension(dim, len(dims[dim]['values']))
    v =  ncfile.createVariable(dim, dims[dim]['vartype'], (dim))
    v.units=dims[dim]['units']
    v.standard_name=dims[dim]['standard_name']
    if dims[dim]['vartype'] is str:
      v[:] = np.array((dims[dim]['values']),dtype='object')
    else:
      v[:] = dims[dim]['values']


  dataVar = ncfile.createVariable('data','B',dimstring,zlib=True,least_significant_digit=3)
  dataVar.units = 'km'
  dataVar.standard_name = 'data'


  lat[:] = [latvar]
  lon[:] = [lonvar]


  def Recurse (dims, number, l):
    dimKeyList = list(dims.keys());

    for value in range(len(dims[dimKeyList[number-1]]['values'])):
      l[number-1] = value
      if number > 1:
          Recurse ( dims, number - 1 ,l)
      else:
        draw.rectangle(((0,0,len(lonvar),len(latvar))), fill = 0)
        colorValue = l[2]+fileNumber*3+l[1]*6+(5-l[0])*6*9
        print(l, colorValue);
        draw.rectangle(((350,0,len(lonvar),10)), fill = colorValue)
        draw.text((5, 5), str(l), font=font,  fill=255)
        
        for i in range(len(l)):
          value = (dims[dimKeyList[i]]['values'])[l[i]]
          units = dims[dimKeyList[i]]['units']
          standardname = dims[dimKeyList[i]]['standard_name']
          
          if standardname is "time":
            value =  netCDF4.num2date(value, units);
          draw.text((5, 50 + i*20), str(value), font=font,  fill=255)

        draw.text((5, 25), filepath, font=font,  fill=255)
        
        
        datavar=np.reshape(list(im2.getdata(0)), (180,360))

        dataVar[l[0],l[1],l[2],:,:] = datavar

        
  l = []

  for i in range(len(dims)):
        l.append(0)      
  Recurse(dims,len(dims),l)

  ncfile.Conventions = "CF-1.4";
  ncfile.close()
