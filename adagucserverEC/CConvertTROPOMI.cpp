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

#include "CConvertTROPOMI.h"
#include "CFillTriangle.h"
#include "CImageWarper.h"

// #define CCONVERTTROPOMI_DEBUG

void writeLogFile4(const char * msg){
  char * logfile=getenv("ADAGUC_LOGFILE");
  if(logfile!=NULL){
    FILE * pFile = NULL;
    pFile = fopen (logfile , "a" );
    if(pFile != NULL){
      fputs  (msg, pFile );
      if(strncmp(msg,"[D:",3)==0||strncmp(msg,"[W:",3)==0||strncmp(msg,"[E:",3)==0){
        time_t myTime = time(NULL);
        tm *myUsableTime = localtime(&myTime);
        char szTemp[128];
        snprintf(szTemp,127,"%.4d-%.2d-%.2dT%.2d:%.2d:%.2dZ ",
                 myUsableTime->tm_year+1900,myUsableTime->tm_mon+1,myUsableTime->tm_mday,
                 myUsableTime->tm_hour,myUsableTime->tm_min,myUsableTime->tm_sec
                );
        fputs  (szTemp, pFile );
      }
      fclose (pFile);
    }//else CDBError("Unable to write logfile %s",logfile);
  }
}
const char *CConvertTROPOMI::className="CConvertTROPOMI";

int CConvertTROPOMI::isThisTROPOMIData( CDFObject *cdfObject){
  try{
    if(cdfObject->getAttribute("cdm_data_type")->getDataAsString().equals("Swath")==false)return 1;
    if(cdfObject->getAttribute("sensor")->getDataAsString().equals("TROPOMI")==false)return 1;
  }catch(int e){
    return 1;
  }
  return 0;//OK!
};


/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertTROPOMI::convertTROPOMIHeader( CDFObject *cdfObject,CServerParams *srvParams  ){
  //Check whether this is really an TROPOMI file
  if(isThisTROPOMIData(cdfObject)!=0)return 1;
  CDBDebug("Using CConvertTROPOMI.h");

  //Standard bounding box of adaguc data is worldwide
  CDF::Variable *pointLon;
  CDF::Variable *pointLat;
  
  try{
    pointLon = cdfObject->getVariable("PRODUCT/SUPPORT_DATA/GEOLOCATIONS/longitude_bounds");
    pointLat = cdfObject->getVariable("PRODUCT/SUPPORT_DATA/GEOLOCATIONS/latitude_bounds");
  }catch(int e){
    CDBError("lat or lon variables not found");
    return 1;
  }
  
  
  CDBDebug("start reading latlon coordinates");
  
  pointLon->readData(CDF_FLOAT,true);
  pointLat->readData(CDF_FLOAT,true);
  
  #ifdef CCONVERTTROPOMI_DEBUG
    StopWatch_Stop("DATA READ");
  #endif
  MinMax lonMinMax; 
  MinMax latMinMax;
  lonMinMax.min = -180; //Initialize to whole world
  latMinMax.min = -90;
  lonMinMax.max = 180;
  latMinMax.max = 90;
  if(pointLon->getSize() > 0){
    lonMinMax = getMinMax(pointLon);
    latMinMax = getMinMax(pointLat);
  }
  #ifdef CCONVERTTROPOMI_DEBUG
    StopWatch_Stop("MIN/MAX Calculated");
  #endif
  double dfBBOX[]={lonMinMax.min-0.5,latMinMax.min-0.5,lonMinMax.max+0.5,latMinMax.max+0.5};
  //double dfBBOX[]={-180,-90,180,90};
  //CDBDebug("Datasource dfBBOX:%f %f %f %f",dfBBOX[0],dfBBOX[1],dfBBOX[2],dfBBOX[3]);
  
  //Default size of adaguc 2dField is 2x2
  int width=2;
  int height=2;
  
  double cellSizeX=(dfBBOX[2]-dfBBOX[0])/double(width);
  double cellSizeY=(dfBBOX[3]-dfBBOX[1])/double(height);
  double offsetX=dfBBOX[0];
  double offsetY=dfBBOX[1];
  
  //Add geo variables, only if they are not there already
  CDF::Dimension *dimX = cdfObject->getDimensionNE("x");
  CDF::Dimension *dimY = cdfObject->getDimensionNE("y");
  
  CDF::Variable *varX = cdfObject->getVariableNE("x");
  CDF::Variable *varY = cdfObject->getVariableNE("y");
  if(dimX==NULL||dimY==NULL||varX==NULL||varY==NULL) {
    //If not available, create new dimensions and variables (X,Y,T)
    //For x 
    dimX=new CDF::Dimension();
    dimX->name="x";
    dimX->setSize(width);
    cdfObject->addDimension(dimX);
    varX = new CDF::Variable();
    varX->setType(CDF_DOUBLE);
    varX->name.copy("x");
    varX->isDimension=true;
    varX->dimensionlinks.push_back(dimX);
    cdfObject->addVariable(varX);
    CDF::allocateData(CDF_DOUBLE,&varX->data,dimX->length);

    //For y 
    dimY=new CDF::Dimension();
    dimY->name="y";
    dimY->setSize(height);
    cdfObject->addDimension(dimY);
    varY = new CDF::Variable();
    varY->setType(CDF_DOUBLE);
    varY->name.copy("y");
    varY->isDimension=true;
    varY->dimensionlinks.push_back(dimY);
    cdfObject->addVariable(varY);
    CDF::allocateData(CDF_DOUBLE,&varY->data,dimY->length);
    

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


  CDF::Dimension *dimT = cdfObject->getDimensionNE("time");
  CDF::Variable *varT = cdfObject->getVariableNE("time");
  CDF::Variable *productT = cdfObject->getVariableNE("PRODUCT/time");
  if (!productT) {
    CDBError("Could not get PRODUCT/time variable");
    return 1;
  }
  dimT=new CDF::Dimension();
  dimT->name="time";
  dimT->setSize(1);
  cdfObject->addDimension(dimT);
  varT = new CDF::Variable();
  varT->setType(CDF_DOUBLE);
  varT->name.copy("time");
  varT->isDimension=true;
  varT->dimensionlinks.push_back(dimT);
  cdfObject->addVariable(varT);
  CDF::allocateData(CDF_DOUBLE,&varT->data,dimT->length);
  varT->setAttributeText("standard_name", "time");
  try {
    varT->setAttributeText("units", productT->getAttribute("units")->toString().c_str());
    CTime myTime;
    myTime.init(productT);
    //CTime::cleanInstances();
    CTime::Date date =myTime.freeDateStringToDate(cdfObject->getAttribute("time_coverage_start")->toString().c_str());
    ((double*)varT->data)[0] = myTime.dateToOffset(date);
  } catch (int) {
    CDBError("Could not get units for time_coverage_start");
    return 1;
  }

  CT::StackList<CT::string> varsToConvert;
  for(size_t v=0;v<cdfObject->variables.size();v++){
    CDF::Variable *var = cdfObject->variables[v];
    if(var->isDimension==false){
      if(var->name.startsWith("PRODUCT/")&&var->dimensionlinks.size()>2){
        if(
          !var->name.equals("PRODUCT/longitude")&&
          !var->name.equals("PRODUCT/latitude")&&
          var->name.indexOf("bounds")==-1&&
          !var->name.equals("PRODUCT/time")
        ){
          varsToConvert.add(CT::string(var->name.c_str()));
        }
      }

      var->setAttributeText("ADAGUC_SKIP","true");

    }
  }

  //Create the new 2D field variables based on the swath variables
  for(size_t v=0;v<varsToConvert.size();v++){
    CDF::Variable *swathVar=cdfObject->getVariable(varsToConvert[v].c_str());

    #ifdef CCONVERTTROPOMI_DEBUG
    CDBDebug("Converting %s",swathVar->name.c_str());
    #endif

    CDF::Variable *new2DVar = new CDF::Variable();
    cdfObject->addVariable(new2DVar);

    //Assign X,Y,T dims
/*
    CDF::Variable *newTimeVar=cdfObject->getVariableNE("time");
    if(newTimeVar!=NULL){
      new2DVar->dimensionlinks.push_back(newTimeVar->dimensionlinks[0]);
    }*/

    //new2DVar->dimensionlinks.push_back(dimT);
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
    new2DVar->removeAttribute("ADAGUC_SKIP");
    swathVar->setAttributeText("ADAGUC_SKIP","true");
    
    //Scale and offset are already applied
    new2DVar->removeAttribute("scale_factor");
    new2DVar->removeAttribute("add_offset");
    
    new2DVar->setType(CDF_FLOAT);
  }
  
//     CDF::Variable *new2DVar = new CDF::Variable();
//     cdfObject->addVariable(new2DVar);
//     new2DVar->dimensionlinks.push_back(dimY);
//     new2DVar->dimensionlinks.push_back(dimX);
//     new2DVar->setType(CDF_FLOAT);
//     new2DVar->name="testno2";
//     CT::string data = CDF::dump(cdfObject);
    
    //CDBDebug("%s",data.c_str());
   // writeLogFile4(data.c_str());

  
  return 0;
}



void CConvertTROPOMIline2(float *imagedata,int w,int h,float x1,float y1,float x2, float y2,float value){
  int xyIsSwapped=0;
  float dx = x2 - x1;
  float dy = y2 - y1;
  if(fabs(dx) < fabs(dy)){
    std::swap(x1, y1);
    std::swap(x2, y2);
    std::swap(dx, dy);
    xyIsSwapped=1;
  }
  if(x2 < x1){
    std::swap(x1, x2);
    std::swap(y1, y2);
  }
  float gradient = dy / dx;
  float y=y1;
  
  if(xyIsSwapped==0){
    
    
    for(int x=int(x1);x<x2;x++){
      //         plot(x,int(y),1);
      if(y>=0&&y<h&&x>=0&&x<w)imagedata[int(x)+int(y)*w]=value;
      y+=gradient;
    }
  }else{
    
    for(int x=int(x1);x<x2;x++){
      if(y>=0&&y<w&&x>=0&&x<h)imagedata[int(y)+int(x)*w]=value;
      y+=gradient;
    }
  }
}

void CConvertTROPOMIDrawlines(float *imagedata,int w,int h,int polyCorners,int *polyX,int *polyY,float value){
  for(int j=0;j<polyCorners-1;j++){
    CConvertTROPOMIline2(imagedata, w, h,polyX[j],polyY[j],polyX[j+1],polyY[j+1],value);
  }
  CConvertTROPOMIline2(imagedata, w, h,polyX[polyCorners-1],polyY[polyCorners-1],polyX[0],polyY[0],value);
}

/**
 * This function draws the virtual 2D variable into a new 2D field
 */
int CConvertTROPOMI::convertTROPOMIData(CDataSource *dataSource,int mode){
 
  CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
  if(isThisTROPOMIData(cdfObject)!=0)return 1;
  
  CDF::Variable *pointLon;
  CDF::Variable *pointLat;
  try{
    pointLon = cdfObject->getVariable("PRODUCT/SUPPORT_DATA/GEOLOCATIONS/longitude_bounds");
    pointLat = cdfObject->getVariable("PRODUCT/SUPPORT_DATA/GEOLOCATIONS/latitude_bounds");
  }catch(int e){
    CDBError("lat or lon variables not found");
    return 1;
  }
  
  pointLon->readData(CDF_FLOAT,true);
  pointLat->readData(CDF_FLOAT,true);
  
  size_t nrDataObjects = dataSource->getNumDataObjects();
  
   //Make the width and height of the new 2D adaguc field the same as the viewing window
  dataSource->dWidth=dataSource->srvParams->Geo->dWidth;
  dataSource->dHeight=dataSource->srvParams->Geo->dHeight;      
  
  if(dataSource->dWidth == 1 && dataSource->dHeight == 1){
    dataSource->srvParams->Geo->dfBBOX[0]=dataSource->srvParams->Geo->dfBBOX[0];
    dataSource->srvParams->Geo->dfBBOX[1]=dataSource->srvParams->Geo->dfBBOX[1];
    dataSource->srvParams->Geo->dfBBOX[2]=dataSource->srvParams->Geo->dfBBOX[2];
    dataSource->srvParams->Geo->dfBBOX[3]=dataSource->srvParams->Geo->dfBBOX[3];
  }
  
  //Width needs to be at least 2 in this case.
  if(dataSource->dWidth == 1)dataSource->dWidth=2;
  if(dataSource->dHeight == 1)dataSource->dHeight=2;
  double cellSizeX=(dataSource->srvParams->Geo->dfBBOX[2]-dataSource->srvParams->Geo->dfBBOX[0])/double(dataSource->dWidth);
  double cellSizeY=(dataSource->srvParams->Geo->dfBBOX[3]-dataSource->srvParams->Geo->dfBBOX[1])/double(dataSource->dHeight);
  double offsetX=dataSource->srvParams->Geo->dfBBOX[0];
  double offsetY=dataSource->srvParams->Geo->dfBBOX[1];
  
  if(mode==CNETCDFREADER_MODE_OPEN_ALL){
   
    
    
    CDF::Dimension *dimX;
    CDF::Dimension *dimY;
    CDF::Variable *varX ;
    CDF::Variable *varY;
  
    //Create new dimensions and variables (X,Y,T)
    dimX=cdfObject->getDimension("x");
    dimX->setSize(dataSource->dWidth);
    
    dimY=cdfObject->getDimension("y");
    dimY->setSize(dataSource->dHeight);
    
    varX = cdfObject->getVariable("x");
    varY = cdfObject->getVariable("y");
    
    CDF::allocateData(CDF_DOUBLE,&varX->data,dimX->length);
    CDF::allocateData(CDF_DOUBLE,&varY->data,dimY->length);
    
    //Fill in the X and Y dimensions with the array of coordinates
    for(size_t j=0;j<dimX->length;j++){
      double x=offsetX+double(j)*cellSizeX+cellSizeX/2;
      ((double*)varX->data)[j]=x;
    }
    for(size_t j=0;j<dimY->length;j++){
      double y=offsetY+double(j)*cellSizeY+cellSizeY/2;
      ((double*)varY->data)[j]=y;
    }
    
    #ifdef CCONVERTTROPOMI_DEBUG
      StopWatch_Stop("Dimensions set");
    #endif
  }
  
  
  if(mode==CNETCDFREADER_MODE_OPEN_ALL){
      
    CDF::Variable *new2DVar[nrDataObjects];
    CDF::Variable *pointVar[nrDataObjects];
    
    
    for(size_t d=0;d<nrDataObjects;d++){
      new2DVar[d] = dataSource->getDataObject(d)->cdfVariable;
      CT::string origSwathName=new2DVar[d]->name.c_str();
      origSwathName.concat("_backup");
      pointVar[d]=dataSource->getDataObject(d)->cdfObject->getVariableNE(origSwathName.c_str());
      if(pointVar[d]==NULL){
        CDBError("Unable to find orignal swath variable with name %s",origSwathName.c_str());
        return 1;
      }
      pointVar[d]->readData(CDF_FLOAT);
    }
    
    
    CDF::Attribute *fillValue = pointVar[0]->getAttributeNE("_FillValue");
    if(fillValue!=NULL){
      
      dataSource->getDataObject(0)->hasNodataValue=true;
      fillValue->getData(&dataSource->getDataObject(0)->dfNodataValue,1);
      #ifdef CCONVERTCURVILINEAR_DEBUG
      CDBDebug("_FillValue = %f",dataSource->getDataObject(0)->dfNodataValue);
      #endif
      CDF::Attribute *fillValue2d = pointVar[0]->getAttributeNE("_FillValue");
      if(fillValue2d == NULL){
        fillValue2d = new CDF::Attribute();
        fillValue2d->name = "_FillValue";
      }
      float f=dataSource->getDataObject(0)->dfNodataValue;
      fillValue2d->setData(CDF_FLOAT,&f,1);
    }else {
      dataSource->getDataObject(0)->hasNodataValue=true;
      dataSource->getDataObject(0)->dfNodataValue=NC_FILL_FLOAT;
      float f=dataSource->getDataObject(0)->dfNodataValue;
      pointVar[0]->setAttribute("_FillValue",CDF_FLOAT,&f,1);
    }
    
    float fill = (float)dataSource->getDataObject(0)->dfNodataValue;
    if(dataSource->stretchMinMax){
      if(dataSource->statistics==NULL){

        dataSource->statistics = new CDataSource::Statistics();
        dataSource->statistics->calculate(pointVar[0]->getSize(),(float*)pointVar[0]->data,CDF_FLOAT,dataSource->getDataObject(0)->dfNodataValue, dataSource->getDataObject(0)->hasNodataValue);
//         dataSource->statistics->setMaximum(max);
//         dataSource->statistics->setMinimum(min);
      }
    }
    
    
     //Allocate 2D field
    for(size_t d=0;d<nrDataObjects;d++){
      size_t fieldSize = dataSource->dWidth*dataSource->dHeight;
      new2DVar[d]->setSize(fieldSize);
      CDF::allocateData(new2DVar[d]->getType(),&(new2DVar[d]->data),fieldSize);
      //CDF::fill((new2DVar[d]->data),new2DVar[d]->getType(),NAN,fieldSize);
      
    //Fill in nodata
      if(dataSource->getDataObject(d)->hasNodataValue){
        for(size_t j=0;j<fieldSize;j++){
          ((float*)dataSource->getDataObject(d)->cdfVariable->data)[j]=NAN;//(float)dataSource->getDataObject(d)->dfNodataValue;
        }
      }else{
        for(size_t j=0;j<fieldSize;j++){
          ((float*)dataSource->getDataObject(d)->cdfVariable->data)[j]=NAN;
        }
      }
    }
    
    CImageWarper imageWarper;
    bool projectionRequired=false;
    if(dataSource->srvParams->Geo->CRS.length()>0){
      projectionRequired=true;
      for(size_t d=0;d<nrDataObjects;d++){
        new2DVar[d]->setAttributeText("grid_mapping","customgridprojection");
      }
      if(cdfObject->getVariableNE("customgridprojection")==NULL){
        CDF::Variable *projectionVar = new CDF::Variable();
        projectionVar->name.copy("customgridprojection");
        cdfObject->addVariable(projectionVar);
        dataSource->nativeEPSG = dataSource->srvParams->Geo->CRS.c_str();
        imageWarper.decodeCRS(&dataSource->nativeProj4,&dataSource->nativeEPSG,&dataSource->srvParams->cfg->Projection);
        if(dataSource->nativeProj4.length()==0){
          dataSource->nativeProj4=LATLONPROJECTION;
          dataSource->nativeEPSG="EPSG:4326";
          projectionRequired=false;
        }
        projectionVar->setAttributeText("proj4_params",dataSource->nativeProj4.c_str());
      }
    }
    
    
    #ifdef CCONVERTTROPOMI_DEBUG
    CDBDebug("Datasource CRS = %s nativeproj4 = %s",dataSource->nativeEPSG.c_str(),dataSource->nativeProj4.c_str());
    CDBDebug("Datasource bbox:%f %f %f %f",dataSource->srvParams->Geo->dfBBOX[0],dataSource->srvParams->Geo->dfBBOX[1],dataSource->srvParams->Geo->dfBBOX[2],dataSource->srvParams->Geo->dfBBOX[3]);
    CDBDebug("Datasource width height %d %d",dataSource->dWidth,dataSource->dHeight);
    #endif
    
    
    if(projectionRequired){
      int status = imageWarper.initreproj(dataSource,dataSource->srvParams->Geo,&dataSource->srvParams->cfg->Projection);
      if(status !=0 ){
        CDBError("Unable to init projection");
        return 1;
      }
    }
    
      
    float *sdata = ((float*)dataSource->getDataObject(0)->cdfVariable->data);

    float *lonData = (float*)pointLon->data;
    float *latData = (float*)pointLat->data;
    double lons[4],lats[4];
    float vals[4];
    
    int swathLonWidth = pointLon->dimensionlinks[pointLon->dimensionlinks.size()-2]->getSize();
    int swathLonHeight = pointLon->dimensionlinks[pointLon->dimensionlinks.size()-3]->getSize();
    
//     for(size_t j=0;j<pointLon->dimensionlinks.size();j++){
//       CDBDebug("%d %s %d",j,pointLon->dimensionlinks[j]->name.c_str(),pointLon->dimensionlinks[j]->getSize());
//     }
//     
//     CDBDebug("swathLonWidth: %d",swathLonWidth);
//     CDBDebug("swathLonHeight: %d",swathLonHeight);
    
    float fillValueLat = fill;
    float fillValueLon = fill;
    int mode = 0;
    //for( mode=0;mode<2;mode++)
    {
    for(int y=0;y<swathLonHeight;y++){
      for(int x=0;x<swathLonWidth;x++){
        
        size_t pSwath = (x+y*swathLonWidth);
        float val  = ((float*)pointVar[0]->data)[pSwath];
        lons[0] = (float)lonData[pSwath*4+0];
        lons[1] = (float)lonData[pSwath*4+1];
        lons[2] = (float)lonData[pSwath*4+3];
        lons[3] = (float)lonData[pSwath*4+2];
        
        lats[0] = (float)latData[pSwath*4+0];
        lats[1] = (float)latData[pSwath*4+1];
        lats[2] = (float)latData[pSwath*4+3];
        lats[3] = (float)latData[pSwath*4+2];
        
        //CDBDebug("%d %d = %f %f  %f",x,y, lons[0],lats[0],val);
        
        vals[0] = val;
        vals[1]=  val;
        vals[2] = val;
        vals[3] = val;
          
        
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
        
        int dlons[4],dlats[4];
        bool projectionIsOk = true;
        if(tileHasNoData==false){

          for(int j=0;j<4;j++){
            if(projectionRequired){
              if(imageWarper.reprojfromLatLon(lons[j],lats[j])!=0)projectionIsOk = false;
            }
            dlons[j]=int((lons[j]-offsetX)/cellSizeX);
            dlats[j]=int((lats[j]-offsetY)/cellSizeY);
          }
          if(projectionIsOk){
            if(mode==0){fillQuadGouraud(sdata, vals, dataSource->dWidth,dataSource->dHeight, dlons,dlats);}
            val=1;
//             if(mode==1){
//             drawCircle(sdata,1,dataSource->dWidth,dataSource->dHeight,dlons[0],dlats[0],8);
//             drawCircle(sdata,2,dataSource->dWidth,dataSource->dHeight,dlons[1],dlats[1],8);
//             drawCircle(sdata,3,dataSource->dWidth,dataSource->dHeight,dlons[2],dlats[2],8);
//             drawCircle(sdata,4,dataSource->dWidth,dataSource->dHeight,dlons[3],dlats[3],8);
//             
//             CConvertTROPOMIDrawlines(sdata,dataSource->dWidth,dataSource->dHeight,4,dlons,dlats,val);
//             }
          }
        
        }
        
      }
    }
    }
  }
  #ifdef CCONVERTTROPOMI_DEBUG
  CDBDebug("/convertTROPOMIData");
  #endif
  return 0;
}
