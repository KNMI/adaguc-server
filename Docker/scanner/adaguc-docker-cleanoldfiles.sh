#/bin/bash

#This scripts calls the adaguc-server docker to clean files older then n days.

#Usage: bash adaguc-docker-cleanoldfiles.sh  -p <directory> -f <filter> -d <number of days> -q filetimedate -t <delete_db or delete_db_and_fs>
#Example: bash adaguc-docker-cleanoldfiles.sh -p /data/adaguc-autowms/EGOWS_radar -f ".*\.nc$" -d 7 -q filetimedate -t delete_db


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
    --entrypoint '/adaguc/adaguc-server-cleandb.sh' \
    openearth/adaguc-dataset-scanner $@
