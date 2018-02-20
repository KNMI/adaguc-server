#!/bin/bash
export ADAGUC_PATH=/adaguc/adaguc-server-master/
export ADAGUC_TMP=/tmp

if [[ $1 ]]; then

  # Update a specific dataset
  for configfile in /data/adaguc-datasets/$1.xml ;do
    filename=/data/adaguc-datasets/"${configfile##*/}" 
    echo "Starting update for ${filename}" 
    /adaguc/adaguc-server-master/bin/adagucserver --updatedb --config /adaguc/adaguc-server-config.xml,${filename}
    /adaguc/adaguc-server-master/bin/adagucserver --createtiles --config /adaguc/adaguc-server-config.xml,${filename}
  done

else
  echo "Please specify a dataset"
fi
