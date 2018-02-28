#!/bin/bash

PGUSERNAME=postgres

# Detect postgres user id
if [ -z ${PGUSERID+x} ] || [ -z ${PGUSERID} ]; then 
  echo "PGUSERID is unset, trying to get id from directory"; 
  export PGUSERID=$(stat -c "%u" ${ADAGUCDB})
  echo "Got ${PGUSERID} from owner of dir ${ADAGUCDB}"
  if [ ${PGUSERID} == 0 ]; then 
    echo "PGUSERID has root id, setting to postgres"; 
    export PGUSERID=`id -u postgres`
  fi
else 
  echo "PGUSERID is set to '$PGUSERID'"; 
fi

# Create postgres user
echo "Using PGUSERID : ${PGUSERID}"
useradd --shell /bin/bash -u ${PGUSERID} -o -c "" -m $PGUSERNAME
export HOME=/home/$PGUSERNAME

# Set postgres permissions
chmod 777 /var/run/postgresql/
runuser -l $PGUSERNAME -c "touch /var/log/adaguc/postgresql.log"
runuser -l $PGUSERNAME -c "chmod 777 /var/log/adaguc/postgresql.log"
chown $PGUSERNAME ${ADAGUCDB}
runuser -l $PGUSERNAME -c "chmod 700 ${ADAGUCDB}"

# Check if a db already exists for given path
dbexists=`runuser -l $PGUSERNAME -c "(ls ${ADAGUCDB}/postgresql.conf >> /dev/null 2>&1 && echo yes) || echo no"`
if [ ${dbexists} == "no" ]
then
  echo "Initializing new postgresql database"
  #mkdir -p ${ADAGUCDB} && chmod 777 ${ADAGUCDB} && chown postgres: ${ADAGUCDB} && #TODO NOT NEEDED ANYMORE?
  runuser -l $PGUSERNAME -c "pg_ctl initdb -U $PGUSERNAME -w -D ${ADAGUCDB}" && \
  runuser -l $PGUSERNAME -c "pg_ctl -w -U $PGUSERNAME -D ${ADAGUCDB} -l /var/log/adaguc/postgresql.log start" && \
  echo "Configuring new postgresql database" && \
  runuser -l $PGUSERNAME -c "createuser --superuser adaguc" && \
  runuser -l $PGUSERNAME -c "psql -U $PGUSERNAME postgres -c \"ALTER USER adaguc PASSWORD 'adaguc';\"" && \
  runuser -l $PGUSERNAME -c "psql -U $PGUSERNAME postgres -c \"CREATE DATABASE adaguc;\""
  
  if [ $? -ne 0 ]
  then
  exit 1
  fi
else 
  echo "Re-using persistent postgresql database from ${ADAGUCDB}" && \
  runuser -l $PGUSERNAME -c "pg_ctl -w -U $PGUSERNAME -D ${ADAGUCDB} -l /var/log/adaguc/postgresql.log start"
  if [ $? -ne 0 ]
  then
  exit 1
  fi
fi

echo "Checking POSTGRESQL DB" &&  runuser -l $PGUSERNAME -c "psql -U $PGUSERNAME postgres -c \"show data_directory;\""
if [ $? -ne 0 ]
then
  echo "Unable to connect to postgres database"
  exit 1
fi  

# Update baselayers and check if this succeeds
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
    
