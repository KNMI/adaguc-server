#!/bin/bash

if [[ $1 == dev ]]; then
  export ADAGUCCOMPILERSETTINGS="-Wall -DMEMLEAKCHECK -std=c++11 -g -pipe"
  ulimit -c unlimited
fi
#For developing, use:
#export ADAGUCCOMPILERSETTINGS="-Wall -DMEMLEAKCHECK -std=c++11 -g"

#For time measurement of components use
#export ADAGUCCOMPILERSETTINGS="-Wall -DMEMLEAKCHECK -DMEASURETIME -std=c++11 -g"

#For operational, use:
#export ADAGUCCOMPILERSETTINGS="-msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 -std=c++11"

DEFAULTCOMPILERSETTINGS="-msse -msse2 -msse3 -mssse3 -mfpmath=sse -O2 -std=c++11"
DEFAULTADAGUCCOMPONENTS="-DENABLE_CURL -DADAGUC_USE_GDAL -DADAGUC_USE_SQLITE -DADAGUC_USE_POSTGRESQL -DADAGUC_USE_WEBP"

CURRENTDIR=`pwd`

if [ -z "${ADAGUCCOMPILERSETTINGS}" ]; then 
  export BUILDER_ADAGUCCOMPILERSETTINGS=$DEFAULTCOMPILERSETTINGS
   echo "BUILDER_ADAGUCCOMPILERSETTINGS is set to default 'BUILDER_ADAGUCCOMPILERSETTINGS'";
else 
  echo "Note: ADAGUCCOMPILERSETTINGS is set to '$ADAGUCCOMPILERSETTINGS'"; 
  export BUILDER_ADAGUCCOMPILERSETTINGS=$ADAGUCCOMPILERSETTINGS
fi

#Minimal instalation can be compiled by settign:
#export ADAGUCCOMPONENTS="-DADAGUC_USE_SQLITE"

if [ -z "${ADAGUCCOMPONENTS}" ]; then 
  export BUILDER_ADAGUCCOMPONENTS=$DEFAULTADAGUCCOMPONENTS
  echo "BUILDER_ADAGUCCOMPONENTS is set to default '$BUILDER_ADAGUCCOMPONENTS'";
else 
  echo "ADAGUCCOMPONENTS is set to '$ADAGUCCOMPONENTS'";
  export BUILDER_ADAGUCCOMPONENTS=$ADAGUCCOMPONENTS
fi  




function quit {
  echo "Make sure include directories are indicated with CPPFLAGS and library directories with LDFLAGS"
  echo "  For example:"
  echo "  export CPPFLAGS=-I/home/user/software/install/include -I/home/user/othersoftware/install/include"
  echo "  export LDFLAGS=-L/home/user/software/install/lib/ -L/home/user/othersoftware/install/lib/" 
  echo ""
  exit 1;
}

function clean {
  cd $CURRENTDIR/hclasses
  rm -f *.o
  rm -f *.a

  cd $CURRENTDIR/CCDFDataModel
  rm -f *.o
  rm -f *.a

  cd $CURRENTDIR/adagucserverEC
  rm -f *.o
  rm -f adagucserver
  rm -f h5ncdump
  rm -f aggregate_time
  rm -f geojsondump

  test -d $CURRENTDIR/bin || mkdir $CURRENTDIR/bin/
  rm -f $CURRENTDIR/bin/adagucserver
  rm -f $CURRENTDIR/bin/h5ncdump
  rm -f $CURRENTDIR/bin/aggregate_time
  rm -f $CURRENTDIR/bin/geojsondump
}

function build {
  clean
  cd $CURRENTDIR/hclasses
  make -j4

  if [ -f hclasses.a ]
    then
    echo "[OK] hclasses have succesfully been compiled."
    else
      echo "[FAILED] hclasses compilation failed"
      quit;
    fi

  cd $CURRENTDIR/CCDFDataModel
  make -j4


  if [ -f CCDFDataModel.a ]
    then
    echo "[OK] CCDFDataModel has been succesfully compiled."
    else
      echo "[FAILED] CCDFDataModel compilation failed"
      quit;
    fi
    
  cd $CURRENTDIR/adagucserverEC
  make -j4


  if [ -f adagucserver ]
    then
    echo "[OK] ADAGUC has been succesfully compiled."
    else
      echo "[FAILED] ADAGUC compilation failed"
      quit;
  fi


  test -d $CURRENTDIR/bin || mkdir $CURRENTDIR/bin/
  cp adagucserver $CURRENTDIR/bin/
  cp h5ncdump $CURRENTDIR/bin/
  cp aggregate_time $CURRENTDIR/bin/
  cp geojsondump $CURRENTDIR/bin/
  echo "[OK] Everything is installed in the ./bin directory"
}

if [ "$*" = "--clean" ]; then
  clean
  else
  build
fi
