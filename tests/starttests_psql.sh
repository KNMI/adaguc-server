#!/bin/bash
echo "Starting adaguc-server functional tests"
pushd `dirname $0` > /dev/null
SCRIPTPATH=`pwd`
popd > /dev/null
unset ADAGUC_CONFIG
unset QUERY_STRING
export ADAGUC_PATH=${SCRIPTPATH}/../
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


if [[ "${TEST_IN_CONTAINER}" == "github_build" ]]; then
    db_host="localhost"
elif [[ "${TEST_IN_CONTAINER}" == "local_build" ]]; then
    db_host="host.docker.internal"
else
    db_host="localhost"
fi

# This assumes you can reach psql on port 5432
# Tests will use a separate `adaguc_test` database
# You cannot drop the database you are currently logged in as
export ADAGUC_DB="user=adaguc password=adaguc host=${db_host} dbname=postgres port=54321"

psql "${ADAGUC_DB}" -c "SELECT 1";
if [[ $? -ne 0 ]]; then
    echo ""
    echo -e "No Adaguc test database available. Please run:\n"
    echo -e "\tdocker compose -f Docker/docker-compose-test.yml up -Vd"
    exit 1
fi

psql "$ADAGUC_DB" -c "DROP DATABASE IF EXISTS adaguc_test;"
psql "$ADAGUC_DB" -c "CREATE DATABASE adaguc_test;"
export ADAGUC_DB="user=adaguc password=adaguc host=${db_host} dbname=adaguc_test port=54321"

ulimit -c unlimited

python3 ${ADAGUC_PATH}/tests/functional_test.py $1 && \
cd ../python/python_fastapi_server && \
bash ./test_server.sh