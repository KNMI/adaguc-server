#!/bin/bash

### Check if conditions are OK ###

if [ "$ADAGUC_DB" = "" ]
then
   echo "ADAGUC_DB env is not set"
   exit 1
fi

if [ "$EXTERNALADDRESS" = "" ]
then
   echo "EXTERNALADDRESS env is not set"
   exit 1
fi

### Wait till DB is up ###
RETRIES=60
until psql "${ADAGUC_DB}" -c "select 1" 2>&1 || [ $RETRIES -eq 0 ]; do 
  echo "Waiting for postgres server to start, $((RETRIES)) remaining attempts..." RETRIES=$((RETRIES-=1)) 
  sleep 1 
done


### Update baselayers and check if this succeeds ###
export ADAGUC_CONFIG=/adaguc/adaguc-server-config.xml
export ADAGUC_DATARESTRICTION=FALSE
export ADAGUC_ENABLELOGBUFFER=FALSE
export ADAGUC_FONT=/adaguc/adaguc-server-master/data/fonts/FreeSans.ttf
export ADAGUC_PATH=/adaguc/adaguc-server-master/ 
export ADAGUC_ONLINERESOURCE="${EXTERNALADDRESS}/adaguc-server?"
export ADAGUC_TMP=/tmp
export ADAGUC_LOGFILE=/var/log/adaguc-server.log
export ADAGUC_ERRORFILE=/dev/stderr


/adaguc/adaguc-server-master/bin/adagucserver --updatedb \
  --config /adaguc/adaguc-server-config.xml,baselayers.xml

if [ $? -ne 0 ]
then
  echo "Unable to update baselayers with adaguc-server --updatedb"
  exit 1
fi  

 exec /usr/sbin/apachectl -DFOREGROUND
    
