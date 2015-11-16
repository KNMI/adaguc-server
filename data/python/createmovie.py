import isodate 
import time
import urllib
import os

def daterange(start_date, end_date, delta):
  d = start_date
  while d < end_date:
    yield d
    d += delta

url="https://esgf.knmi.nl/cgi-bin/adaguc.cgi?SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=RGB-vulkaanas_rgbknmi&WIDTH=1280&HEIGHT=720&CRS=EPSG%3A32661&BBOX=-1285037.9242223408,-4688824.700451897,5839742.892276522,-395975.69114185695&STYLES=auto%2Frgba&FORMAT=image/png&TRANSPARENT=TRUE&"

TIME="2013-10-12T12:15:00Z/2013-10-14T11:45:00Z/PT15M"
TIME="2013-10-12T12:15:00Z/2013-10-12T23:15:00Z/PT15M"
start_date = isodate.parse_datetime(TIME.split("/")[0]);
end_date = isodate.parse_datetime(TIME.split("/")[1]);
timeres = isodate.parse_duration(TIME.split("/")[2]);

datestodo = list(daterange(start_date, end_date, timeres));

num=0

for date in datestodo:
  wcstime=time.strftime("%Y-%m-%dT%H:%M:%SZ", date.timetuple())
  wcsurl=url+"time="+wcstime+"&"
  filetowrite = "img%03d.png"%(num)
  urllib.urlretrieve (wcsurl, filetowrite)
  print wcsurl
  print "Saving "+str(num)+"/"+str(len(datestodo))+": "+filetowrite
  num = num+1


os.system("ffmpeg -y -i img%03d.png -c:v libx264 -preset ultrafast -r 30 -qp 0 output.mkv")  