find_package(LibXml2 REQUIRED)
find_package(NetCDF REQUIRED)
find_package(HDF5 REQUIRED)
find_package(UDUNITS2 REQUIRED)
find_package(PNG REQUIRED)

add_library(CCDFDataModel
    CCDFDataModel.h
    CCDFNetCDFIO.h
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
    CCDFCache.h
    CCache.h
    CCDFStore.h
    CCDFGeoJSONIO.h
    CCDFCSVReader.h
    CCDFDataModel.cpp
    CCDFNetCDFIO.cpp
    CTime.cpp
    CCDFHDF5IO.cpp
    CProj4ToCF.cpp
    CCDFVariable.cpp
    CCDFObject.cpp
    CCDFTypes.cpp
    CCDFCache.cpp
    CCache.cpp
    CCDFStore.cpp
    CCDFGeoJSONIO.cpp
    CCDFCSVReader.cpp
    utils.cpp
)

target_include_directories(CCDFDataModel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${LIBXML2_INCLUDE_DIR} ${HDF5_INCLUDE_DIR})
target_link_libraries(CCDFDataModel hclasses ${NetCDF_LIBRARIES} ${HDF5_LIBRARIES} ${UDUNITS2_LIBRARIES} ${NetCDF_LIBRARIES} ${HDF5_LIBRARIES} ${LIBXML2_LIBRARY} ${PNG_LIBRARY} )

# Build unit test executable
add_executable(testccdfdatamodel testccdfdatamodel.cpp)
target_link_libraries(testccdfdatamodel PRIVATE CCDFDataModel hclasses)

add_test(NAME testccdfdatamodel COMMAND "testccdfdatamodel" WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})