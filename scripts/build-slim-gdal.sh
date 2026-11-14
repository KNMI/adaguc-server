#!/bin/bash
# Builds a slim GDAL from source with only the drivers adaguc-server needs, and installs it in a private prefix.
# Use it with: bash compile.sh --gdal-prefix <prefix>   (see doc/Developing.md)
#
# Usage: scripts/build-slim-gdal.sh [prefix]
#   prefix   install directory, default $GDAL_PREFIX or $HOME/gdal-slim
# Environment:
#   GDAL_VERSION       GDAL version to build, default 3.12.2 (best: the version of your system GDAL)
#   GDAL_BUILD_DIR     work directory for download and build, default /tmp/gdal-slim-build
#   GDAL_DEPS_PREFIX   where PROJ, netCDF and HDF5 are found (CMAKE_PREFIX_PATH). Default: the Homebrew
#                      prefix (/opt/homebrew) on macOS, the system locations elsewhere.
#
# The drivers come from adaguc's WCS output formats (CGDALDataWriter.cpp): GTiff, AAIGrid and "NetCDF" (the
# configurable alternative to adaguc's own ADAGUCNetCDF writer), plus MEM, which is always built. All other
# optional drivers and external libraries are switched off, so loading libgdal is cheap. PROJ and netCDF are
# the system ones, so adaguc and GDAL share a single copy of those libraries in the process. libtiff and
# libgeotiff are the copies bundled with GDAL (GDAL renames their symbols, so there are no clashes).

set -euo pipefail

GDAL_VERSION=${GDAL_VERSION:-3.12.2}
PREFIX=${1:-${GDAL_PREFIX:-$HOME/gdal-slim}}
BUILD_DIR=${GDAL_BUILD_DIR:-/tmp/gdal-slim-build}
DEPS_PREFIX=${GDAL_DEPS_PREFIX:-}
if [ -z "$DEPS_PREFIX" ] && [ "$(uname)" = "Darwin" ]; then
  DEPS_PREFIX=$(brew --prefix 2>/dev/null || echo /opt/homebrew)
fi

# The script changes directory and the prefix is stored as an rpath, so the prefix must be absolute.
case "$PREFIX" in
  /*) ;;
  *) PREFIX="$PWD/$PREFIX" ;;
esac

# Only install into an empty directory or into a previous slim install (marked by .slim-gdal). This keeps the
# script from overwriting other software, e.g. Homebrew's own GDAL when the prefix is /opt/homebrew or /usr/local.
if [ -n "$(ls -A "$PREFIX" 2>/dev/null)" ] && [ ! -f "$PREFIX/.slim-gdal" ]; then
  echo "Refusing to install into $PREFIX: it is not empty and not a previous slim GDAL install." >&2
  echo "Use a dedicated directory, for example $HOME/gdal-slim" >&2
  exit 1
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
TARBALL=gdal-$GDAL_VERSION.tar.gz
if [ ! -f "$TARBALL" ]; then
  # Download to a temporary name, so an interrupted download is not mistaken for a finished one on the next run
  curl -fL -o "$TARBALL.part" "https://github.com/OSGeo/gdal/releases/download/v$GDAL_VERSION/gdal-$GDAL_VERSION.tar.gz"
  mv "$TARBALL.part" "$TARBALL"
fi
rm -rf "gdal-$GDAL_VERSION" build
tar -xf "$TARBALL"

# Install name / rpath: CMake gives the library the install name @rpath/libgdal.N.dylib on macOS (soname
# libgdal.so.N on Linux). adagucserver gets the rpath <prefix>/lib from adaguc's own CMake build
# (compile.sh --gdal-prefix), so no DYLD_LIBRARY_PATH is needed. CMAKE_INSTALL_RPATH lets libgdal itself
# find the libraries in the same prefix. GDAL_USE_EXTERNAL_LIBS=OFF keeps GDAL from picking up the many
# optional Homebrew libraries; PROJ is mandatory and netCDF is switched on explicitly.
cmake -S "gdal-$GDAL_VERSION" -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PREFIX" \
  ${DEPS_PREFIX:+-DCMAKE_PREFIX_PATH="$DEPS_PREFIX"} \
  -DCMAKE_INSTALL_RPATH="$PREFIX/lib" \
  -DGDAL_BUILD_OPTIONAL_DRIVERS=OFF \
  -DOGR_BUILD_OPTIONAL_DRIVERS=OFF \
  -DGDAL_USE_EXTERNAL_LIBS=OFF \
  -DGDAL_USE_TIFF_INTERNAL=ON \
  -DGDAL_USE_GEOTIFF_INTERNAL=ON \
  -DGDAL_USE_NETCDF=ON \
  -DGDAL_ENABLE_DRIVER_GTIFF=ON \
  -DGDAL_ENABLE_DRIVER_AAIGRID=ON \
  -DGDAL_ENABLE_DRIVER_NETCDF=ON \
  -DBUILD_APPS=OFF \
  -DBUILD_PYTHON_BINDINGS=OFF \
  -DBUILD_TESTING=OFF

cmake --build build --parallel 4
cmake --install build
touch "$PREFIX/.slim-gdal"

echo "[OK] Slim GDAL $GDAL_VERSION installed in $PREFIX"
echo "Now build adaguc with: bash compile.sh --gdal-prefix $PREFIX"
