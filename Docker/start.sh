#!/bin/bash

echo "Starting POSTGRESQL DB" && \
    runuser -l postgres -c "pg_ctl -w -D /postgresql -l /var/log/postgresql.log start" && \
    cp /adaguc/adaguc-server-master/data/datasets/testdata.nc /data/adaguc-autowms && \
    cp /adaguc/adaguc-server-master/data/config/datasets/baselayers.xml /data/adaguc-datasets && \
    echo "Configuring POSTGRESQL DB" && \
    runuser -l postgres -c "createuser --superuser adaguc" && \
    runuser -l postgres -c "psql postgres -c \"ALTER USER adaguc PASSWORD 'adaguc';\"" && \
    runuser -l postgres -c "psql postgres -c \"CREATE DATABASE adaguc;\"" && \
    echo "Starting TOMCAT Server" && \
    /usr/libexec/tomcat/server start 
    
