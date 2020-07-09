#/bin/bash

# Author: maarten.plieger@knmi.nl
# 2020-04-17
#
# This script is for scanning (adding) files with adaguc containers.
# This script calls the adaguc-server container to add a specified file. 
# Adaguc will detect automatically to which dataset(s) the file belongs.
# This script uses the settings as provided in the Docker/.env file.
#
# Usage: bash ./Docker/scanner/adaguc-docker-scanner.sh -f <filetoadd>
# Example: bash ./Docker/scanner/adaguc-docker-scanner.sh -f /data/adaguc-data/tg_ens_mean_0.25deg_reg_v20.0e.nc
#
# With -f you can optionally specify the file to add
# With -d you can optionally specify the dataset to add.
#
# Usage: ./adaguc-docker-scanner.sh -d <datasetname (optional)> -f <file to add>
# Usage: ./adaguc-docker-scanner.sh -d <datasetname> 
# Usage: ./adaguc-docker-scanner.sh  

#
# This script uses the scanner container, 
# which can be obtained by doing:
#   docker pull openearth/adaguc-dataset-scanner
# or can be built by doing:  
#   docker build -t openearth/adaguc-dataset-scanner -f dataset-scanner.Dockerfile .
#
# ENVIRONMENT VARIABLES (optional), if not provided they are taken from the .env file.
#  - ADAGUC_DATA_DIR=/data/adaguc-data
#  - ADAGUC_AUTOWMS_DIR=/data/adaguc-autowms
#  - ADAGUC_DATASET_DIR=/data/adaguc-datasets


THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

#Check if docker-compose .env is present. If so check if environment variables are already set, if not set them based on the .env file.
ENVFILE=${THISSCRIPTDIR}/../.env
if [ -f "$ENVFILE" ]; then
    ENVFILECONTENTS=`cat ${ENVFILE}`;for ENVLINE in $ENVFILECONTENTS;do 
    ENVVAR=${ENVLINE%=*};
    if [[ ! -v "${ENVVAR}" ]]; then
        # echo "Using $ENVLINE"
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
    --entrypoint '/adaguc/adaguc-server-addfile.sh' \
    openearth/adaguc-dataset-scanner $@

