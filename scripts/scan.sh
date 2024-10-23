#!/bin/bash
#
# This script is for scanning files with adaguc-server. Adaguc can detect automatically to which dataset(s) the file belongs.
#
# With -f you can optionally specify the file to add
# With -d you can optionally specify the dataset to add.
# If only a file is specified without a dataset, adaguc-server will try to find the matching dataset that belongs to the file.
#
# Usage: ./scan.sh -d <datasetname (optional)> -f <file to add>
# Usage: ./scan.sh -f <file to add> 
# Usage: ./scan.sh -d <datasetname> 
# Usage: ./scan.sh -d "*" to scan all datasets
# Usage: ./scan.sh -l to list datasets
# Usage: ./scan.sh -e to see current environment


THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
. ${THISSCRIPTDIR}/adaguc-server-chkconfig.sh

ADAGUC_DATASET=''

while getopts "d:f:le" o; do
    case "${o}" in
        d)
            ADAGUC_DATASET=${OPTARG}
            ;;
        f)
            ADAGUC_DATAFILE=${OPTARG}
            ;;
        l)
            ls ${ADAGUC_DATASET_DIR}
            ;;
        e)
            env | grep "ADAGUC"
            ;;
    esac
done


if [ $OPTIND -eq 1 ]; then ADAGUC_DATASET="*"; fi

### Scan a file for a specified dataset ###
if [[ -n "${ADAGUC_DATASET}" &&  -n "${ADAGUC_DATAFILE}" ]]; then
  STATUSCODE=0
  echo "Adding file [${ADAGUC_DATAFILE}] to dataset [${ADAGUC_DATASET}]:"
  command="${ADAGUC_PATH}/bin/adagucserver --updatedb --config ${ADAGUC_CONFIG},${ADAGUC_DATASET} --path ${ADAGUC_DATAFILE}"
  echo $command
  $command
  OUT=$?
  if [ ${OUT} -ne 0 ]; then
    STATUSCODE=1
  fi
  exit ${STATUSCODE} 
fi


### Scan a file ###
if [[ -n "${ADAGUC_DATAFILE}" ]]; then

  basediroffile=${ADAGUC_DATAFILE%/*}
  basenameoffile="${ADAGUC_DATAFILE##*/}"
  
  STATUSCODE=0
  echo "Adding file [${ADAGUC_DATAFILE}] to dataset [${alldatasets}]:"
  command="${ADAGUC_PATH}/bin/adagucserver --updatedb --autofinddataset --verboseoff --config ${ADAGUC_CONFIG} --path ${ADAGUC_DATAFILE}"
  echo $command
  $command
  OUT=$?
  if [ ${OUT} -ne 0 ]; then
    STATUSCODE=1
  fi
  exit ${STATUSCODE} 
fi

# Scan a dataset
if [[ -n "${ADAGUC_DATASET}" ]] && [ "${ADAGUC_DATASET}" != "*" ]; then
  STATUSCODE=0
  echo "Scanning full dataset [${ADAGUC_DATASET}]:"
  command="${ADAGUC_PATH}/bin/adagucserver --updatedb --verboseoff --config ${ADAGUC_CONFIG},${ADAGUC_DATASET}"
  echo $command
  $command
  OUT=$?
  if [ ${OUT} -ne 0 ]; then
    STATUSCODE=1
  fi
  exit ${STATUSCODE} 
fi

# Scan al datasets
if [[ -n "${ADAGUC_DATASET}" ]] && [ "${ADAGUC_DATASET}" == "*" ]; then
  echo "Scanning all datasets"
  for configfile in ${ADAGUC_DATASET_DIR}/*xml ;do
    STATUSCODE=0
    echo "Scanning full dataset [${configfile}]:"
    command="${ADAGUC_PATH}/bin/adagucserver --updatedb --verboseoff --config ${ADAGUC_CONFIG},${configfile}"
    echo $command
    $command
    OUT=$?
    if [ ${OUT} -ne 0 ]; then
      STATUSCODE=1
    fi
  done
  exit ${STATUSCODE} 
fi

