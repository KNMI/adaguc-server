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

#include "CDataReader.h"
#include <math.h>
#include <float.h>
#include "CConvertASCAT.h"
#include "CConvertUGRIDMesh.h"
#include "CConvertADAGUCVector.h"
#include "CConvertADAGUCPoint.h"
#include "CConvertCurvilinear.h"
#include "CConvertHexagon.h"
#include "CConvertGeoJSON.h"
#include "CConvertEProfile.h"
#include "CConvertTROPOMI.h"
#include "CDBFactory.h"
#include "CReporter.h"
#include "CCDFHDF5IO.h"
#include "CDBFileScanner.h"
const char *CDataReader::className="CDataReader";


// #define CDATAREADER_DEBUG
// #define MEASURETIME

#define uchar unsigned char
#define MAX_STR_LEN 8191

class Proc{
  public:
    DEF_ERRORFUNCTION();
    static int swapPixelsAtLocation(CDataSource *dataSource,CDF::Variable *variable,int mode){
      if(dataSource->useLonTransformation == -1)return 0;
      switch (variable->getType()){
      case CDF_CHAR  : _swapPixelsAtLocation<char>        (dataSource,variable,mode);break;
      case CDF_BYTE  : _swapPixelsAtLocation<char>        (dataSource,variable,mode);break;
      case CDF_UBYTE : _swapPixelsAtLocation<uchar>       (dataSource,variable,mode);break;
      case CDF_SHORT : _swapPixelsAtLocation<short>       (dataSource,variable,mode);break;
      case CDF_USHORT: _swapPixelsAtLocation<ushort>      (dataSource,variable,mode);break;
      case CDF_INT   : _swapPixelsAtLocation<int>         (dataSource,variable,mode);break;
      case CDF_UINT  : _swapPixelsAtLocation<unsigned int>(dataSource,variable,mode);break;
      case CDF_FLOAT : _swapPixelsAtLocation<float>       (dataSource,variable,mode);break;
      case CDF_DOUBLE: _swapPixelsAtLocation<double>      (dataSource,variable,mode);break;
      default: {CDBError("Unknown data type"); return 1;}
    }
    return 0;
  }
  private:
  template <class T>
  static void _swapPixelsAtLocation(CDataSource *dataSource,CDF::Variable *variable,int mode){

#ifdef CDATAREADER_DEBUG
      CDBDebug("Applying LON warp to -180 till 180 on the original data");
#endif

      dataSource->dWidth=int(360.0/dataSource->dfCellSizeX);

      double left =  ((double*)dataSource->varX->data)[0]-dataSource->dfCellSizeX/2;
      double right = ((double*)dataSource->varX->data)[dataSource->varX->getSize()-1]+dataSource->dfCellSizeX/2;



      dataSource->dfBBOX[0] = left;
      dataSource->dfBBOX[2] = right;
      #ifdef CDATAREADER_DEBUG
      CDBDebug("Old bbox = %f %f",dataSource->dfBBOX[0],dataSource->dfBBOX[2]);
#endif
      while( dataSource->dfBBOX[0]>-180){dataSource->dfBBOX[0]-=dataSource->dfCellSizeX;}
      while(dataSource->dfBBOX[0]<-180){ dataSource->dfBBOX[0]+=dataSource->dfCellSizeX;}
      dataSource->dfBBOX[2]=dataSource->dfBBOX[0]+360;
#ifdef CDATAREADER_DEBUG
      CDBDebug("New bbox = %f %f",dataSource->dfBBOX[0],dataSource->dfBBOX[2]);
#endif



      if(dataSource->getDataObject(0)->hasNodataValue == false){
        dataSource->getDataObject(0)->hasNodataValue = true;
        dataSource->getDataObject(0)->dfNodataValue = INFINITY;

      }

    if(mode == 0){return;}



    double origBBOXWidth = (right - left) ;
    size_t imageSize = size_t(dataSource->dWidth)*size_t(dataSource->dHeight);
    #ifdef CDATAREADER_DEBUG
    CDBDebug(" Image size = %dx%d", dataSource->dWidth,dataSource->dHeight);
#endif
    if(imageSize==0)imageSize=1;
    if(variable->data == NULL)return;
    
    size_t origSize = size_t(dataSource->dOrigWidth)*size_t(dataSource->dHeight);
    if (imageSize < origSize){
      CDBError("Unable to apply LON warp to -180 till 180 on the original data: imageSize < origSize");
      return;
    }

    T*data = (T*) variable->data;
    T *tempData = new T[imageSize];
    for(size_t j=0;j<origSize;j++){
      tempData[j]=data[j];
    }

    variable->freeData();
    variable->allocateData(imageSize);

    data = (T*) variable->data;
    for(size_t j=0;j<imageSize;j++){
      data[j]=(T)dataSource->getDataObject(0)->dfNodataValue;
    }

    for(int y=0;y<dataSource->dHeight;y++){
      for(int x=0;x<dataSource->dOrigWidth;x=x+1){
        double lonX=((double(x)/double(dataSource->dOrigWidth))*origBBOXWidth)+left;
        while(lonX<-180)lonX+=360;
        while(lonX>=180)lonX-=360;

        int newXIndex = int(floor((((lonX-dataSource->dfBBOX[0])/360))*double(dataSource->dWidth)+0.5));
        T value = tempData[x+y*dataSource->dOrigWidth];
        if(newXIndex>=0&&newXIndex<dataSource->dWidth){
          data[newXIndex+y*dataSource->dWidth]=value;
        }
      }
    }
#ifdef CDATAREADER_DEBUG
    CDBDebug("Done");
#endif
    delete[] tempData;

    dataSource->lonTransformDone = true;
  }
};

const char *Proc::className="Proc";

int CDataReader::open(CDataSource *dataSource, int mode){
  return open(dataSource,mode,-1,-1);
}
int CDataReader::open(CDataSource *dataSource, int x,int y){
  return open(dataSource,CNETCDFREADER_MODE_OPEN_ALL,x,y);
}

int CDataReader::getCRS(CDataSource *dataSource) {

  //Check if projection is overidden in the config file.
  if (copyCRSFromConfigToDataSource(dataSource)){
    return 0;
  }

  //Check if we can copy the CRS from a projection variable.
  if (copyCRSFromProjectionVariable(dataSource)) {
    return 0;
  }

  // If undefined, set standard lat lon projection
  CREPORT_WARN_NODOC(CT::string("No correct projection found, using by default the geographic coordinate system (latitude and longitude)."), CReportMessage::Categories::GENERAL);
  copyLatLonCRS(dataSource);
  return 0;
}

bool CDataReader::copyCRSFromConfigToDataSource(CDataSource *dataSource) const {

  // Check if the projection is defined in the config file.
  if(dataSource->cfgLayer->Projection.size() != 1) {
    return false;
  }

  CREPORT_INFO_NODOC("Projection is obtained from ADAGUC config file", CReportMessage::Categories::GENERAL);

  //Read the EPSG-code from configuration.
  if(dataSource->cfgLayer->Projection[0]->attr.id.empty() == false) {
    dataSource->nativeEPSG.copy(dataSource->cfgLayer->Projection[0]->attr.id.c_str());
  } else {
    CT::string defaultEPSGCode = "EPSG:4326";
    CREPORT_WARN_NODOC(CT::string("Projection id not in config, using default value ") + defaultEPSGCode, CReportMessage::Categories::GENERAL);
    dataSource->nativeEPSG.copy(defaultEPSGCode);
  }

  //Read proj4 string from configuration.
  if(dataSource->cfgLayer->Projection[0]->attr.proj4.empty() == false) {
    dataSource->nativeProj4.copy(dataSource->cfgLayer->Projection[0]->attr.proj4.c_str());
  }
  else {
    CT::string defaultProj4String = "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs";
    CREPORT_WARN_NODOC(CT::string("Proj4 string not in config, using default value ") + defaultProj4String, CReportMessage::Categories::GENERAL);
    dataSource->nativeProj4.copy(defaultProj4String);
  }

  return true;
}

void CDataReader::copyLatLonCRS(CDataSource *dataSource) const {
  CREPORT_INFO_NODOC(CT::string("Using the geographic coordinate system (latitude and longitude)"), CReportMessage::Categories::GENERAL);
  dataSource->nativeProj4.copy("+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs");
  dataSource->nativeEPSG.copy("EPSG:4326");
}

bool CDataReader::copyCRSFromProjectionVariable(CDataSource *dataSource) const {

  // Determine if there is a grid mapping attribute is defined.
  CDF::Variable * dataSourceVar=dataSource->getDataObject(0)->cdfVariable;
  CDF::Attribute *projvarnameAttr = dataSourceVar->getAttributeNE("grid_mapping");

  if (projvarnameAttr==NULL) {
    return false;
  }

  // Determine if the grid mapping attribute is set to latitude_longitude.
  if (projvarnameAttr!=NULL && projvarnameAttr->toString().equals("latitude_longitude")) {
    copyLatLonCRS(dataSource);
    return true;
  }

  // Retrieve the projection variable.
  CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
  CDF::Variable *projVar = cdfObject->getVariableNE(projvarnameAttr->toString().c_str());
  if(projVar == NULL) {
    CREPORT_WARN_NODOC(CT::string("The projection variable name defined in grid_mapping attribute is not found: ") + projvarnameAttr->toString(), CReportMessage::Categories::GENERAL);
    return false;
  }

  // Try to retrieve the projection according to the ADAGUC metadata standard.
  if (copyCRSFromADAGUCProjectionVariable(dataSource, projVar)) {
    return true;
  }

  // Try to retrieve the projection according to the Climate and Forecast Conventions.
  if (copyCRSFromCFProjectionVariable(dataSource, projVar)) {
    return true;
  }

  // It was not possible to determine the projection from the projection variable.
  return false;

}

bool CDataReader::copyCRSFromADAGUCProjectionVariable(CDataSource *dataSource, const CDF::Variable *projVar) const {
  //Get proj4_params according to ADAGUC metadata
  CDF::Attribute *proj4Attr = projVar->getAttributeNE("proj4_params");

  //If not found try alternative one
  if(proj4Attr == NULL) proj4Attr = projVar->getAttributeNE("proj4");

  // If not found, the projection is not defined according to the ADAGUC metadata standard.
  if (proj4Attr == NULL) {
    return false;
  }

  if (proj4Attr->toString().length() == 0) {
    CREPORT_WARN_NODOC(CT::string("Found a proj4 or proj4_params attribute, but it is empty. Skipping the attribute."), CReportMessage::Categories::GENERAL);
    return false;
  }

  if (this->_enableReporting) {
    CREPORT_INFO_NODOC(CT::string("Retrieving the projection according to the ADAGUC standards from the proj4_params or proj4 attribute: ") + proj4Attr->toString(), CReportMessage::Categories::GENERAL);
  }
  dataSource->nativeProj4.copy(proj4Attr->toString().c_str());

  // Copy the EPSG code.
  copyEPSGCodeFromProjectionVariable(dataSource, projVar);

  return true;
}

bool CDataReader::copyCRSFromCFProjectionVariable(CDataSource *dataSource, CDF::Variable *projVar) const {

  CProj4ToCF proj4ToCF;
  proj4ToCF.debug = true;
  CT::string projString;
  int status = proj4ToCF.convertCFToProj(projVar, &projString);

  if (status != 0) {
    CREPORT_WARN_NODOC(CT::string("Unknown CF conventions projection."), CReportMessage::Categories::GENERAL);
    return false;
  }

  //Projection string was created, set it in the datasource.
  CREPORT_INFO_NODOC(CT::string("Determined the projection string using the CF conventions: ") + projString, CReportMessage::Categories::GENERAL);
  dataSource->nativeProj4.copy(projString.c_str());
  projVar->setAttributeText("autogen_proj", projString.c_str());

  // Copy the EPSG code.
  copyEPSGCodeFromProjectionVariable(dataSource, projVar);

  return true;
}


void CDataReader::copyEPSGCodeFromProjectionVariable(CDataSource *dataSource, const CDF::Variable *projVar) const {
  //Get EPSG_code
  CDF::Attribute *epsgAttr = projVar->getAttributeNE("EPSG_code");
  if(epsgAttr != NULL) {
    CREPORT_INFO_NODOC(CT::string("Using EPSG_code defined in projection variable ") + projVar->name, CReportMessage::Categories::GENERAL);
    dataSource->nativeEPSG.copy((char *) epsgAttr->data);
  } else {
    //Make a projection code based on PROJ4: namespace
    if (this->_enableReporting) {
      CREPORT_INFO_NODOC(CT::string("Using projection string to create EPSG code.") + dataSource->nativeProj4, CReportMessage::Categories::GENERAL);
    }
    dataSource->nativeEPSG.print("PROJ4:%s", dataSource->nativeProj4.c_str());
    dataSource->nativeEPSG.replaceSelf("\"", "");
    dataSource->nativeEPSG.replaceSelf("\n", "");
    dataSource->nativeEPSG.trimSelf();
    dataSource->nativeEPSG.encodeURLSelf();
  }
}

int CDataReader::parseDimensions(CDataSource *dataSource,int mode,int x, int y, int *gridExtent){

  /**************************************************************************************************/
  /*  Check if the format needs to be converted. */
  /**************************************************************************************************/
  dataSource->formatConverterActive = false;
  if(!dataSource->formatConverterActive)if(CConvertEProfile::convertEProfileData(dataSource,mode)==0)dataSource->formatConverterActive=true;
  if(!dataSource->formatConverterActive)if(CConvertASCAT::convertASCATData(dataSource,mode)==0)dataSource->formatConverterActive=true;
  if(!dataSource->formatConverterActive)if(CConvertUGRIDMesh::convertUGRIDMeshData(dataSource,mode)==0)dataSource->formatConverterActive=true;
  if(!dataSource->formatConverterActive)if(CConvertADAGUCVector::convertADAGUCVectorData(dataSource,mode)==0)dataSource->formatConverterActive=true;
  if(!dataSource->formatConverterActive)if(CConvertADAGUCPoint::convertADAGUCPointData(dataSource,mode)==0)dataSource->formatConverterActive=true;
  if(!dataSource->formatConverterActive)if(CConvertCurvilinear::convertCurvilinearData(dataSource,mode)==0)dataSource->formatConverterActive=true;
  if(!dataSource->formatConverterActive)if(CConvertHexagon::convertHexagonData(dataSource,mode)==0)dataSource->formatConverterActive=true;
  if(!dataSource->formatConverterActive)if(CConvertGeoJSON::convertGeoJSONData(dataSource,mode)==0)dataSource->formatConverterActive=true;
  if(!dataSource->formatConverterActive)if(CConvertTROPOMI::convertTROPOMIData(dataSource,mode)==0)dataSource->formatConverterActive=true;

  CDF::Variable * dataSourceVar=dataSource->getDataObject(0)->cdfVariable;
  CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;

  if(dataSource->cfgLayer->Dimension.size()==0){
    #ifdef CDATAREADER_DEBUG
    CDBDebug("Auto configuring dims");
    #endif
    if(CAutoConfigure::autoConfigureDimensions(dataSource)!=0){
      CDBError("Unable to configure dimensions automatically");
      return 1;
    }
  }

  bool singleCellMode = false;

  if(x!=-1&&y!=-1){
    singleCellMode = true;
  }

  // -------------------------------------------
  // Retrieve X, Y Dimensions and Width, Height |
  // -------------------------------------------
  dataSource->dNetCDFNumDims = dataSourceVar->dimensionlinks.size();

  if(dataSource->dNetCDFNumDims<2){
    CREPORT_ERROR_NODOC(CT::string("The following variable has less than two dimensions, while at least x and y dimensions are required: ") + dataSourceVar->name.c_str(), CReportMessage::Categories::GENERAL);
    return 1;
  }

  #ifdef CDATAREADER_DEBUG
  CDBDebug("Number of dimensions = %d",dataSource->dNetCDFNumDims);
  #endif

  // Determine the X and Y dimensions and variables.
  determineXAndYDimIndices(dataSource, dataSourceVar);
  if(!determineXandYVars(dataSource, dataSourceVar, cdfObject)) {
    return 1;
  }

  // Determine the width and height based on the stride.
  determineStride2DMap(dataSource);
  determineDWidthAndDHeight(dataSource, singleCellMode, gridExtent, mode);

  // ----------------------------------------------------------------------------------------------------
  // Read X and Y variable data. NOTE: For converted files this has already been done in the converters. |
  // ----------------------------------------------------------------------------------------------------
  if(dataSource->formatConverterActive == false){

    size_t start[dataSource->dNetCDFNumDims+1]; // Saves the start position of the dimension variable data.

    //Determine the indices of the data. Everything starts at 0 by default.
    for(int j=0;j<dataSource->dNetCDFNumDims;j++){start[j]=0;}

    if( mode == CNETCDFREADER_MODE_OPEN_EXTENT && gridExtent != NULL ){
      start[dataSource->dimXIndex] = gridExtent[0];
      start[dataSource->dimYIndex] = gridExtent[1];
    }

    if(gridExtent != NULL){
      //TODO We are using a different start/count, but the data is not re-read until it is freed first.
      dataSource->varX->freeData();
      dataSource->varY->freeData();
    }

    // Read the X data.
    size_t sta[1],sto[1];ptrdiff_t str[1];
    sta[0]=start[dataSource->dimXIndex];str[0]=dataSource->stride2DMap; sto[0]=dataSource->dWidth;
    if(singleCellMode) {sta[0]=0;str[0]=1;sto[0]=2;}

    #ifdef CDATAREADER_DEBUG
    CDBDebug("[%d %d %d] for %s/%s",sta[0],str[0],sto[0],dataSourceVar->name.c_str(),dataSource->varX->name.c_str());
    #endif

    int statusX = dataSource->varX->readData(CDF_DOUBLE,sta,sto,str,true);
    if(statusX!=0){
      CREPORT_ERROR_NODOC(CT::string("Not possible to read data for dimension ") + dataSource->varX->name, CReportMessage::Categories::GENERAL);
      return 1;
    }

    // Read the Y data
    sta[0]=start[dataSource->dimYIndex];str[0]=dataSource->stride2DMap; sto[0]=dataSource->dHeight;
    if(singleCellMode){
      sta[0]=0;str[0]=1;sto[0]=2;
    }

    #ifdef CDATAREADER_DEBUG
    CDBDebug("[%d %d %d] for %s/%s",sta[0],str[0],sto[0],dataSourceVar->name.c_str(),dataSource->varY->name.c_str());
    #endif

    int statusY = dataSource->varY->readData(CDF_DOUBLE,sta,sto,str,true);
    if(statusY!=0){
      CREPORT_ERROR_NODOC(CT::string("Not possible to read data for dimension ") + dataSource->varY->name, CReportMessage::Categories::GENERAL);
      for(size_t j=0;j<dataSource->varY->dimensionlinks.size();j++){
        CDBDebug("For var %s, reading dim %s of size %d (%d %d %d)", dataSource->varY->name.c_str(),dataSource->varY->dimensionlinks[j]->name.c_str(),dataSource->varY->dimensionlinks[j]->getSize(),sta[j],sto[j],str[j]);
      }
      return 1;
    }

    // Get CRS info DataObject. Note: It is not necessary to call this for converted files, since the CRS info should be set correctly in the converters.
    getCRS(dataSource);

    //if units="rad" and grid_mapping variable contains perspective_point_height attribute
    //coordinates can be converted to (k)m
    //
    CDF::Attribute* units=dataSource->varX->getAttributeNE("units");

    if (units!=NULL) {
      const CT::string unitString = units->toString();
      if(unitString.equals("rad") || unitString.equals("radian")){
        CDBDebug("units: %s", units->toString().c_str());
        CDBDebug("Correct varX and varY");
        double *xdata=(double*)dataSource->varX->data;
        double *ydata=(double*)dataSource->varY->data;
        CDF::Attribute *projvarnameAttr = dataSourceVar->getAttributeNE("grid_mapping");
        if(projvarnameAttr!=NULL){
          CDF::Variable * projVar = cdfObject->getVariableNE(projvarnameAttr->toString().c_str());
          if(projVar==NULL){
            CDBWarning("projection variable '%s' not found",(char*)projvarnameAttr->data);
          } else {
            CDF::Attribute *grid_mapping_name = projVar->getAttributeNE("grid_mapping_name");
            if ((grid_mapping_name!=NULL) && grid_mapping_name->toString().equals("geostationary")) {
              //Get perspective_height
              CDF::Attribute *perspectiveHeightAttr = projVar->getAttributeNE("perspective_point_height");
              double perspectiveHeight=35786000.;
              if (perspectiveHeightAttr!=NULL) {
                try{
                  perspectiveHeight = perspectiveHeightAttr->toString().toDouble();
                }catch(int e){
                  CDBDebug("Falling back to default perspective_point_height: 35786000");
                }
              }
              // TODO: Dit moet voor alle data gebeuren toch, niet een subset van de data?
              sta[0]=start[dataSource->dimXIndex];str[0]=dataSource->stride2DMap; sto[0]=dataSource->dWidth;
              if(singleCellMode){sta[0]=0;str[0]=1;sto[0]=2;}
              for (size_t j=sta[0];j<sto[0];j+=str[0]) {
                xdata[j]=xdata[j]*perspectiveHeight;
              }

              sta[0]=start[dataSource->dimYIndex];str[0]=dataSource->stride2DMap; sto[0]=dataSource->dHeight;
              if(singleCellMode){sta[0]=0;str[0]=1;sto[0]=2;}
              for (size_t j=sta[0];j<sto[0];j+=str[0]) {
                ydata[j]=ydata[j]*perspectiveHeight;
              }
            }
          }
        }
      }
    }
  }

  // Calculate cellsize and BBOX based on read X,Y dims.
  if (!calculateCellSizeAndBBox(dataSource, dataSourceVar)) {
    return 1;
  }

#ifdef MEASURETIME
  StopWatch_Stop("XY dimensions read");
#endif

  #ifdef CDATAREADER_DEBUG
  CDBDebug("PROJ4 = [%s]",dataSource->nativeProj4.c_str());
  #endif

  // Lon transformation is used to swap datasets from 0-360 degrees to -180 till 180 degrees
  //Swap data from >180 degrees to domain of -180 till 180 in case of lat lon source data
    dataSource->useLonTransformation = -1;

  if(singleCellMode == true){

    return 0;
  }
  if(dataSource->srvParams->requestType==REQUEST_WMS_GETFEATUREINFO||dataSource->srvParams->requestType==REQUEST_WMS_GETPOINTVALUE){

    return 0;
  }



  if(dataSource->formatConverterActive==false && dataSource->dfCellSizeX > 0){


    if( CGeoParams::isLonLatProjection(&dataSource->nativeProj4)){
      //If the lon variable name contains an X, it is probably not longitude.
      if(dataSource->varX->name.indexOf("x")==-1&&dataSource->varX->name.indexOf("X")==-1){
        size_t j=0;
        for(j=0;j<dataSource->varX->getSize();j++){
          //CDBDebug("%d == %f",j,((double*)dataSource->varX->data)[j]);
          double xvalue = ((double*)dataSource->varX->data)[j];

          if(xvalue>=180.0&&xvalue<400)break;
          if(xvalue<=-180.0&&xvalue>-400)break;
        }
        if(j!=dataSource->varX->getSize()){
          dataSource->useLonTransformation=j;
        }
      }

    }
  }

  return 0;
}

void CDataReader::determineXAndYDimIndices(CDataSource *dataSource, const CDF::Variable *dataSourceVar) const {

  dataSource->dimXIndex=dataSource->dNetCDFNumDims-1;
  dataSource->dimYIndex=dataSource->dNetCDFNumDims-2;

  dataSource->swapXYDimensions = false;

  //If our X dimension has a character y/lat in it, XY dims are probably swapped.
  CT::string dimensionXName=dataSourceVar->dimensionlinks[dataSource->dimXIndex]->name.c_str();
  CT::string dimensionYName=dataSourceVar->dimensionlinks[dataSource->dimYIndex]->name.c_str();

  dimensionXName.toLowerCaseSelf();
  if(dimensionXName.indexOf("y")!=-1||dimensionXName.indexOf("lat")!=-1) {
    dataSource->swapXYDimensions=true;
    dataSource->dimXIndex=dataSource->dNetCDFNumDims-2;
    dataSource->dimYIndex=dataSource->dNetCDFNumDims-1;

    if (this->_enableReporting) {
      CREPORT_WARN_NODOC(CT::string("For variable ") + dataSourceVar->name +
                                           CT::string(" the dimension on the X position, ") + dimensionXName +
                                           CT::string(", contains 'y' or 'lat' in its name, and is therefore swapped with the dimension on the Y position, ") + dimensionYName, CReportMessage::Categories::GENERAL);
    }
  }

  CDF::Dimension *dimX=dataSourceVar->dimensionlinks[dataSource->dimXIndex];
  CDF::Dimension *dimY=dataSourceVar->dimensionlinks[dataSource->dimYIndex];
  if (this->_enableReporting) {
    CREPORT_INFO_NODOC(CT::string("Assuming that for variable ") + dataSourceVar->name +
                                      CT::string(" the x dim equals ") + dimX->name +
                                      CT::string(" and the y dim equals ") + dimY->name +
                                      CT::string(" based on their position and name."), CReportMessage::Categories::GENERAL);
  }
}

bool CDataReader::determineXandYVars(CDataSource *dataSource, const CDF::Variable *dataSourceVar, CDFObject *cdfObject) const {
  CDF::Dimension *dimX = dataSourceVar->dimensionlinks[dataSource->dimXIndex];
  CDF::Dimension *dimY = dataSourceVar->dimensionlinks[dataSource->dimYIndex];

  if(dimX == NULL || dimY == NULL) {
    CREPORT_ERROR_NODOC(CT::string("X and or Y dims not found."), CReportMessage::Categories::GENERAL);
    return false;
  }

  //Read X and Y dimension data completely.
  dataSource->varX = cdfObject->getVariableNE(dimX->name.c_str());
  dataSource->varY = cdfObject->getVariableNE(dimY->name.c_str());
  if(dataSource->varX == NULL || dataSource->varY == NULL) {
    CREPORT_ERROR_NODOC(
        CT::string("Not possible to find variable for dimensions with names ") + dimX->name +
        CT::string(" and ") + dimY->name +
        CT::string(" for variable ") + dataSourceVar->name, CReportMessage::Categories::GENERAL);
    return false;
  }
  if (this->_enableReporting) {
    CREPORT_INFO_NODOC(
        CT::string("Using variable ") + dataSource->varX->name +
        CT::string(" as X variable and variable ") + dataSource->varY->name +
        CT::string(" as Y variable."), CReportMessage::Categories::GENERAL);
  }
  return true;
}

void CDataReader::determineStride2DMap(CDataSource *dataSource) const {

  if(dataSource->formatConverterActive) {
    dataSource->stride2DMap = 1;
    CREPORT_INFO_NODOC(CT::string("In level 2 compatibility mode, using a default stride of 1, not considering RenderSettings."), CReportMessage::Categories::GENERAL);
    return;
  }

  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  if(styleConfiguration != NULL && styleConfiguration->styleConfig != NULL) {
    if(styleConfiguration->styleConfig->RenderSettings.size() == 1) {
      if((styleConfiguration->styleConfig->RenderSettings[0])->attr.striding.empty() == false) {
        dataSource->stride2DMap = styleConfiguration->styleConfig->RenderSettings[0]->attr.striding.toInt();
        CREPORT_INFO_NODOC(CT::string("Determined a stride of ") +
                                              styleConfiguration->styleConfig->RenderSettings[0]->attr.striding +
                                              CT::string(" based on RenderSettings."), CReportMessage::Categories::GENERAL);
        return;
      }
    }
  }

  dataSource->stride2DMap = 1;
  if (this->_enableReporting) {
    CREPORT_INFO_NODOC(CT::string("No stride defined in the RenderSettings, using a default stride of 1."), CReportMessage::Categories::GENERAL);
  }
  return;
}

void CDataReader::determineDWidthAndDHeight(CDataSource *dataSource, const bool singleCellMode, const int *gridExtent, int mode) const {

  // Determine the width and height based on dimension length and stride.
  CDF::Variable * dataSourceVar=dataSource->getDataObject(0)->cdfVariable;
  CDF::Dimension *dimX=dataSourceVar->dimensionlinks[dataSource->dimXIndex];
  CDF::Dimension *dimY=dataSourceVar->dimensionlinks[dataSource->dimYIndex];
  dataSource->dWidth = dimX->length / dataSource->stride2DMap;
  dataSource->dHeight = dimY->length / dataSource->stride2DMap;
  dataSource->dOrigWidth = dataSource->dWidth;

  #ifdef CDATAREADER_DEBUG
  if(gridExtent!=NULL){
      CDBDebug("gridExtent = [%d %d %d %d]", gridExtent[0], gridExtent[1], gridExtent[2], gridExtent[3]);
  }else{
      CDBDebug("gridExtent = NULL");
  }
  #endif

  // Check if we need to apply a gridExtent.
  if(mode == CNETCDFREADER_MODE_OPEN_EXTENT && gridExtent != NULL) {
    if (this->_enableReporting) {
      CREPORT_INFO_NODOC(CT::string("Determining the width based on the given gridExtent instead of dimension lengths."), CReportMessage::Categories::GENERAL);
    }
    dataSource->dWidth = (gridExtent[2] - gridExtent[0]) / dataSource->stride2DMap;
    dataSource->dHeight = (gridExtent[3] - gridExtent[1]) / dataSource->stride2DMap;
  }

  // Check if we operate in single cell mode.
  if(singleCellMode) {
    CREPORT_INFO_NODOC(CT::string("Running in single cell mode, setting width and height equal to 2."), CReportMessage::Categories::GENERAL);
    dataSource->dWidth = 2;
    dataSource->dHeight = 2;
  }
}

bool CDataReader::calculateCellSizeAndBBox(CDataSource *dataSource, const CDF::Variable *dataSourceVar) const {

  double *dfdim_X=(double*)dataSource->varX->data;
  double *dfdim_Y=(double*)dataSource->varY->data;

  if(dfdim_X == NULL || dfdim_Y == NULL){
    CREPORT_ERROR_NODOC(
        CT::string("No data available for ") + dataSource->varX->name +
        CT::string(" and ") + dataSource->varY->name +
        CT::string(" dimensions."), CReportMessage::Categories::GENERAL);
    return false;
  }

  dataSource->dfCellSizeX=(dfdim_X[dataSource->dWidth-1]-dfdim_X[0])/double(dataSource->dWidth-1);
  dataSource->dfCellSizeY=(dfdim_Y[dataSource->dHeight-1]-dfdim_Y[0])/double(dataSource->dHeight-1);

  CT::string dimensionXName=dataSourceVar->dimensionlinks[dataSource->dimXIndex]->name.c_str();
  dimensionXName.toLowerCaseSelf();
  if(dimensionXName.equals("col")){
    CREPORT_WARN_NODOC(CT::string("X dimension name equals 'col', bounding box is calculated differently."), CReportMessage::Categories::GENERAL);
    dataSource->dfBBOX[2]=dfdim_X[0]-dataSource->dfCellSizeX/2.0f;
    dataSource->dfBBOX[3]=dfdim_Y[dataSource->dHeight-1]+dataSource->dfCellSizeY/2.0f;
    dataSource->dfBBOX[0]=dfdim_X[dataSource->dWidth-1]+dataSource->dfCellSizeX/2.0f;
    dataSource->dfBBOX[1]=dfdim_Y[0]-dataSource->dfCellSizeY/2.0f;
  } else {
    dataSource->dfBBOX[0]=dfdim_X[0]-dataSource->dfCellSizeX/2.0f;
    dataSource->dfBBOX[1]=dfdim_Y[dataSource->dHeight-1]+dataSource->dfCellSizeY/2.0f;
    dataSource->dfBBOX[2]=dfdim_X[dataSource->dWidth-1]+dataSource->dfCellSizeX/2.0f;
    dataSource->dfBBOX[3]=dfdim_Y[0]-dataSource->dfCellSizeY/2.0f;
  }

  dataSource->origBBOXLeft = dataSource->dfBBOX[0];
  dataSource->origBBOXRight = dataSource->dfBBOX[2];

  return true;
}


pthread_mutex_t CDataReader_open_lock;

int CDataReader::open(CDataSource *dataSource,int mode,int x,int y){
  return open(dataSource, mode, x, y, NULL);
}
int CDataReader::openExtent(CDataSource *dataSource,int mode,int *gridExtent){
  return open(dataSource, mode, -1, -1, gridExtent);
}
int CDataReader::open(CDataSource *dataSource,int mode,int x,int y, int *gridExtent){

  //Perform some checks on pointers
  if(dataSource==NULL){CDBError("Invalid dataSource");return 1;}
  if(dataSource->getFileName()==NULL){CDBError("Invalid NetCDF filename (NULL)");return 1;}

  #ifdef CDATAREADER_DEBUG
    CDBDebug("Open mode:%d x:%d y:%d",mode,x,y);
#endif



  bool singleCellMode = false;

  if(x!=-1&&y!=-1){
    singleCellMode = true;
  }


  CT::string dataSourceFilename;
  dataSourceFilename.copy(dataSource->getFileName());


  //CCache cache;
  //CT::string cacheFilename;
  //pthread_mutex_lock(&CDataReader_open_lock);
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  //pthread_mutex_unlock(&CDataReader_open_lock);
  //Use autoscale of legendcolors when the legendscale factor has been set to zero.
  if(styleConfiguration != NULL){
    if(dataSource->stretchMinMaxDone == false){
      if(styleConfiguration->legendScale==0.0f && !(styleConfiguration->renderMethod&RM_RGBA)&& !(styleConfiguration->renderMethod&RM_AVG_RGBA))dataSource->stretchMinMax=true;else dataSource->stretchMinMax=false;
    }
  }

  CDFObject *cdfObject = NULL;

#ifdef CDATAREADER_DEBUG
  CDBDebug("Working on [%s] with mode %d and (%d,%d)",dataSourceFilename.c_str(),mode,x,y);
#endif

  if (mode == CNETCDFREADER_MODE_OPEN_ALL){
    CDBDebug("Working on [%s]", dataSourceFilename.c_str());
  }
  if(mode == CNETCDFREADER_MODE_OPEN_DIMENSIONS  || mode == CNETCDFREADER_MODE_OPEN_HEADER ){
      //pthread_mutex_lock(&CDataReader_open_lock);
    cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeader(dataSource, dataSource->srvParams,dataSourceFilename.c_str());
     //pthread_mutex_unlock(&CDataReader_open_lock);

//     enableDataCache = false;
  }

  if(mode == CNETCDFREADER_MODE_OPEN_ALL|| mode == CNETCDFREADER_MODE_GET_METADATA || mode == CNETCDFREADER_MODE_OPEN_EXTENT){
   // CDBDebug("Working on [%s] with mode %d",dataSourceFilename.c_str(),mode);
    //CDBDebug("Getting datasource %s",dataSourceFilename.c_str());
    //pthread_mutex_lock(&CDataReader_open_lock);
    cdfObject=CDFObjectStore::getCDFObjectStore()->getCDFObject(dataSource,dataSourceFilename.c_str());
    //pthread_mutex_unlock(&CDataReader_open_lock);
  }
  //pthread_mutex_lock(&CDataReader_open_lock);
   // //pthread_mutex_lock(&CDataReader_open_lock);
//return 0;//CHECK
  //Check whether we really have a cdfObject
  if(cdfObject==NULL){CDBError("Unable to get CDFObject from store");return 1;}


  if(dataSource->attachCDFObject(cdfObject)!=0){
    CDBError("Unable to attach CDFObject");
    return 1;
  }


  for(size_t varNr=0;varNr<dataSource->getNumDataObjects();varNr++){
    //Check if our variable has a statusflag
    std::vector<CDataSource::StatusFlag*> *statusFlagList=&dataSource->getDataObject(varNr)->statusFlagList;
    CDataSource::readStatusFlags(dataSource->getDataObject(varNr)->cdfVariable,statusFlagList);
    if(statusFlagList->size()>0){
      dataSource->getDataObject(varNr)->hasStatusFlag=true;
    }
    dataSource->getDataObject(varNr)->points.clear();
  }


  // TODO: Tot hier heb ik gecontroleerd op checker logica.

  if(parseDimensions(dataSource,mode,x,y, gridExtent)!=0){
    CDBError("Unable to parseDimensions");
    return 1;
  }

  if(dataSource->useLonTransformation!=-1 && gridExtent == NULL){
    for(size_t varNr=0;varNr<dataSource->getNumDataObjects();varNr++){
      Proc::swapPixelsAtLocation(dataSource,dataSource->getDataObject(varNr)->cdfVariable,0);
    }
  }

  if(mode == CNETCDFREADER_MODE_OPEN_DIMENSIONS){


#ifdef CDATAREADER_DEBUG
    CDBDebug("Dimensions parsed");
#endif
    return 0;
  }





  size_t start[dataSource->dNetCDFNumDims+1];
  size_t count[dataSource->dNetCDFNumDims+1];
  ptrdiff_t stride[dataSource->dNetCDFNumDims+1];

  //Set X and Y dimensions start, count and stride
  for(int j=0;j<dataSource->dNetCDFNumDims;j++){start[j]=0; count[j]=1;stride[j]=1;}


  count[dataSource->dimXIndex]=dataSource->dOrigWidth;
  count[dataSource->dimYIndex]=dataSource->dHeight;
  stride[dataSource->dimXIndex]=dataSource->stride2DMap;
  stride[dataSource->dimYIndex]=dataSource->stride2DMap;

if( mode == CNETCDFREADER_MODE_OPEN_EXTENT && gridExtent != NULL ){
      start[dataSource->dimXIndex] = gridExtent[0];
      start[dataSource->dimYIndex] = gridExtent[1];

      count[dataSource->dimXIndex] = dataSource->dWidth;
      count[dataSource->dimYIndex]=dataSource->dHeight;
    }


  //Set other dimensions than X and Y.
  if(dataSource->dNetCDFNumDims>2){
    for(int j=0;j<dataSource->dNetCDFNumDims-2;j++){
      start[j]=dataSource->getDimensionIndex(dataSource->getDataObject(0)->cdfVariable->dimensionlinks[j]->name.c_str());//dOGCDimValues[0];// time dim
      //CDBDebug("%s==%d",dataSourceVar->dimensionlinks[j]->name.c_str(),start[j]);
    }
  }

  if(singleCellMode){
    dataSource->dWidth=2;
    dataSource->dHeight=2;
    start[dataSource->dimXIndex]=x;
    start[dataSource->dimYIndex]=y;
    count[dataSource->dimXIndex]=1;
    count[dataSource->dimYIndex]=1;
    stride[dataSource->dimXIndex]=1;
    stride[dataSource->dimYIndex]=1;
    #ifdef CDATAREADER_DEBUG
     CDBDebug("Single cell mode for x,y: (%d, %d)",x,y);
    #endif

  }




  /*
   * Determine dataSource->getDataObject(0)->dataType of the variable we are going to read
   *
   * Important: ax+b DatapostProc needs this information, so it needs to be called after these statements
   */
  for(size_t varNr=0;varNr<dataSource->getNumDataObjects();varNr++){

    #ifdef CDATAREADER_DEBUG
    CDBDebug("Working on variable %s, %d/%d",dataSource->getDataObject(varNr)->cdfVariable->name.c_str(),varNr,dataSource->getNumDataObjects());
    #endif
    //Get Unit
    CDF::Attribute *varUnits=dataSource->getDataObject(varNr)->cdfVariable->getAttributeNE("units");
    if(varUnits!=NULL){
      dataSource->getDataObject(varNr)->setUnits(varUnits->toString()); //units.copy((char*)varUnits->data,varUnits->length);
    }else dataSource->getDataObject(varNr)->setUnits("");

    // Check for packed data / hasScaleOffset

    CDF::Attribute *scale_factor = dataSource->getDataObject(varNr)->cdfVariable->getAttributeNE("scale_factor");
    CDF::Attribute *add_offset = dataSource->getDataObject(varNr)->cdfVariable->getAttributeNE("add_offset");
    if(scale_factor!=NULL){
      //Scale and offset will be applied further downwards in this function.
      dataSource->getDataObject(varNr)->hasScaleOffset=true;
      // Currently two unpacked data types are supported (CF-1.4): float and double

      dataSource->getDataObject(varNr)->cdfVariable->setType(CDF_FLOAT);

      if(scale_factor->getType()==CDF_FLOAT){
        //dataSource->getDataObject(varNr)->dataType=CDF_FLOAT;
        dataSource->getDataObject(varNr)->cdfVariable->setType(CDF_FLOAT);
      }
      if(scale_factor->getType()==CDF_DOUBLE){
        dataSource->getDataObject(varNr)->cdfVariable->setType(CDF_DOUBLE);
      }

      //Internally we use always double for scale and offset parameters:
      scale_factor->getData(&dataSource->getDataObject(varNr)->dfscale_factor,1);

      if(add_offset!=NULL){
        add_offset->getData(&dataSource->getDataObject(varNr)->dfadd_offset,1);
      }else dataSource->getDataObject(varNr)->dfadd_offset=0;
    }



     #ifdef CDATAREADER_DEBUG
    CDBDebug("/Finished Working on variable %s",dataSource->getDataObject(varNr)->cdfVariable->name.c_str());
    #endif
  }

  /*
  * DataPostProc: Here our datapostprocessor comes into action! It needs scale and offset from datasource.
  * This is stage1, only AX+B will be applied to scale and offset parameters
  */
  CDataPostProcessor::getCDPPExecutor()->executeProcessors(dataSource,CDATAPOSTPROCESSOR_RUNBEFOREREADING);



  if(mode==CNETCDFREADER_MODE_GET_METADATA){
    #ifdef CDATAREADER_DEBUG
    CDBDebug("Get metadata");
    #endif


    CT::string *variableName;
    std::vector<CDF::Attribute *> *attributes;

    for(size_t i=0;i<cdfObject->variables.size()+1;i++){
      if(i==0){
        attributes=&cdfObject->attributes;
        variableName=&cdfObject->name;
      }else {
        attributes=&cdfObject->variables[i-1]->attributes;
        variableName=&cdfObject->variables[i-1]->name;
      }
      for(size_t j=0;j<attributes->size();j++){
        CDF::Attribute *attribute;
        attribute=(*attributes)[j];
        dataSource->metaDataItems.push_back(CDataSource::KVP(variableName->c_str(),attribute->name.c_str(),attribute->toString().c_str()));
      }
    }
    #ifdef CDATAREADER_DEBUG
    CDBDebug("Metadata Finished.");
    #endif
    return 0;
  }



    for(size_t varNr=0;varNr<dataSource->getNumDataObjects();varNr++)    {
       // double dfNoData = 0;
      #ifdef MEASURETIME
      StopWatch_Stop("Reading _FillValue");
      #endif
      CDF::Attribute *fillValue = dataSource->getDataObject(varNr)->cdfVariable->getAttributeNE("_FillValue");
      if(fillValue!=NULL){

        fillValue->getData(&dataSource->getDataObject(varNr)->dfNodataValue,1);
        dataSource->getDataObject(varNr)->hasNodataValue = true;
      }else {
        dataSource->getDataObject(varNr)->hasNodataValue=false;
      }

    }


  if(mode==CNETCDFREADER_MODE_OPEN_ALL || mode==CNETCDFREADER_MODE_OPEN_EXTENT){
      #ifdef CDATAREADER_DEBUG
    CDBDebug("CNETCDFREADER_MODE_OPEN_ALL || CNETCDFREADER_MODE_OPEN_EXTENT");
  #endif

  #ifdef MEASURETIME
    StopWatch_Stop("start reading image data");
  #endif
    //for(size_t varNr=0;varNr<dataSource->getNumDataObjects();varNr++)
          for(size_t varNr=0;varNr<dataSource->getNumDataObjects();varNr++)    {



      //if( dataSource->getDataObject(varNr)->cdfVariable->data==NULL){
      if( dataSource->formatConverterActive == false){
        //#ifdef MEASURETIME
        //StopWatch_Stop("Freeing data");
        //#endif

        //Read variable data
        dataSource->getDataObject(varNr)->cdfVariable->freeData();

        #ifdef MEASURETIME
        StopWatch_Stop("start reading data");
        #endif

        #ifdef CDATAREADER_DEBUG
        CDBDebug("READING DATA FOR varNR [%d], name=\"%s\"",varNr,dataSource->getDataObject(varNr)->cdfVariable->name.c_str());
        for(size_t d=0;d<dataSource->getDataObject(varNr)->cdfVariable->dimensionlinks.size();d++){
          CDBDebug("%s  \tstart: %d\tcount %d\tstride %d",dataSource->getDataObject(varNr)->cdfVariable->dimensionlinks[d]->name.c_str(),start[d],count[d],stride[d]);
        }
        #endif



        if(dataSource->getDataObject(varNr)->cdfVariable->readData(dataSource->getDataObject(varNr)->cdfVariable->getType(),start,count,stride)!=0){
          CDBError("Unable to read data for variable %s in file %s",dataSource->getDataObject(varNr)->cdfVariable->name.c_str(),dataSource->getFileName());

          for(size_t j=0;j<dataSource->getDataObject(varNr)->cdfVariable->dimensionlinks.size();j++){
            CDBDebug("%s %d %d %d",dataSource->getDataObject(varNr)->cdfVariable->dimensionlinks[j]->name.c_str(),start[j],count[j],stride[j]);
          }




          return 1;
        }

        #ifdef CDATAREADER_DEBUG
        CDBDebug("DATA IS READ FOR varNR [%d], name=\"%s\": DATA IS READ",varNr,dataSource->getDataObject(varNr)->cdfVariable->name.c_str());

        #endif

        //Swap data from >180 degrees to domain of -180 till 180 in case of lat lon source data
        if(dataSource->useLonTransformation!=-1){
          //int splitPX=dataSource->useLonTransformation;
          if(Proc::swapPixelsAtLocation(dataSource,dataSource->getDataObject(varNr)->cdfVariable,1)!=0){
            return 1;
          }

        }


      }
      dataSource->getDataObject(varNr)->appliedScaleOffset = false;

      #ifdef MEASURETIME
      StopWatch_Stop("data read");
      #endif


      //Swap X, Y dimensions so that pointer x+y*w works correctly


      if(dataSource->swapXYDimensions){
        size_t imgSize=dataSource->dHeight*dataSource->dWidth;
        size_t w=dataSource->dWidth;size_t h=dataSource->dHeight;size_t x,y;
        void *vd=NULL;                 //destination data
        void *vs=dataSource->getDataObject(varNr)->cdfVariable->data;     //source data

        //Allocate data for our new memory block
        CDF::allocateData(dataSource->getDataObject(varNr)->cdfVariable->getType(),&vd,imgSize);
        //TODO This could also be solved using a template. But this works fine.
        switch (dataSource->getDataObject(varNr)->cdfVariable->getType()){
          case CDF_CHAR  : {char   *s=(char  *)vs;char   *d=(char  *)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          case CDF_BYTE  : {char   *s=(char  *)vs;char   *d=(char  *)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          case CDF_UBYTE : {uchar  *s=(uchar *)vs;uchar  *d=(uchar *)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          case CDF_SHORT : {short  *s=(short *)vs;short  *d=(short *)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          case CDF_USHORT: {ushort *s=(ushort*)vs;ushort *d=(ushort*)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          case CDF_INT   : {int    *s=(int   *)vs;int    *d=(int   *)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          case CDF_UINT  : {uint   *s=(uint  *)vs;uint   *d=(uint  *)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          case CDF_FLOAT : {float  *s=(float *)vs;float  *d=(float *)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          case CDF_DOUBLE: {double *s=(double*)vs;double *d=(double*)vd;for(y=0;y<h;y++)for(x=0;x<w;x++){d[x+y*w]=s[y+x*h];}}break;
          default: {CDBError("Unknown data type"); return 1;}
        }
        //We will replace our old memory block with the new one, but we have to free our old one first.
        free(dataSource->getDataObject(varNr)->cdfVariable->data);
        //Replace the memory block.
        dataSource->getDataObject(varNr)->cdfVariable->data=vd;
      }


      //Apply scale and offset factor on the data
      if(dataSource->getDataObject(varNr)->appliedScaleOffset == false && dataSource->getDataObject(varNr)->hasScaleOffset){
        dataSource->getDataObject(varNr)->appliedScaleOffset=true;

        double dfscale_factor = dataSource->getDataObject(varNr)->dfscale_factor;
        double dfadd_offset = dataSource->getDataObject(varNr)->dfadd_offset;

        #ifdef CDATAREADER_DEBUG
        CDBDebug("Applying scale and offset with %f and %f (var size=%d) type=%s",dfscale_factor,dfadd_offset,dataSource->getDataObject(varNr)->cdfVariable->getSize(),CDF::getCDFDataTypeName(dataSource->getDataObject(varNr)->cdfVariable->getType()).c_str());
        #endif
        /*if(dataSource->getDataObject(varNr)->dataType==CDF_FLOAT){
          //Preserve the original nodata value, so it remains a nice short rounded number.
          float fNoData=dfNoData;
          // packed data to be unpacked to FLOAT:
          float *_data=(float*)dataSource->getDataObject(varNr)->cdfVariable->data;
          for(size_t j=0;j<dataSource->getDataObject(varNr)->cdfVariable->getSize();j++){
            //Only apply scale and offset when this actual data (do not touch the nodata)
            if(_data[j]!=fNoData){
              _data[j]*=dfscale_factor;
              _data[j]+=dfadd_offset;
            }
          }
        }*/
        if(dataSource->getDataObject(varNr)->cdfVariable->getType()==CDF_FLOAT){
          //Preserve the original nodata value, so it remains a nice short rounded number.
          float fNoData=(float)dataSource->getDataObject(varNr)->dfNodataValue;
          float fscale_factor=(float)dfscale_factor;
          float fadd_offset=(float)dfadd_offset;
          // packed data to be unpacked to FLOAT:
          if(fscale_factor!=1.0f||fadd_offset!=0.0f){
            float *_data=(float*)dataSource->getDataObject(varNr)->cdfVariable->data;
            size_t l=dataSource->getDataObject(varNr)->cdfVariable->getSize();
            for(size_t j=0;j<l;j++){
              _data[j]=_data[j]*fscale_factor+fadd_offset;
            }
          }
          //Convert the nodata type
          dataSource->getDataObject(varNr)->dfNodataValue=fNoData*fscale_factor+fadd_offset;
        }

        if(dataSource->getDataObject(varNr)->cdfVariable->getType()==CDF_DOUBLE){
          // packed data to be unpacked to DOUBLE:
          double *_data=(double*)dataSource->getDataObject(varNr)->cdfVariable->data;
          for(size_t j=0;j<dataSource->getDataObject(varNr)->cdfVariable->getSize();j++){
            //if(j%10000==0){CDBError("%d = %f",j,_data[j]);}
            _data[j]=_data[j]*dfscale_factor+dfadd_offset;
          }

          //Convert the nodata type
          dataSource->getDataObject(varNr)->dfNodataValue=dataSource->getDataObject(varNr)->dfNodataValue*dfscale_factor+dfadd_offset;
        }
      }

      #ifdef MEASURETIME
      StopWatch_Stop("Scale and offset applied");
      #endif




   /**
    *Cache is deprecated since 2014-01-01
    *
//       In order to write good cache files we need to modify
//         our cdfobject. Data is now unpacked, so we need to remove
//         scale_factor and add_offset attributes and change datatypes
//         of the data and _FillValue
//
      //remove scale_factor and add_offset attributes, otherwise they are stored in the cachefile again and reapplied over and over again.
      dataSource->getDataObject(varNr)->cdfVariable->removeAttribute("scale_factor");
      dataSource->getDataObject(varNr)->cdfVariable->removeAttribute("add_offset");

      //Set original var datatype correctly for the cdfobject
      //dataSource->getDataObject(varNr)->cdfVariable->getType()=dataSource->getDataObject(varNr)->dataType;

      //Reset _FillValue to correct datatype and adjust scale and offset values.
      if( dataSource->getDataObject(varNr)->hasNodataValue){
        CDF::Attribute *fillValue = dataSource->getDataObject(varNr)->cdfVariable->getAttributeNE("_FillValue");
        if(fillValue!=NULL){
          if(dataSource->getDataObject(varNr)->cdfVariable->getType()==CDF_FLOAT){float fNoData=(float)dataSource->getDataObject(varNr)->dfNodataValue;fillValue->setData(CDF_FLOAT,&fNoData,1);}
          if(dataSource->getDataObject(varNr)->cdfVariable->getType()==CDF_DOUBLE)fillValue->setData(CDF_DOUBLE,&dataSource->getDataObject(varNr)->dfNodataValue,1);
        }
      }*/
    }



    if(dataSource->stretchMinMax){//&&((dataSource->dWidth!=2||dataSource->dHeight!=2))){
      if(dataSource->stretchMinMaxDone == false){
        if(dataSource->statistics==NULL){
          #ifdef CDATAREADER_DEBUG
          CDBDebug("No statistics available");
          #endif
          dataSource->statistics = new CDataSource::Statistics();
          dataSource->statistics->calculate(dataSource);
          #ifdef MEASURETIME
          StopWatch_Stop("Calculated statistics");
          #endif
        }
        float min=(float)dataSource->statistics->getMinimum();
        float max=(float)dataSource->statistics->getMaximum();
        #ifdef CDATAREADER_DEBUG
          CDBDebug("Statistics: Min = %f, Max = %f",min,max);
        #endif

        //Make sure that there is always a range in between the min and max.
        if(max==min)max=min+0.1;
        //Calculate legendOffset legendScale
        float ls=240/(max-min);
        float lo=-(min*ls);
        styleConfiguration->legendScale=ls;
        styleConfiguration->legendOffset=lo;
        dataSource->stretchMinMaxDone = true;
      }
    }

    //Check for infinities
    if(styleConfiguration->legendScale!=styleConfiguration->legendScale||
      styleConfiguration->legendScale==INFINITY||
      styleConfiguration->legendScale==NAN||
      styleConfiguration->legendScale==-INFINITY){
      styleConfiguration->legendScale=240;
    }
    if(styleConfiguration->legendOffset!=styleConfiguration->legendOffset||
      styleConfiguration->legendOffset==INFINITY||
      styleConfiguration->legendOffset==NAN||
      styleConfiguration->legendOffset==-INFINITY){
      styleConfiguration->legendOffset=0;
    }

    #ifdef CDATAREADER_DEBUG
    CDBDebug("styleConfiguration->legendScale = %f, styleConfiguration->legendOffset = %f",styleConfiguration->legendScale,styleConfiguration->legendOffset);
    #endif

  #ifdef MEASURETIME
    StopWatch_Stop("all read");
  #endif

    /*
    * DataPostProc: Here our datapostprocessor comes into action!
    * This is stage2, running on data, not metadata
    */

   CDataPostProcessor::getCDPPExecutor()->executeProcessors(dataSource,CDATAPOSTPROCESSOR_RUNAFTERREADING);

  }
 //pthread_mutex_unlock(&CDataReader_open_lock);
  #ifdef CDATAREADER_DEBUG
    CDBDebug("/Finished datareader");
  #endif
  return 0;
}



CDF::Variable *CDataReader::getTimeDimension(CDataSource *dataSource){
  return getDimensionVariableByType(dataSource->getDataObject(0)->cdfVariable,dtype_time);
}

//DEPRECATED
CT::string CDataReader::getTimeUnit(CDataSource *dataSource){
  CDF::Variable *time = getDimensionVariableByType(dataSource->getDataObject(0)->cdfVariable,dtype_time);
  if(time == NULL){CDBDebug("No time variable found");throw(1);}
  CDF::Attribute *timeUnits = time->getAttributeNE("units");
  if(timeUnits == NULL){CDBDebug("No time units found");throw(2);}

  CT::string timeUnitsString=timeUnits->toString().c_str();

  return timeUnitsString;
}

CDF::Variable *CDataReader::addBlankDimVariable(CDFObject* cdfObject, const char *dimName) {
  CDF::Dimension *dim = cdfObject->getDimensionNE(dimName);
  if( dim == NULL ){
    CDBError("Unable to find dimension %s", dimName);
    return NULL;
  }
  CDF::Variable *dimVar = cdfObject->addVariable(new CDF::Variable(dim->getName().c_str(),CDF_DOUBLE,&dim,1, true));

  dimVar->setCDFReaderPointer(NULL);
  dimVar->setParentCDFObject(cdfObject);
  dimVar->allocateData(dim->getSize());
  for(size_t j=0;j<dim->getSize();j++){
    ((double*)dimVar->data)[j]=j+1;
  }
  return dimVar;
}
CDataReader::DimensionType CDataReader::getDimensionType(CDFObject *cdfObject,const char *ncname){
  CDF::Dimension *dimension = cdfObject->getDimensionNE(ncname);
  if(dimension != NULL){
    return getDimensionType(cdfObject,dimension);
  }else{
    CT::string none="none";
    if(none.equals(ncname)){
      return dtype_normal;
    }
    return getDimensionType(cdfObject,cdfObject->getVariableNE(ncname));
  }
}

CDataReader::DimensionType CDataReader::getDimensionType(CDFObject *cdfObject,CDF::Dimension *dimension){
  if(dimension == NULL){
    CDBWarning("Dimension not defined");
    return dtype_none;
  }
  CDF::Variable *variable = cdfObject->getVariableNE(dimension->name.c_str());
  return getDimensionType(cdfObject,variable);
}

// TODO: Zit hier logica voor de checker?
CDataReader::DimensionType CDataReader::getDimensionType(CDFObject *cdfObject,CDF::Variable *variable){
  if(variable == NULL){
    CDBWarning("Warning no dimension variable specified for dimension ");
    return dtype_none;
  }

  CT::string standardName = "";

  try{
    variable->getAttribute("standard_name")->getDataAsString(&standardName);
  }catch(int e){
  }

  if(standardName.length()==0){
    CDBDebug("Warning no standard name given for dimension %s, using variable name instead.",variable->name.c_str());
    standardName = variable->name;;
  }

  //CDBDebug("Standardname of dimension [%s] is [%s]",variable->name.c_str(), standardName.c_str());


  //CDBDebug("%d %d",standardName.equals("time"),standardName.length());
  if(standardName.equals("time"))return dtype_time;
  if(standardName.equals("forecast_reference_time"))return dtype_reference_time;
  if(standardName.equals("member"))return dtype_member;
  if(standardName.equals("elevation"))return dtype_elevation;
  if(standardName.equals("pressurelevel_0"))return dtype_elevation;
  if(standardName.equals("height"))return dtype_elevation;

  //If no standard_name matches, try to determine dimension type on _CoordinateAxisType attribute, CDM standard
  CT::string coordinateAxisType = "";
  try{
    variable->getAttribute("_CoordinateAxisType")->getDataAsString(&coordinateAxisType);
  }catch(int e){
  }
  coordinateAxisType.toLowerCaseSelf();
  if(coordinateAxisType.equals("ensemble"))return dtype_member;
  if(coordinateAxisType.equals("time"))return dtype_time;
  if(coordinateAxisType.equals("height"))return dtype_elevation;
  if(coordinateAxisType.equals("pressure"))return dtype_elevation;
  if(coordinateAxisType.equals("runtime"))return dtype_reference_time;

  //Try to find elevation dimension based on positive attribute existence (CF)
  try{
    variable->getAttribute("positive");
    return dtype_elevation;
  }catch(int e){
  }
   CDBDebug("Dimension %s with standard_name [%s] is a normal dimension",variable->name.c_str(), standardName.c_str());

  return dtype_normal;
}

CDF::Dimension* CDataReader::searchDimensionByType(CDF::Variable *var,CDataReader::DimensionType dimensionType){
  if(var == NULL)return NULL;
  if(var->dimensionlinks.size()==0){return NULL;}

  CDFObject *cdfObject = (CDFObject*)var->getParentCDFObject();

  for(size_t j=0;j<var->dimensionlinks.size();j++){
    try{
      DimensionType type = getDimensionType(cdfObject,var->dimensionlinks[j]);
      if(type==dimensionType)return var->dimensionlinks[j];
    }catch(int e){
    }
  }
  return NULL;
};

CDF::Variable* CDataReader::getDimensionVariableByType(CDF::Variable *var,CDataReader::DimensionType dimensionType){
  CDF::Dimension *dim = searchDimensionByType(var,dimensionType);
  if(dim == NULL) return NULL;
  CDFObject *cdfObject = (CDFObject*)var->getParentCDFObject();
  return cdfObject->getVariableNE(dim->name.c_str());
}
