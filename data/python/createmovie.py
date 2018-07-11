import isodate 
import time
import urllib
import os
import ssl

"""
  This script creates a movie from an ADAGUC Web Map Service.
  The script will work out of the box with the sat dataset which can be 
  configured in the tutorial as described inside the Readme of adaguc-server.
  
  You can also add overlays or baselayers if you configure these as layers in your service:
  
  <!-- Layer with name baselayer from geoservices.knmi.nl -->
  <Layer type="cascaded" hidden="false">
    <Name force="true">baselayer</Name>
    <Title>NPS - Natural Earth II</Title>
    <WMSLayer service="http://geoservices.knmi.nl/cgi-bin/bgmaps.cgi?" layer="naturalearth2"/>
    <LatLonBox minx="-180"  miny="-90" maxx="180" maxy="90"/>
  </Layer>
  <!-- Layer with name overlay from geoservices.knmi.nl -->
  <Layer type="cascaded" hidden="false">
    <Name force="true">overlay</Name>
    <Title>NPS - Natural Earth II</Title>
    <WMSLayer service="http://geoservices.knmi.nl/cgi-bin/worldmaps.cgi?" layer="world_line_thick"/>
    <LatLonBox minx="-180"  miny="-90" maxx="180" maxy="90"/>
  </Layer>
  <!-- Layer with name grid10 from geoservices.knmi.nl -->
  <Layer type="grid">
    <Name force="true">grid10</Name>
    <Title>grid 10 degrees</Title>
    <Grid resolution="10"/>
    <WMSFormat name="image/png32"/>
  </Layer>
  
  The movie can be displayed in a browser by using:
  $ firefox out.mp4
"""

# Pick a WMS request and remoce the TIME key value pair
url="http://localhost:8090//adaguc-services/adagucserver?DATASET=sat&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=HRVIS&WIDTH=926&HEIGHT=982&CRS=EPSG%3A3857&BBOX=-3416822.7594446274,5182866.049499422,877249.2808489851,9736622.792013815&STYLES=hrvis_0till30000%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&"

# URL with overlays
url="https://compute-test.c3s-magic.eu:8443//adaguc-services/adagucserver?DATASET=sat&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=HRVIS,overlay,grid10&WIDTH=926&HEIGHT=982&CRS=EPSG%3A32661&BBOX=43923.30841535237,-5093568.6573570175,4865123.617713443,19194.521617847146&STYLES=hrvis_0till30000%2Fnearest&FORMAT=image/png32&TRANSPARENT=TRUE&"

# Specify the time range, it is start/stop/resolution, in this case 15 minutes. 
# This is the ISO8601 convention, check https://dev.knmi.nl/projects/adagucserver/wiki/ISO8601
TIME="2015-06-05T10:45:00Z/2015-06-05T23:45:00Z/PT15M"


# Allow self signed certificates over HTTPS: Note, not secure!
ssl._create_default_https_context = ssl._create_unverified_context

def daterange(start_date, end_date, delta):
  d = start_date
  while d < end_date:
    yield d
    d += delta

start_date = isodate.parse_datetime(TIME.split("/")[0]);
end_date = isodate.parse_datetime(TIME.split("/")[1]);
timeres = isodate.parse_duration(TIME.split("/")[2]);

datestodo = list(daterange(start_date, end_date, timeres));

num=0

for date in datestodo:
  wmstime=time.strftime("%Y-%m-%dT%H:%M:%SZ", date.timetuple())
  wmsurl=url+"&time="+wmstime+"&"
  filetowrite = "img%03d.png"%(num)
  urllib.urlretrieve (wmsurl, filetowrite)
  print wmsurl
  print "Saving "+str(num)+"/"+str(len(datestodo))+": "+filetowrite
  num = num+1

#ffmpeg -i img%03d.png -c:v libx264 -vf fps=25 -pix_fmt yuv420p out.mp4


os.system("ffmpeg -y -i img%03d.png -c:v libx264 -vf fps=25 -pix_fmt yuv420p out.mp4")  