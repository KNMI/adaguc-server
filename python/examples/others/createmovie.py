import isodate
import time
import urllib
import urllib.request
import os
import ssl

"""
  This script creates a movie from an ADAGUC Web Map Service.
  The script will work out of the box with the sat dataset which can be
  configured in the tutorial as described inside the Readme of adaguc-server.

  You can also add overlays or baselayers if you configure these as layers in your service:

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
bbox = "BBOX=246032.58891774912,6415470.813887446,976823.012917749,7229569.237160173"

# Pick a WMS request and remoce the TIME key value pair
url = "https://geoservices.knmi.nl/adagucserver?DATASET=RADAR&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=RAD_NL25_PCP_CM&WIDTH=926&HEIGHT=982&CRS=EPSG%3A3857&" + \
    bbox+"&STYLES=&FORMAT=image/png&TRANSPARENT=TRUE&"

# URL with overlays
# url="https://compute-test.c3s-magic.eu:8443//adagucserver?DATASET=sat&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=HRVIS,overlay,grid10&WIDTH=926&HEIGHT=982&CRS=EPSG%3A32661&BBOX=43923.30841535237,-5093568.6573570175,4865123.617713443,19194.521617847146&STYLES=hrvis_0till30000%2Fnearest&FORMAT=image/png32&TRANSPARENT=TRUE&"

# Specify the time range, it is start/stop/resolution, in this case 15 minutes.
# This is the ISO8601 convention, check https://dev.knmi.nl/projects/adagucserver/wiki/ISO8601
TIME = "2021-10-12T22:45:00Z/2021-10-12T23:45:00Z/PT5M"


# Allow self signed certificates over HTTPS: Note, not secure!
ssl._create_default_https_context = ssl._create_unverified_context


def daterange(start_date, end_date, delta):
  d = start_date
  while d < end_date:
    yield d
    d += delta


start_date = isodate.parse_datetime(TIME.split("/")[0])
end_date = isodate.parse_datetime(TIME.split("/")[1])
timeres = isodate.parse_duration(TIME.split("/")[2])

datestodo = list(daterange(start_date, end_date, timeres))

num = 0

for date in datestodo:
  wmstime = time.strftime("%Y-%m-%dT%H:%M:%SZ", date.timetuple())
  wmsurl = url+"&time="+wmstime+"&"
  filetowrite = "img%03d.png" % (num)
  urllib.request.urlretrieve(wmsurl, filetowrite)
  print(wmsurl)
  print("Saving "+str(num)+"/"+str(len(datestodo))+": "+filetowrite)
  num = num+1

# ffmpeg -i img%03d.png -c:v libx264 -vf fps=25 -pix_fmt yuv420p out.mp4


os.system("ffmpeg -y -i img%03d.png -c:v libx264 -vf fps=25 -pix_fmt yuv420p out.mp4")
