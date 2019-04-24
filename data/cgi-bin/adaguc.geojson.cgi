#!/bin/bash
export ADAGUC_PATH=~/adagucserver/ # Replace with the location where you installed ADAGUC
export ADAGUC_TMP=/tmp/

export ADAGUC_CONFIG="${ADAGUC_PATH}/data/config/adaguc.geojson.xml"
export ADAGUC_DATARESTRICTION="FALSE" 
export ADAGUC_LOGFILE="${ADAGUC_TMP}/adaguc.geojson.log"
export ADAGUC_ERRORFILE="${ADAGUC_TMP}/adaguc.geojson.errlog"
export ADAGUC_FONT="${ADAGUC_PATH}/data/fonts/FreeSans.ttf"
export ADAGUC_ONLINERESOURCE="http://${HTTP_HOST}/cgi-bin/adaguc.geojson.cgi?"

#export LD_LIBRARY_PATH=:$LD_LIBRARY_PATH  # LD_LIBRARY_PATH not needed when netcdf and hdf5 installed through yum.
#export PROJ_LIB=/data/build/share/proj/   # Optional, needed when custom proj.4 library is installed.

$ADAGUC_PATH/adagucserverEC/adagucserver
