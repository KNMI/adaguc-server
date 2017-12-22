#!/bin/bash
export ADAGUC_PATH=/adaguc/adaguc-server-master/
export ADAGUC_TMP=/tmp
for configfile in /data/adaguc-datasets/*xml ;do
  filename=/data/adaguc-datasets/"${configfile##*/}" 
  echo "Starting update for ${filename}" 
  /adaguc/adaguc-server-master/bin/adagucserver --updatedb --config /adaguc/adaguc-server-config.xml,${filename}
done
