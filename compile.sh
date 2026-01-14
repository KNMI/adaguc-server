#!/bin/bash

# Fail script if any step fails
set -euo pipefail

CURRENTDIR=`pwd`

function quit {
  exit 1;
}

function clean {
  rm -rf $CURRENTDIR/bin
  mkdir -p $CURRENTDIR/bin

  cd $CURRENTDIR/hclasses
  rm -f *.o
  rm -f *.a
  rm -rf CMakeFiles CMakeCache.txt

  cd $CURRENTDIR/CCDFDataModel
  rm -f *.o
  rm -f *.a
  rm -rf CMakeFiles CMakeCache.txt

  cd $CURRENTDIR/adagucserverEC
  rm -f *.o
  rm -f adagucserver
  rm -f h5ncdump
  rm -f aggregate_time
  rm -f geojsondump
  rm -rf CMakeFiles CMakeCache.txt

  test -d $CURRENTDIR/bin || mkdir $CURRENTDIR/bin/
  rm -f $CURRENTDIR/bin/adagucserver
  rm -f $CURRENTDIR/bin/h5ncdump
  rm -f $CURRENTDIR/bin/aggregate_time
  rm -f $CURRENTDIR/bin/geojsondump

  rm -f $CURRENTDIR/CMakeCache.txt $CURRENTDIR/CMakeFiles
}

function build {

  # clean
  cd $CURRENTDIR/bin
  cmake .. &&  cmake  --build . --parallel 4

  if [ -f adagucserver ]
    then
    echo "[OK] ADAGUC has been successfully compiled."
    else
      echo "[FAILED] ADAGUC compilation failed"
      quit;
  fi

  echo "[OK] Everything is installed in the ./bin directory"

  echo "Testing..."
  ctest --verbose
  retVal=$?
  if [ $retVal -ne 0 ]; then
    echo "[FAILED] Some of the tests failed"
  else
    echo "[OK] Tests succeeded"
  fi
  exit $retVal

}

if [ "$*" = "--clean" ]; then
  clean
  else
  build
fi
