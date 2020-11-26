find_package(LibXml2 REQUIRED)
find_package(HDF5 REQUIRED)

add_library(CCDFDataModel
    CCDFDataModel.h
    CCDFNetCDFIO.h
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
)

target_include_directories(CCDFDataModel PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${LIBXML2_INCLUDE_DIR} ${HDF5_INCLUDE_DIR})
target_link_libraries(CCDFDataModel hclasses ${NetCDF_LIBRARIES} ${HDF5_LIBRARIES})

# add_executable(cmake-adaguc test.cpp)

# target_link_libraries(cmake-adaguc PRIVATE hclasses)