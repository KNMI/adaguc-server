#!/bin/bash
chmod 777 /var/log/adaguc/

files=$(shopt -s nullglob dotglob; echo ${ADAGUCDB}/*)
if (( ${#files} ))
then
  echo "Re-using persistent postgresql database from ${ADAGUCDB}" && \
  runuser -l postgres -c "pg_ctl -w -D ${ADAGUCDB} -l /var/log/adaguc/postgresql.log start"
else 
  echo "Initializing new postgresql database"
  mkdir -p ${ADAGUCDB} && chmod 777 ${ADAGUCDB} && chown postgres: ${ADAGUCDB} && \
  runuser -l postgres -c "pg_ctl initdb -w -D ${ADAGUCDB}" && \
  runuser -l postgres -c "pg_ctl -w -D ${ADAGUCDB} -l /var/log/adaguc/postgresql.log start" && \
  echo "Configuring new postgresql database" && \
  runuser -l postgres -c "createuser --superuser adaguc" && \
  runuser -l postgres -c "psql postgres -c \"ALTER USER adaguc PASSWORD 'adaguc';\"" && \
  runuser -l postgres -c "psql postgres -c \"CREATE DATABASE adaguc;\""
fi

echo "Checking POSTGRESQL DB" && \
    runuser -l postgres -c "psql postgres -c \"show data_directory;\"" && \
    echo "Starting adaguc-services Server" && \
    echo "Starting tomcat server with adaguc-services application" && \
    /usr/libexec/tomcat/server start 
    
