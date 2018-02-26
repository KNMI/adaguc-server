from PIL import Image
import numpy as np
import netCDF4
import dateutil.parser
from os import listdir
from os.path import isfile, join
import os.path
import datetime
import urllib
import imghdr
import subprocess
#Maarten Plieger, KNMI (2018)

class HimawariConverter:
    imgextent = [-5500000,-5500000,5500000,5500000]
    imgproj = "+proj=geos +h=35785863 +a=6378137.0 +b=6356752.3 +lon_0=140.7 +no_defs"
    product = "full_disk_ahi_true_color"
    directorytoscan = "/home/osm/adaguc-server-docker/adaguc-data/himawari8_rammb/jpg"
    outputfolder = "/home/osm/adaguc-server-docker/adaguc-data/himawari8_rammb/netcdf/"
    adagucdatasetname = "himawari"

    def getDataURLFromDate(self, start_date):
        himawaritimestring =  start_date.strftime("%Y%m%d%H%M00")    
        datafilename = "full_disk_ahi_true_color_%s.jpg" % himawaritimestring
        return "http://rammb.cira.colostate.edu/ramsdis/online/images/hi_res/himawari-8/full_disk_ahi_true_color/%s" % datafilename

    def drange(self, start, stop, step):
        r = start
        if(step > 0):
            while r < stop:
                yield r
                r += step
        if(step < 0):
            while r > stop:
                yield r
                r += step

    def convertonc(self, inputname,mfile,imgextent,imgproj,product,outputname):
        print("Converting file [%s]"%inputname)
        isodate=mfile.split("_")[5].split(".")[0]
        image = Image.open(inputname)

        imgwidth = image.size[0]
        imgheight = image.size[1]
        rgbaimage=image.convert("RGBA")
        print "Get data"
        rgbArray=rgbaimage.getdata()
        
        #print "Setting zeroes"
        #rgbArray = np.zeros((len(d),4), 'uint8')
        #print "Setting data"
        #rgbArray[:] = d

        print "Writing to "+outputname
        ncfile = netCDF4.Dataset(outputname,'w')
            
        ncfile.Conventions = "CF-1.4";

        lat_dim = ncfile.createDimension('y', imgheight)     # latitude axis
        lon_dim = ncfile.createDimension('x', imgwidth)     # longitude axis


        lat = ncfile.createVariable('y', 'd', ('y'), zlib=True)
        lat.units = 'degrees_north'
        lat.standard_name = 'latitude'

        lon = ncfile.createVariable('x', 'd', ('x'), zlib=True)
        lon.units = 'degrees_east'
        lon.standard_name = 'longitude'



        projection = ncfile.createVariable('projection','byte')
        projection.proj4 = imgproj

        cellsizex=((imgextent[2]-imgextent[0])/float(imgwidth))
        cellsizey=((imgextent[1]-imgextent[3])/float(imgheight))
        print len(list(self.drange(imgextent[0]+cellsizex/2,imgextent[2],cellsizex)))
        print len(list(self.drange(imgextent[3]+cellsizey/2,imgextent[1],cellsizey)))
        lon[:] = list(self.drange(imgextent[0]+cellsizex/2,imgextent[2],cellsizex))
        lat[:] = list(self.drange(imgextent[3]+cellsizey/2,imgextent[1],cellsizey))

        width = len(lon);
        height= len(lat);

        
        if len(isodate)>0:
            print "Setting date " + isodate
            time_dim=ncfile.createDimension('time', 1)
            timevar =  ncfile.createVariable('time', 'd', ('time'))
            timevar.units="seconds since 1970-01-01 00:00:00"
            timevar.standard_name='time'
            timevar[:] = epochtime = int(dateutil.parser.parse(isodate).strftime("%s")) ;
            rgbdata = ncfile.createVariable(product,'u4',('time','y','x'),chunksizes=(1,100,width), zlib=True)
        else:
            rgbdata = ncfile.createVariable(product,'u4',('y','x'))
        rgbdata.units = 'rgba'
        rgbdata.standard_name = 'rgba'
        rgbdata.long_name = product
        rgbdata.grid_mapping= 'projection'
        """ Convert to uint32 and assign to nc variable """
        print "Setting data"
        def chunks(self, l, n):
            """Yield successive n-sized chunks from l."""
            for i in range(0, len(l), n):
                yield l[i:i + n]
        
        print "Making newdata"
        
        
        print len(rgbArray);
        
        for y in range(0, height/100):
            y=y*100
            print "Line % d till %d" % (y, y+100)
            d=rgbaimage.crop((0,y,width,y+100)).getdata()
            rgbArray = np.zeros((len(d),4), 'uint8')
            rgbArray[:] = d
            rgbdata[0,y:y+100,0:width] = rgbArray.view(np.uint32)[:]

        ncfile.close()
        
        
        print "Ok!"


    def start(self):
        self.convertjpgdirtonc()
        start_date = datetime.datetime.utcnow().replace(microsecond=0, second=0,minute=0)  - datetime.timedelta(hours=10)
        end_date = (datetime.datetime.utcnow().replace(microsecond=0, second=0,minute=0)  - datetime.timedelta(hours=8));
        totaldirectorylisting = listdir(self.directorytoscan)

        directorylisting = [];
        for localfilename in totaldirectorylisting:
            datafilelocation = self.directorytoscan + "/" + localfilename
            if imghdr.what(datafilelocation) == 'jpeg':
                directorylisting.append(localfilename)
        print len(directorylisting)
        print directorylisting

        while start_date < end_date:
            start_date = start_date + datetime.timedelta(minutes=10)
            remotedataurl = self.getDataURLFromDate(start_date);
            
            
            datafilename = os.path.basename(remotedataurl)
            if datafilename not in directorylisting:
                datafilelocation = self.directorytoscan + "/" + datafilename
                try:
                    print "Start Fetching %s" % datafilename
                    urllib.urlretrieve (remotedataurl, datafilelocation)
                    filetype = imghdr.what(datafilelocation)
                    if filetype == 'jpeg':
                        print "Succeeded %s" % localfilename
                        self.convert(datafilename)
                    else:
                        print remotedataurl + " not succeeded: file type is %s" % filetype
                        if filetype is not None:
                            os.remove(datafilelocation)
                except Exception, e:
                    print "Error downloading image %s because %s" % (remotedataurl,e)
                    pass

    def convert(self, mfile):
        print "Start converting %s to netcdf" % mfile
        inputname=self.directorytoscan+"/"+mfile
        
        outputname = self.outputfolder+"/"+mfile+".nc"
        if os.path.isfile(outputname) == False:
            try:
                self.convertonc(inputname,mfile, self.imgextent,self.imgproj,self.product,outputname)  
                subprocess.call(("docker exec -i -t my-adaguc-server bash /adaguc/adaguc-server-createtiles.sh " + self.adagucdatasetname).split())
            except:
                print "Error converting image %s" % inputname
                os.remove(inputname)
                pass

    def convertjpgdirtonc(self):
        #convertonc(inputname,imgextent,imgproj,product,outputname)
        totaldirectorylisting = listdir(self.directorytoscan)
        directorylisting = []
        for localfilename in totaldirectorylisting:
            datafilelocation = self.directorytoscan + "/" + localfilename
            if imghdr.what(datafilelocation) == 'jpeg':
                directorylisting.append(localfilename)
        
        print len(directorylisting)

        onlyfiles = [f for f in directorylisting if isfile(join(self.directorytoscan, f))]
        filenr=0
        
        filteredlist = [mfile for mfile in onlyfiles if "jpg" in mfile]    

        for mfile in filteredlist:
        
            filenr+=1
            print("File %d/%d/%s"%(filenr,len(filteredlist),mfile))
                
            self.convert(mfile);

HimawariConverter().start()
