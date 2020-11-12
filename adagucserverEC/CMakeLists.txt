find_package(LibXml2 REQUIRED)
find_package(Cairo REQUIRED) 
find_package(Freetype REQUIRED)
find_package(NetCDF REQUIRED)
find_package(Threads REQUIRED)
find_package(PNG REQUIRED)
find_package(PROJ4 REQUIRED)
find_package(UDUNITS2 REQUIRED)
find_package(GD REQUIRED)
find_package(CURL)
find_package(HDF5 REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(GDAL REQUIRED)
find_package(SQLite3 REQUIRED) 
find_package(WebP REQUIRED) 

add_library(
    adagucserverEC
    CReadPNG.h
    CCDFPNGIO.h
    CDataReader.h
    COGCDims.h
    CImageWarper.h
    CGeoParams.h
    CCairoPlotter.h
    CDrawImage.h
    CServerError.h
    CRequest.h
    CXMLGen.h
    CServerParams.h
    CGDALDataWriter.h
    CImageDataWriter.h
    CXMLSerializerInterface.h
    CDataSource.h
    CImgWarpBilinear.h
    CImgWarpBoolean.h
    CImgWarpNearestNeighbour.h
    CGenericDataWarper.h
    CImgWarpNearestRGBA.h
    CPGSQLDB.h
    CDBFileScanner.h
    CDFObjectStore.h
    CFillTriangle.h
    CConvertASCAT.h
    CConvertUGRIDMesh.h
    CConvertADAGUCVector.h
    CConvertEProfile.h
    CConvertADAGUCPoint.h
    CImgRenderPoints.h
    CConvertCurvilinear.h
    CConvertHexagon.h
    CInspire.h
    CGetFileInfo.h
    CStyleConfiguration.h
    CMakeJSONTimeSeries.h
    COpenDAPHandler.h
    CDataPostProcessor.h
    CDBFactory.h
    CDBAdapterPostgreSQL.h
    CAutoResource.h
    CDBAdapterSQLLite.h
    CDBAdapterMongoDB.h
    COctTreeColorQuantizer.h
    CCreateLegend.h
    CCreateHistogram.h
    CNetCDFDataWriter.h
    CAutoConfigure.h
    CMakeEProfile.h
    CImgRenderStippling.h
    CConvertGeoJSON.h
    CGeoJSONData.h
    json.h
    json.c
    CCreateScaleBar.h
    CConvertTROPOMI.h
    CImgRenderPolylines.h
    CAreaMapper.h
    CSLD.h
    CHandleMetadata.h
    CReadPNG.cpp
    CCDFPNGIO.cpp
    CDataReader.cpp
    COGCDims.cpp
    CImageWarper.cpp
    CGeoParams.cpp
    CCairoPlotter.cpp
    CDrawImage.cpp
    CServerError.cpp
    CRequest.cpp
    CXMLGen.cpp
    CServerParams.cpp
    CGDALDataWriter.cpp
    CImageDataWriter.cpp
    CXMLSerializerInterface.cpp
    CDataSource.cpp
    CImgWarpBilinear.cpp
    CImgWarpBoolean.cpp
    CImgWarpNearestNeighbour.cpp
    CGenericDataWarper.cpp
    CImgWarpNearestRGBA.cpp
    CPGSQLDB.cpp
    CDBFileScanner.cpp
    CDBFileScannerCleanFiles.cpp
    CDFObjectStore.cpp
    CFillTriangle.cpp
    CConvertASCAT.cpp
    CConvertUGRIDMesh.cpp
    CConvertADAGUCVector.cpp
    CConvertEProfile.cpp
    CConvertADAGUCPoint.cpp
    CImgRenderPoints.cpp
    CConvertCurvilinear.cpp
    CConvertHexagon.cpp
    CInspire.cpp
    CGetFileInfo.cpp
    CStyleConfiguration.cpp
    CMakeJSONTimeSeries.cpp
    COpenDAPHandler.cpp
    CDataPostProcessor.cpp
    CDBFactory.cpp
    CDBAdapterPostgreSQL.cpp
    CAutoResource.cpp
    CDBAdapterSQLLite.cpp
    CDBAdapterMongoDB.cpp
    COctTreeColorQuantizer.cpp
    CCreateLegend.cpp
    CCreateHistogram.cpp
    CNetCDFDataWriter.cpp
    CAutoConfigure.cpp
    CMakeEProfile.cpp
    CImgRenderStippling.cpp
    CConvertGeoJSON.cpp
    CGeoJSONData.cpp
    CCreateScaleBar.cpp
    CConvertTROPOMI.cpp
    CImgRenderPolylines.cpp
    CAreaMapper.cpp
    CSLD.cpp
    CHandleMetadata.cpp
)

target_include_directories(adagucserverEC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${Cairo_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS} ${PostgreSQL_INCLUDE_DIRS} ${GDAL_INCLUDE_DIRS})
target_link_libraries(adagucserverEC CCDFDataModel hclasses ${NetCDF_LIBRARIES} ${HDF5_LIBRARIES} ${Cairo_LIBRARIES} ${FREETYPE_LIBRARIES} ${LIBXML2_LIBRARY} Threads::Threads ${PNG_LIBRARY} ${PROJ4_LIBRARIES} ${UDUNITS2_LIBRARIES} ${GD_LIBRARIES} ${CURL_LIBRARIES} ${SQLite3_LIBRARIES} ${GDAL_LIBRARIES} ${PostgreSQL_LIBRARIES} ${WebP_LIBRARIES})

