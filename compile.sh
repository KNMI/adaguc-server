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

# --measuretime and --gdal-prefix <dir> can be combined with the other build options, e.g. ./compile.sh --debug --measuretime
# --gdal-prefix uses a GDAL installed in <dir> (see scripts/build-slim-gdal.sh) instead of the system GDAL.
MEASURETIME=OFF
GDAL_CMAKE_ARGS=()
ARGS=()
while [ $# -gt 0 ]; do
  case "$1" in
    --measuretime)
      MEASURETIME=ON
      ;;
    --gdal-prefix)
      if [ $# -lt 2 ]; then echo "--gdal-prefix needs a directory"; exit 1; fi
      # First in CMAKE_PREFIX_PATH, so this GDAL wins over the system one
      GDAL_CMAKE_ARGS=(-DCMAKE_PREFIX_PATH="$2")
      shift
      ;;
    *)
      ARGS+=("$1")
      ;;
  esac
  shift
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
  cmake ${GDAL_CMAKE_ARGS[@]+"${GDAL_CMAKE_ARGS[@]}"} -DCMAKE_BUILD_TYPE=Debug -DADAGUC_MEASURETIME=${MEASURETIME} -S .. -B .
  build; res=$?; exit $res;
elif [ "$*" = "--sanitize" ]; then
  echo "Making sanitize build"
  cmake ${GDAL_CMAKE_ARGS[@]+"${GDAL_CMAKE_ARGS[@]}"} -DCMAKE_BUILD_TYPE=Sanitize  -S .. -B .
  build; res=$?; exit $res;
elif [ "$*" = "--profile" ]; then
  echo "Making profile build"
  cmake ${GDAL_CMAKE_ARGS[@]+"${GDAL_CMAKE_ARGS[@]}"} -DCMAKE_BUILD_TYPE=Profile -S .. -B .
  build 
elif [ "$*" = "" ]; then
  echo "Making Release build!"
  cmake ${GDAL_CMAKE_ARGS[@]+"${GDAL_CMAKE_ARGS[@]}"} -DCMAKE_BUILD_TYPE=Release -DADAGUC_MEASURETIME=${MEASURETIME} -S .. -B .
  build; res=$?; exit $res;
else
  echo "Unrecognized build type"
  exit 1
fi
