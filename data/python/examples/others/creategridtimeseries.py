import datetime
import netCDF4
import numpy as np
from random import randint
from PIL import Image
from PIL import ImageDraw, ImageFont


font = ImageFont.truetype("../fonts/FreeSans.ttf", 12)

fileOutDir="../datasets"
dimsset = {
  'netcdf_5dims/netcdf_5dims_seq1/nc_5D_20170101000000-20170101001000.nc':{
    'time':{
      'vartype':'d',
      'units':"seconds since 1970-01-01 00:00:00",
      'standard_name':'time',
      'values':[netCDF4.date2num(datetime.datetime(2017,01,01,00,00,00), "seconds since 1970-01-01 00:00:00"),
                netCDF4.date2num(datetime.datetime(2017,01,01,00,05,00), "seconds since 1970-01-01 00:00:00"),
                netCDF4.date2num(datetime.datetime(2017,01,01,00,10,00), "seconds since 1970-01-01 00:00:00")]
      },
    'height':{
      'vartype':'d',
      'units':"meters",
      'standard_name':'height',    
      'values':[1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000]
      },
    'member':{
      'vartype':str,
      'units':"member number",
      'standard_name':'member',
      'values':['member6','member5','member4','member3','member2','member1']
      }
    },
    'netcdf_5dims/netcdf_5dims_seq2/nc_5D_20170101001500-20170101002500.nc':{
    'time':{
      'vartype':'d',
      'units':"seconds since 1970-01-01 00:00:00",
      'standard_name':'time',
      'values':[netCDF4.date2num(datetime.datetime(2017,01,01,00,15,00), "seconds since 1970-01-01 00:00:00"),
                netCDF4.date2num(datetime.datetime(2017,01,01,00,20,00), "seconds since 1970-01-01 00:00:00"),
                netCDF4.date2num(datetime.datetime(2017,01,01,00,25,00), "seconds since 1970-01-01 00:00:00")]
      },
    'height':{
      'vartype':'d',
      'units':"meters",
      'standard_name':'height',    
      'values':[1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000]
      },
    'member':{
      'vartype':str,
      'units':"member number",
      'standard_name':'member',
      'values':['member6','member5','member4','member3','member2','member1']
      }
    }    
}
    

for filename in dimsset:
  dims = dimsset[filename]
  dimstring = []
  for a in dims:
    dimstring.append(a);
    print dims[a]['values']
    
  dimstring.append('lat')  
  dimstring.append('lon')
  print dimstring

  latvar = []
  lonvar = []
  latvar = list(reversed(np.arange(-90. + (2./180.), 90 + (2./180.), 1.0)))
  lonvar = np.arange(-180. + (2./360.),180 + (2./360.),1.0)

  print (len(lonvar),len(latvar))

  im2 = Image.new("L", (len(lonvar),len(latvar)))


  draw = ImageDraw.Draw(im2) 

  draw.fontmode = "1"
  filepath = fileOutDir +"/"+filename
  print "writing to " + filepath
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
    for value in range(len(dims[dims.keys()[number-1]]['values'])):
      l[number-1] = value
      if number > 1:
          Recurse ( dims, number - 1 ,l)
      else:
        draw.rectangle(((0,0,len(lonvar),len(latvar))), fill = 0)
        draw.text((5, 5), str(l), font=font,  fill=255)
        
        for i in range(len(l)):
          value = (dims[dims.keys()[i]]['values'])[l[i]]
          units = dims[dims.keys()[i]]['units']
          standardname = dims[dims.keys()[i]]['standard_name']
          
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
