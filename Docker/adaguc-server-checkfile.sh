#!/bin/bash
export ADAGUC_PATH=/adaguc/adaguc-server-master/
export ADAGUC_TMP=/tmp
export ADAGUC_ONLINERESOURCE=""
export CHECKERFILE="/tmp/"`uuidgen`.report
export ADAGUC_CONFIG=/adaguc/adaguc-server-config.xml
export QUERY_STRING="source=$1&&service=WMS&request=getcapabilities"
/adaguc/adaguc-server-master/bin/adagucserver --report=$CHECKERFILE > /dev/null
cat $CHECKERFILE