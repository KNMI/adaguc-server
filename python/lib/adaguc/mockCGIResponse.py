"""
This python script acts like a tiny cgi program. It reads in cgidata.bin and outputs it on stdout. 

cgidata.bin contains CGI output, e.g. headers and content.

This script can be used to write a python script which efficiently separates the headers from the content.


To create cgidata.bin, do:

   export ADAGUC_DB="user=adaguc password=adaguc host=localhost dbname=adaguc"
   export ADAGUC_CONFIG=./Docker/adaguc-server-config-python-postgres.xml 
   export ADAGUC_DATA_DIR=/data/adaguc-data
   export ADAGUC_AUTOWMS_DIR=/data/adaguc-autowms
   export ADAGUC_DATASET_DIR=/data/adaguc-datasets
   #export QUERY_STRING="source=testdata.nc&&service=WMS&request=getmap&format=image/png&layers=testdata&width=80&CRS=EPSG:4326&STYLES=&EXCEPTIONS=INIMAGE&showlegend=true&0.6600876848451054"
   export QUERY_STRING="source%3Dtestdata.nc&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=testdata&WIDTH=8000&CRS=EPSG%3A3857&BBOX=-1900028.4595329016,4186308.413385826,2753660.6953178523,9667705.962693866&STYLES=auto%2Fnearest&FORMAT=image/png&TRANSPARENT=TRUE&&0.8416868811008116"
   export ADAGUC_PATH=/home/plieger/code/github/KNMI/adaguc-server/
   export ADAGUC_TMP=/tmp
   export ADAGUC_LOGFILE=/tmp/test.log
   rm -rf $ADAGUC_LOGFILE
   #./bin/adagucserver
   #cat /tmp/test.log 
  ./bin/adagucserver > python/lib/adaguc/mockcgidata_headersandcontent80px.bin
 
"""
import shutil
import sys
with open(sys.argv[1], "rb") as f:
    sys.stdout.buffer.write(f.read())