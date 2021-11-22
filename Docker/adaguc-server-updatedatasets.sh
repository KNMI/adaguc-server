#!/bin/bash

# This script requires ADAGUC_CONFIG to be set. In case of running locally with adaguc-services, this is probably 
# `export ADAGUC_CONFIG=./data/config/adaguc.vm.xml`

# TODO: https://github.com/KNMI/adaguc-server/issues/71

THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

. ${THISSCRIPTDIR}/adaguc-server-chkconfig.sh

# Unbufferd logging for realtime output
export ADAGUC_ENABLELOGBUFFER=FALSE

if [[ $1 ]]; then
  # Update a specific dataset
  for configfile in /data/adaguc-datasets/$1 ;do
    filename=/data/adaguc-datasets/"${configfile##*/}" 
    filebasename=${filename##*/}
    echo ""
    if [[ $2 ]]; then
      echo "*** Starting update with tailpath $2 for dataset ${filename}" 
      ${ADAGUC_PATH}/bin/adagucserver --updatedb --config ${ADAGUC_CONFIG},${filename} --tailpath $2
      OUT=$?
    else
      echo "*** Starting update for ${filename}" 
      ${ADAGUC_PATH}/bin/adagucserver --updatedb --config ${ADAGUC_CONFIG},${filename}
      OUT=$?
    fi
  
    echo ""
  done

else
 
  # Update all datasets
  for configfile in /data/adaguc-datasets/*xml ;do
    filename=/data/adaguc-datasets/"${configfile##*/}" 
    filebasename=${filename##*/}
    if [[ "${ADAGUC_DATASET_MASK}" && `echo ${filebasename} | grep -E ${ADAGUC_DATASET_MASK}` != ${filebasename} ]] ; then
        if [[ "${ADAGUC_DATASET_MASK}" ]] ; then
            echo "${filebasename} doesn't match ${ADAGUC_DATASET_MASK}"
        fi
        continue
    fi
    echo ""
    echo "Starting update for ${filename}" 
    ${ADAGUC_PATH}/bin/adagucserver --updatedb --config ${ADAGUC_CONFIG},${filename}
    OUT=$?
     done

fi
