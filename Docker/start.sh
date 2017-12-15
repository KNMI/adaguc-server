#!/bin/bash

ADAGUCDB=/adaguc/adagucdb

files=$(shopt -s nullglob dotglob; echo ${ADAGUCDB}/*)
if (( ${#files} ))
then
  echo "Re-using database files from ${ADAGUCDB}" && \
  runuser -l postgres -c "pg_ctl -w -D ${ADAGUCDB} -l /var/log/adaguc/postgresql.log start"
else 
  echo "Initializing postgresdb"
  mkdir -p ${ADAGUCDB} && chmod 777 ${ADAGUCDB} && chown postgres: ${ADAGUCDB} && \
  runuser -l postgres -c "pg_ctl initdb -w -D ${ADAGUCDB}" && \
  runuser -l postgres -c "pg_ctl -w -D ${ADAGUCDB} -l /var/log/adaguc/postgresql.log start" && \
  echo "Configuring POSTGRESQL DB" && \
  runuser -l postgres -c "createuser --superuser adaguc" && \
  runuser -l postgres -c "psql postgres -c \"ALTER USER adaguc PASSWORD 'adaguc';\"" && \
  runuser -l postgres -c "psql postgres -c \"CREATE DATABASE adaguc;\""
fi

echo "Checking POSTGRESQL DB" && \
    runuser -l postgres -c "psql postgres -c \"show data_directory;\"" && \
    echo "Creating directories" && \
    cp /adaguc/adaguc-server-master/data/datasets/testdata.nc /data/adaguc-autowms && \
    cp /adaguc/adaguc-server-master/data/config/datasets/baselayers.xml /data/adaguc-datasets && \
    cp /adaguc/adaguc-server-master/data/config/datasets/dataset_a.xml /data/adaguc-datasets && \
    echo "Starting TOMCAT Server" && \
    /usr/libexec/tomcat/server start 
    
