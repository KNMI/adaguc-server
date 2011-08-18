#!/bin/bash

rm -r testresults
mkdir testresults

dropdb testdb
createdb testdb

echo "### TESTSCRIPT: ADDING files to the DB ###"
../adagucserverEC/adagucserverEC --updatedb --config data/RADNL_OPER_R___25PCPRR_L3.xml  1>/dev/null 

echo "### TESTSCRIPT: ADDING files again, the server should not need to open the files again ###"
../adagucserverEC/adagucserverEC --updatedb --config data/RADNL_OPER_R___25PCPRR_L3.xml  1>/dev/null 

export ADAGUC_CONFIG=data/RADNL_OPER_R___25PCPRR_L3.xml
export ADAGUC_LOGFILE=testscript.log
export ADAGUC_ERRORFILE=testscript.errlog

rm -f $ADAGUC_LOGFILE
rm -f $ADAGUC_ERRORFILE

echo "### TESTSCRIPT: WMS request ###"
export QUERY_STRING="service=WMS&REQUEST=GetMap&format=image/png&width=256&height=256&BBOX=0,48,10,58&SRS=EPSG:4326&LAYERS=RADNL_OPER_R___25PCPRR_L3_COLOR"
../adagucserverEC/adagucserverEC >  testresults/test_wms-getmap.out

#Cut first two lines, because these contain the content-type information of the file
sed '1,2d' testresults/test_wms-getmap.out  >   testresults/test_wms-getmap.png


echo "### TESTSCRIPT: GetCapabilities request ###"
export QUERY_STRING="service=WMS&REQUEST=GetCapabilities"
 ../adagucserverEC/adagucserverEC > testresults/test_wms-getcapabilities.out
sed '1,2d' testresults/test_wms-getcapabilities.out  > testresults/test_wms-getcapabilities.xml
xmllint testresults/test_wms-getcapabilities.xml 1>/dev/null 

echo "### TESTSCRIPT: GetFeatureInfo request ###"
export QUERY_STRING="service=WMS&REQUEST=GetFeatureInfo&width=256&height=256&BBOX=0,48,10,58&SRS=EPSG:4326&LAYERS=RADNL_OPER_R___25PCPRR_L3_COLOR&info_format=text/plain&X=128&Y=128"
 ../adagucserverEC/adagucserverEC > testresults/test_wms-getfeaturinfo.txt
 
 echo "### TESTSCRIPT: GetMap animation request ###"
 export QUERY_STRING="service=WMS&REQUEST=GetMap&format=image/png&width=256&height=256&BBOX=0,48,10,58&SRS=EPSG:4326&LAYERS=RADNL_OPER_R___25PCPRR_L3_COLOR&TIME=2011-06-17T15:00:00Z/2011-06-17T15:55:00Z"
../adagucserverEC/adagucserverEC >  testresults/test_wms-getmap_animation.out

#Cut first two lines, because these contain the content-type information of the file
sed '1,2d' testresults/test_wms-getmap_animation.out  >   testresults/test_wms-getmap_animation.gif


export QUERY_STRING="service=WMS&REQUEST=GetMetadata&format=text/plain&width=256&height=256&BBOX=0,48,10,58&SRS=EPSG:4326&LAYER=RADNL_OPER_R___25PCPRR_L3_COLOR"
  ../adagucserverEC/adagucserverEC > testresults/test_wms-getmetadata.txt