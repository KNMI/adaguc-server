#!/bin/sh
echo "Building build adaguc-server"
docker build -t adaguc-server ..
#echo "Exporting image"
#docker save -o adaguc-server.dockerimage adaguc-server
