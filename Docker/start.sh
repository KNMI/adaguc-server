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
java -jar /adaguc/adaguc-services.jar
    
