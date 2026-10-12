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

  rm -rf $CURRENTDIR/CMakeCache.txt $CURRENTDIR/CMakeFiles

  cd $CURRENTDIR
  cmake --fresh -S ${CURRENTDIR} -B ${CURRENTDIR}/bin -G "Unix Makefiles"

  echo "Cleaned. You can now run the script again to build the project."

}

function build {

  cmake  --build . --parallel 4

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
  return $retVal

}

# --measuretime can be combined with the other build options, e.g. ./compile.sh --debug --measuretime
MEASURETIME=OFF
ARGS=()
for arg in "$@"; do
  if [ "$arg" = "--measuretime" ]; then
    MEASURETIME=ON
  else
    ARGS+=("$arg")
  fi
done
set -- "${ARGS[@]+"${ARGS[@]}"}"

mkdir -p $CURRENTDIR/bin
cd $CURRENTDIR/bin
echo "MEASURETIME timing output: ${MEASURETIME}"
if [ "$*" = "--clean" ]; then
  echo "Cleaning"
  clean
elif [ "$*" = "--debug" ]; then
  echo "Making Debug build"
  cmake -DCMAKE_BUILD_TYPE=Debug -DADAGUC_MEASURETIME=${MEASURETIME} -S .. -B .
  build; res=$?; exit $res;
elif [ "$*" = "--sanitize" ]; then
  echo "Making sanitize build"
  cmake -DCMAKE_BUILD_TYPE=Sanitize  -S .. -B .
  build; res=$?; exit $res;
elif [ "$*" = "--profile" ]; then
  echo "Making profile build"
  cmake -DCMAKE_BUILD_TYPE=Profile -S .. -B .
  build 
elif [ "$*" = "" ]; then
  echo "Making Release build!"
  cmake -DCMAKE_BUILD_TYPE=Release -DADAGUC_MEASURETIME=${MEASURETIME} -S .. -B .
  build; res=$?; exit $res;
else
  echo "Unrecognized build type"
  exit 1
fi
