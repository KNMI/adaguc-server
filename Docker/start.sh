#!/bin/bash

### Check if conditions are OK ###

if [ "$ADAGUC_DB" = "" ]
then
   echo "ADAGUC_DB env is not set"
   exit 1
fi

if [ "$ADAGUC_SERVER_PORT" = "" ]
then
   export ADAGUC_SERVER_PORT=8080
fi


### Wait till DB is up ###
RETRIES=60
sleep 1
until psql "${ADAGUC_DB}" -c "select 1" 2>&1 || [ $RETRIES -eq 0 ]; do
  echo "Waiting for postgres server to start, $((RETRIES)) remaining attempts..." RETRIES=$((RETRIES-=1))
  sleep 2
done


### Update baselayers and check if this succeeds ###

export ADAGUC_PATH=/adaguc/adaguc-server-master/ && \
export ADAGUC_TMP=/tmp && \
env -u ADAGUC_FORK_ENABLE /adaguc/adaguc-server-master/bin/adagucserver --updatedb \
  --config /adaguc/adaguc-server-config.xml,baselayers.xml

if [ $? -ne 0 ]
then
  echo "Unable to update baselayers with adaguc-server --updatedb"
  exit 1
fi

echo "Start serving on 0.0.0.0"
# keep-alive should be greater than the idle timout of the proxy/load balancer sitting in front of adaguc
# See: https://iximiuz.com/en/posts/reverse-proxy-http-keep-alive-and-502s/
gunicorn --bind 0.0.0.0:${ADAGUC_SERVER_PORT} --workers=1 -k no_proxy_headers_uvicorn_worker.NoProxyHeadersUvicornWorker --disable-redirect-access-to-syslog --keep-alive 200 --access-logfile - main:app