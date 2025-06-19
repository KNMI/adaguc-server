find_package(LibXml2 REQUIRED)
find_package(Cairo REQUIRED)
find_package(Freetype REQUIRED)
find_package(NetCDF REQUIRED)
find_package(Threads REQUIRED)
find_package(PNG REQUIRED)
find_package(PROJ REQUIRED)
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
    CAreaMapper.cpp
    CAreaMapper.h
    CAutoConfigure.cpp
    CAutoConfigure.h
    CAutoResource.cpp
    CAutoResource.h
    CCairoPlotter.cpp
    CCairoPlotter.h
    CColor.h
    CConvertADAGUCPoint.cpp
    CConvertADAGUCPoint.h
    CConvertADAGUCVector.cpp
    CConvertADAGUCVector.h
    CConvertASCAT.cpp
    CConvertASCAT.h
    CConvertCurvilinear.cpp
    CConvertCurvilinear.h
    CConvertEProfile.cpp
    CConvertEProfile.h
    CConvertGeoJSON.cpp
    CConvertGeoJSON.h
    CConvertHexagon.cpp
    CConvertHexagon.h
    CConvertKNMIH5EchoToppen.cpp
    CConvertKNMIH5EchoToppen.h
    CConvertH5VolScan.cpp
    CConvertH5VolScan.h
    CConvertH5VolScanUtils.cpp
    CConvertH5VolScanUtils.h
    CConvertLatLonGrid.h
    CConvertLatLonGrid.cpp
    CConvertLatLonGridHeader.cpp
    CConvertLatLonGridData.cpp
    CConvertLatLonBnds.h
    CConvertLatLonBnds.cpp
    CConvertLatLonBndsHeader.cpp
    CConvertLatLonBndsData.cpp
    CConvertTROPOMI.cpp
    CConvertTROPOMI.h
    CConvertUGRIDMesh.cpp
    CConvertUGRIDMesh.h
    CConvertLatLonBnds.cpp
    CConvertLatLonBnds.h
    CCreateHistogram.cpp
    CCreateHistogram.h
    CCreateScaleBar.cpp
    CCreateScaleBar.h
    CCreateTiles.cpp
    CCreateTiles.h
    CDBAdapterPostgreSQL.cpp
    CDBAdapterPostgreSQL.h
    CDBAdapterSQLLite.cpp
    CDBAdapterSQLLite.h
    CDBFactory.cpp
    CDBFactory.h
    CDBFileScanner.cpp
    CDBFileScanner.h
    CDBFileScannerCleanFiles.cpp
    CDFObjectStore.cpp
    CDFObjectStore.h
    utils/LayerMetadataStore.h
    utils/LayerMetadataStore.cpp
    utils/XMLGenUtils.h
    utils/XMLGenUtils.cpp
    utils/CXMLTemplates.h
    utils/LayerUtils.h
    utils/LayerUtils.cpp
    utils/LayerMetadataToJson.h
    utils/LayerMetadataToJson.cpp
    utils/UpdateLayerMetadata.h
    utils/UpdateLayerMetadata.cpp
    utils/ConfigurationUtils.h
    utils/ConfigurationUtils.cpp    
    utils/CRequestUtils.h
    utils/CRequestUtils.cpp
    Types/LayerMetadataType.h
    GenericDataWarper/gdwDrawTriangle.h
    GenericDataWarper/gdwDrawTriangle.cpp
    CGenericDataWarper.cpp
    CGenericDataWarper.h
    GenericDataWarper/gdwFindPixelExtent.cpp
    GenericDataWarper/gdwFindPixelExtent.h
    CDataPostProcessors/CDataPostProcessor_AddFeatures.cpp
    CDataPostProcessors/CDataPostProcessor_AddFeatures.h
    CDataPostProcessors/CDataPostProcessor_CDPDBZtoRR.cpp
    CDataPostProcessors/CDataPostProcessor_CDPDBZtoRR.h
    CDataPostProcessors/CDataPostProcessors_MSGCPP.cpp
    CDataPostProcessors/CDataPostProcessors_MSGCPP.h
    CDataPostProcessors/CDPPGoes16Metadata.cpp
    CDataPostProcessors/CDPPGoes16Metadata.h
    CDataPostProcessors/CDataPostProcessor.cpp
    CDataPostProcessors/CDataPostProcessor.h
    CDataPostProcessors/CDataPostProcessor_AXplusB.cpp
    CDataPostProcessors/CDataPostProcessor_AXplusB.h
    CDataPostProcessors/CDataPostProcessor_Beaufort.cpp
    CDataPostProcessors/CDataPostProcessor_Beaufort.h
    CDataPostProcessors/CDataPostProcessor_ToKnots.cpp
    CDataPostProcessors/CDataPostProcessor_ToKnots.h
    CDataPostProcessors/CDataPostProcessor_WindSpeedKnotsToMs.cpp
    CDataPostProcessors/CDataPostProcessor_WindSpeedKnotsToMs.h
    CDataPostProcessors/CDataPostProcessor_ClipMinMax.cpp
    CDataPostProcessors/CDataPostProcessor_ClipMinMax.h
    CDataPostProcessors/CDataPostProcessor_IncludeLayer.cpp
    CDataPostProcessors/CDataPostProcessor_IncludeLayer.h
    CDataPostProcessors/CDataPostProcessor_Operator.cpp
    CDataPostProcessors/CDataPostProcessor_Operator.h
    CDataPostProcessors/CDataPostProcessor_SolarTerminator.h
    CDataPostProcessors/CDataPostProcessor_SolarTerminator.cpp
    CDataPostProcessors/solar/solar_terminator.h
    CDataPostProcessors/solar/solar_terminator.cpp
    CDataPostProcessors/CDataPostProcessor_WFP.h
    CDataPostProcessors/CDataPostProcessor_WFP.cpp
    CDataPostProcessors/CDataPostProcessor_UVComponents.h
    CDataPostProcessors/CDataPostProcessor_UVComponents.cpp
    CDataPostProcessors/CDataPostProcessor_FilterDataObjects.h
    CDataPostProcessors/CDataPostProcessor_FilterDataObjects.cpp
    CDataPostProcessors/CDataPostProcessor_MetadataVariable.h
    CDataPostProcessors/CDataPostProcessor_MetadataVariable.cpp    
    CDataReader.cpp
    CDataReader.h
    CDataSource.cpp
    CDataSource.h
    CDrawFunction.cpp
    CDrawFunction.h
    CDrawImage.cpp
    CDrawImage.h
    CFillTriangle.cpp
    CFillTriangle.h
    CGDALDataWriter.cpp
    CGDALDataWriter.h
    CGeoJSONData.cpp
    CGeoJSONData.h
    CGeoParams.cpp
    CGeoParams.h
    CGetFileInfo.cpp
    CGetFileInfo.h
    CHandleMetadata.cpp
    CHandleMetadata.h
    CImageDataWriter.cpp
    CImageDataWriter.h
    CImageWarper.cpp
    CImageWarper.h
    CImgRenderPolylines.cpp
    CImgRenderPolylines.h
    CImgRenderStippling.cpp
    CImgRenderStippling.h
    CImgRenderers/CImgRenderPoints.cpp
    CImgRenderers/CImgRenderPoints.h
    CImgWarpBilinear.cpp
    CImgWarpBilinear.h
    CImgWarpBoolean.cpp
    CImgWarpBoolean.h
    CImgWarpGeneric.cpp
    CImgWarpGeneric.h
    CImgWarpHillShaded.cpp
    CImgWarpHillShaded.h
    CImgWarpNearestNeighbour.cpp
    CImgWarpNearestNeighbour.h
    CImgWarpNearestRGBA.cpp
    CImgWarpNearestRGBA.h
    CImgRenderFieldVectors.h
    CImgRenderFieldVectors.cpp
    CInspire.cpp
    CInspire.h
    CLegendRenderers/CCreateLegend.cpp
    CLegendRenderers/CCreateLegend.h
    CMakeEProfile.cpp
    CMakeEProfile.h
    CMakeJSONTimeSeries.cpp
    CMakeJSONTimeSeries.h
    CNetCDFDataWriter.cpp
    CNetCDFDataWriter.h
    COGCDims.cpp
    COGCDims.h
    COctTreeColorQuantizer.cpp
    COctTreeColorQuantizer.h
    COpenDAPHandler.cpp
    COpenDAPHandler.h
    CPGSQLDB.cpp
    CPGSQLDB.h
    CRectangleText.cpp
    CRectangleText.h
    CRequest.cpp
    CRequest.h
    CSLD.cpp
    CSLD.h
    CServerError.cpp
    CServerError.h
    CServerParams.cpp
    CServerParams.h
    CStyleConfiguration.cpp
    CStyleConfiguration.h
    CUniqueRequests/CURTypes.h
    CUniqueRequests/requestUtils.h
    CUniqueRequests/requestUtils.cpp
    CUniqueRequests/CURUniqueRequests.h
    CUniqueRequests/CURUniqueRequests.cpp
    CXMLGen.cpp
    CXMLGen.h
    CXMLSerializerInterface.cpp
    CXMLSerializerInterface.h
    json.c
    json.h
    LayerTypeLiveUpdate/LayerTypeLiveUpdate.cpp
    LayerTypeLiveUpdate/LayerTypeLiveUpdate.h
    Types/BBOX.h
    Types/BBOX.cpp
    Types/ProjectionStore.h
    Types/ProjectionStore.cpp
    testadagucserver.cpp
    testtimeutils.cpp
    timeutils.cpp
    timeutils.h
)

target_include_directories(adagucserverEC PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${Cairo_INCLUDE_DIRS} ${FREETYPE_INCLUDE_DIRS} ${PostgreSQL_INCLUDE_DIRS} ${GDAL_INCLUDE_DIRS} ${PROJ_INCLUDE_DIR})
target_link_libraries(adagucserverEC CCDFDataModel hclasses ${NetCDF_LIBRARIES} ${HDF5_LIBRARIES} ${Cairo_LIBRARIES} ${FREETYPE_LIBRARIES} ${LIBXML2_LIBRARY} Threads::Threads ${PNG_LIBRARY} ${PROJ_LIBRARIES} ${UDUNITS2_LIBRARIES} ${GD_LIBRARIES} ${CURL_LIBRARIES} ${SQLite3_LIBRARIES} ${GDAL_LIBRARIES} ${PostgreSQL_LIBRARIES} ${WebP_LIBRARIES})

# Build unit test executable
add_executable(testadagucserver testadagucserver.cpp)
target_link_libraries(testadagucserver PRIVATE adagucserverEC CCDFDataModel hclasses CppUnitLite)

# Build unit tests for timeutils
add_executable(testtimeutils testtimeutils.cpp)
target_link_libraries(testtimeutils PRIVATE adagucserverEC hclasses CppUnitLite)