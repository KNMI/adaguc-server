cmake_minimum_required(VERSION 3.15)
project(adaguc-server VERSION 3.0.0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
set(CMAKE_CXX_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -msse2")


add_definitions(-DENABLE_CURL -DADAGUC_USE_GDAL -DADAGUC_USE_SQLITE -DADAGUC_USE_POSTGRESQL -DADAGUC_USE_WEBP)

add_subdirectory(hclasses)
add_subdirectory(CCDFDataModel)
add_subdirectory(adagucserverEC)

add_executable(adagucserver ./adagucserverEC/adagucserver.cpp)
add_executable(h5ncdump ./adagucserverEC/h5ncdump.cpp)
add_executable(aggregate_time ./adagucserverEC/aggregate_time.cpp)
add_executable(geojsondump ./adagucserverEC/geojsondump.cpp)

target_link_libraries(adagucserver adagucserverEC hclasses CCDFDataModel)
target_link_libraries(h5ncdump adagucserverEC hclasses CCDFDataModel)
target_link_libraries(aggregate_time adagucserverEC hclasses CCDFDataModel)
target_link_libraries(geojsondump adagucserverEC hclasses CCDFDataModel)

