#!/bin/bash
export ADAGUC_PATH=/adaguc/adaguc-server-master/
export ADAGUC_TMP=/tmp

if [[ $1 ]]; then
  # remove all old service such that only active services are monitored
  
  
  # Update a specific dataset
  for configfile in /data/adaguc-datasets/$1.xml ;do
    filename=/data/adaguc-datasets/"${configfile##*/}" 
    filebasename=${filename##*/}
    rm -f /servicehealth/${filebasename%.*}
    echo "Starting update for ${filename}" 
    /adaguc/adaguc-server-master/bin/adagucserver --updatedb --config /adaguc/adaguc-server-config.xml,${filename}
    OUT=$?
    echo "$OUT" > /servicehealth/${filebasename%.*}
  done

else
  # remove all old services such that only active services are monitored
  rm -f /servicehealth/*
  # Update all datasets
  for configfile in /data/adaguc-datasets/*xml ;do
    filename=/data/adaguc-datasets/"${configfile##*/}" 
    filebasename=${filename##*/}
    echo "Starting update for ${filename}" 
    /adaguc/adaguc-server-master/bin/adagucserver --updatedb --config /adaguc/adaguc-server-config.xml,${filename}
    OUT=$?
    echo "$OUT" > /servicehealth/${filebasename%.*}
  done

fi
