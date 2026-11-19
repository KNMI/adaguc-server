find_package(LibXml2 REQUIRED)
find_package(NetCDF REQUIRED)
find_package(HDF5 REQUIRED)
find_package(UDUNITS2 REQUIRED)
find_package(PNG REQUIRED)
find_package(PROJ REQUIRED)
find_package(CURL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

# Optional compressors for Zarr chunks
find_path(ZSTD_INCLUDE_DIR zstd.h)
find_library(ZSTD_LIBRARY zstd)
find_path(BLOSC_INCLUDE_DIR blosc.h)
find_library(BLOSC_LIBRARY blosc)

add_library(CCDFDataModel
    traceTimings/traceTimings.h
    traceTimings/traceTimings.cpp
    traceTimings/traceTimingsTests.cpp
    CCDFDataModel.h
    CCDFNetCDFIO.h
    CCDFAttribute.h
    CCDFAttribute.cpp
    CCDFDimension.h
    CCDFDimension.cpp
    CCDFReader.h
    CCDFReader.cpp
    CDFCopyData.h
    CDFCopyData.cpp
    CReadPNG.h
    CReadPNG.cpp
    CCDFPNGIO.h
    CCDFPNGIO.cpp
    CTime.h
    CCDFHDF5IO.h
    CProj4ToCF.h
    CCDFVariable.h
    CCDFObject.h
    CCDFTypes.h
    CCDFStore.h
    CCDFGeoJSONIO.h
    CCDFCSVReader.h
    ProjCache.h
    CCDFDataModel.cpp
    CCDFNetCDFIO.cpp
    CTime.cpp
    CCDFHDF5IO.cpp
    CCDFHDF5IO_ODIM.cpp
    CProj4ToCF.cpp
    CCDFVariable.cpp
    CCDFObject.cpp
    CCDFTypes.cpp
    CCDFStore.cpp
    CCDFGeoJSONIO.cpp
    CCDFCSVReader.cpp
    ProjCache.cpp
    utils.cpp
    cdfVariableCache.h
    cdfVariableCache.cpp
    CCDFZarrIO.h
    CCDFZarrIO.cpp
    zarr/ZarrStore.h
    zarr/ZarrStore.cpp
    zarr/ZarrCodecs.h
    zarr/ZarrCodecs.cpp
    zarr/ZarrWorkerPool.h
)

target_include_directories(CCDFDataModel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${LIBXML2_INCLUDE_DIR} ${HDF5_INCLUDE_DIR} ${PROJ_INCLUDE_DIR} ${CURL_INCLUDE_DIRS})
target_link_libraries(CCDFDataModel hclasses ${NetCDF_LIBRARIES} ${HDF5_LIBRARIES} ${UDUNITS2_LIBRARIES} ${NetCDF_LIBRARIES} ${HDF5_LIBRARIES} ${LIBXML2_LIBRARY} ${PNG_LIBRARY} ${PROJ_LIBRARY} ${CURL_LIBRARIES} ZLIB::ZLIB Threads::Threads)

if(ZSTD_INCLUDE_DIR AND ZSTD_LIBRARY)
  MESSAGE(STATUS "Zarr: zstd compressor enabled (${ZSTD_LIBRARY})")
  target_compile_definitions(CCDFDataModel PRIVATE ADAGUC_HAVE_ZSTD)
  target_include_directories(CCDFDataModel PRIVATE ${ZSTD_INCLUDE_DIR})
  target_link_libraries(CCDFDataModel ${ZSTD_LIBRARY})
else()
  MESSAGE(STATUS "Zarr: zstd not found, Zarr chunks compressed with zstd cannot be read")
endif()
if(BLOSC_INCLUDE_DIR AND BLOSC_LIBRARY)
  MESSAGE(STATUS "Zarr: blosc compressor enabled (${BLOSC_LIBRARY})")
  target_compile_definitions(CCDFDataModel PRIVATE ADAGUC_HAVE_BLOSC)
  target_include_directories(CCDFDataModel PRIVATE ${BLOSC_INCLUDE_DIR})
  target_link_libraries(CCDFDataModel ${BLOSC_LIBRARY})
else()
  MESSAGE(STATUS "Zarr: blosc not found, Zarr chunks compressed with blosc cannot be read")
endif()

# Build unit test executable
add_executable(testccdfdatamodel testccdfdatamodel.cpp)
target_link_libraries(testccdfdatamodel PRIVATE CCDFDataModel hclasses)

add_test(NAME testccdfdatamodel COMMAND "testccdfdatamodel" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
add_test(traceTimingsTests traceTimingsTests)
add_executable(traceTimingsTests traceTimings/traceTimingsTests.cpp)
target_link_libraries(traceTimingsTests PRIVATE adagucserverEC hclasses CppUnitLite)
