#/bin/bash

# Author: maarten.plieger@knmi.nl
# 2020-04-17
#
# This script is for listing all available datasets
# This script calls the adaguc-server container to list the datasets. 
# This script uses the settings as provided in the Docker/.env file.
#
# Usage: bash ./Docker/scanner/adaguc-docker-listdatasets.sh
#
# ENVIRONMENT VARIABLES (optional), if not provided they are taken from the .env file.
#  - ADAGUC_DATA_DIR=/data/adaguc-data
#  - ADAGUC_AUTOWMS_DIR=/data/adaguc-autowms
#  - ADAGUC_DATASET_DIR=/data/adaguc-datasets


export ADAGUC_DATASET=$1

THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

#Check if docker-compose .env is present. If so check if environment variables are already set, if not set them based on the .env file.
ENVFILE=${THISSCRIPTDIR}/../.env
if [ -f "$ENVFILE" ]; then
    ENVFILECONTENTS=`cat ${ENVFILE}`;for ENVLINE in $ENVFILECONTENTS;do 
    ENVVAR=${ENVLINE%=*};
    if [[ ! -v "${ENVVAR}" ]]; then
        #echo "Using $ENVLINE"
        export $ENVLINE;
        # else
        # echo "Re-using ${ENVVAR}=${!ENVVAR}"
    fi

    done
fi


docker run -it --rm --network docker_adaguc-network \
    -v ${ADAGUC_DATA_DIR}:/data/adaguc-data \
    -v ${ADAGUC_DATASET_DIR}:/data/adaguc-datasets \
    -v ${ADAGUC_AUTOWMS_DIR}:/data/adaguc-autowms \
    -e ADAGUC_DB="host=adaguc-db port=5432 user=adaguc password=adaguc dbname=adaguc" \
    --tmpfs /tmp \
    --name adaguc-dataset-sync \
    --entrypoint 'ls' \
    openearth/adaguc-dataset-scanner /data/adaguc-datasets $@

