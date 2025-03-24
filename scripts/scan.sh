#!/bin/bash

THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
. ${THISSCRIPTDIR}/adaguc-server-chkconfig.sh

ADAGUC_DATASET=''
VERBOSE='--verboseoff'
RESCAN=''
RECREATETABLES=''
NOCLEAN=''

usage () {
    echo "This script uses adaugc-server to scan files and datasets. It ingests indexing information into the database"
    echo "  [-f] <file to add> [-d] <datasetname>             [Scan a single file for specified dataset]"
    echo "  [-f] <file to add>                                [Scan a single file, dataset is automatically detected]"
    echo "  [-d] <datasetname>                                [Scan a dataset, all layers within dataset will be checked]"
    echo "  [-d] \"*\"                                          [Scan all available datasets]"
    echo "  [-l]                                              [List all datasets]"
    echo "  [-v]                                              [Verbose logging]"
    echo "  [-r]                                              [Rescan by ignoring file modification date of files]"
    echo "  [-t]                                              [Drops and recreates database tables for mathing file or dataset]"
    echo "  [-k]                                              [Keep index information in database, disable cleaning]"
    echo "  [-e]                                              [Inspect environment]"
    exit
}


while getopts "d:f:vtkrleh" o; do
    case "${o}" in
        d)
            ADAGUC_DATASET=${OPTARG}
            ;;
        f)
            ADAGUC_DATAFILE=${OPTARG}
            ;;
        v)
            VERBOSE=''
            ;;
        r)
            RESCAN='--rescan'
            ;;
        t)
            RECREATETABLES='--recreate'
            ;;
        k)
            NOCLEAN='--noclean'
            ;;
        l)
            for i in ${ADAGUC_DATASET_DIR}/*xml;do
              echo "${i##*/}"
            done
            ;;
        e)
            env | grep "ADAGUC"
            ;;
        h)
            usage
            ;;
        *)
            usage
            ;;
    esac
done


if [ $OPTIND -eq 1 ]; then usage; fi



### Scan a file for a specified dataset ###
if [[ -n "${ADAGUC_DATASET}" &&  -n "${ADAGUC_DATAFILE}" ]]; then
  STATUSCODE=0
  echo "Adding file [${ADAGUC_DATAFILE}] to dataset [${ADAGUC_DATASET}]:"
  command="${ADAGUC_PATH}/bin/adagucserver --updatedb ${NOCLEAN} ${VERBOSE} ${RESCAN} ${RECREATETABLES} --config ${ADAGUC_CONFIG},${ADAGUC_DATASET} --path ${ADAGUC_DATAFILE}"
  echo $command
  stdbuf -i0 -o0 -e0 $command
  OUT=$?
  if [ ${OUT} -ne 0 ]; then
    STATUSCODE=${OUT}
  fi
  exit ${STATUSCODE}
fi

### Scan a file ###
if [[ -n "${ADAGUC_DATAFILE}" ]]; then

  basediroffile=${ADAGUC_DATAFILE%/*}
  basenameoffile="${ADAGUC_DATAFILE##*/}"
  
  STATUSCODE=0
  echo "Adding file [${ADAGUC_DATAFILE}] to dataset [${alldatasets}]:"
  command="${ADAGUC_PATH}/bin/adagucserver --updatedb --autofinddataset ${NOCLEAN} ${VERBOSE} ${RESCAN} ${RECREATETABLES} --config ${ADAGUC_CONFIG} --path ${ADAGUC_DATAFILE}"
  echo $command
  stdbuf -i0 -o0 -e0 $command
  OUT=$?
  if [ ${OUT} -ne 0 ]; then
    STATUSCODE=1
  fi
  exit ${STATUSCODE} 
fi

# Scan a dataset
if [[ -n "${ADAGUC_DATASET}" ]] && [ "${ADAGUC_DATASET}" != "*" ]; then
  STATUSCODE=0
  echo "Scanning full dataset [${ADAGUC_DATASET}]:"
  command="${ADAGUC_PATH}/bin/adagucserver --updatedb ${NOCLEAN} ${VERBOSE} ${RESCAN} ${RECREATETABLES} --config ${ADAGUC_CONFIG},${ADAGUC_DATASET}"
  echo $command
  stdbuf -i0 -o0 -e0 $command
  OUT=$?
  echo adagucserver --updatedb exited with status code ${OUT}
  if [ ${OUT} -ne 0 ]; then
    STATUSCODE=1
  fi
  exit ${STATUSCODE} 
fi

# Scan all datasets
if [[ -n "${ADAGUC_DATASET}" ]] && [ "${ADAGUC_DATASET}" == "*" ]; then
  echo "Scanning all datasets"
  STATUSCODE=0
  for configfile in ${ADAGUC_DATASET_DIR}/*xml ;do
    echo "Scanning full dataset [${configfile}]:"
    command="${ADAGUC_PATH}/bin/adagucserver --updatedb ${NOCLEAN} ${VERBOSE} ${RESCAN} ${RECREATETABLES} --config ${ADAGUC_CONFIG},${configfile}"
    echo $command
    $command
    OUT=$?
    if [ ${OUT} -ne 0 ]; then
      STATUSCODE=${OUT}
    fi
  done
  exit ${STATUSCODE} 
fi

