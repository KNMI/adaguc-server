#!/bin/bash 
ADAGUC_PATH=`pwd`/../
export ADAGUC_PATH=`realpath ${ADAGUC_PATH}`/
export PYTHONPATH=`pwd`/../data/python/
echo $ADAGUC_PATH
export ADAGUC_TMP=`pwd`/tmp/
export ADAGUC_LOGFILE=`pwd`/log/
rm -rf $ADAGUC_TMP && mkdir -p $ADAGUC_TMP
rm -rf $ADAGUC_LOGFILE && mkdir -p $ADAGUC_LOGFILE
export ADAGUC_LOGFILE=`pwd`/log/adaguc-server.log

export ADAGUC_CONFIG="${ADAGUC_PATH}/data/config/adaguc.autoresource.xml"
export ADAGUC_FONT="${ADAGUC_PATH}/data/fonts/FreeSans.ttf"
export ADAGUC_ONLINERESOURCE=""

python functional_test.py
