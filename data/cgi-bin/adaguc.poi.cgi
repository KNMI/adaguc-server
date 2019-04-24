#!/bin/bash
export ADAGUC_PATH=/home/c4m/adagucserver/ # Replace with the location where you installed ADAGUC
export ADAGUC_TMP=/data/tmp/

export LD_LIBRARY_PATH="/data/build/lib/:$LD_LIBRARY_PATH"


export ADAGUC_CONFIG="${ADAGUC_PATH}/data/config/adaguc.poi.xml" 
export ADAGUC_DATARESTRICTION="FALSE" 
export ADAGUC_LOGFILE="${ADAGUC_TMP}/adaguc.poi.log" 
export ADAGUC_ERRORFILE="${ADAGUC_TMP}/adaguc.poi.errlog" 
export ADAGUC_FONT="${ADAGUC_PATH}/data/fonts/FreeSans.ttf" 
export ADAGUC_ONLINERESOURCE="http://${HTTP_HOST}/cgi-bin/adaguc.poi.cgi?" 

#export LD_LIBRARY_PATH=:$LD_LIBRARY_PATH  # LD_LIBRARY_PATH not needed when netcdf and hdf5 installed through yum.
#export PROJ_LIB=/data/build/share/proj/   # Optional, needed when custom proj.4 library is installed.

${ADAGUC_PATH}/adagucserverEC/adagucserver