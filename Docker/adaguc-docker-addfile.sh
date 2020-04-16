#/bin/bash

#This scripts calls the adaguc-server docker to add a file. The file will be automatically added to the correct dataset(s).

#Usage: bash adaguc-docker-addfile.sh /data/adaguc-data/tg_ens_mean_0.25deg_reg_v20.0e.nc

export ADAGUC_DATAFILE=$1

THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

a=`cat ${THISSCRIPTDIR}/.env`;for b in $a;do export $b;done

docker run -it --rm --network docker_adaguc-network \
    -v ${ADAGUC_DATA_DIR}:/data/adaguc-data \
    -v ${ADAGUC_DATASET_DIR}:/data/adaguc-datasets \
    -v ${ADAGUC_AUTOWMS_DIR}:/data/adaguc-autowms \
    -e ADAGUC_DB="host=adaguc-db port=5432 user=adaguc password=adaguc dbname=adaguc" \
    --tmpfs /tmp \
    --name adaguc-dataset-sync \
    --entrypoint '/adaguc/adaguc-server-addfile.sh' \
    openearth/adaguc-server -f ${ADAGUC_DATAFILE}

