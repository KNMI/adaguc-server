#!/bin/bash

# Author: maarten.plieger@knmi.nl
# 2018-05-23
# This script cleans the adaguc database for files which are older than specified nr of days
# TODO: Should also clean tables depending on other dimensions

if [ "$#" -ne 3 ]; then
    echo "This script cleans the database for files which are older than specified nr of days"
    echo "Error: Expected three parameters: <path> <filter> <number of days>"
    exit
fi

datadir=$1 #, as configured in your dataset
datafilter=$2
limitdaysold=$3

# Cut trailing slash
datadir=${datadir%/} 

pathfiltertablelookuptable="pathfiltertablelookup_v2_0_23"
export ADAGUC_PATH=/adaguc/adaguc-server-master/
export ADAGUC_TMP=/tmp

postgrescredentials="host=localhost port=5432 user=adaguc password=adaguc dbname=adaguc" # From adaguc-server-config.xml

# Get the right tablenames from the database, based on the directory.
tablenames=$(psql -t "${postgrescredentials}" -c "select tablename from ${pathfiltertablelookuptable} where path = E'P_${datadir}' and filter = E'F_${datafilter}' and dimension='time';")

echo $tablenames

for tablename in $tablenames;do
  # Get the list of files with more than $limitdaysold days old.
  timeolder=`date --date="${limitdaysold} days ago" +%Y-%m-%d`T00:00:00Z
  filelist=$(psql -t "${postgrescredentials}" -c "select path from ${tablename} where time < '${timeolder}' order by time asc;")
  for file in $filelist;do
    echo "$file" 
    #rm $file
    psql -t "${postgrescredentials}" -c "delete from ${tablename} where path = '${file}';" >/dev/null
  done
done
