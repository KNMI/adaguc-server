#!/bin/bash

THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
. ${THISSCRIPTDIR}/adaguc-server-chkconfig.sh

echo "Using adagucserver from  ${ADAGUC_PATH}"
echo "Using config from ${ADAGUC_CONFIG}"

if [[ $1 ]]; then
  # Update a specific dataset
  for configfile in /data/adaguc-datasets/$1 ;do
    filename=/data/adaguc-datasets/"${configfile##*/}" 
    filebasename=${filename##*/}
    echo ""
    echo "*** Starting updatedb for ${filename}" 
    ${ADAGUC_PATH}/bin/adagucserver --updatedb --config ${ADAGUC_CONFIG},${filename}
    echo "*** Starting create tiles for ${filename}" 
    ${ADAGUC_PATH}/bin/adagucserver --createtiles --config ${ADAGUC_CONFIG},${filename}
    OUT=$?
  done

else
  echo "Please specify a dataset"
fi


