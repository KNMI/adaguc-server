#!/bin/bash

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
}

function build {

  clean  
  cd $CURRENTDIR/bin
  
  cmake .. &&  cmake  --build . --parallel -v 

  if [ -f adagucserver ]
    then
    echo "[OK] ADAGUC has been succesfully compiled."
    else
      echo "[FAILED] ADAGUC compilation failed"
      quit;
  fi
 
  echo "[OK] Everything is installed in the ./bin directory"
}

if [ "$*" = "--clean" ]; then
  clean
  else
  build
fi
