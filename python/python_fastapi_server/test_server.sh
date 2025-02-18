#!/bin/bash
echo "Starting adaguc-server python server functional tests"
pushd `dirname $0` > /dev/null
SCRIPTPATH=`pwd`
popd > /dev/null
unset ADAGUC_CONFIG
unset QUERY_STRING
export ADAGUC_PATH=${SCRIPTPATH}/../..
export ADAGUC_PATH=`readlink -f ${ADAGUC_PATH}`/
export PYTHONPATH=${ADAGUC_PATH}/python/lib/
echo "ADAGUC-Server path is [$ADAGUC_PATH]"
export ADAGUC_TMP=${ADAGUC_PATH}/tests/tmp/
export ADAGUC_LOGFILE=${ADAGUC_PATH}/tests/log/
rm -rf $ADAGUC_LOGFILE && mkdir -p $ADAGUC_LOGFILE
export ADAGUC_LOGFILE=${ADAGUC_PATH}/tests/log/adaguc-server.log
export ADAGUC_FONT="${ADAGUC_PATH}/data/fonts/FreeSans.ttf"
export ADAGUC_ONLINERESOURCE=""
export ADAGUC_ENABLELOGBUFFER=FALSE
export ADAGUC_DATASET_DIR=${ADAGUC_PATH}/data/config/datasets/
export ADAGUC_DATA_DIR=${ADAGUC_PATH}/data/datasets/
export ADAGUC_AUTOWMS_DIR=${ADAGUC_PATH}/data/datasets/
ulimit -c unlimited

[[ $ADAGUC_DB == *.db ]] && exit
pytest -s -v python_fastapi_server/test_ogc_api_edr.py && pytest -s python_fastapi_server/test_ogc_api_features.py
