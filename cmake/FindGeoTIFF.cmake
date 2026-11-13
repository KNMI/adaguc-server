#[=======================================================================[.rst:
FindGeoTIFF
-----------

Find the libgeotiff headers and library. Debian/Ubuntu (libgeotiff-dev) install the headers in include/geotiff/
and only provide pkg-config, Homebrew installs them in include/.

Result Variables
^^^^^^^^^^^^^^^^

``GeoTIFF_FOUND``
  true if libgeotiff is available.
``GeoTIFF_INCLUDE_DIRS``
  the directory with geotiffio.h, include the headers as <geotiffio.h>.
``GeoTIFF_LIBRARIES``
  the libraries to link against.

#]=======================================================================]

find_package(PkgConfig QUIET)
if (PkgConfig_FOUND)
  pkg_check_modules(PC_GEOTIFF QUIET libgeotiff)
endif ()

find_path(GeoTIFF_INCLUDE_DIR
  NAMES geotiffio.h
  HINTS ${PC_GEOTIFF_INCLUDEDIR} ${PC_GEOTIFF_INCLUDE_DIRS}
  PATH_SUFFIXES geotiff libgeotiff
)

find_library(GeoTIFF_LIBRARY
  NAMES geotiff libgeotiff
  HINTS ${PC_GEOTIFF_LIBDIR} ${PC_GEOTIFF_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GeoTIFF
  REQUIRED_VARS GeoTIFF_LIBRARY GeoTIFF_INCLUDE_DIR
  VERSION_VAR PC_GEOTIFF_VERSION
)

mark_as_advanced(GeoTIFF_INCLUDE_DIR GeoTIFF_LIBRARY)

if (GeoTIFF_FOUND)
  set(GeoTIFF_INCLUDE_DIRS ${GeoTIFF_INCLUDE_DIR})
  set(GeoTIFF_LIBRARIES ${GeoTIFF_LIBRARY})
endif ()
