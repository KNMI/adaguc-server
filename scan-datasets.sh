#!/bin/sh


usage() {
  echo ""
  echo "Usage: `basename $0` -a <adaguc_home_dir> -d <db_host> -s <dataset_name>"
  echo "\n-a\t<adaguc_home_dir>:\troot folder under which adaguc directories were created"
  echo "-d\t<db_host>:\t\thostname or ip address of postgres database"
  echo "-s\t<dataset>:\t\tname of dataset to scan"
  echo ""
  exit 1
}

scan_dataset() {
  docker run -it \
    -v ${ADAGUCHOME}/adaguc-server-docker/adaguc-data:/data/adaguc-data \
    -v ${ADAGUCHOME}/adaguc-server-docker/adaguc-datasets:/data/adaguc-datasets \
    -v ${ADAGUCHOME}/adaguc-server-docker/adaguc-autowms:/data/adaguc-autowms \
    -v ${ADAGUCHOME}/adaguc-server-docker/adaguc-logs:/var/log/adaguc \
    -e ADAGUC_DB="host=${DB_HOST} port=5432 user=adaguc password=adaguc dbname=adaguc" \
    --tmpfs /tmp \
    --name adaguc-dataset-sync \
    openearth/adaguc-dataset-scanner ${DATASET_NAME}
}

while getopts a:d:s: opt; do
  case $opt in
    a) ADAGUCHOME=${OPTARG};;
    d) DB_HOST=${OPTARG};;
    s) DATASET_NAME=${OPTARG};;
    \?) usage;;
  esac
done

shift $((OPTIND-1))

if [[ -z ${ADAGUCHOME} ]] || [[ -z ${DB_HOST} ]] || [[ -z ${DATASET_NAME} ]]; then
  usage
else
  echo "Scanning"
fi
