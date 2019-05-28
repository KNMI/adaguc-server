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
    
