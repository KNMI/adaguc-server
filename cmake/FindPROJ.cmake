# Based on https://github.com/MapServer/MapServer/blob/main/cmake/FindProj.cmake

# Find Proj
#
# If it's found it sets PROJ_FOUND to TRUE
# and following variables are set:
#    PROJ_INCLUDE_DIR
#    PROJ_LIBRARY
#    PROJ_VERSION_MAJOR


FIND_PATH(PROJ_INCLUDE_DIR NAMES proj.h proj_api.h)

FIND_LIBRARY(PROJ_LIBRARY NAMES proj proj_i)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PROJ DEFAULT_MSG PROJ_LIBRARY PROJ_INCLUDE_DIR)
mark_as_advanced(PROJ_LIBRARY PROJ_INCLUDE_DIR)


IF (PROJ_INCLUDE_DIR AND PROJ_LIBRARY)
	SET(PROJ_FOUND TRUE)
ENDIF (PROJ_INCLUDE_DIR AND PROJ_LIBRARY)

IF (PROJ_FOUND)
	IF (EXISTS ${PROJ_INCLUDE_DIR}/proj.h)
		FILE(READ ${PROJ_INCLUDE_DIR}/proj.h proj_version)
		STRING(REGEX REPLACE "^.*PROJ_VERSION_MAJOR +([0-9]+).*$" "\\1" PROJ_VERSION_MAJOR "${proj_version}")
		STRING(REGEX REPLACE "^.*PROJ_VERSION_MINOR +([0-9]+).*$" "\\1" PROJ_VERSION_MINOR "${proj_version}")
		STRING(REGEX REPLACE "^.*PROJ_VERSION_PATCH +([0-9]+).*$" "\\1" PROJ_VERSION_PATCH "${proj_version}")

		MESSAGE(STATUS "Found Proj ${PROJ_VERSION_MAJOR}.${PROJ_VERSION_MINOR}")
		MESSAGE(STATUS "Include dir ${PROJ_INCLUDE_DIR} ${PROJ_LIBRARY}")

		IF (PROJ_VERSION_MAJOR LESS 6)
			MESSAGE (FATAL_ERROR "Adaguc requires PROJ API 6 or later.")
		ENDIF()

		ADD_DEFINITIONS(-DPROJ_VERSION_MAJOR=${PROJ_VERSION_MAJOR})
	ELSE()
		MESSAGE(STATUS "Found Proj 4.x")
		MESSAGE (FATAL_ERROR "Adaguc requires PROJ API 6 or later.")
	ENDIF()
ENDIF (PROJ_FOUND)
