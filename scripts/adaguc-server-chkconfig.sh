DEFAULT_ADAGUC_PATH=/adaguc/adaguc-server-master
DEFAULT_ADAGUC_CONFIG=/adaguc/adaguc-server-config.xml

THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

### Check if ADAGUC_PATH is set externally, if not set it to default ###
if [ ! -f "${ADAGUC_PATH}/bin/adagucserver" ]; then
  export ADAGUC_PATH=${DEFAULT_ADAGUC_PATH}
fi

### Check if we can find adaguc executable at default location, otherwise try it from this location ###
if [ ! -f "${ADAGUC_PATH}/bin/adagucserver" ]; then
  export ADAGUC_PATH=${THISSCRIPTDIR}/../
fi

### Check if we could find adaguc executable ###
if [ ! -f "${ADAGUC_PATH}/bin/adagucserver" ]; then
  >&2 echo "No adagucserver executable found in path ADAGUC_PATH/bin/adagucserver [${ADAGUC_PATH}/bin/adagucserver] "
  exit 1
fi

### Check configuratiion file location ###
if [ ! -f "${ADAGUC_CONFIG}" ]; then
  export ADAGUC_CONFIG=${DEFAULT_ADAGUC_CONFIG}
fi

### Checks if configuration file exists
if [ ! -f "${ADAGUC_CONFIG}" ]; then
  >&2 echo "No configuration file found ADAGUC_CONFIG variable [${ADAGUC_CONFIG}] "
  exit 1
fi

### Check ADAGUC_DATA_DIR directory and set it if not set
if [ ! -d "${ADAGUC_DATA_DIR}" ]; then
  export ADAGUC_DATA_DIR="/data/adaguc-data"
fi

### Check ADAGUC_DATASET_DIR directory and set it if not set
if [ ! -d "${ADAGUC_DATASET_DIR}" ]; then
  export ADAGUC_DATASET_DIR="/data/adaguc-datasets"
fi

### Check ADAGUC_AUTOWMS_DIR directory and set it if not set
if [ ! -d "${ADAGUC_AUTOWMS_DIR}" ]; then
  export ADAGUC_AUTOWMS_DIR="/data/adaguc-autowms"
fi

export ADAGUC_TMP=/tmp
export ADAGUC_ONLINERESOURCE=""
