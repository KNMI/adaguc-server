#!/bin/bash

# Author: maarten.plieger@knmi.nl
# 2020-04-23
# This script cleans the adaguc database for files which are older than specified nr of days

#Usage: bash adaguc-server-cleandb.sh -p <directory> -f <filter> -d <number of days> -q filetimedate -t <delete_db or delete_db_and_fs>
#Example: bash adaguc-server-cleandb.sh  -p /data/adaguc-autowms/EGOWS_radar -f ".*\.nc$" -d 7 -q filetimedate -t delete_db

# The script should only print the deleted files.

usage () {
    echo "This script cleans the database for files which are older than specified nr of days"
    echo "-p: is for the filepath as configured in your Layers FilePath value"
    echo "-f: is for the filefilter as configured in your Layers FilePath filter attribute. Can be set to * to ignore or left out"
    echo "-d: Specify how many days old the files need to be for removal"
    echo "-q: Querytype, currently filetimedate, this is the date inside the time variable of the NetCDF file"
    echo "-t: Deletetype, delete_db means it will be removed from the db only, delete_db_and_fs will also delete the files from disk"
    exit
}


while getopts "p:f:d:q:t:" o; do
    case "${o}" in
        p)
            FILEPATH=${OPTARG}
            ;;
        f)
            FILEFILTER=${OPTARG}
            ;;
        d)
            FILEDAYS=${OPTARG}
            ;;
        q)
            QUERYTYPE=${OPTARG}
            ;;
        t)
            DELETEMODE=${OPTARG}
            ;;
        h)
            usage
            ;;
        *)
            usage
            ;;
    esac
done

if [ ! -n "${FILEPATH}" ]; then
  echo "You did not supply a filepath with -p <filepath>"
  exit 1
fi

if [ ! -n "${FILEDAYS}" ]; then
  echo "You did not supply the days argument with -d <nr of days>"
  exit 1
fi

if [ ! -n "${QUERYTYPE}" ]; then
  echo "You did not supply the QUERYTYPE argument with -q filetimedate"
  exit 1
fi

if [ ! -n "${DELETEMODE}" ]; then
  echo "You did not supply the delete mode argument with -t <delete_db or delete_db_and_fs>"
  exit 1
fi

    
THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

. ${THISSCRIPTDIR}/adaguc-server-chkconfig.sh


datadir=${FILEPATH} #, as configured in your dataset
datafilter=${FILEFILTER:-""}
limitdaysold=${FILEDAYS}

# Cut trailing slash
datadir=${datadir%/} 

pathfiltertablelookuptable="pathfiltertablelookup_v2_0_23"

defaultpostgrescredentials="host=localhost port=5432 user=adaguc password=adaguc dbname=adaguc" # From adaguc-server-config.xml

postgrescredentials=${ADAGUC_DB:-${defaultpostgrescredentials}}


encodeddatafilter="and filter = E'F_${datafilter}'"

if [[ "${datafilter}" == "*" ]];then
encodeddatafilter=""
fi

if [[ "${datafilter}" == "" ]];then
encodeddatafilter=""
fi
#echo "select tablename from ${pathfiltertablelookuptable} where path = E'P_${datadir}' ${encodeddatafilter} and dimension!='time';"
# Get the right tablenames from the database, based on the directory.
tablenames=$(psql -t "${postgrescredentials}" -c "select tablename from ${pathfiltertablelookuptable} where path = E'P_${datadir}' ${encodeddatafilter} and dimension!='time';")
timetablename=$(psql -t "${postgrescredentials}" -c "select tablename from ${pathfiltertablelookuptable} where path = E'P_${datadir}' ${encodeddatafilter} and dimension='time';")

# Get the list of files with more than $limitdaysold days old.
timeolder=`date --date="${limitdaysold} days ago" +%Y-%m-%d`T00:00:00Z
filelist=$(psql -t "${postgrescredentials}" -c "select path from ${timetablename} where time < '${timeolder}' order by time asc;")


# Delete the files from the FS
if [ ${DELETEMODE} == "delete_db_and_fs" ]; then
  for f in $filelist; do
    rm -f $f
  done
fi

function join_by { local d=$1; shift; echo -n "$1"; shift; printf "%s" "${@/#/$d}"; }

echo {\"filelist\":[\"`join_by "\",\"" ${filelist}`\"]}

# First delete from all other dims
for tablename in $tablenames;do
  psql -t "${postgrescredentials}" -c "delete from ${tablename} where path in (select path from ${timetablename} where time < '${timeolder}' order by time asc);" >/dev/null
done

# Finally delete also from time dim
psql -t "${postgrescredentials}" -c "delete from ${timetablename} where path in (select path from ${timetablename} where time < '${timeolder}' order by time asc);" >/dev/null
