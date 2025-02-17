#!/bin/bash
shopt -s nullglob
if [[ $1 ]]; then
  . ${ADAGUC_PATH}/scripts/scan.sh -d $1
else
. ${ADAGUC_PATH}/scripts/scan.sh -d "*"
fi
