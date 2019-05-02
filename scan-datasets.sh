#!/bin/sh


usage() {
  echo ""
  echo "Usage: `basename $0` -h <adaguc_home_dir> -d <dataset_name>"
  echo "\n-h\t<adaguc_home_dir>:\tfully-qualified root folder under which adaguc directories were created"
  echo "-d\t<dataset>:\t\tname of dataset to scan"
  echo ""
  exit 1
}

scan_dataset() {
  docker run -it --rm --network docker_adaguc-network \
    -v ${ADAGUCHOME}/adaguc-server-docker/adaguc-data:/data/adaguc-data \
    -v ${ADAGUCHOME}/adaguc-server-docker/adaguc-datasets:/data/adaguc-datasets \
    -v ${ADAGUCHOME}/adaguc-server-docker/adaguc-autowms:/data/adaguc-autowms \
    -v ${ADAGUCHOME}/adaguc-server-docker/adaguc-logs:/var/log/adaguc \
    -e ADAGUC_DB="\"host=adaguc-db port=5432 user=adaguc password=adaguc dbname=adaguc\"" \
    --tmpfs /tmp \
    --name adaguc-dataset-sync \
    openearth/adaguc-dataset-scanner ${DATASET_NAME}
}

while getopts h:d: opt; do
  case $opt in
    h) ADAGUCHOME=${OPTARG};;
    d) DATASET_NAME=${OPTARG};;
    \?) usage;;
  esac
done

shift $((OPTIND-1))

if [[ -z ${ADAGUCHOME} ]] || [[ -z ${DATASET_NAME} ]]; then
  usage
else
  scan_dataset
fi
