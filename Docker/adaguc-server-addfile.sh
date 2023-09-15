#!/bin/bash

# TODO: https://github.com/KNMI/adaguc-server/issues/71

# Author: maarten.plieger@knmi.nl
# 2020-04-17
#
# This script is for scanning files with adaguc-server. Adaguc can detect automatically to which dataset(s) the file belongs.
#
# With -f you can optionally specify the file to add
# With -d you can optionally specify the dataset to add.
#
# Usage: ./adaguc-server-addfile.sh -d <datasetname (optional)> -f <file to add>
# Usage: ./adaguc-server-addfile.sh -d <datasetname> 


THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

. ${THISSCRIPTDIR}/adaguc-server-chkconfig.sh

ADAGUC_DATASET=''

while getopts "d:f:" o; do
    case "${o}" in
        d)
            ADAGUC_DATASET=${OPTARG}
            ;;
        f)
            ADAGUC_DATAFILE=${OPTARG}
            ;;
        h)
            usage
            ;;
        *)
            usage
            ;;
    esac
done


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
if [[ -n "${ADAGUC_DATASET}" ]]; then
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
