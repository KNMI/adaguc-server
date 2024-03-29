"""
 This Python script runs the ADAGUC executable without an webserver. It can be used as example to run ADAGUC in your own environment from python.
 Created by Maarten Plieger - 2020-09-02
"""

import os

url="source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=256&HEIGHT=256&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&"

from adaguc.runAdaguc import runAdaguc

adagucInstance = runAdaguc()


adagucServerHome = os.getenv('ADAGUC_PATH', os.getcwd() + "/../../../../")
adagucInstance.setAdagucPath(adagucServerHome)
adagucInstance.setConfiguration(adagucServerHome + "/python/lib/adaguc/adaguc-server-config-python.xml")
adagucInstance.setAutoWMSDir(adagucServerHome + "/data/datasets/")
adagucInstance.setTmpDir(adagucServerHome + "/python/examples/runautowms/")

img, logfile = adagucInstance.runGetMapUrl(url)

print(logfile)

if img is not None:
  img.save("result.png")
  img.show()





""" 
  Other interesting URL's:
  1) An url for the ADAGUCServer used to create maps, with baselayer and overlay coming from http://geoservices.knmi.nl/ as configured in DefaultLayers.include.xml 
  - url="source=testdata.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=baselayer,testdata,overlay&WIDTH=1000&HEIGHT=750&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&showlegend=true&title=test&subtitle=test&showdims=true&showscalebar=true&shownortharrow=true"

  2) Load GetMap's with data URL's from remote opendap services 
  - url="source=http://opendap.knmi.nl/knmi/thredds/dodsC/ADAGUC/testsets/projectedgrids/t2m.KNMI-2014.KNXT12.HCAST2.DD.nc.fixed.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=baselayer,t2m,grid10,overlay&WIDTH=1000&HEIGHT=750&CRS=EPSG%3A4326&BBOX=30,-30,75,30&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&showlegend=true&scalebar=true&showsdims=true&title=2+metre+temperature"

  3) A preview can be automatically generated by leaving out HEIGHT and BBOX
  - url="source=http://opendap.knmi.nl/knmi/thredds/dodsC/ADAGUC/testsets/projectedgrids/t2m.KNMI-2014.KNXT12.HCAST2.DD.nc.fixed.nc&SERVICE=WMS&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=baselayer,t2m,grid10,overlay&WIDTH=500&CRS=EPSG%3A4326&STYLES=testdata%2Fnearest&FORMAT=image/png&TRANSPARENT=FALSE&showlegend=true&scalebar=true&showsdims=true&title=2+metre+temperature"
"""