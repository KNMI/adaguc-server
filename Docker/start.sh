#!/bin/bash

echo "Starting POSTGRESQL DB" && \
    runuser -l postgres -c "pg_ctl -D /postgresql -l /var/log/postgresql.log -w start" && \
    sleep 1 && \
    cp /adaguc/adaguc-server-master/data/datasets/testdata.nc /data/adaguc-autowms && \
    cp /adaguc/adaguc-server-master/data/config/datasets/baselayers.xml /data/adaguc-datasets && \
    cp /adaguc/adaguc-server-master/data/config/datasets/dataset_a.xml /data/adaguc-datasets && \
    echo "Configuring POSTGRESQL DB" && \
    runuser -l postgres -c "createuser --superuser adaguc" && \
    runuser -l postgres -c "psql postgres -c \"ALTER USER adaguc PASSWORD 'adaguc';\"" && \
    runuser -l postgres -c "psql postgres -c \"CREATE DATABASE adaguc;\"" && \
    echo "Starting TOMCAT Server" && \
    /usr/libexec/tomcat/server start 
    
