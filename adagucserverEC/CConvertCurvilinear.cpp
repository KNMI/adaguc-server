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

#include "CConvertCurvilinear.h"
#include "CFillTriangle.h"
#include "CImageWarper.h"
// #define CCONVERTCURVILINEAR_DEBUG
const char *CConvertCurvilinear::className="CConvertCurvilinear";

int CConvertCurvilinear::checkIfIsCurvilinear(CDFObject *cdfObject,CServerParams *srvParams, bool &hasLatLonBounds){
    //Check whether this is really a curvilinear file
  try{
    //cdfObject->getDimension("col");
    //cdfObject->getDimension("row");
    cdfObject->getDimension("time");
    
    cdfObject->getVariable("lon");
    cdfObject->getVariable("lat");
    CDF::Attribute *adagucType = cdfObject->getAttributeNE("ADAGUC_DATATYPE");
    if (adagucType != NULL && adagucType->toString().equals("ADAGUC_CURVILINEAR")) {
      CDBDebug("Enforcing ADAGUC_TYPE ADAGUC_CURVILINEAR");
    } else {
      cdfObject->getDimension("bounds");
      CDF::Variable *var=cdfObject->getVariable("lon_bnds");
      if(var->dimensionlinks.size()<2){
        return 1;
      }
      cdfObject->getVariable("lat_bnds");
      hasLatLonBounds = true;
    }
  }catch(int e){
    return 1;
  }
  
  if(hasLatLonBounds && cdfObject->getDimension("bounds")->getSize()!=4){
    return 1;
  }
  return 0;
}

double* CConvertCurvilinear::getBBOXFromLatLonFields( CDF::Variable *swathMiddleLon, CDF::Variable *swathMiddleLat){
  
  
  int numRows = swathMiddleLon->dimensionlinks[0]->getSize();
  int numCols = swathMiddleLon->dimensionlinks[1]->getSize();
  
  
  swathMiddleLon->readData(CDF_FLOAT,true);
  swathMiddleLat->readData(CDF_FLOAT,true);

  float *lonData=(float*)swathMiddleLon->data;
  float *latData=(float*)swathMiddleLat->data;
  float fillValueLon = NAN;
  float fillValueLat = NAN;
  
  try{swathMiddleLon->getAttribute("_FillValue")->getData(&fillValueLon,1);}catch(int e){};
  try{swathMiddleLat->getAttribute("_FillValue")->getData(&fillValueLat,1);}catch(int e){};

  double minx=NAN,maxx=NAN,miny=NAN,maxy=NAN;
  size_t numCells = numRows*numCols;
  bool firstLATLONDone = false;
  int increment=1;
  if (numCells > 256*256){
    increment = 256;
  }
  for(size_t j=0;j<numCells;j=j + increment){
    float lon = lonData[j];
    float lat = latData[j];
    if(!(lon == fillValueLon ||lat == fillValueLat)){
      if(firstLATLONDone == false){
        minx=lon;maxx=lon;
        miny=lat;maxy=lat;
        firstLATLONDone = true;
      }else{
        
        if(lon<minx)minx=lon;
        if(lat<miny)miny=lat;
        if(lon>maxx)maxx=lon;
        if(lat>maxy)maxy=lat;
      }
    }
  }
  double *dfBBOX = new double[4];
  dfBBOX[0]=minx;
  dfBBOX[1]=miny;
  dfBBOX[2]=maxx;
  dfBBOX[3]=maxy;
  
  if(dfBBOX[2] - dfBBOX[0]>300){
    if((dfBBOX[0] + dfBBOX[2])/2>170&&(dfBBOX[0] + dfBBOX[2])/2<190){
      dfBBOX[0]-=180;
      dfBBOX[2]-=180;
    }
  }
  
  return dfBBOX ;
}

/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertCurvilinear::convertCurvilinearHeader( CDFObject *cdfObject,CServerParams *srvParams ){
  bool hasLatLonBounds = false;
  
  if (checkIfIsCurvilinear(cdfObject, srvParams, hasLatLonBounds) != 0) return 1;
  if (cdfObject->getAttributeNE("ADAGUC_HEADER_DONE") != NULL) return 0;
  CDBDebug("Using CConvertCurvilinear.h");
  //bool hasTimeData = false;
  
  //Is there a time variable
 /* CDF::Variable *origT = cdfObject->getVariableNE("time");
  if(origT!=NULL){
    hasTimeData=true;

    //Create a new time dimension for the new 2D fields.
    CDF::Dimension *dimT=new CDF::Dimension();
    dimT->name="time2D";
    dimT->setSize(1);
    cdfObject->addDimension(dimT);
    
    //Create a new time variable for the new 2D fields.
    CDF::Variable *varT = new CDF::Variable();
    cdfObject->addVariable(varT);
    varT->setType(CDF_DOUBLE);
    varT->name.copy(dimT->name.c_str());
    varT->setAttributeText("standard_name","time");
    varT->setAttributeText("long_name","time");
    varT->dimensionlinks.push_back(dimT);
    CDF::allocateData(CDF_DOUBLE,&varT->data,dimT->length);
    
    
    //Detect time from the netcdf data and copy the same units from the original time variable
    if(origT!=NULL){
      try{
        varT->setAttributeText("units",origT->getAttribute("units")->toString().c_str());
        if(origT->readData(CDF_DOUBLE)!=0){
          CDBError("Unable to read time variable");
        }else{
          //Loop through the time variable and detect the earliest time
          double tfill;
          bool hastfill =false;
          try{
            origT->getAttribute("_FillValue")->getData(&tfill,1);
            hastfill=true;
          }catch(int e){}
          double *tdata=((double *)origT->data);
          size_t tlength = origT->getSize();
//           double firstTimeValue = tdata[0];
//           size_t tsize = origT->getSize();
//           if(hastfill==true){
//             for(size_t j=0;j<tsize;j++){
//               if(tdata[j]!=tfill){
//                 firstTimeValue = tdata[j];
//               }
//             }
//           }
//           #ifdef CCONVERTCURVILINEAR_DEBUG
//           CDBDebug("firstTimeValue  = %f",firstTimeValue );
//           #endif
          //Set the time data
          varT->setData(CDF_DOUBLE,tdata,tlength);
        }
      }catch(int e){}
    }
  }
*/
  //Determine boundingbox of variable
 
  CDF::Variable *swathMiddleLon;
  CDF::Variable *swathMiddleLat;
  try{
    swathMiddleLon = cdfObject->getVariable("lon");
    swathMiddleLat = cdfObject->getVariable("lat");
  }catch(int e){
    CDBError("lat or lon variables not found");
    return 1;
  }
  double *dfBBOX = getBBOXFromLatLonFields(swathMiddleLon,swathMiddleLat);
  
  
  
  //BBOX should be converted to projection of dataSource
//   CT::string proj4String = "+proj=utm +zone=31 +ellps=WGS84 +datum=WGS84 +units=m +no_defs ";
//    if(dataSource->nativeProj4.length()>0){
//      CImageWarper imageWarper;
//      imageWarper.initreproj(proj4String.c_str(),NULL,NULL);
//      
//     if(status !=0 ){
//       CDBError("Unable to init projection");
//       return 1;
//     }
//   }
//   
    

  //Default size of adaguc 2dField is 2x2
  int width=2;//swathMiddleLon->dimensionlinks[1]->getSize();
  int height=2;//swathMiddleLon->dimensionlinks[0]->getSize();
  
  if (srvParams == NULL){
    CDBError("srvParams is not set");
    return 1;
  }
  if(srvParams->Geo->dWidth>1&&srvParams->Geo->dHeight>1){
    width  = srvParams->Geo->dWidth;
    height = srvParams->Geo->dHeight;
  }
  
  #ifdef CCONVERTCURVILINEAR_DEBUG
  CDBDebug("Width = %d, Height = %d", width, height);
  #endif
  if (width < 2 || height < 2){
    CDBError("width and height are too small");
    return 1;
  }
  
  
  
  
  
  
  double cellSizeX=(dfBBOX[2]-dfBBOX[0])/double(width);
  double cellSizeY=(dfBBOX[3]-dfBBOX[1])/double(height);
  double offsetX=dfBBOX[0];
  double offsetY=dfBBOX[1];
  delete [] dfBBOX;  dfBBOX = NULL;
  //Add geo variables, only if they are not there already
  CDF::Dimension *dimX = cdfObject->getDimensionNE("adaguccoordinatex");
  CDF::Dimension *dimY = cdfObject->getDimensionNE("adaguccoordinatey");
  CDF::Variable *varX = cdfObject->getVariableNE("adaguccoordinatex");
  CDF::Variable *varY = cdfObject->getVariableNE("adaguccoordinatey");
  if(dimX==NULL||dimY==NULL||varX==NULL||varY==NULL) {
    //If not available, create new dimensions and variables (X,Y,T)
    //For x 
    dimX=new CDF::Dimension();
    dimX->name="adaguccoordinatex";
    dimX->setSize(width);
    cdfObject->addDimension(dimX);
    varX = new CDF::Variable();
    varX->setType(CDF_DOUBLE);
    varX->name.copy("adaguccoordinatex");
    varX->isDimension=true;
    varX->dimensionlinks.push_back(dimX);
    cdfObject->addVariable(varX);
    CDF::allocateData(CDF_DOUBLE,&varX->data,dimX->length);

    //For y 
    dimY=new CDF::Dimension();
    dimY->name="adaguccoordinatey";
    dimY->setSize(height);
    cdfObject->addDimension(dimY);
    varY = new CDF::Variable();
    varY->setType(CDF_DOUBLE);
    varY->name.copy("adaguccoordinatey");
    varY->isDimension=true;
    varY->dimensionlinks.push_back(dimY);
    cdfObject->addVariable(varY);
    CDF::allocateData(CDF_DOUBLE,&varY->data,dimY->length);
    
#ifdef CCONVERTCURVILINEAR_DEBUG
    CDBDebug("Data allocated for 'x' and 'y' variables (%d x %d)", varX->getSize(), varY->getSize());
#endif
    
    //Fill in the X and Y dimensions with the array of coordinates
    for(size_t j=0;j<dimX->length;j++){
      double x=offsetX+double(j)*cellSizeX+cellSizeX/2;
      ((double*)varX->data)[j]=x;
    }
    for(size_t j=0;j<dimY->length;j++){
      double y=offsetY+double(j)*cellSizeY+cellSizeY/2;
      ((double*)varY->data)[j]=y;
    }
  }
  
  //Make a list of variables which will be available as 2D fields  
  CT::StackList<CT::string> varsToConvert;
  for(size_t v=0;v<cdfObject->variables.size();v++){
    CDF::Variable *var = cdfObject->variables[v];
    if(var->isDimension==false){
      if(!var->name.equals("time2D")&&
        !var->name.equals("time")&&
        !var->name.equals("wgs84")&&
        !var->name.equals("epsg")&&
        !var->name.equals("lon")&&
        !var->name.equals("lat")&&
        !var->name.equals("custom")&&
        !var->name.equals("projection")&&
        !var->name.equals("product")&&
        !var->name.equals("iso_dataset")&&
        !var->name.equals("tile_properties")&&
        (var->name.indexOf("bnds")==-1)
      ){
        if(var->dimensionlinks.size()>=2){
          int numDims = var->dimensionlinks.size();
#ifdef CCONVERTCURVILINEAR_DEBUG
          CDBDebug("CurviX name for %s = %s",var->name.c_str(),var->dimensionlinks[numDims-1]->name.c_str());
          CDBDebug("CurviY name for %s = %s",var->name.c_str(),var->dimensionlinks[numDims-2]->name.c_str());
#endif          
          CDF::Variable *curviX = cdfObject->getVariableNE(var->dimensionlinks[numDims-1]->name.c_str());
          CDF::Variable *curviY = cdfObject->getVariableNE(var->dimensionlinks[numDims-2]->name.c_str());
          if(curviX == NULL && curviY == NULL){
             varsToConvert.add(CT::string(var->name.c_str()));
          }else if(curviX->getSize()==2&&curviY->getSize()==2){
            varsToConvert.add(CT::string(var->name.c_str()));
          }else{
            CT::string xName = curviX->name.c_str();xName.toUpperCaseSelf();
            if(!xName.equals("X")&&!xName.equals("LAT")&&!xName.equals("ROW")&&!xName.equals("COL")){
              varsToConvert.add(CT::string(var->name.c_str()));
            }
          }
        }
      }
      if(var->name.equals("projection")){
        var->setAttributeText("ADAGUC_SKIP","true");
      }
    }
  }
  
  //Create the new 2D field variables based on the swath variables
  for(size_t v=0;v<varsToConvert.size();v++){
    CDF::Variable *swathVar=cdfObject->getVariable(varsToConvert[v].c_str());
    
    //Remove projection attribute if we use lat/lon for projecting
    swathVar->removeAttribute("grid_mapping");
   
    #ifdef CCONVERTCURVILINEAR_DEBUG
    CDBDebug("Converting %d/%d %s",v,varsToConvert.size(),swathVar->name.c_str());
    #endif
    
    CDF::Variable *new2DVar = new CDF::Variable();
    cdfObject->addVariable(new2DVar);
    
    //Assign X,Y,T dims 
    //if(hasTimeData){
      /*CDF::Variable *newTimeVar=cdfObject->getVariableNE("time");             
      if(newTimeVar!=NULL){
        new2DVar->dimensionlinks.push_back(newTimeVar->dimensionlinks[0]);
      }*/
    //}
    
    for(size_t d=0;d<swathVar->dimensionlinks.size()-2;d++){
      new2DVar->dimensionlinks.push_back( swathVar->dimensionlinks[d]);
    }
    
    new2DVar->dimensionlinks.push_back(dimY);
    new2DVar->dimensionlinks.push_back(dimX);
    
    new2DVar->setType(swathVar->getType());
    new2DVar->name=swathVar->name.c_str();
    swathVar->name.concat("_backup");
    
    //Copy variable attributes
    for(size_t j=0;j<swathVar->attributes.size();j++){
      CDF::Attribute *a =swathVar->attributes[j];
      new2DVar->setAttribute(a->name.c_str(),a->getType(),a->data,a->length);
      new2DVar->setAttributeText("ADAGUC_VECTOR","true");
    }
    
    //The swath variable is not directly plotable, so skip it
    swathVar->setAttributeText("ADAGUC_SKIP","true");
    
    //Scale and offset are already applied
    new2DVar->removeAttribute("scale_factor");
    new2DVar->removeAttribute("add_offset");
    
    new2DVar->setType(CDF_FLOAT);
  }
  
  cdfObject->setAttributeText("ADAGUC_HEADER_DONE", "true");
  
  #ifdef CCONVERTCURVILINEAR_DEBUG
  CDBDebug("Header done");
  #endif
  return 0;
}




/**
 * This function draws the virtual 2D variable into a new 2D field
 */
int CConvertCurvilinear::convertCurvilinearData(CDataSource *dataSource,int mode){


//   CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
//   if(cdfObject->getVariableNE("lat_vertices")!=NULL&&cdfObject->getVariableNE("lon_vertices")!=NULL&&cdfObject->getDimensionNE("vertices")!=NULL){
//     try{cdfObject->getDimension("vertices")->name="bounds";}catch(int e){}
//     try{cdfObject->getVariable("lat_vertices")->name="lat_bnds";}catch(int e){}
//     try{cdfObject->getVariable("lon_vertices")->name="lon_bnds";}catch(int e){}
//   }
  bool hasLatLonBounds = false;
  CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
  if (checkIfIsCurvilinear(cdfObject, dataSource->srvParams, hasLatLonBounds) != 0) return 1;


#ifdef CCONVERTCURVILINEAR_DEBUG
  CDBDebug("THIS IS Curvilinear VECTOR DATA");
#endif
  
   size_t nrDataObjects = dataSource->getNumDataObjects();
  CDataSource::DataObject *dataObjects[nrDataObjects];
  for(size_t d=0;d<nrDataObjects;d++){
    dataObjects[d] =  dataSource->getDataObject(d);
  }
  CDF::Variable *new2DVar;
  new2DVar = dataObjects[0]->cdfVariable;
  
  CDF::Variable *swathVar;
  CT::string origSwathName=new2DVar->name.c_str();
  origSwathName.concat("_backup");
  swathVar=cdfObject->getVariableNE(origSwathName.c_str());
  if(swathVar==NULL){
    CDBError("Unable to find orignal swath variable with name %s",origSwathName.c_str());
    return 1;
  }
 
  size_t numDims = swathVar->dimensionlinks.size();
  size_t start[numDims];
  size_t count[numDims];
  ptrdiff_t stride[numDims];
  for(size_t dimInd = 0;dimInd<numDims;dimInd++){
    start[dimInd]=0; count[dimInd]=1;stride[dimInd]=1;
    CT::string dimName = swathVar->dimensionlinks[dimInd]->name.c_str();
    
    if(numDims-dimInd<3){
      start[dimInd] = 0;
      count[dimInd] =  swathVar->dimensionlinks[dimInd]->getSize();
    }else{      
      start[dimInd]=dataSource->getDimensionIndex(dimName.c_str());
    }
    #ifdef CCONVERTCURVILINEAR_DEBUG
    CDBDebug("%s = %d %d",dimName.c_str(),start[dimInd],count[dimInd]);
    #endif
  }
  
  

  //Read original data first 
  
 
 
  CDF::Attribute *fillValue = swathVar->getAttributeNE("_FillValue");
  if(fillValue!=NULL){
   
    dataObjects[0]->hasNodataValue=true;
    fillValue->getData(&dataObjects[0]->dfNodataValue,1);
    #ifdef CCONVERTCURVILINEAR_DEBUG
    CDBDebug("_FillValue = %f",dataObjects[0]->dfNodataValue);
    #endif
    CDF::Attribute *fillValue2d = new2DVar->getAttributeNE("_FillValue");
    if(fillValue2d == NULL){
      fillValue2d = new CDF::Attribute();
      fillValue2d->name = "_FillValue";
    }
    float f=dataObjects[0]->dfNodataValue;
    fillValue2d->setData(CDF_FLOAT,&f,1);
  }else {
    dataObjects[0]->hasNodataValue=true;
    dataObjects[0]->dfNodataValue=NC_FILL_FLOAT;
    float f=dataObjects[0]->dfNodataValue;
    new2DVar->setAttribute("_FillValue",CDF_FLOAT,&f,1);
  }
  
  
  
  
  //Detect minimum and maximum values
  float fill = (float)dataObjects[0]->dfNodataValue;
  
#ifdef CCONVERTCURVILINEAR_DEBUG
  CDBDebug("Size == %d",swathVar->getSize());
#endif
 
  #ifdef CCONVERTCURVILINEAR_DEBUG
  CDBDebug("Calculated min/max : %f %f",min,max);
  #endif
  
  //Set statistics
  swathVar->readData(CDF_FLOAT,start,count,stride,true);

  if(dataSource->stretchMinMax){
    #ifdef CCONVERTCURVILINEAR_DEBUG
    CDBDebug("dataSource->stretchMinMax");
    #endif
    if(dataSource->statistics==NULL){
      #ifdef CCONVERTCURVILINEAR_DEBUG
      CDBDebug("Setting statistics: min/max : %f %f",min,max);
      #endif
      dataSource->statistics = new CDataSource::Statistics();
      dataSource->statistics->calculate(swathVar->getSize(), swathVar->data, swathVar->getType(), dataObjects[0]->dfNodataValue, dataObjects[0]->hasNodataValue);
      
    }
  }
  
  //Make the width and height of the new 2D adaguc field the same as the viewing window
  dataSource->dWidth=dataSource->srvParams->Geo->dWidth;
  dataSource->dHeight=dataSource->srvParams->Geo->dHeight;      
  
  /*if(dataSource->dWidth == 1 && dataSource->dHeight == 1){
    dataSource->srvParams->Geo->dfBBOX[0]=dataSource->srvParams->Geo->dfBBOX[0];
    dataSource->srvParams->Geo->dfBBOX[1]=dataSource->srvParams->Geo->dfBBOX[1];
    dataSource->srvParams->Geo->dfBBOX[2]=dataSource->srvParams->Geo->dfBBOX[2];
    dataSource->srvParams->Geo->dfBBOX[3]=dataSource->srvParams->Geo->dfBBOX[3];
  }*/
  
  //Width needs to be at least 2, the bounding box is calculated from these.
  if(dataSource->dWidth == 1)dataSource->dWidth=2;
  if(dataSource->dHeight == 1)dataSource->dHeight=2;
  double cellSizeX=(dataSource->srvParams->Geo->dfBBOX[2]-dataSource->srvParams->Geo->dfBBOX[0])/double(dataSource->dWidth);
  double cellSizeY=(dataSource->srvParams->Geo->dfBBOX[3]-dataSource->srvParams->Geo->dfBBOX[1])/double(dataSource->dHeight);
  double offsetX=dataSource->srvParams->Geo->dfBBOX[0];
  double offsetY=dataSource->srvParams->Geo->dfBBOX[1];
 
 




  
  
  #ifdef CCONVERTCURVILINEAR_DEBUG
  CDBDebug("Drawing %s with WH = [%d,%d]",new2DVar->name.c_str(),dataSource->dWidth,dataSource->dHeight);
  #endif
  
  CDF::Dimension *dimX;
  CDF::Dimension *dimY;
  CDF::Variable *varX ;
  CDF::Variable *varY;

  //Create new dimensions and variables (X,Y,T)
  dimX=cdfObject->getDimension("adaguccoordinatex");
  dimX->setSize(dataSource->dWidth);
  
  dimY=cdfObject->getDimension("adaguccoordinatey");
  dimY->setSize(dataSource->dHeight);
  
  varX = cdfObject->getVariable("adaguccoordinatex");
  varY = cdfObject->getVariable("adaguccoordinatey");
  
  CDF::allocateData(CDF_DOUBLE,&varX->data,dimX->length);
  CDF::allocateData(CDF_DOUBLE,&varY->data,dimY->length);

  #ifdef CCONVERTCURVILINEAR_DEBUG 
  CDBDebug("Data allocated for 'x' and 'y' variables");
#endif
  
  //Fill in the X and Y dimensions with the array of coordinates
  for(size_t j=0;j<dimX->length;j++){
    double x=offsetX+double(j)*cellSizeX+cellSizeX/2;
    ((double*)varX->data)[j]=x;
  }
  for(size_t j=0;j<dimY->length;j++){
    double y=offsetY+double(j)*cellSizeY+cellSizeY/2;
    ((double*)varY->data)[j]=y;
  }
  
  CImageWarper imageWarper;
  bool projectionRequired=false;
  CDBDebug("dataSource->srvParams->Geo->CRS = %s", dataSource->srvParams->Geo->CRS.c_str());
  if(dataSource->srvParams->Geo->CRS.length()>0){
    projectionRequired=true;
    new2DVar->setAttributeText("grid_mapping","customgridprojection");
    //Apply once
    if(cdfObject->getVariableNE("customgridprojection")==NULL){
      CDBDebug("Adding customgridprojection");
      CDF::Variable *projectionVar = new CDF::Variable();
      projectionVar->name.copy("customgridprojection");
      cdfObject->addVariable(projectionVar);
      dataSource->nativeEPSG = dataSource->srvParams->Geo->CRS.c_str();
      imageWarper.decodeCRS(&dataSource->nativeProj4,&dataSource->nativeEPSG,&dataSource->srvParams->cfg->Projection);
      CDBDebug("dataSource->nativeProj4 %s", dataSource->nativeProj4.c_str());
      if(dataSource->nativeProj4.length()==0){
        dataSource->nativeProj4=LATLONPROJECTION;
        dataSource->nativeEPSG="EPSG:4326";
        projectionRequired=false;
      }
      if (projectionRequired) {
        CDBDebug("Reprojection is needed");
      }

      projectionVar->setAttributeText("proj4_params",dataSource->nativeProj4.c_str());
    }
  }

  #ifdef CCONVERTCURVILINEAR_DEBUG
  CDBDebug("Datasource CRS = %s nativeproj4 = %s",dataSource->nativeEPSG.c_str(),dataSource->nativeProj4.c_str());
  CDBDebug("Datasource bbox:%f %f %f %f",dataSource->srvParams->Geo->dfBBOX[0],dataSource->srvParams->Geo->dfBBOX[1],dataSource->srvParams->Geo->dfBBOX[2],dataSource->srvParams->Geo->dfBBOX[3]);
  CDBDebug("Datasource width height %d %d",dataSource->dWidth,dataSource->dHeight);
  #endif
    
    
  if(mode==CNETCDFREADER_MODE_OPEN_ALL){
    size_t fieldSize = dataSource->dWidth*dataSource->dHeight;
    new2DVar->setSize(fieldSize);
    CDF::allocateData(new2DVar->getType(),&(new2DVar->data),fieldSize);
    
    //Draw data!
    if(dataObjects[0]->hasNodataValue){
      for(size_t j=0;j<fieldSize;j++){
        ((float*)dataObjects[0]->cdfVariable->data)[j]=(float)dataObjects[0]->dfNodataValue;
      }
    }else{
      for(size_t j=0;j<fieldSize;j++){
        ((float*)dataObjects[0]->cdfVariable->data)[j]=NAN;
      }
    }
    CDBDebug("Data initialized");
    
    float *sdata = ((float*)dataObjects[0]->cdfVariable->data);
    
    
    if(projectionRequired){
      int status = imageWarper.initreproj(dataSource,dataSource->srvParams->Geo,&dataSource->srvParams->cfg->Projection);
      if(status !=0 ){
        CDBError("Unable to init projection");
        return 1;
      }
    }
  
    float *swathData = (float*)swathVar->data;
    
    bool drawBilinear=false;
    bool drawNearestWithGouraud = false;
    CStyleConfiguration *styleConfiguration = dataSource->getStyle();
    if(styleConfiguration->styleCompositionName.indexOf("bilinear")>=0){
      
      drawBilinear=true;
    }
    try{
      cdfObject->getVariable("lon_bnds");
      cdfObject->getVariable("lat_bnds");
    }catch(int e){
      drawBilinear=true;
      drawNearestWithGouraud = true;
    }
    /*
     * Bilinear rendering is based on gouraud shading using the center of each quads by using lat and lon variables, while nearest neighbour rendering is based on lat_bnds and lat_bnds variables, drawing the corners of the quads..
     */
      
 //drawBilinear=true;
    //Bilinear rendering
    if(drawBilinear){
     
      CDF::Variable *swathMiddleLon;
      CDF::Variable *swathMiddleLat;
  
      try{
        swathMiddleLon = cdfObject->getVariable("lon");
        swathMiddleLat = cdfObject->getVariable("lat");
      }catch(int e){
        CDBError("lat or lon variables not found");
        return 1;
      }
      
//       int numRows = cdfObject->getDimension("row")->getSize();
//       int numCols = cdfObject->getDimension("col")->getSize();

      int numRows = swathMiddleLon->dimensionlinks[0]->getSize();
      int numCols = swathMiddleLon->dimensionlinks[1]->getSize();
      
      swathMiddleLon->readData(CDF_FLOAT,true);
      swathMiddleLat->readData(CDF_FLOAT,true);
    
  
      float *lonData=(float*)swathMiddleLon->data;
      float *latData=(float*)swathMiddleLat->data;
      
        
      float fillValueLon = NAN;
      float fillValueLat = NAN;
      
      try{swathMiddleLon->getAttribute("_FillValue")->getData(&fillValueLon,1);}catch(int e){};
      try{swathMiddleLat->getAttribute("_FillValue")->getData(&fillValueLat,1);}catch(int e){};
      if (1==1) {
        for(int y=0;y<numRows-1;y++){
          for(int x=0;x<numCols-1;x++){ 
            size_t pSwath = x+y*numCols;
            //CDBDebug("%d %d %d",x,y,pSwath);
            double lons[4],lats[4];
            float vals[4];
            lons[0] = (float)lonData[pSwath];
            lons[1] = (float)lonData[pSwath+1];
            lons[2] = (float)lonData[pSwath+numCols];
            lons[3] = (float)lonData[pSwath+numCols+1];
            
            
          
            lats[0] = (float)latData[pSwath];
            lats[1] = (float)latData[pSwath+1];
            lats[2] = (float)latData[pSwath+numCols];
            lats[3] = (float)latData[pSwath+numCols+1];
            
            vals[0] = swathData[pSwath];
            vals[1]=  swathData[pSwath+1];
            vals[2] = swathData[pSwath+numCols];
            vals[3] = swathData[pSwath+numCols+1];
            
            if (drawNearestWithGouraud){
              vals[1]=vals[0];
              vals[2]=vals[0];
              vals[3]=vals[0];
            }

            bool tileHasNoData = false;
            float lonMin,lonMax,lonMiddle=0;
            for(int j=0;j<4;j++){
              float lon = lons[j];
              if(j==0){lonMin=lon;lonMax=lon;}else{
                if(lon<lonMin)lonMin=lon;
                if(lon>lonMax)lonMax=lon;
              }
              lonMiddle+=lon;
              float lat = lats[j];
              float val = vals[j];
              if(val==fill||val==INFINITY||val==NAN||val==-INFINITY||!(val==val)){tileHasNoData=true;break;}
              if(lat==fillValueLat||lat==INFINITY||lat==-INFINITY||!(lat==lat)){tileHasNoData=true;break;}
              if(lon==fillValueLon||lon==INFINITY||lon==-INFINITY||!(lon==lon)){tileHasNoData=true;break;}
            
            }
            lonMiddle/=4;
            if(lonMax-lonMin>=350){
              if(lonMiddle>0){
                for(int j=0;j<4;j++)if(lons[j]<lonMiddle)lons[j]+=360;
              }else{
                for(int j=0;j<4;j++)if(lons[j]>lonMiddle)lons[j]-=360;
              }
            }
            if(tileHasNoData==false){
              int dlons[4],dlats[4];
              bool projectionIsOk = true;
              for(int j=0;j<4;j++){
  //               if(tileIsOverDateBorder){
  //                 lons[j]-=360;
  //                 if(lons[j]<-280)lons[j]+=360;
  //               }
  //               
                if(projectionRequired){
                  if(imageWarper.reprojfromLatLon(lons[j],lats[j])!=0)projectionIsOk = false;
                }
                
                dlons[j]=int((lons[j]-offsetX)/cellSizeX);
                dlats[j]=int((lats[j]-offsetY)/cellSizeY);
              }
              if(projectionIsOk){
                fillQuadGouraud(sdata, vals, dataSource->dWidth,dataSource->dHeight, dlons,dlats);
              }
                
            }
          }
        }
      }
    }
    
    //Nearest neighbour rendering
    if(drawBilinear==false){
      CDF::Variable *swathLon;
      CDF::Variable *swathLat;

      try{
        swathLon = cdfObject->getVariable("lon_bnds");
        swathLat = cdfObject->getVariable("lat_bnds");
      }catch(int e){
        CDBError("lat or lon variables not found");
        return 1;
      }
      
      int numRows = swathLon->dimensionlinks[1]->getSize();
      int numCols = swathLon->dimensionlinks[0]->getSize();
 #ifdef CCONVERTCURVILINEAR_DEBUG      
      CDBDebug("NumRows %d, NumCols %d",numRows,numCols);
#endif      
      int numTiles = numRows * numCols;
      
     // int numTiles =     cdfObject->getDimension("col")->getSize()*cdfObject->getDimension("row")->getSize();
      
      #ifdef CCONVERTCURVILINEAR_DEBUG
      CDBDebug("There are %d tiles",numTiles);
      #endif
     
      swathLon->readData(CDF_FLOAT,true);
      swathLat->readData(CDF_FLOAT,true);
      float *lonData=(float*)swathLon->data;
      float *latData=(float*)swathLat->data;
      
      float fillValueLon = NAN;
      float fillValueLat = NAN;
      
      try{swathLon->getAttribute("_FillValue")->getData(&fillValueLon,1);}catch(int e){};
      try{swathLat->getAttribute("_FillValue")->getData(&fillValueLat,1);}catch(int e){};
      
      for(int pSwath=0;pSwath<numTiles;pSwath++){ 

        
        double lons[4],lats[4];
        float vals[4];
        lons[0] = (double)lonData[pSwath*4+0];
        lons[1] = (double)lonData[pSwath*4+1];
        lons[2] = (double)lonData[pSwath*4+3];
        lons[3] = (double)lonData[pSwath*4+2];
        
        lats[0] = (double)latData[pSwath*4+0];
        lats[1] = (double)latData[pSwath*4+1];
        lats[2] = (double)latData[pSwath*4+3];
        lats[3] = (double)latData[pSwath*4+2];
        
        vals[0] = swathData[pSwath];
        vals[1]=  vals[0];
        vals[2] = vals[0];
        vals[3] = vals[0];

        bool tileHasNoData = false;
        
      
        float lonMin,lonMax,lonMiddle=0;
        for(int j=0;j<4;j++){
          float lon = lons[j];
          if(j==0){lonMin=lon;lonMax=lon;}else{
            if(lon<lonMin)lonMin=lon;
            if(lon>lonMax)lonMax=lon;
          }
          lonMiddle+=lon;
          float lat = lats[j];
          float val = vals[j];
          if(val==fill||val==INFINITY||val==NAN||val==-INFINITY||!(val==val)){tileHasNoData=true;break;}
          if(lat==fillValueLat||lat==INFINITY||lat==-INFINITY||!(lat==lat)){tileHasNoData=true;break;}
          if(lon==fillValueLon||lon==INFINITY||lon==-INFINITY||!(lon==lon)){tileHasNoData=true;break;}
        
        }
        lonMiddle/=4;
        if(lonMax-lonMin>=350){
          if(lonMiddle>0){
             for(int j=0;j<4;j++)if(lons[j]<lonMiddle)lons[j]+=360;
          }else{
            for(int j=0;j<4;j++)if(lons[j]>lonMiddle)lons[j]-=360;
          }
        }
        if(tileHasNoData==false){
          int dlons[4],dlats[4];
          for(int j=0;j<4;j++){
            if(projectionRequired)if(imageWarper.reprojfromLatLon(lons[j],lats[j])!=0){tileHasNoData=true;break;}
            dlons[j]=int((lons[j]-offsetX)/cellSizeX);
            dlats[j]=int((lats[j]-offsetY)/cellSizeY);
          }
          if(tileHasNoData==false){
            fillQuadGouraud(sdata, vals, dataSource->dWidth,dataSource->dHeight, dlons,dlats);
          }
        }
      }
    }
    imageWarper.closereproj();
   
  }
  #ifdef CCONVERTCURVILINEAR_DEBUG
  CDBDebug("/convertCurvilinearData");
  #endif
  return 0;
}
