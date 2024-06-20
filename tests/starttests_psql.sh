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
ulimit -c unlimited

# Rewrite the xml configuration files used by tests.
# Unfortunately macos does not use GNU sed, so the sed command differs slightly (-i '' vs -i)
if [[ "$OSTYPE" == "darwin"* ]]; then
    find data/config -type f -name '*.xml' | xargs sed -i '' 's;<DataBase dbtype="sqlite" parameters="{ADAGUC_TMP}/adaguc.autoresource.db"/>;<DataBase parameters="{ADAGUC_DB}"/>;g'
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    find data/config -type f -name '*.xml' | xargs sed -i 's;<DataBase dbtype="sqlite" parameters="{ADAGUC_TMP}/adaguc.autoresource.db"/>;<DataBase parameters="{ADAGUC_DB}"/>;g'
fi

# This assumes you can reach psql on port 5432
# Tests will use a separate `adaguc_test` database
export ADAGUC_DB="user=adaguc password=adaguc host=localhost dbname=postgres"
psql "$ADAGUC_DB" -c "DROP DATABASE IF EXISTS adaguc_test;"
psql "$ADAGUC_DB" -c "CREATE DATABASE adaguc_test;"
export ADAGUC_DB="user=adaguc password=adaguc host=host.docker.internal dbname=adaguc_test"

python3 ${ADAGUC_PATH}/tests/functional_test.py $1 && \
cd ../python/python_fastapi_server && \
bash ./test_server.sh

# Undo changes made to the xml configuration files used by tests
if [[ "$OSTYPE" == "darwin"* ]]; then
    find data/config -type f -name '*.xml' | xargs sed -i '' 's;<DataBase parameters="{ADAGUC_DB}"/>;<DataBase dbtype="sqlite" parameters="{ADAGUC_TMP}/adaguc.autoresource.db"/>;g'
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    find data/config -type f -name '*.xml' | xargs sed -i 's;<DataBase parameters="{ADAGUC_DB}"/>;<DataBase dbtype="sqlite" parameters="{ADAGUC_TMP}/adaguc.autoresource.db"/>;g'
fi
