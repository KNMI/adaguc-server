#!/bin/bash

# nullglob is a Bash shell option which modifies [[glob]] expansion such
# that patterns that match no files expand to zero arguments, rather than to themselves.
shopt -s nullglob

THISSCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

. ${THISSCRIPTDIR}/adaguc-server-chkconfig.sh

# Unbufferd logging for realtime output
export ADAGUC_ENABLELOGBUFFER=FALSE
# Do export ADAGUC_VERBOSE="--verboseon" to enable verbose logging
ADAGUC_VERBOSE="${ADAGUC_VERBOSE:=--verboseoff}" 

if [[ $1 ]]; then
  . ${THISSCRIPTDIR}/scan.sh -d $1
else
. ${THISSCRIPTDIR}/scan.sh -d "*"
fi
