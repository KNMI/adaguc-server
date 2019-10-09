#!/bin/bash 
echo "Starting adaguc-server Polygon overlay"
pushd `dirname $0` > /dev/null
SCRIPTPATH=`pwd`
popd > /dev/null
unset ADAGUC_CONFIG
unset QUERY_STRING
export ADAGUC_PATH=${SCRIPTPATH}/../../
export ADAGUC_PATH=`readlink -f ${ADAGUC_PATH}`/
export PYTHONPATH=${ADAGUC_PATH}/data/python/
echo "ADAGUC-Server path is [$ADAGUC_PATH]"
export ADAGUC_TMP=${ADAGUC_PATH}/tests/tmp/
export ADAGUC_LOGFILE=${ADAGUC_PATH}/tests/log/
rm -rf $ADAGUC_LOGFILE && mkdir -p $ADAGUC_LOGFILE
export ADAGUC_LOGFILE=${ADAGUC_PATH}/tests/log/adaguc-server.log
export ADAGUC_FONT="${ADAGUC_PATH}/data/fonts/FreeSans.ttf"
export ADAGUC_ONLINERESOURCE=""
export ADAGUC_ENABLELOGBUFFER=FALSE
ulimit -c unlimited
python ${ADAGUC_PATH}/data/python/polygonoverlay.py $1
