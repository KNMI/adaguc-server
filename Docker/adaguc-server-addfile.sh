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
# Usage: ./adaguc-server-addfile.sh  

THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

. ${THISSCRIPTDIR}/adaguc-server-chkconfig.sh

ADAGUC_DATASET='*.xml'

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

# if [ -n "${ADAGUC_DATAFILE}" ]; then
#   echo "Adding ${ADAGUC_DATAFILE} to the available datasets."
# else
#   echo "You did not supply a file to add!"
#   exit 1
# fi

# Update all datasets
for configfile in /data/adaguc-datasets/${ADAGUC_DATASET} ;do
  filename=/data/adaguc-datasets/"${configfile##*/}" 
  filebasename=${filename##*/}
  if [[ "${ADAGUC_DATASET_MASK}" && `echo ${filebasename} | grep -E ${ADAGUC_DATASET_MASK}` != ${filebasename} ]] ; then
      if [[ "${ADAGUC_DATASET_MASK}" ]] ; then
          echo "${filebasename} doesn't match ${ADAGUC_DATASET_MASK}"
      fi
      continue
  fi
  
  if [ -n "${ADAGUC_DATAFILE}" ]; then
    echo ""
    echo "Starting update for dataset ${filename} and datafile ${ADAGUC_DATAFILE}" 
    ${ADAGUC_PATH}/bin/adagucserver --updatedb --config ${ADAGUC_CONFIG},${filename} --path ${ADAGUC_DATAFILE}
    OUT=$?
    if [ -d /servicehealth ]; then
      echo "$OUT" > /servicehealth/${filebasename%.*}
    fi
  else
    echo ""
    echo "Starting update for dataset ${filename}" 
    ${ADAGUC_PATH}/bin/adagucserver --updatedb --config ${ADAGUC_CONFIG},${filename}
    OUT=$?
    if [ -d /servicehealth ]; then
      echo "$OUT" > /servicehealth/${filebasename%.*}
    fi
  fi
done

