/******************************************************************************
 * 
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2013-06-01
 *
 ******************************************************************************
 *
 * Copyright 2013, Royal Netherlands Meteorological Institute (KNMI)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 ******************************************************************************/

#include "Definitions.h"
#ifdef ADAGUC_USE_GDAL
#include "CGDALDataWriter.h"

#define CGDALDATAWRITER_DEBUG

const char * CGDALDataWriter::className = "CGDALDataWriter";

int  CGDALDataWriter::init(CServerParams *_srvParam,CDataSource *dataSource, int _NrOfBands){
#ifdef CGDALDATAWRITER_DEBUG  
  CDBDebug("INIT");
#endif
  srvParam=_srvParam;
  NrOfBands=_NrOfBands;
  int status;
  _dataSource=dataSource;
  // Init projections
  if(srvParam->Geo->CRS.indexOf("PROJ4:")==0){
    CT::string temp(srvParam->Geo->CRS.c_str()+6);
    srvParam->Geo->CRS.copy(&temp);
  }
  // Load metadata from the dataSource
#ifdef CGDALDATAWRITER_DEBUG
  CDBDebug("CNETCDFREADER_MODE_GET_METADATA");
#endif
  status = reader.open(dataSource,CNETCDFREADER_MODE_GET_METADATA);
  if(status!=0){
    CDBError("Could not open file: %s",dataSource->getFileName());return 1;
  }
#ifdef CGDALDATAWRITER_DEBUG  
  CDBDebug("/CNETCDFREADER_MODE_GET_METADATA");
#endif
  for(size_t j=0;j<dataSource->nrMetadataItems;j++){
    CT::string *metaDataItem = new CT::string();
    metaDataItem->copy(&dataSource->metaData[j]);
    if(metaDataItem->indexOf("product#[C]variables>")!=0){
      metaDataList.push_back(metaDataItem);
    }else delete metaDataItem;
  }
  CT::string *metaDataItem = new CT::string();
  metaDataItem->copy("product#[C]variables>");
  metaDataItem->concat(dataSource->getDataObject(0)->variableName.c_str());
  metaDataList.push_back(metaDataItem);
#ifdef CGDALDATAWRITER_DEBUG  
  CDBDebug("dataSource->getDataObject(0)->variableName.c_str() %s",dataSource->getDataObject(0)->variableName.c_str());
#endif

  // Get Time unit
#ifdef CGDALDATAWRITER_DEBUG  
  CDBDebug("Get time units");
#endif  
  
  try{
    TimeUnit = reader.getTimeUnit(dataSource);
  }catch(int e){
    TimeUnit = "";
  }
  
#ifdef CGDALDATAWRITER_DEBUG  
  CDBDebug("Time unit: %s",TimeUnit.c_str());
#endif  
  
  reader.close();
#ifdef CGDALDATAWRITER_DEBUG  
  CDBDebug("Reader closed");
#endif  



  // Set up geo parameters
  if(srvParam->WCS_GoNative==1){
    //Native!
    for(int j=0;j<4;j++)dfSrcBBOX[j]=dataSource->dfBBOX[j];
    dfDstBBOX[0]=dfSrcBBOX[0];
    dfDstBBOX[1]=dfSrcBBOX[3];
    dfDstBBOX[2]=dfSrcBBOX[2];
    dfDstBBOX[3]=dfSrcBBOX[1];
    srvParam->Geo->dWidth=dataSource->dWidth;
    srvParam->Geo->dHeight=dataSource->dHeight;
    srvParam->Geo->CRS.copy(&dataSource->nativeProj4);
    if(srvParam->Format.length()==0)srvParam->Format.copy("NetCDF4");
  }else{
    // Non native projection
    for(int j=0;j<4;j++)dfSrcBBOX[j]=dataSource->dfBBOX[j];
    for(int j=0;j<4;j++)dfDstBBOX[j]=srvParam->Geo->dfBBOX[j];
    // dfResX and dfResY are in the CRS ore ResponseCRS
    if(srvParam->dWCS_RES_OR_WH==1){
      srvParam->Geo->dWidth=int(((dfDstBBOX[2]-dfDstBBOX[0])/srvParam->dfResX));
      srvParam->Geo->dHeight=int(((dfDstBBOX[1]-dfDstBBOX[3])/srvParam->dfResY));
      srvParam->Geo->dHeight=abs(srvParam->Geo->dHeight);
    }
    if(srvParam->Geo->dWidth>20000||srvParam->Geo->dHeight>20000){
      CDBError("Requested Width or Height is larger than 20000 pixels. Aborting request.");
      return 1;
    }
  }


  //Setup Destination geotransform
  adfDstGeoTransform[0]=dfDstBBOX[0];
  adfDstGeoTransform[1]=(dfDstBBOX[2]-dfDstBBOX[0])/double(srvParam->Geo->dWidth);
  adfDstGeoTransform[2]=0;
  adfDstGeoTransform[3]=dfDstBBOX[3];
  adfDstGeoTransform[4]=0;
  adfDstGeoTransform[5]=(dfDstBBOX[1]-dfDstBBOX[3])/double(srvParam->Geo->dHeight);

  //Setup Source geotransform
  adfSrcGeoTransform[0]=dfSrcBBOX[0];
  adfSrcGeoTransform[1]=(dfSrcBBOX[2]-dfSrcBBOX[0])/double(dataSource->dWidth);
  adfSrcGeoTransform[2]=0;
  adfSrcGeoTransform[3]=dfSrcBBOX[3];
  adfSrcGeoTransform[4]=0;
  adfSrcGeoTransform[5]=(dfSrcBBOX[1]-dfSrcBBOX[3])/double(dataSource->dHeight);

  // Retrieve output format
  CT::string driverName;
  
  for(size_t j=0;j<srvParam->cfg->WCS[0]->WCSFormat.size();j++){
    if(srvParam->Format.equals(srvParam->cfg->WCS[0]->WCSFormat[j]->attr.name.c_str())){
      driverName.copy(srvParam->cfg->WCS[0]->WCSFormat[j]->attr.driver.c_str());
      mimeType.copy(srvParam->cfg->WCS[0]->WCSFormat[j]->attr.mimetype.c_str());
      customOptions.copy(srvParam->cfg->WCS[0]->WCSFormat[j]->attr.options.c_str());
      break;
    }
  }
  srvParam->Format.toUpperCaseSelf();
  if(driverName.length()==0){
    if(srvParam->Format.equals("GEOTIFF")){
      driverName.copy("GTiff");
    }
    if(srvParam->Format.equals("AAIGRID")){
      driverName.copy("AAIGRID");
      if(NrOfBands>1){
        CDBError("This WCS format ('%s') does not support multiple bands. Select a single image, or choose an other format.",srvParam->Format.c_str());
        return 1;
      }
    }
  }
  if(driverName.length()==0){
    CDBError("Unknown format %s",srvParam->Format.c_str());
    return 1;
  }
  // GDAL Init

  GDALAllRegister();
  hOutputDriver = GDALGetDriverByName(driverName.c_str());
  if( hOutputDriver == NULL ){
    CDBError("Failed to find GDAL driver: \"%s\"",driverName.c_str());return 1;
  }

  // Init memory drivers
  hMemDriver1 = GDALGetDriverByName( "MEM" );
  if( hMemDriver1 == NULL ) {CDBError("Failed to find GDAL MEM driver.");return 1;}
  hMemDriver2 = GDALGetDriverByName( "MEM" );
  if( hMemDriver2 == NULL ) {CDBError("Failed to find GDAL MEM driver.");return 1;}

  // Setup data types
  datatype=GDT_Unknown;
  if(dataSource->getDataObject(0)->cdfVariable->getType()==CDF_CHAR)  datatype = GDT_Byte;
  if(dataSource->getDataObject(0)->cdfVariable->getType()==CDF_UBYTE)  datatype = GDT_Byte;
  if(dataSource->getDataObject(0)->cdfVariable->getType()==CDF_SHORT) datatype = GDT_Int16;
  if(dataSource->getDataObject(0)->cdfVariable->getType()==CDF_USHORT) datatype = GDT_UInt16;
  if(dataSource->getDataObject(0)->cdfVariable->getType()==CDF_INT)   datatype = GDT_Int32;
  if(dataSource->getDataObject(0)->cdfVariable->getType()==CDF_UINT)   datatype = GDT_UInt32;
  if(dataSource->getDataObject(0)->cdfVariable->getType()==CDF_FLOAT) datatype = GDT_Float32;
  if(dataSource->getDataObject(0)->cdfVariable->getType()==CDF_DOUBLE)datatype = GDT_Float64;
  if(datatype==GDT_Unknown){
    char temp[100];
    CDF::getCDFDataTypeName(temp,99,dataSource->getDataObject(0)->cdfVariable->getType());
    CDBError("Invalid datatype: dataSource->getDataObject(0)->cdfVariable->getType()=%s",temp);
    return 1;
  }
  
#ifdef CGDALDATAWRITER_DEBUG  
  char dataTypeName[256];
  CDF::getCDFDataTypeName(dataTypeName,255,dataSource->getDataObject(0)->cdfVariable->getType());
  CDBDebug("Dataset datatype = %s WH = [%d,%d], NrOfBands = [%d]",dataTypeName,dataSource->dWidth,dataSource->dHeight,NrOfBands);
#endif  
  
  
  hMemDS2 = GDALCreate( hMemDriver2, "memory_dataset_2",
                        dataSource->dWidth, dataSource->dHeight, NrOfBands, datatype, NULL );
  if( hMemDS2 == NULL ){CDBError("Failed to create GDAL MEM dataset.");reader.close();return 1;}
  // Set source projection
  OGRSpatialReference oSRCSRS;
  char *pszSRS_WKT = NULL;
  GDALSetGeoTransform(hMemDS2,adfSrcGeoTransform );
  if(oSRCSRS.SetFromUserInput(dataSource->nativeProj4.c_str())!=OGRERR_NONE){
    CDBError("WCS: Invalid source projection: [%s]",dataSource->nativeProj4.c_str());
    return 1;
  }
  oSRCSRS.exportToWkt( &pszSRS_WKT );
  GDALSetProjection( hMemDS2,pszSRS_WKT );
  CPLFree( pszSRS_WKT );
  currentBandNr=0;
  if(InputProducts!=NULL)delete[] InputProducts;
  InputProducts=new CT::string[NrOfBands+1];
  if(Times!=NULL)delete[] Times;
  Times=new CT::string[NrOfBands+1];
  
#ifdef CGDALDATAWRITER_DEBUG  
  CDBDebug("/INIT");
#endif
  return 0;


}

int  CGDALDataWriter::addData(std::vector <CDataSource*>&dataSources){
#ifdef CGDALDATAWRITER_DEBUG
  CDBDebug("addData");
#endif
  int status;
  CDataSource *dataSource = dataSources[0];
  status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_ALL);
 
  
  if(status!=0){
    CDBError("Could not open file: %s",dataSource->getFileName());
    return 1;
  }
#ifdef CGDALDATAWRITER_DEBUG
  CDBDebug("Reading %s for bandnr %d",dataSource->getFileName(),currentBandNr);
#endif
  GDALRasterBandH hSrcBand = GDALGetRasterBand( hMemDS2, currentBandNr+1 );
  if(dataSource->getDataObject(0)->hasNodataValue==1){
    dfNoData=dataSource->getDataObject(0)->dfNodataValue;
    GDALSetRasterNoDataValue(hSrcBand, dfNoData);
  }
  
#ifdef CGDALDATAWRITER_DEBUG  
  CDBDebug("copying data in addData, WH= [%d,%d] type = %s", dataSource->dWidth, dataSource->dHeight, CDF::getCDFDataTypeName(dataSource->getDataObject(0)->cdfVariable->getType()).c_str());

#endif
  
  GDALRasterIO( hSrcBand, GF_Write, 0, 0,
                dataSource->dWidth, dataSource->dHeight,
                dataSource->getDataObject(0)->cdfVariable->data,
                dataSource->dWidth, dataSource->dHeight,
                datatype, 0, 0 );
#ifdef CGDALDATAWRITER_DEBUG  
  CDBDebug("finished copying data in addData");
#endif
  // Get time
  status = reader.getTimeString(dataSource,szTemp);
  if(status==0)Times[currentBandNr].copy(szTemp);else Times[currentBandNr].copy("");
  // Get filename
  CT::string basename(dataSource->getFileName());
  int offset = basename.lastIndexOf("/")+1;
  if(offset<0)offset=0;if(offset>MAX_STR_LEN)offset=0;
  InputProducts[currentBandNr].copy(basename.c_str()+offset);
  reader.close();
  currentBandNr++;
#ifdef CGDALDATAWRITER_DEBUG
  CDBDebug("/addData");
#endif  
  return 0;

}

int  CGDALDataWriter::end(){
#ifdef CGDALDATAWRITER_DEBUG  
  CDBDebug("END");
#endif
  
  // Generate a temporary filename for storage
  char szTempFileName[MAX_STR_LEN+1];
  generateUniqueGetCoverageFileName(szTempFileName);
  
  tmpFileName.copy(srvParam->cfg->TempDir[0]->attr.value.c_str());
  tmpFileName.concat("/");
  tmpFileName.concat(szTempFileName);
#ifdef CGDALDATAWRITER_DEBUG    
  CDBDebug("Generating a tmp file with name");
  CDBDebug("%s",szTempFileName);
#endif
  
  // Warp the image from hMemDS2 to hMemDS1
  GDALDataType eDT;
  eDT = GDALGetRasterDataType(GDALGetRasterBand(hMemDS2,1));
  //CDBDebug("Check");
  // Get coordinate systems
  const char *pszSrcWKT;
  char *pszDstWKT = NULL;
  pszSrcWKT = GDALGetProjectionRef( hMemDS2 );
  //CDBDebug("Check");
  //CPLAssert( pszSrcWKT != NULL && strlen(pszSrcWKT) > 0 );
  if(pszSrcWKT == NULL){
    CDBError("GDALGetProjectionRef( hMemDS2 ) == NULL");return 1;
  }
  //CDBDebug("Check");
  OGRSpatialReference oSRS;
  CImageWarper imageWarper;
//  imageWarper.setSrvParam(srvParam);
  //imageWarper.decodeCRS(&srvParam->Geo->CRS);
  CT::string destinationCRS(&srvParam->Geo->CRS);
  CDBDebug("Check %s",srvParam->Geo->CRS.c_str());
  //imageWarper.decodeCRS(&destinationCRS,&srvParam->Geo->CRS,&srvParam->cfg->Projection);
  //CDBDebug("Check");
  if(oSRS.SetFromUserInput(destinationCRS.c_str())!=OGRERR_NONE){
    CDBError("WCS: Invalid destination projection: [%s]",destinationCRS.c_str());
    return 1;
  }
  oSRS.exportToWkt( &pszDstWKT );
  //CDBDebug("Check");
  // Create the output file.
  hMemDS1 = GDALCreate( hMemDriver1,"memory_dataset_1",srvParam->Geo->dWidth,srvParam->Geo->dHeight,NrOfBands,eDT,NULL );
  if( hMemDS1 == NULL ){CDBError("Failed to create GDAL MEM dataset.");return 1;}

  if(_dataSource->getDataObject(0)->hasNodataValue==1){
    dfNoData=_dataSource->getDataObject(0)->dfNodataValue;
    for(int j=0;j<NrOfBands;j++){
      GDALRasterBandH hDstcBand = GDALGetRasterBand( hMemDS1,j+1 );
      GDALSetRasterNoDataValue(hDstcBand, dfNoData);
    }
  }

  GDALSetProjection( hMemDS1, pszDstWKT );
  GDALSetGeoTransform( hMemDS1, adfDstGeoTransform );
  //CDBDebug("Check");
  GDALWarpOptions *psWarpOptions = GDALCreateWarpOptions();
  psWarpOptions->hSrcDS = hMemDS2;
  psWarpOptions->hDstDS = hMemDS1;
  psWarpOptions->nBandCount = NrOfBands;
  psWarpOptions->panSrcBands =
      (int *) CPLMalloc(sizeof(int) * psWarpOptions->nBandCount );
  for(int j=0;j<NrOfBands;j++)psWarpOptions->panSrcBands[j] = j+1;
  psWarpOptions->panDstBands =
      (int *) CPLMalloc(sizeof(int) * psWarpOptions->nBandCount );
  for(int j=0;j<NrOfBands;j++)psWarpOptions->panDstBands[j] = j+1;
  //CDBDebug("Check");
  //CDBDebug("END");
  // nodata
  //printf("<br>Nodata=[%f]<br>\n",dfNoData);
  psWarpOptions->padfSrcNoDataReal =
      (double *) CPLMalloc(sizeof(double) * psWarpOptions->nBandCount );
  for(int j=0;j<NrOfBands;j++)psWarpOptions->padfSrcNoDataReal[j] = dfNoData;
  psWarpOptions->padfDstNoDataReal =
      (double *) CPLMalloc(sizeof(double) * psWarpOptions->nBandCount );
  for(int j=0;j<NrOfBands;j++)psWarpOptions->padfDstNoDataReal[j] = dfNoData;
  //CDBDebug("Check");
  psWarpOptions->padfSrcNoDataImag =
      (double *) CPLMalloc(sizeof(double) * psWarpOptions->nBandCount );
  for(int j=0;j<NrOfBands;j++)psWarpOptions->padfSrcNoDataImag[j] = 0.0f;
  psWarpOptions->padfDstNoDataImag =
      (double *) CPLMalloc(sizeof(double) * psWarpOptions->nBandCount );
  for(int j=0;j<NrOfBands;j++)psWarpOptions->padfDstNoDataImag[j] = 0.0;

  char **papszWarpOptions = NULL;
  papszWarpOptions = CSLSetNameValue(papszWarpOptions,"INIT_DEST","NO_DATA");
  psWarpOptions->papszWarpOptions=papszWarpOptions;
 // Establish reprojection transformer.
  //CDBDebug("Check");
  psWarpOptions->pTransformerArg =  GDALCreateGenImgProjTransformer( hMemDS2,
      GDALGetProjectionRef(hMemDS2),
      hMemDS1,
      GDALGetProjectionRef(hMemDS1),
      FALSE, 0.0, 1 );

  //CDBDebug("Check");
  psWarpOptions->pfnTransformer = GDALGenImgProjTransform;
  psWarpOptions->dfWarpMemoryLimit = 1147483647.0f;
  GDALSetCacheMax(1147483647);
  // Initialize and execute the warp operation.
  GDALWarpOperation oOperation;

  oOperation.Initialize( psWarpOptions );
  CDBDebug("start GDAL ChunkAndWarpImage");
  /*oOperation.ChunkAndWarpImage( 0, 0,
                                GDALGetRasterXSize( hMemDS1 ),
  GDALGetRasterYSize( hMemDS1 ) );*/
  oOperation.ChunkAndWarpImage( 0, 0,
                                GDALGetRasterXSize( hMemDS1 ),
                                GDALGetRasterYSize( hMemDS1 ) );
  GDALDestroyGenImgProjTransformer( psWarpOptions->pTransformerArg );
  GDALDestroyWarpOptions( psWarpOptions );
  CDBDebug("Completed ChunkAndWarpImage");
  // Set metadata for hMemDS1
  char **papszMetadata = NULL;
  for(size_t j=0;j<metaDataList.size();j++){
    CT::string *attrib = metaDataList[j]->splitToArray(">");
    //attrib[1].concat("<end>");
    //CDBDebug("%s=%s",attrib[0].c_str(),attrib[1].c_str());
    papszMetadata = CSLSetNameValue(papszMetadata,attrib[0].c_str(),attrib[1].c_str());
    delete[] attrib;
  }
  // Update Input_products
  //const char *currentInputProduct=CSLFetchNameValue(papszMetadata,"product#[C]input_products");
  //CDBDebug("END");
  CT::string GDALinputproducts;
  GDALinputproducts.concat(&InputProducts[0]);
  for(int j=1;j<NrOfBands;j++){
    GDALinputproducts.concat(", ");
    GDALinputproducts.concat(&InputProducts[j]);
  }
  papszMetadata = CSLSetNameValue(papszMetadata,"product#[C]input_products",GDALinputproducts.c_str());

  GDALSetMetadata( hMemDS1, papszMetadata,"");
  CSLDestroy( papszMetadata);
  papszMetadata = NULL;
  // Set metadata for each band in hMemDS1
  if(TimeUnit.length()>10&&Times[0].length()>10){
    for(int b=0;b<NrOfBands;b++){

      char **papszMetadata = NULL;
      double offset;
      //CADAGUC_time *ADTime = new CADAGUC_time(TimeUnit.c_str());
      CTime adagucTime;
      try{
        adagucTime.init(TimeUnit.c_str());
        offset = adagucTime.dateToOffset(adagucTime.ISOStringToDate(Times[b].c_str()));
        snprintf(szTemp,MAX_STR_LEN,"DIMENSION#time");
        snprintf(szTemp2,MAX_STR_LEN,"%f",offset);
        papszMetadata = CSLSetNameValue(papszMetadata,szTemp,szTemp2);

      }catch(int e){
        snprintf(szTemp,MAX_STR_LEN,"DIMENSION#time");
        snprintf(szTemp2,MAX_STR_LEN,"%s",Times[b].c_str());
        papszMetadata = CSLSetNameValue(papszMetadata,szTemp,szTemp2);
      }
      /*int status=ADTime->ISOTimeToOffset(offset,Times[b].c_str());
      if(status==0){
        snprintf(szTemp,MAX_STR_LEN,"DIMENSION#time");
        snprintf(szTemp2,MAX_STR_LEN,"%f",offset);
        papszMetadata = CSLSetNameValue(papszMetadata,szTemp,szTemp2);
      }else{
        snprintf(szTemp,MAX_STR_LEN,"DIMENSION#time");
        snprintf(szTemp2,MAX_STR_LEN,"%s",Times[b].c_str());
        papszMetadata = CSLSetNameValue(papszMetadata,szTemp,szTemp2);
      }
      delete ADTime;*/

      snprintf(szTemp,MAX_STR_LEN,"UNITS#time");
      snprintf(szTemp2,MAX_STR_LEN,"%s",TimeUnit.c_str());
      papszMetadata = CSLSetNameValue(papszMetadata,szTemp,szTemp2);
      snprintf(szTemp,MAX_STR_LEN,"VARNAME");
      snprintf(szTemp2,MAX_STR_LEN,"%s",_dataSource->getDataObject(0)->variableName.c_str());
      papszMetadata = CSLSetNameValue(papszMetadata,szTemp,szTemp2);
      // Set the metadata
      GDALRasterBandH hSrcBand = GDALGetRasterBand( hMemDS1, b+1 );
      GDALSetRasterNoDataValue(hSrcBand, dfNoData);
      //printf("<br>Setting Nodata2=[%f] for band %d<br>\n",dfNoData,b+1);
      GDALSetMetadata( hSrcBand, papszMetadata,"");
      CSLDestroy( papszMetadata);
      papszMetadata = NULL;
    }
  }
  // Copy the hMemDS1 dataset to the output driver
  char ** papszOptions = NULL;

  if(customOptions.length()>2){
    CT::string *co = customOptions.splitToArray(",");
    for(size_t j=0;j<co->count;j++){
      CT::string *splittedco = customOptions.splitToArray("=");
      papszOptions = CSLSetNameValue( papszOptions, splittedco[0].c_str(), splittedco[1].c_str() );
      delete[] splittedco;
    }
    delete[] co;
  }
  CDBDebug("Copying hMemDS1 to hOutputDriver");
  hOutputDS = GDALCreateCopy( hOutputDriver, tmpFileName.c_str(), hMemDS1, FALSE, papszOptions, NULL, NULL );
  CDBDebug("GDALCreateCopy completed");
  if( hOutputDS == NULL ){
    CDBError("WriteGDALRaster: Failed to create output file:<br>\n\"%s\"",tmpFileName.c_str());
    CDBError("LastErrorMsg: %s",CPLGetLastErrorMsg());
    GDALClose( hMemDS1 );
    return 1;
  }
  CSLDestroy( papszOptions );

  GDALClose( hOutputDS );
  GDALClose( hMemDS1 );
  GDALClose( hMemDS2 );

  // Output the file to stdout
  
  if(mimeType.length()<2)mimeType.copy("Content-Type:text/plain");
//  printf("%s\n",tmpFileName.c_str());
  int returnCode=0;
  FILE *fp=fopen(tmpFileName.c_str(), "r");
  if (fp == NULL) {
    CDBError("Invalid File: %s<br>\n", tmpFileName.c_str());
    returnCode = 1;
  }else{
    //Send the binary data
    fseek( fp, 0L, SEEK_END );
    size_t endPos = ftell( fp);
    fseek( fp, 0L, SEEK_SET );
    //CDBDebug("File opened: size = %d",endPos);
    CDBDebug("Now start streaming %d bytes to the client with mimetype %s",endPos,mimeType.c_str());
    printf("Content-Disposition: attachment; filename=%s\n",generateGetCoverageFileName().c_str());
    printf("Content-Description: File Transfer\n");
    printf("Content-Transfer-Encoding: binary\n");
    printf("Content-Length: %zu\n",endPos); 
    printf("%s%c%c\n",mimeType.c_str(),13,10);
    for(size_t j=0;j<endPos;j++)putchar(getc(fp));
    fclose(fp);
    fclose(stdout);
  }
  //Remove temporary files
  remove(tmpFileName.c_str());
  tmpFileName.setChar(tmpFileName.length()-3,'p');
  tmpFileName.setChar(tmpFileName.length()-2,'r');
  tmpFileName.setChar(tmpFileName.length()-1,'j');
  remove(tmpFileName.c_str());
  tmpFileName.setChar(tmpFileName.length()-3,'x');
  tmpFileName.setChar(tmpFileName.length()-2,'m');
  tmpFileName.setChar(tmpFileName.length()-1,'l');
  remove(tmpFileName.c_str());
  tmpFileName.setChar(tmpFileName.length()-3,'t');
  tmpFileName.setChar(tmpFileName.length()-2,'m');
  tmpFileName.setChar(tmpFileName.length()-1,'p');
  tmpFileName.concat(".aux.xml");
  remove(tmpFileName.c_str());

  if(InputProducts!=NULL)delete[] InputProducts;InputProducts=NULL;
  if(Times!=NULL)delete[] Times;Times=NULL;
  return returnCode;

  return 0;
}


/*******************************/
/* String generation functions */
/*******************************/

void CGDALDataWriter::generateString(char *s, const int _len) {
  //CDBDebug("generateString");
  int len=_len-1;
  timespec timevar;
  
#if _POSIX_TIMERS > 0
  clock_gettime(CLOCK_REALTIME, &timevar);
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  timevar.tv_sec = tv.tv_sec;
  timevar.tv_nsec = tv.tv_usec*1000;
#endif
  //CDBDebug("generateString");
  //double dfTime;= double((unsigned)timevar.tv_nsec)+double(timevar.tv_sec)*1000000000.0f;
  unsigned int dTime = (unsigned int)(timevar.tv_nsec);
  //CDBDebug("generateString %d",dTime);
  //srand (time(NULL)+(unsigned int)dfTime);
  srand (time(NULL)+dTime);
  //CDBDebug("generateString");
  static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";
  //CDBDebug("generateString");
  for (int i = 0; i < len; ++i) {
    s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }
  s[len] = 0;
  //CDBDebug("generateString");
}
CT::string CGDALDataWriter::generateGetCoverageFileName(){
  CT::string humanReadableString;
  humanReadableString.copy(srvParam->Format.c_str());
  humanReadableString.concat("_");
  humanReadableString.concat(_dataSource->getDataObject(0)->variableName.c_str());
  
  
  
  for(size_t i=0;i<_dataSource->requiredDims.size();i++){
    humanReadableString.printconcat("_%s",_dataSource->requiredDims[i]->value.c_str());
  }
  
  humanReadableString.replaceSelf(":","_");
  humanReadableString.replaceSelf(".","_");
  
  
  CT::string extension=".bin";
  CT::string formatUpperCase;
  formatUpperCase.copy(srvParam->Format.c_str());
  formatUpperCase.toUpperCaseSelf();
  if(formatUpperCase.equals("AAIGRID")){
    extension=".asc";
  }
  if(formatUpperCase.indexOf("NETCDF")!=-1){
    extension=".nc";
  }
  if(formatUpperCase.indexOf("TIF")!=-1){
    extension=".tif";
  }
  
  if(formatUpperCase.indexOf("IMAGE/PNG")!=-1){
    extension=".png";
  }
  if(formatUpperCase.indexOf("IMAGE/BMP")!=-1){
    extension=".bmp";
  }
  if(formatUpperCase.indexOf("IMAGE/GIF")!=-1){
    extension=".gif";
  }
  if(formatUpperCase.indexOf("IMAGE/JPG")!=-1){
    extension=".jpg";
  }
  humanReadableString.concat(extension.c_str());
  
  return humanReadableString;
}
void CGDALDataWriter::generateUniqueGetCoverageFileName(char *pszTempFileName){
  //CDBDebug("generateUniqueGetCoverageFileName");
  int len,offset;
  char szRandomPart[MAX_STR_LEN+1];
  char szTemp[MAX_STR_LEN+1];
  generateString(szRandomPart,12);
  snprintf(pszTempFileName,MAX_STR_LEN,
           "FORMAT--_VARIABLENAME_BBOX0_BBOX2_BBOX3_BBOX4_WIDTH_HEIGH_RESX-_RESY-_CONFIG--_DIM_DIM_DIM_PROJECTION_RAND------------______.tmp");
  //CDBDebug("generateUniqueGetCoverageFileName");
  for(int j=0;j<118;j++)pszTempFileName[j]='_';pszTempFileName[MAX_STR_LEN]='\0';
  //CDBDebug("generateUniqueGetCoverageFileName");
  //Format
  strncpy(pszTempFileName+0,srvParam->Format.c_str(),8);
  for(int j=srvParam->Format.length();j<8;j++)pszTempFileName[j]='_';
  //CDBDebug("generateUniqueGetCoverageFileName");
  //VariableName
  offset=9;
  strncpy(pszTempFileName+offset,_dataSource->getDataObject(0)->variableName.c_str(),10);
  size_t varNameLength = strlen(_dataSource->getDataObject(0)->variableName.c_str());
  for(int j=varNameLength+offset;j<offset+10;j++)pszTempFileName[j]='_';
  //BBOX
  offset=22;
  snprintf(szTemp,MAX_STR_LEN,"%f",srvParam->Geo->dfBBOX[0]);
  strncpy(pszTempFileName+offset,szTemp,6);
  len=strlen(szTemp);
  for(int j=len+offset;j<offset+6;j++)pszTempFileName[j]='_';
  offset=29;
  snprintf(szTemp,MAX_STR_LEN,"%f",srvParam->Geo->dfBBOX[1]);
  strncpy(pszTempFileName+offset,szTemp,6);
  len=strlen(szTemp);
  for(int j=len+offset;j<offset+6;j++)pszTempFileName[j]='_';
  offset=36;
  snprintf(szTemp,MAX_STR_LEN,"%f",srvParam->Geo->dfBBOX[2]);
  strncpy(pszTempFileName+offset,szTemp,6);
  len=strlen(szTemp);
  for(int j=len+offset;j<offset+6;j++)pszTempFileName[j]='_';
  offset=43;
  snprintf(szTemp,MAX_STR_LEN,"%f",srvParam->Geo->dfBBOX[3]);
  strncpy(pszTempFileName+offset,szTemp,6);
  len=strlen(szTemp);
  for(int j=len+offset;j<offset+6;j++)pszTempFileName[j]='_';
  //CDBDebug("generateUniqueGetCoverageFileName");
  //Width
  offset=50;
  snprintf(szTemp,MAX_STR_LEN,"%d",srvParam->Geo->dWidth);
  strncpy(pszTempFileName+offset,szTemp,5);
  len=strlen(szTemp);
  for(int j=len+offset;j<offset+5;j++)pszTempFileName[j]='_';
  //Height
  offset=56;
  snprintf(szTemp,MAX_STR_LEN,"%d",srvParam->Geo->dHeight);
  strncpy(pszTempFileName+offset,szTemp,5);
  len=strlen(szTemp);
  for(int j=len+offset;j<offset+5;j++)pszTempFileName[j]='_';
  //RESX
  offset=62;
  snprintf(szTemp,MAX_STR_LEN,"%f",srvParam->dfResX);
  strncpy(pszTempFileName+offset,szTemp,5);
  len=strlen(szTemp);
  for(int j=len+offset;j<offset+5;j++)pszTempFileName[j]='_';
  //RESY
  offset=68;
  snprintf(szTemp,MAX_STR_LEN,"%f",srvParam->dfResY);
  strncpy(pszTempFileName+offset,szTemp,5);
  len=strlen(szTemp);
  for(int j=len+offset;j<offset+5;j++)pszTempFileName[j]='_';
  //ConfigFile
  offset=74;
  CT::string tmp;
  tmp.copy(srvParam->configFileName.c_str()+srvParam->configFileName.lastIndexOf("/"));
  snprintf(szTemp,MAX_STR_LEN,"%s",tmp.c_str());
  strncpy(pszTempFileName+offset,szTemp,11);
  len=strlen(szTemp);
  for(int j=len+offset;j<offset+11;j++)pszTempFileName[j]='_';
  //DIMS
  for(size_t i=0;i<_dataSource->cfgLayer->Dimension.size()&&i<4;i++){
    offset=86+i*4;
    snprintf(szTemp,MAX_STR_LEN,"%d",int(_dataSource->getDimensionIndex(i)));
    strncpy(pszTempFileName+offset,szTemp,3);
    len=strlen(szTemp);
    for(int j=len+offset;j<offset+3;j++)pszTempFileName[j]='_';
  }
  //Projection
  //CDBDebug("generateUniqueGetCoverageFileName");
  offset=102;
  snprintf(szTemp,MAX_STR_LEN,"%s",srvParam->Geo->CRS.c_str());
  strncpy(pszTempFileName+offset,szTemp,10);
  len=strlen(szTemp);
  for(int j=len+offset;j<offset+10;j++)pszTempFileName[j]='_';
  //RandomString
  offset=113;
  snprintf(szTemp,MAX_STR_LEN,"%s",szRandomPart);
  strncpy(pszTempFileName+offset,szTemp,11);
  len=strlen(szTemp);
  for(int j=len+offset;j<offset+11;j++)pszTempFileName[j]='_';

  //Check for characters
  for(int j=0;j<124;j++){
    if(!isalnum(pszTempFileName[j])&&pszTempFileName[j]!='.')pszTempFileName[j]='_';
  }
  pszTempFileName[128]='\0';
  //CDBDebug("generateUniqueGetCoverageFileName");
}
#endif
