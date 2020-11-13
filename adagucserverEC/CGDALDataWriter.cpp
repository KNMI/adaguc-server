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
//   for(size_t j=0;j<dataSource->nrMetadataItems;j++){
//     CT::string *metaDataItem = new CT::string();
//     metaDataItem->copy(&dataSource->metaData[j]);
//     if(metaDataItem->indexOf("product#[C]variables>")!=0){
//       metaDataList.push_back(metaDataItem);
//     }else delete metaDataItem;
//   }
//   CT::string *metaDataItem = new CT::string();
//   metaDataItem->copy("product#[C]variables>");
//   metaDataItem->concat(dataSource->getDataObject(0)->variableName.c_str());
//   metaDataList.push_back(metaDataItem);
// #ifdef CGDALDATAWRITER_DEBUG  
//   CDBDebug("dataSource->getDataObject(0)->variableName.c_str() %s",dataSource->getDataObject(0)->variableName.c_str());
// #endif

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
    // Non native projection units
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
//   adfSrcGeoTransform[0]=dfSrcBBOX[0];
//   adfSrcGeoTransform[1]=(dfSrcBBOX[2]-dfSrcBBOX[0])/double(dataSource->dWidth);
//   adfSrcGeoTransform[2]=0;
//   adfSrcGeoTransform[3]=dfSrcBBOX[3];
//   adfSrcGeoTransform[4]=0;
//   adfSrcGeoTransform[5]=(dfSrcBBOX[1]-dfSrcBBOX[3])/double(dataSource->dHeight);

  // Retrieve output format

  
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
  
  
  destinationGDALDataSet = GDALCreate( hMemDriver2, "memory_dataset_2",
                        srvParam->Geo->dWidth,srvParam->Geo->dHeight, NrOfBands, datatype, NULL );
  if( destinationGDALDataSet == NULL ){CDBError("Failed to create GDAL MEM dataset.");reader.close();return 1;}
  GDALSetGeoTransform(destinationGDALDataSet,adfDstGeoTransform );
  OGRSpatialReference oDSTSRS;

  char *pszSRS_WKT = NULL;
  if(oDSTSRS.SetFromUserInput(srvParam->Geo->CRS.c_str())!=OGRERR_NONE){
    CDBError("WCS: Invalid source projection: [%s]",srvParam->Geo->CRS.c_str());
    return 1;
  }
  oDSTSRS.exportToWkt( &pszSRS_WKT );
  GDALSetProjection( destinationGDALDataSet,pszSRS_WKT );
  CPLFree( pszSRS_WKT );
  
  currentBandNr=0;
  if(InputProducts!=NULL)delete[] InputProducts;
  InputProducts=new CT::string[NrOfBands+1];
//   if(Times!=NULL)delete[] Times;
//   Times=new CT::string[NrOfBands+1];
  
    

  
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
  GDALRasterBandH hSrcBand = GDALGetRasterBand( destinationGDALDataSet, currentBandNr+1 );
  dfNoData = NAN;
  if(dataSource->getDataObject(0)->hasNodataValue==1){
    dfNoData=dataSource->getDataObject(0)->dfNodataValue;
   
  }
  GDALSetRasterNoDataValue(hSrcBand, dfNoData);
  
#ifdef CGDALDATAWRITER_DEBUG  
  CDBDebug("copying data in addData, WH= [%d,%d] type = %s", srvParam->Geo->dWidth,srvParam->Geo->dHeight, CDF::getCDFDataTypeName(dataSource->getDataObject(0)->cdfVariable->getType()).c_str());

#endif
  //CDBDebug("Element 0 = %f",((float*)dataSource->getDataObject(0)->cdfVariable->data)[0]);
  
  
  //Warp
  void *warpedData = NULL;
//  CDBDebug("DFNODATA = %f",dfNoData);
  CDF::allocateData(dataSource->getDataObject(0)->cdfVariable->getType(),&warpedData,srvParam->Geo->dWidth*srvParam->Geo->dHeight);

  if(CDF::fill(warpedData,dataSource->getDataObject(0)->cdfVariable->getType(),dfNoData,srvParam->Geo->dWidth*srvParam->Geo->dHeight)!=0){
    CDBError("Unable to initialize data field to nodata value");
    return 1;
  }
  
  CImageWarper warper;
  
  status = warper.initreproj(dataSource,srvParam->Geo,&srvParam->cfg->Projection);
  if(status != 0){
    CDBError("Unable to initialize projection");
    return 1;
  }
  CGeoParams sourceGeo;
      
  sourceGeo.dWidth = dataSource->dWidth;
  sourceGeo.dHeight = dataSource->dHeight;
  sourceGeo.dfBBOX[0] = dataSource->dfBBOX[0];
  sourceGeo.dfBBOX[1] = dataSource->dfBBOX[1];
  sourceGeo.dfBBOX[2] = dataSource->dfBBOX[2];
  sourceGeo.dfBBOX[3] = dataSource->dfBBOX[3];
  sourceGeo.dfCellSizeX = dataSource->dfCellSizeX;
  sourceGeo.dfCellSizeY = dataSource->dfCellSizeY;
  sourceGeo.CRS = dataSource->nativeProj4;
  
  CDFType dataType=dataSource->getDataObject(0)->cdfVariable->getType();
  void *sourceData = dataSource->getDataObject(0)->cdfVariable->data;
  
//   for(int y=0;y< dataSource->dHeight;y++){
//     for(int x=0;x< dataSource->dWidth;x++){
//     ((float*)sourceData)[x+y* dataSource->dWidth]=256.;
//   }
//   }
//   
//    
//   for(int j=0;j<srvParam->Geo->dWidth/2;j++){
//     ((float*)warpedData)[j+j*srvParam->Geo->dWidth]=129.0f;
//   }
  
  Settings settings;
  settings.width = srvParam->Geo->dWidth;
  settings.height = srvParam->Geo->dHeight;
  settings.data=warpedData;

  switch(dataType){
    case CDF_CHAR  :  GenericDataWarper::render<char>  (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction);break;
    case CDF_BYTE  :  GenericDataWarper::render<char>  (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction);break;
    case CDF_UBYTE :  GenericDataWarper::render<unsigned char> (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction);break;
    case CDF_SHORT :  GenericDataWarper::render<short> (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction);break;
    case CDF_USHORT:  GenericDataWarper::render<ushort>(&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction);break;
    case CDF_INT   :  GenericDataWarper::render<int>   (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction);break;
    case CDF_UINT  :  GenericDataWarper::render<uint>  (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction);break;
    case CDF_FLOAT :  GenericDataWarper::render<float> (&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction);break;
    case CDF_DOUBLE:  GenericDataWarper::render<double>(&warper,sourceData,&sourceGeo,srvParam->Geo,&settings,&drawFunction);break;
  }

  
  CPLErr gdalStatus = GDALRasterIO( hSrcBand, GF_Write, 0, 0,
                srvParam->Geo->dWidth,srvParam->Geo->dHeight,
                warpedData,
                srvParam->Geo->dWidth,srvParam->Geo->dHeight,
                datatype, 0, 0 );
  CDF::freeData(&warpedData);
  
  if( gdalStatus != CE_None ) {
    CDBError("GDALRasterIO failed");return 1;
  }
  
#ifdef CGDALDATAWRITER_DEBUG  
  CDBDebug("finished copying data in addData");
#endif
  
  
  {
    
    /* Band metadata */
    char **papszMetadata = NULL;
    
   
   
    CCDFDims *dims = _dataSource->getCDFDims();
    CT::string debugInfo;
    for(size_t d=0;d<dims->getNumDimensions();d++){
      CT::string dimName = dims->getDimensionName(d);
      if(dimName.equals("forecast_reference_time")==false){
        CT::string name = "NETCDF_DIM_";
        name.concat(dimName.c_str());
        CT::string value = getDimensionValue(d,dims);

        papszMetadata = CSLSetNameValue(papszMetadata, name.c_str(),value.c_str());
        
        debugInfo.printconcat("[%s,%s]",name.c_str(),value.c_str());
      }
    }
    
    //papszMetadata = CSLSetNameValue(papszMetadata, "NETCDF_DIM_%s", );
    papszMetadata = CSLSetNameValue(papszMetadata, "NETCDF_VARNAME", _dataSource->getDataObject(0)->cdfVariable->name.c_str());
    
//     papszMetadata = CSLSetNameValue(papszMetadata, "standard_name", "bla");
//     papszMetadata = CSLSetNameValue(papszMetadata, "units", "bla2");
    GDALSetMetadata( hSrcBand, papszMetadata,"");
    
    CSLDestroy( papszMetadata);papszMetadata = NULL;
  }
  // Get time
//   status = reader.getTimeString(dataSource,szTemp);
//   if(status==0)Times[currentBandNr].copy(szTemp);else Times[currentBandNr].copy("");
//   // Get filename
//   CT::string basename(dataSource->getFileName());
//   int offset = basename.lastIndexOf("/")+1;
//   if(offset<0)offset=0;if(offset>MAX_STR_LEN)offset=0;
//   InputProducts[currentBandNr].copy(basename.c_str()+offset);
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
  CT::string tmpFileName;
  bool writeToStdout = true;
  
  const char * pszADAGUCWriteToFile=getenv("ADAGUC_WRITETOFILE");      
  if(pszADAGUCWriteToFile != NULL){
    tmpFileName = pszADAGUCWriteToFile;
    writeToStdout = false;
    CDBDebug("Write to ADAGUC_WRITETOFILE %s",pszADAGUCWriteToFile);
  }else{
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
  }
  
  
  

  
  // Warp the image from destinationGDALDataSet to hMemDS1
  //GDALDataType eDT;
  //eDT = GDALGetRasterDataType(GDALGetRasterBand(destinationGDALDataSet,1));
  //CDBDebug("Check");
  // Get coordinate systems
  const char *pszSrcWKT;
  char *pszDstWKT = NULL;
  pszSrcWKT = GDALGetProjectionRef( destinationGDALDataSet );
  //CDBDebug("Check");
  //CPLAssert( pszSrcWKT != NULL && strlen(pszSrcWKT) > 0 );
  if(pszSrcWKT == NULL){
    CDBError("GDALGetProjectionRef( destinationGDALDataSet ) == NULL");return 1;
  }
  //CDBDebug("Check");
  OGRSpatialReference oSRS;
  CImageWarper imageWarper;
//  imageWarper.setSrvParam(srvParam);
  //imageWarper.decodeCRS(&srvParam->Geo->CRS);
  CT::string destinationCRS(&srvParam->Geo->CRS);
  //CDBDebug("Check %s",srvParam->Geo->CRS.c_str());
  //imageWarper.decodeCRS(&destinationCRS,&srvParam->Geo->CRS,&srvParam->cfg->Projection);
  //CDBDebug("Check");
  if(oSRS.SetFromUserInput(destinationCRS.c_str())!=OGRERR_NONE){
    CDBError("WCS: Invalid destination projection: [%s]",destinationCRS.c_str());
    return 1;
  }
  oSRS.exportToWkt( &pszDstWKT );
 


  
  {
    /* dataset metadata */
    char **papszMetadata = NULL;
    
    CT::string extraDimNames = "{";
      
    bool first = true;
    
    CCDFDims *dims = _dataSource->getCDFDims();
    for(size_t d=0;d<_dataSource->requiredDims.size();d++){
      CT::string dimName = "null";
      try{
        dimName = dims->getDimensionName(d);
      }catch(int e){
        CDBError("Exception code %d",e);throw e;
      }
      if(dimName.equals("forecast_reference_time")==false){
        if(first == false){
          extraDimNames.concat(",");
        }
        first = false;
  
        
        CDFType cdf_type = -1;
        try{
          cdf_type=_dataSource->getDataObject(0)->cdfObject->getVariable(dimName.c_str())->getType();
        }catch(int e){
          CDBDebug("Exception code %d for dimension name %s",e,dimName.c_str());
        }
        if(cdf_type!=-1){
  #ifdef CGDALDATAWRITER_DEBUG          
          CDBDebug("%s = %s",_dataSource->requiredDims[d]->netCDFDimName.c_str(),dimName.c_str());
  #endif
          try{
            extraDimNames.concat(dimName.c_str());
          }catch(int e){
            CDBError("Exception code %d",e);throw e;
          }
          CT::string dimDef;
          dimDef.print("{%d,%d}",_dataSource->requiredDims[d]->uniqueValues.size(),CDFNetCDFWriter::NCtypeConversion(cdf_type));
          CT::string key;
          try{
            key.print("NETCDF_DIM_%s_DEF",dimName.c_str());
          }catch(int e){
            CDBError("Exception code %d",e);throw e;
          }
          #ifdef CGDALDATAWRITER_DEBUG  
            CDBDebug("%s:%s",key.c_str(), dimDef.c_str());
          #endif    
          papszMetadata = CSLSetNameValue(papszMetadata, key.c_str(), dimDef.c_str());
        
          
          CT::string values = "{";
          std::set<std::string> myset;
          std::set<std::string>::iterator mysetit;
          CDBDebug("Nr Of timesteps : %d",_dataSource->timeSteps.size());
          for(size_t t=0;t<_dataSource->timeSteps.size();t++){
            try{
              CDBDebug("getDimensionValue %d %s",
                       d,
                       _dataSource->timeSteps[t]->dims.getDimensionName(0));
              myset.insert(getDimensionValue(d,&_dataSource->timeSteps[t]->dims).c_str());
              CDBDebug("getDimensionValue");
            }catch(int e){
              CDBError("Exception code %d",e);throw e;
            }
          }
          bool first = true;
          for (mysetit=myset.begin(); mysetit!=myset.end(); ++mysetit){
            if(first == false){
              values.concat(",");
            }
            first = false;
            values.concat((*mysetit).c_str());
          }
          values.concat("}");
          try{
            key.print("NETCDF_DIM_%s_VALUES",dimName.c_str());
          }catch(int e){
            CDBError("Exception code %d",e);throw e;
          }
          #ifdef CGDALDATAWRITER_DEBUG  
            CDBDebug("%s:%s",key.c_str(), values.c_str() );
          #endif    
          papszMetadata = CSLSetNameValue(papszMetadata, key.c_str(), values.c_str() );
        }
      }
    }
    extraDimNames.concat("}");

   
    if(extraDimNames.length()>2){
      #ifdef CGDALDATAWRITER_DEBUG  
        CDBDebug("%s:%s","NETCDF_DIM_EXTRA", extraDimNames.c_str() );
      #endif  
      papszMetadata = CSLSetNameValue(papszMetadata, "NETCDF_DIM_EXTRA", extraDimNames.c_str());
    }

    for(size_t j=0;j<_dataSource->metaDataItems.size();j++){
      CDataSource::KVP * kvp = &_dataSource->metaDataItems[j];
      CT::string attributekey;
      attributekey.printconcat("%s#%s",kvp->varname.c_str(),kvp->attrname.c_str());
      #ifdef CGDALDATAWRITER_DEBUG  
      CDBDebug("%s:%s",attributekey.c_str(),kvp->value.c_str());
      #endif
      papszMetadata = CSLSetNameValue(papszMetadata,attributekey.c_str(),kvp->value.c_str());
    }
    #ifdef CGDALDATAWRITER_DEBUG  
    CDBDebug("Setting metadata");
    #endif
    ((GDALDataset*)destinationGDALDataSet)->SetMetadata(papszMetadata);
    #ifdef CGDALDATAWRITER_DEBUG  
    CDBDebug("Destroying metadata");
    #endif
    CSLDestroy( papszMetadata);papszMetadata = NULL;
  }
  
  
  // Copy the destinationGDALDataSet dataset to the output driver
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
  
  //CDBDebug("TEST %s?",driverName.c_str());
  
  
  if(driverName.equalsIgnoreCase("AAIGRID")){
    CDBDebug("Setting FORCE_CELLSIZE to TRUE for AAIGRID");
    papszOptions = CSLSetNameValue( papszOptions, "FORCE_CELLSIZE", "TRUE" );
    //papszOptions = CSLSetNameValue( papszOptions, "GDAL_VALIDATE_CREATION_OPTIONS", "FALSE" );
    
  };

#ifdef CGDALDATAWRITER_DEBUG  
  CDBDebug("Copying destinationGDALDataSet to hOutputDriver");
#endif  
  hOutputDS = GDALCreateCopy( hOutputDriver, tmpFileName.c_str(), destinationGDALDataSet, FALSE, papszOptions, NULL, NULL );
  

#ifdef CGDALDATAWRITER_DEBUG      
  CDBDebug("GDALCreateCopy completed");
#endif
  if( hOutputDS == NULL ){
    CDBError("WriteGDALRaster: Failed to create output file:<br>\n\"%s\"",tmpFileName.c_str());
    CDBError("LastErrorMsg: %s",CPLGetLastErrorMsg());
    GDALClose( destinationGDALDataSet );
    return 1;
  }
  CSLDestroy( papszOptions );

  GDALClose( hOutputDS );
  GDALClose( destinationGDALDataSet );
//   GDALClose( destinationGDALDataSet );
  
  
  
  if(writeToStdout == false){
    return 0;
  }

  /* Output the file to stdout */
  
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
    printf("Content-Disposition: attachment; filename=%s\r\n",generateGetCoverageFileName().c_str());
    printf("Content-Description: File Transfer\r\n");
    printf("Content-Transfer-Encoding: binary\r\n");
    printf("Content-Length: %zu\r\n",endPos); 
    printf("%s\r\n\r\n",mimeType.c_str());
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

  if(InputProducts!=NULL){delete[] InputProducts;}InputProducts=NULL;
//   if(Times!=NULL)delete[] Times;Times=NULL;
  return returnCode;
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
  char szRandomPart[MAX_STR_LEN];
  char szTemp[MAX_STR_LEN+1];
  generateString(szRandomPart,12);
  snprintf(pszTempFileName,MAX_STR_LEN,
           "FORMAT--_VARIABLENAME_BBOX0_BBOX2_BBOX3_BBOX4_WIDTH_HEIGH_RESX-_RESY-_CONFIG--_DIM_DIM_DIM_PROJECTION_RAND------------______.tmp");
  //CDBDebug("generateUniqueGetCoverageFileName");
  for(int j=0;j<118;j++){pszTempFileName[j]='_';};pszTempFileName[MAX_STR_LEN]='\0';
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

  
CT::string CGDALDataWriter::getDimensionValue(int d,CCDFDims *dims){
  
  CT::string value;
  if(dims->isTimeDimension(d)){
    CTime adagucTime;
    try{
      value = "0";
      adagucTime.init(TimeUnit.c_str(),NULL);//TODO replace with var
      double offset = adagucTime.dateToOffset(adagucTime.ISOStringToDate(dims->getDimensionValue(d).c_str()));
      value.print("%f",offset);
    }catch(int e){
      CDBDebug("Warning in getDimensionValue: Unable to get string value from time dimension");
    }
           
  }else{
    value.print("%s",dims->getDimensionValue(d).c_str());
  }
         CDBDebug("Continuing %s",value.c_str());
         return value;
}
#endif
