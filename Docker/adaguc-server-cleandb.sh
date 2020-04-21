#!/bin/bash

# Author: maarten.plieger@knmi.nl
# 2020-04-23
# This script cleans the adaguc database for files which are older than specified nr of days
# TODO: Should also clean tables depending on other dimensions

#Usage: bash adaguc-server-cleandb.sh  <directory> <filter> <number of days>
#Example: bash adaguc-server-cleandb.sh  /data/adaguc-autowms/EGOWS_radar ".*\.nc$" 7

# The script should only print the deleted files.

if [ "$#" -ne 3 ]; then
    echo "This script cleans the database for files which are older than specified nr of days"
    echo "Error: Expected three parameters: <path> <filter> <number of days>"
    exit
fi

THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

. ${THISSCRIPTDIR}/adaguc-server-chkconfig.sh


datadir=$1 #, as configured in your dataset
datafilter=$2
limitdaysold=$3

# Cut trailing slash
datadir=${datadir%/} 

pathfiltertablelookuptable="pathfiltertablelookup_v2_0_23"

defaultpostgrescredentials="host=localhost port=5432 user=adaguc password=adaguc dbname=adaguc" # From adaguc-server-config.xml

postgrescredentials=${ADAGUC_DB:-${defaultpostgrescredentials}}


# Get the right tablenames from the database, based on the directory.
tablenames=$(psql -t "${postgrescredentials}" -c "select tablename from ${pathfiltertablelookuptable} where path = E'P_${datadir}' and filter = E'F_${datafilter}' and dimension!='time';")
timetablename=$(psql -t "${postgrescredentials}" -c "select tablename from ${pathfiltertablelookuptable} where path = E'P_${datadir}' and filter = E'F_${datafilter}' and dimension='time';")

# Get the list of files with more than $limitdaysold days old.
timeolder=`date --date="${limitdaysold} days ago" +%Y-%m-%d`T00:00:00Z
filelist=$(psql -t "${postgrescredentials}" -c "select path from ${timetablename} where time < '${timeolder}' order by time asc;")

function join_by { local d=$1; shift; echo -n "$1"; shift; printf "%s" "${@/#/$d}"; }

echo {\"filelist\":[\"`join_by "\",\"" ${filelist}`\"]}

# First delete from all other dims
for tablename in $tablenames;do
  psql -t "${postgrescredentials}" -c "delete from ${tablename} where path in (select path from ${timetablename} where time < '${timeolder}' order by time asc);" >/dev/null
done

# Finally delete also from tim dim
psql -t "${postgrescredentials}" -c "delete from ${timetablename} where path in (select path from ${timetablename} where time < '${timeolder}' order by time asc);" >/dev/null
