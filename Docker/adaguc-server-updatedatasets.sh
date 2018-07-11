#!/bin/bash
export ADAGUC_PATH=/adaguc/adaguc-server-master/
export ADAGUC_TMP=/tmp
export ADAGUC_ONLINERESOURCE=""

if [[ $1 ]]; then
  # Update a specific dataset
  for configfile in /data/adaguc-datasets/$1 ;do
    filename=/data/adaguc-datasets/"${configfile##*/}" 
    filebasename=${filename##*/}
    # remove all old service status file such that only active services are monitored
    rm -f /servicehealth/${filebasename%.*}
    echo ""
    if [[ $2 ]]; then
      echo "*** Starting update with tailpath $2 for dataset ${filename}" 
      /adaguc/adaguc-server-master/bin/adagucserver --updatedb --config /adaguc/adaguc-server-config.xml,${filename} --tailpath $2
      OUT=$?
    else
      echo "*** Starting update for ${filename}" 
      /adaguc/adaguc-server-master/bin/adagucserver --updatedb --config /adaguc/adaguc-server-config.xml,${filename}
      OUT=$?
    fi
    echo "$OUT" > /servicehealth/${filebasename%.*}
    echo ""
  done

else
  # remove all old service status files such that only active services are monitored
  rm -f /servicehealth/*
  # Update all datasets
  for configfile in /data/adaguc-datasets/*xml ;do
    filename=/data/adaguc-datasets/"${configfile##*/}" 
    filebasename=${filename##*/}
    echo ""
    echo "Starting update for ${filename}" 
    /adaguc/adaguc-server-master/bin/adagucserver --updatedb --config /adaguc/adaguc-server-config.xml,${filename}
    OUT=$?
    echo "$OUT" > /servicehealth/${filebasename%.*}
  done

fi
