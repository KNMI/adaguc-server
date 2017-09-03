#!/bin/bash 
pushd `dirname $0` > /dev/null
SCRIPTPATH=`pwd`
popd > /dev/null
ADAGUC_PATH=${SCRIPTPATH}/../
export ADAGUC_PATH=`realpath ${ADAGUC_PATH}`/
export PYTHONPATH=${ADAGUC_PATH}/data/python/
echo $ADAGUC_PATH
export ADAGUC_TMP=${ADAGUC_PATH}/tests/tmp/
export ADAGUC_LOGFILE=${ADAGUC_PATH}/tests/log/
rm -rf $ADAGUC_LOGFILE && mkdir -p $ADAGUC_LOGFILE
export ADAGUC_LOGFILE=${ADAGUC_PATH}/tests/log/adaguc-server.log

export ADAGUC_CONFIG="${ADAGUC_PATH}/data/config/adaguc.autoresource.xml"
export ADAGUC_FONT="${ADAGUC_PATH}/data/fonts/FreeSans.ttf"
export ADAGUC_ONLINERESOURCE=""

python ${ADAGUC_PATH}/tests/functional_test.py
