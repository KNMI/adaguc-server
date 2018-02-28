#!/bin/bash

if [ -z ${PGUSERID+x} ] || [ -z ${PGUSERID} ]; then 
  echo "PGUSERID is unset, trying to get id from directory"; 
  PGUSERID=$(stat -c "%u" ${ADAGUCDB})
  echo "Using ${PGUSERID} from owner of dir ${ADAGUCDB}"
else 
  echo "PGUSERID is set to '$PGUSERID'"; 
fi

echo "Using PGUSERID : ${PGUSERID}"
useradd --shell /bin/bash -u ${PGUSERID} -o -c "" -m user
export HOME=/home/user
chmod 777 /var/run/postgresql/
runuser -l user -c "touch /var/log/adaguc/postgresql.log"
runuser -l user -c "chmod 777 /var/log/adaguc/postgresql.log"
runuser -l user -c "chmod 700 ${ADAGUCDB}"
dbexists=`runuser -l user -c "(ls ${ADAGUCDB}/postgresql.conf >> /dev/null 2>&1 && echo yes) || echo no"`
if [ ${dbexists} == "no" ]
then
  echo "Initializing new postgresql database"
  #mkdir -p ${ADAGUCDB} && chmod 777 ${ADAGUCDB} && chown postgres: ${ADAGUCDB} && #TODO NOT NEEDED ANYMORE?
  runuser -l user -c "pg_ctl initdb -U user -w -D ${ADAGUCDB}" && \
  runuser -l user -c "pg_ctl -w -U user -D ${ADAGUCDB} -l /var/log/adaguc/postgresql.log start" && \
  echo "Configuring new postgresql database" && \
  runuser -l user -c "createuser --superuser adaguc" && \
  runuser -l user -c "psql -U user postgres -c \"ALTER USER adaguc PASSWORD 'adaguc';\"" && \
  runuser -l user -c "psql -U user postgres -c \"CREATE DATABASE adaguc;\""
  
  if [ $? -ne 0 ]
  then
  exit 1
  fi
else 
  echo "Re-using persistent postgresql database from ${ADAGUCDB}" && \
  runuser -l user -c "pg_ctl -w -U user -D ${ADAGUCDB} -l /var/log/adaguc/postgresql.log start"
  if [ $? -ne 0 ]
  then
  exit 1
  fi
fi

echo "Checking POSTGRESQL DB" &&  runuser -l user -c "psql -U user postgres -c \"show data_directory;\""
if [ $? -ne 0 ]
then
  echo "Unable to connect to postgres database"
  exit 1
fi  


export ADAGUC_PATH=/adaguc/adaguc-server-master/ && \
export ADAGUC_TMP=/tmp && \
/adaguc/adaguc-server-master/bin/adagucserver --updatedb \
  --config /adaguc/adaguc-server-config.xml,baselayers.xml

if [ $? -ne 0 ]
then
  echo "Unable to update baselayers with adaguc-server --updatedb"
  exit 1
fi  

echo "Start serving on ${EXTERNALADDRESS}"

echo "Starting adaguc-services Server" &&  /usr/libexec/tomcat/server start 
    
