cmake_minimum_required(VERSION 3.15)
project(adaguc-server VERSION 3.0.0)
enable_testing()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

MESSAGE(STATUS "Processor: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
IF (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "arm64")
  set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -Wall -Wextra")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -mcpu=native")
ELSE()
  set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -Wall -Wextra -g3")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -msse -msse2 -msse3 -mssse3 -mfpmath=sse")
ENDIF()
MESSAGE(STATUS "CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
MESSAGE(STATUS "CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")

# The additional / is important to remove the last character from the path.
# Note that it does not matter if the OS uses / or \, because we are only
# saving the path size.
string(LENGTH "${CMAKE_SOURCE_DIR}/" SOURCE_PATH_SIZE)
add_definitions("-DSOURCE_PATH_SIZE=${SOURCE_PATH_SIZE}")



add_definitions(-DENABLE_CURL -DADAGUC_USE_GDAL -DADAGUC_USE_SQLITE -DADAGUC_USE_POSTGRESQL -DADAGUC_USE_WEBP)



add_subdirectory(hclasses)
add_subdirectory(CCDFDataModel)
add_subdirectory(adagucserverEC)

add_executable(adagucserver ./adagucserverEC/adagucserver.cpp)
add_executable(h5ncdump ./adagucserverEC/h5ncdump.cpp)
add_executable(aggregate_time ./adagucserverEC/aggregate_time.cpp)
add_executable(geojsondump ./adagucserverEC/geojsondump.cpp)

add_test(testhclasses hclasses/testhclasses)
add_test(testadagucserver adagucserverEC/testadagucserver)

target_link_libraries(adagucserver adagucserverEC hclasses CCDFDataModel)
target_link_libraries(h5ncdump adagucserverEC hclasses CCDFDataModel)
target_link_libraries(aggregate_time adagucserverEC hclasses CCDFDataModel)
target_link_libraries(geojsondump adagucserverEC hclasses CCDFDataModel)

