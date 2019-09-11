#!/bin/bash

# TODO: https://github.com/KNMI/adaguc-server/issues/71

THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

. ${THISSCRIPTDIR}/adaguc-server-chkconfig.sh

ADAGUC_DATASET='*.xml'

while getopts "d:f:" o; do
    case "${o}" in
        d)
            ADAGUC_DATASET=${OPTARG}
            ;;
        f)
            ADAGUC_DATAFILE=${OPTARG}
            ;;
        h)
            usage
            ;;
        *)
            usage
            ;;
    esac
done

if [ -n "${ADAGUC_DATAFILE}" ]; then
  echo "Adding ${ADAGUC_DATAFILE} to the available datasets."
else
  echo "You did not supply a file to add!"
  exit 1
fi

# Update all datasets
for configfile in /data/adaguc-datasets/${ADAGUC_DATASET} ;do
  filename=/data/adaguc-datasets/"${configfile##*/}" 
  filebasename=${filename##*/}
  if [[ "${ADAGUC_DATASET_MASK}" && `echo ${filebasename} | grep -E ${ADAGUC_DATASET_MASK}` != ${filebasename} ]] ; then
      if [[ "${ADAGUC_DATASET_MASK}" ]] ; then
          echo "${filebasename} doesn't match ${ADAGUC_DATASET_MASK}"
      fi
      continue
  fi
  echo ""
  echo "Starting update for dataset ${filename} and datafile ${ADAGUC_DATAFILE}" 
  ${ADAGUC_PATH}/bin/adagucserver --updatedb --config ${ADAGUC_CONFIG},${filename} --path ${ADAGUC_DATAFILE}
  OUT=$?
  if [ -d /servicehealth ]; then
    echo "$OUT" > /servicehealth/${filebasename%.*}
  fi
done

