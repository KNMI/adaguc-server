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

#include "CConvertADAGUCPoint.h"
#include "CFillTriangle.h"
#include "CImageWarper.h"
// #define CCONVERTADAGUCPOINT_DEBUG
// #define MEASURETIME
const char *CConvertADAGUCPoint::className="CConvertADAGUCPoint";
void drawDot(int px, int py, int v, int W, int H, float * grid){
  for(int x = -4;x<6;x++){
    for(int y = -4;y<6;y++){
      int pointX = px+x;
      int pointY = py+y;
      if(pointX >= 0 && pointY >=0 && pointX < W && pointY< H)grid[pointX+pointY*W]=v;
    }
  }
}

void CConvertADAGUCPoint::lineInterpolated(float *grid , int W,int H, int startX,int startY, int stopX, int stopY, float startVal, float stopVal){
  // drawImage->setPixelTrueColor(startX,startY, 0,0,0,255);

  int dX = stopX - startX;
  int dY = stopY - startY;
  if(abs(dX)>5000)return;
  if(abs(dY)>5000)return;
  float rc = 0;
  int numPoints = 0;
  if(!(startVal == startVal) || !(stopVal==stopVal)){
    startVal = 100;
    stopVal =100;
  }
  float valD = stopVal - startVal;
  float angle = atan2(dY,dX) + (3.141592654/2);
  float dist = 10;
  if(abs(dX) < abs(dY)){
    rc = float(dX)/float(dY);
    int sx = startX;
    int sy = startY;
    float myVal = startVal;
    if(dY > 0)numPoints = dY;else {numPoints = -dY;sx=stopX,sy=stopY;myVal = stopVal;valD = -valD;}
    float valRc = valD / numPoints;
    for(int p = 0; p< numPoints;p++){
      float v = valRc*float(p)+myVal;
      
      float px = float(p) * rc + sx +cos(angle)*dist;
      float py = p + sy+sin(angle)*dist;
      drawDot(px,py,v,W,H, grid);
    }
  }else{
    int sx = startX;
    int sy = startY;
    float myVal = startVal;
    rc = float(dY)/float(dX);
    if(dX > 0)numPoints = dX;else {numPoints = -dX;sx=stopX,sy=stopY;myVal = stopVal;valD = -valD;}
    float valRc = valD / numPoints;
    for(int p = 0; p< numPoints;p++){
      float v = valRc*float(p)+myVal;
      
      float px = p + sx + cos(angle)*dist;
      float py = float(p) * rc +sy + sin(angle)*dist;
      drawDot(px,py,v,W,H, grid);
      
    }
  }
  
  
  
}

void CConvertADAGUCPoint::convert_BIRA_IASB_NETCDF(CDFObject *cdfObject) {
  try{
    if (cdfObject->getAttribute("source")->toString().equals("BIRA-IASB NETCDF") && cdfObject->getVariableNE("obs") == NULL){
      CT::string timeString = cdfObject->getAttribute("measurement_time")->toString();
      cdfObject->setAttributeText("featureType", "point");
      CDF::Variable *time= cdfObject->getVariable("time");
      CDF::Dimension*dim= cdfObject->getDimension("time");
      dim->name="obs";
      time->name="obs";
      

      CDF::Dimension*realTimeDim;
      CDF::Variable*realTimeVar;
      
      realTimeDim=new CDF::Dimension();
      realTimeDim->name="time";
      realTimeDim->setSize(1);
      cdfObject->addDimension(realTimeDim);
      realTimeVar = new CDF::Variable();
      realTimeVar->setType(CDF_DOUBLE);
      realTimeVar->name.copy("time");
      realTimeVar->setAttributeText("standard_name", "time");
      realTimeVar->setAttributeText("units", "seconds since 1970-01-01 0:0:0");
      realTimeVar->isDimension=true;
      realTimeVar->dimensionlinks.push_back(realTimeDim);
      cdfObject->addVariable(realTimeVar);
      CDF::allocateData(CDF_DOUBLE,&realTimeVar->data,realTimeDim->length);
      CTime ctime;
      ctime.init("seconds since 1970",NULL);
      ((double*)realTimeVar->data)[0] = ctime.dateToOffset( ctime.freeDateStringToDate(timeString));
      for(size_t v=0;v<cdfObject->variables.size();v++){
        CDF::Variable *var = cdfObject->variables[v];
        if(var->isDimension==false){
          if(!var->name.equals("time2D")&&
            !var->name.equals("time")&&
            !var->name.equals("lon")&&
            !var->name.equals("lat")&&
            !var->name.equals("x")&&
            !var->name.equals("y")&&
            !var->name.equals("lat_bnds")&&
            !var->name.equals("lon_bnds")&&
            !var->name.equals("custom")&&
            !var->name.equals("projection")&&
            !var->name.equals("product")&&
            !var->name.equals("iso_dataset")&&
            !var->name.equals("tile_properties")&&
            !var->name.equals("forecast_reference_time")
          ){
            var->dimensionlinks.push_back(realTimeDim);
          }
        }
      }
      // CDBDebug("%s", CDF::dump(cdfObject).c_str());

    }
  }catch(int e){
  }
}


int CConvertADAGUCPoint::checkIfADAGUCPointFormat(CDFObject *cdfObject) {
  /* Some conversions for non ADAGUC point data */
  convert_BIRA_IASB_NETCDF(cdfObject);
    
  try{
    if(cdfObject->getAttribute("featureType")->toString().equals("timeSeries")==false&&cdfObject->getAttribute("featureType")->toString().equals("point")==false)return 1;
  }catch(int e){
    return 1;
  }
  return 0;
}

/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertADAGUCPoint::convertADAGUCPointHeader( CDFObject *cdfObject ){
  //Check whether this is really an adaguc file
  if (CConvertADAGUCPoint::checkIfADAGUCPointFormat(cdfObject) == 1) return 1;
  CDBDebug("Using CConvertADAGUCPoint.h");

  
 
  //Standard bounding box of adaguc data is worldwide
  CDF::Variable *pointLon;
  CDF::Variable *pointLat;
  
  try{
    pointLon = cdfObject->getVariable("lon");
    pointLat = cdfObject->getVariable("lat");
  }catch(int e){
    CDBError("lat or lon variables not found");
    return 1;
  }
  
  
  #ifdef MEASURETIME
    StopWatch_Stop("ADAGUC POINT DATA");
  #endif
  if (pointLon->readData(CDF_FLOAT,true)!=0){
    CDBError("Unable to read lon data");
    return 1;
  }
  if (pointLat->readData(CDF_FLOAT,true)!=0){
    CDBError("Unable to read lat data");
    return 1;
  }
  
  #ifdef MEASURETIME
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
  #ifdef MEASURETIME
    StopWatch_Stop("MIN/MAX Calculated");
  #endif
  double dfBBOX[]={lonMinMax.min,latMinMax.min,lonMinMax.max,latMinMax.max};
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
  
  #ifdef MEASURETIME
    StopWatch_Stop("2D Coordinate dimensions created");
  #endif
  
  //Make a list of variables which will be available as 2D fields  
  CT::StackList<CT::string> varsToConvert;
  for(size_t v=0;v<cdfObject->variables.size();v++){
    CDF::Variable *var = cdfObject->variables[v];
    if(var->isDimension==false){
      //if(var->getType()!=CDF_STRING)
      {
        if(!var->name.equals("time2D")&&
          !var->name.equals("time")&&
          !var->name.equals("lon")&&
          !var->name.equals("lat")&&
          !var->name.equals("x")&&
          !var->name.equals("y")&&
          !var->name.equals("lat_bnds")&&
          !var->name.equals("lon_bnds")&&
          !var->name.equals("custom")&&
          !var->name.equals("projection")&&
          !var->name.equals("product")&&
          !var->name.equals("iso_dataset")&&
          !var->name.equals("tile_properties")&&
          !var->name.equals("forecast_reference_time")
        ){
          varsToConvert.add(CT::string(var->name.c_str()));
        }
        if(var->name.equals("projection")){
          var->setAttributeText("ADAGUC_SKIP","true");
        }
      }
    }
  }
  
  //Create the new 2D field variables based on the swath variables
  for(size_t v=0;v<varsToConvert.size();v++){
    CDF::Variable *pointVar=cdfObject->getVariable(varsToConvert[v].c_str());
    
    #ifdef CCONVERTADAGUCPOINT_DEBUG
    CDBDebug("Converting %s",pointVar->name.c_str());
    #endif
    
    CDF::Variable *new2DVar = new CDF::Variable();
    cdfObject->addVariable(new2DVar);
   
    
    //Assign X,Y,T dims 
    
    for(size_t d=0;d<pointVar->dimensionlinks.size();d++){
      bool skip = false;
      if(pointVar->dimensionlinks[d]->name.equals("station")==true){
        skip = true;
      }
      if(d == 0 && pointVar->dimensionlinks[d]->name.equals("time") == false){
        skip = true;
      }
      if(!skip){
        new2DVar->dimensionlinks.push_back( pointVar->dimensionlinks[d]);
      }
    }
    
    //if(dimT!=NULL)new2DVar->dimensionlinks.push_back(dimT);
    new2DVar->dimensionlinks.push_back(dimY);
    new2DVar->dimensionlinks.push_back(dimX);
    
    
    //new2DVar->setType(pointVar->getType());
    
    
    new2DVar->setType(CDF_FLOAT);
    
    
    new2DVar->name=pointVar->name.c_str();
    pointVar->name.concat("_backup");
    
    //Copy variable attributes
    for(size_t j=0;j<pointVar->attributes.size();j++){
      CDF::Attribute *a =pointVar->attributes[j];
      if(a->name.equals("_FillValue")){
        float scaleFactor=1,addOffset=0,fillValue=0;;
        try{
          pointVar->getAttribute("scale_factor")->getData(&scaleFactor,1);
        }catch(int e){}
        try{
          pointVar->getAttribute("add_offset")->getData(&addOffset,1);
        }catch(int e){}
        try{
          pointVar->getAttribute("_FillValue")->getData(&fillValue,1);
        }catch(int e){}
        fillValue*=scaleFactor+addOffset;
        new2DVar->setAttribute("_FillValue",CDF_FLOAT,&fillValue,1);
      }else{
        new2DVar->setAttribute(a->name.c_str(),a->getType(),a->data,a->length);
      }
      
    }
    new2DVar->setAttributeText("ADAGUC_POINT","true");
    
    //if(new2DVar->getType()!=CDF_STRING){
      if(new2DVar->getAttributeNE("_FillValue")==NULL){
        float f=-9999999;
        new2DVar->setAttribute("_FillValue",CDF_FLOAT,&f,1);
      }
    //}
    

    
    //The swath variable is not directly plotable, so skip it
    pointVar->setAttributeText("ADAGUC_SKIP","true");
    
    //Scale and offset are already applied
    new2DVar->removeAttribute("scale_factor");
    new2DVar->removeAttribute("add_offset");
    
   
  }
  
  #ifdef MEASURETIME
    StopWatch_Stop("Header done");
  #endif

 
  return 0;
}




/**
 * This function draws the virtual 2D variable into a new 2D field
 */
int CConvertADAGUCPoint::convertADAGUCPointData(CDataSource *dataSource,int mode){
  #ifdef CCONVERTADAGUCPOINT_DEBUG
  CDBDebug("convertADAGUCPointData");
  #endif


  CDFObject *cdfObject0 = dataSource->getDataObject(0)->cdfObject;
  if (CConvertADAGUCPoint::checkIfADAGUCPointFormat(cdfObject0) == 1) return 1;
  //CDBDebug("THIS IS ADAGUC POINT DATA");
  
  #ifdef MEASURETIME
    StopWatch_Stop("Reading data");
  #endif
    
  CDF::Variable *pointLon;
  CDF::Variable *pointLat;
  
  try{
    pointLon = cdfObject0->getVariable("lon");
    pointLat = cdfObject0->getVariable("lat");
  }catch(int e){
    CDBError("lat or lon variables not found");
    return 1;
  }
  

  size_t nrDataObjects = dataSource->getNumDataObjects();
  
  CDataSource::DataObject *dataObjects[nrDataObjects];
  for(size_t d=0;d<nrDataObjects;d++){
    dataObjects[d] =  dataSource->getDataObject(d);
  }
  CDF::Variable *new2DVar[nrDataObjects];
  CDF::Variable *pointVar[nrDataObjects];
  
  
  for(size_t d=0;d<nrDataObjects;d++){
    new2DVar[d] = dataObjects[d]->cdfVariable;
    CT::string origSwathName=new2DVar[d]->name.c_str();
    origSwathName.concat("_backup");
    pointVar[d]=dataObjects[d]->cdfObject->getVariableNE(origSwathName.c_str());
    if(pointVar[d]==NULL){
      CDBWarning("Unable to find orignal swath variable with name %s",origSwathName.c_str());
    
     // return 1;
    }
    
  }
  
  //Read original data first 

  
  size_t numDims = pointVar[0]->dimensionlinks.size();
  size_t start[numDims];
  size_t count[numDims];
  ptrdiff_t stride[numDims];
  
  /*
   * There is always a station dimension, we wish to read all stations and for the other dimensions just one: count=1;
   * 
   * Therefore we need to know what the station dimension index is
   */
  
  /*First read LAT and LON*/
  
  /*Find which index is the station dim*/
  int stationDimIndexInCoord = -1;
  int numStations = 1;
  for(size_t j=0;j<pointLon->dimensionlinks.size();j++){
    if(
      pointLon->dimensionlinks[j]->name.equals("station")||
      cdfObject0->getVariableNE(pointLon->dimensionlinks[j]->name.c_str()) == NULL){
      stationDimIndexInCoord = j;
      break;
    }
  }
  /*Set dimension indices*/
  for(size_t j=0;j<pointLon->dimensionlinks.size();j++){
    if(stationDimIndexInCoord == int(j)){
      start[j] = 0;
      numStations=pointLon->dimensionlinks[j]->getSize();
      count[j] = numStations;
      stride[j]=1;
    }else{
      start[j] = dataSource->getDimensionIndex(pointLon->dimensionlinks[j]->name.c_str());
      count[j] = 1;
      stride[j]=1;
    }
    #ifdef CCONVERTADAGUCPOINT_DEBUG
    CDBDebug("%s %d %d",pointLon->dimensionlinks[j]->name.c_str(),start[j],count[j]);
    #endif
  }
  
  #ifdef CCONVERTADAGUCPOINT_DEBUG
    CDBDebug("numStations %d ",numStations);
    CDBDebug("numDims %d ",numDims);
  #endif
  
  
  
//   if(pointLon->dimensionlinks.size()>=2 ){
//     #ifdef CCONVERTADAGUCPOINT_DEBUG
//     CDBDebug("Dimension dependant locations");
//     #endif
    pointLon->freeData();
    pointLat->freeData();
    if (pointLon->readData(CDF_FLOAT,start,count,stride,true)!=0){
      CDBError("Unable to read pointLon data");
      return 1;
    }
    if(pointLat->readData(CDF_FLOAT,start,count,stride,true)!=0){
      CDBError("Unable to read pointLat data");
      return 1;
    }
//   }else{
//     #ifdef CCONVERTADAGUCPOINT_DEBUG
//     CDBDebug("NON Dimension dependant location");
//     #endif
//     pointLon->readData(CDF_FLOAT,true);
//     pointLat->readData(CDF_FLOAT,true);
//   }

    
  #ifdef MEASURETIME
    StopWatch_Stop("Lat and lon read");
  #endif

    
    //CDBDebug("pointLon = %f",((float*)pointLon->data)[0]);
//   CT::string data = CDF::dump(cdfObject0);
//   
//   CDBDebug("%s",data.c_str());
//   
    
  
    for(size_t d=0;d<nrDataObjects;d++){
      if(pointVar[d]!=NULL){
        /*Second read actual variables*/
        int stationDimIndexInVariable = -1;
        
        for(size_t j=0;j<pointVar[d]->dimensionlinks.size();j++){
          if(pointVar[d]->dimensionlinks[j]->name.equals("station")||
            dataObjects[d]->cdfObject->getVariableNE(pointVar[d]->dimensionlinks[j]->name.c_str()) == NULL){
            stationDimIndexInVariable = j;
            break;
        }
        }
    
        /*Set dimension indices*/
        for(size_t j=0;j<pointVar[d]->dimensionlinks.size();j++){
        if(stationDimIndexInVariable == int(j)){
            start[j] = 0;
            count[j] = pointVar[d]->dimensionlinks[j]->getSize();
            stride[j]=1;
        }else{
            start[j] = dataSource->getDimensionIndex(pointVar[d]->dimensionlinks[j]->name.c_str());
            count[j] = 1;
            stride[j]=1;
        }
        //CDBDebug("%s %d %d",pointVar[d]->dimensionlinks[j]->name.c_str(),start[j],count[j]);
        }
        
    
        

        
        if(pointVar[d]->nativeType!=CDF_STRING&&pointVar[d]->nativeType!=CDF_CHAR){
        #ifdef CCONVERTADAGUCPOINT_DEBUG
        CDBDebug("Reading FLOAT %s", pointVar[d]->name.c_str());
        #endif
        pointVar[d]->freeData();
        
//           for(size_t j=0;j<pointVar[d]->dimensionlinks.size();j++){
//             CDBDebug("%d %s [%d:%d:%d]",j,pointVar[d]->dimensionlinks[j]->name.c_str(),start[j],count[j],stride[j]);
//           }
//         
        
        if(pointVar[d]->readData(CDF_FLOAT,start,count,stride,true)!=0){
          CDBError("Unable to read pointVar[d] data");
          return 1;
        }
        }else{

        if(pointVar[d]->nativeType==CDF_CHAR){
            //Compatibilty function for LCW data, reading strings stored as fixed arrays of CDF_CHAR
            start[1]=0;
            count[1]=pointVar[d]->dimensionlinks[1]->getSize();
            CT::string data[count[0]];
            if (pointVar[d]->readData(CDF_CHAR,start,count,stride,false)!=0){
              CDBError("Unable to read pointVar[d] data");
              return 1;
            }
            for(size_t j=0;j<count[0];j++){
            data[j].copy(((char*)pointVar[d]->data+j*count[1]),count[1]-1);
            }
            pointVar[d]->freeData();
            
            #ifdef CCONVERTADAGUCPOINT_DEBUG
            CDBDebug("Reading CDF_CHAR array");
            for(int j=0;j<numDims;j++){
            CDBDebug("CDF_CHAR %d: %s %d till %d ",j,pointVar[d]->dimensionlinks[j]->name.c_str(),start[j],count[j]);
            }
            #endif
            pointVar[d]->data = malloc(count[0]*sizeof(size_t));
            for(size_t j=0;j<count[0];j++){
            (((char**)pointVar[d]->data)[j]) = ((char*)malloc((count[1]+1)*sizeof(char)));
            strncpy((((char**)pointVar[d]->data)[j]),data[j].c_str(),count[1]);
            }
        }
        if(pointVar[d]->nativeType==CDF_STRING){
        #ifdef CCONVERTADAGUCPOINT_DEBUG
            CDBDebug("Reading CDF_STRING array");
            for(int j=0;j<numDims;j++){
            CDBDebug("CDF_STRING %d: %s %d till %d ",j,pointVar[d]->dimensionlinks[j]->name.c_str(),start[j],count[j]);
            }
            #endif
            pointVar[d]->freeData();
            if (pointVar[d]->readData(CDF_STRING,start,count,stride,false)!=0){
              CDBError("Unable to read pointVar[d] data");
              return 1;
            }
        }
        }
        //pointVar[d]->readData(CDF_FLOAT,true);
    }
    }

  
  #ifdef MEASURETIME
    StopWatch_Stop("Variables read");
  #endif


  for(size_t d=0;d<nrDataObjects;d++){
    if(pointVar[d]!=NULL){
      CDF::Attribute *fillValue = new2DVar[d]->getAttributeNE("_FillValue");
      if(fillValue!=NULL){
        dataObjects[d]->hasNodataValue=true;
        fillValue->getData(&dataObjects[d]->dfNodataValue,1);
        #ifdef CCONVERTADAGUCPOINT_DEBUG
        CDBDebug("_FillValue = %f",dataObjects[d]->dfNodataValue);
        #endif
      }
    }
  }
  
  #ifdef MEASURETIME
    StopWatch_Stop("FillValues set");
  #endif
  
  //Detect minimum and maximum values
  float fill = (float)dataObjects[0]->dfNodataValue;
 
  
  //Set statistics
  if(dataSource->stretchMinMax) 
  {
    if(dataSource->stretchMinMaxDone == false){
    #ifdef CCONVERTADAGUCPOINT_DEBUG
    CDBDebug("dataSource->stretchMinMax");
    #endif
    if(dataSource->statistics==NULL){
      float min = fill;float max=fill;
      for(size_t j=0;j<pointVar[0]->getSize();j++){
        float v=((float*)pointVar[0]->data)[j];
        if(v!=fill){
          if(min==fill)min=v;
          if(max==fill)max=v;
          if(v<min)min=v;
          if(v>max)max=v;
        }
      }

      
      #ifdef MEASURETIME
        StopWatch_Stop("Min max calculated");
      #endif

      #ifdef CCONVERTADAGUCPOINT_DEBUG
      CDBDebug("Calculated min/max : %f %f",min,max);
      #endif
      #ifdef CCONVERTADAGUCPOINT_DEBUG
      CDBDebug("Setting statistics: min/max : %f %f",min,max);
      #endif
      dataSource->statistics = new CDataSource::Statistics();
      dataSource->statistics->setMaximum(max);
      dataSource->statistics->setMinimum(min);
    }
     
    }

  }
  
  
  
  #ifdef MEASURETIME
    StopWatch_Stop("Statistics set");
  #endif

  
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
   
    
    #ifdef CCONVERTADAGUCPOINT_DEBUG
    for(size_t d=0;d<nrDataObjects;d++){
      if(pointVar[d]!=NULL){
        CDBDebug("Drawing %s",new2DVar[d]->name.c_str());
      }
    }
    #endif
    
    CDF::Dimension *dimX;
    CDF::Dimension *dimY;
    CDF::Variable *varX ;
    CDF::Variable *varY;
  
    //Create new dimensions and variables (X,Y,T)
    dimX=cdfObject0->getDimension("x");
    dimX->setSize(dataSource->dWidth);
    
    dimY=cdfObject0->getDimension("y");
    dimY->setSize(dataSource->dHeight);
    
    varX = cdfObject0->getVariable("x");
    varY = cdfObject0->getVariable("y");
    
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
    
    #ifdef MEASURETIME
      StopWatch_Stop("Dimensions set");
    #endif

    
    //Allocate 2D field
    for(size_t d=0;d<nrDataObjects;d++){
        if(pointVar[d]!=NULL){
        size_t fieldSize = dataSource->dWidth*dataSource->dHeight;
        new2DVar[d]->setSize(fieldSize);
        CDF::allocateData(new2DVar[d]->getType(),&(new2DVar[d]->data),fieldSize);
        
      //Fill in nodata
        if(dataObjects[d]->hasNodataValue){
          for(size_t j=0;j<fieldSize;j++){
            ((float*)dataObjects[d]->cdfVariable->data)[j]=(float)dataObjects[d]->dfNodataValue;
          }
        }else{
          for(size_t j=0;j<fieldSize;j++){
            ((float*)dataObjects[d]->cdfVariable->data)[j]=NAN;
          }
        }
      }
    }
    
    
    #ifdef MEASURETIME
      StopWatch_Stop("2D Field allocated");
    #endif
      
    float *lonData=(float*)pointLon->data;
    float *latData=(float*)pointLat->data;
    
   
    
  
    

  
    CImageWarper imageWarper;
    bool projectionRequired=false;
    if(dataSource->srvParams->Geo->CRS.length()>0){
      projectionRequired=true;
      for(size_t d=0;d<nrDataObjects;d++){
        if(pointVar[d]!=NULL){
          new2DVar[d]->setAttributeText("grid_mapping","customgridprojection");
        }
      }
      if(cdfObject0->getVariableNE("customgridprojection")==NULL){
        CDF::Variable *projectionVar = new CDF::Variable();
        projectionVar->name.copy("customgridprojection");
        cdfObject0->addVariable(projectionVar);
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
   
    
    #ifdef CCONVERTADAGUCPOINT_DEBUG
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
  
    
    //Read ID's
    CDF::Variable *pointID = cdfObject0->getVariableNE("station");
    if(pointID == NULL){
       pointID = cdfObject0->getVariableNE("location_backup");
    }
    if(pointID!=NULL){
      if(pointID->getType() == CDF_STRING){
          pointID->readData(CDF_STRING);
      }else{
        pointID = NULL;
      }
    }
    
    CDF::Variable *roadIdVar = cdfObject0->getVariableNE("roadId_backup");
    float *roadIdData = NULL;
    float prevRoadId = -1;
    if(roadIdVar != NULL){
      //if(roadIdVar->getType() == CDF_SHORT)
      {
          CDBDebug("Start reading data");
        roadIdVar->readData(CDF_FLOAT);
        CDBDebug("Done reading data");
         roadIdData = (float*)roadIdVar->data;
         prevRoadId=roadIdData[0];
      }
    }
    
    
    //Read dates for obs
    bool hasTimeValuePerObs = false;
    CTime obsTime;
    double *obsTimeData = NULL;
    CDF::Variable * timeVarPerObs =  cdfObject0->getVariableNE("time");
    if(timeVarPerObs!= NULL){
      if(timeVarPerObs->isDimension == false){
        if(timeVarPerObs->dimensionlinks[0]->getSize(),pointVar[0]->dimensionlinks[0]->getSize()){
          CDF::Attribute* timeStringAttr = timeVarPerObs->getAttributeNE("units");
          if(timeStringAttr !=NULL){
            if(timeStringAttr -> data != NULL){
              if(obsTime.init(timeVarPerObs)==0){
                hasTimeValuePerObs = true;
                timeVarPerObs->readData(CDF_DOUBLE,start,count,stride,true);
                obsTimeData = (double*)timeVarPerObs->data;
              }
            }
          }
        }
      }
    }
    
    #ifdef MEASURETIME
    CDBDebug("Date numStations = %d",numStations);
    #endif
    
    
    float discRadius = 8;
    float discRadiusX = discRadius;
    float discRadiusY = discRadius;
    bool hasZoomableDiscRadius = false;
    float discSize = 1;
    if (dataSource!=NULL){
      CStyleConfiguration * styleConfiguration = dataSource->getStyle();
       if (styleConfiguration != NULL && styleConfiguration->styleConfig!=NULL){
         if (styleConfiguration->styleConfig->Point.size() == 1){
          
          if(!styleConfiguration->styleConfig->Point[0]->attr.discradius.empty()){
            hasZoomableDiscRadius = true;
             discSize = styleConfiguration->styleConfig->Point[0]->attr.discradius.toFloat();
          }
         }
       }
    }
    
    int prevLon,prevLat;
    float prevVal;
    bool hasPrevLatLon = false;
    
    bool doDrawLinearInterpolated = dataSource->getStyle()->renderMethod&RM_POINT_LINEARINTERPOLATION;
    bool doDrawCircle = dataSource->getStyle()->renderMethod&RM_NEAREST;
    

   
    for(int stationNr=0;stationNr<numStations;stationNr++){ 
      int pPoint = stationNr+0;//dateDimIndex;//*numStations;
      int pGeo = stationNr;
//       //CDBDebug("stationNr %d dateDimIndex %d,pPoint DIM %d",stationNr,dateDimIndex,pPoint);
   
      double lon = (double)lonData[pGeo];
      double lat = (double)latData[pGeo];
      
//      CDBDebug("Found coordinate %f %f",lon,lat);
      double rotation = 0;
      double projectedX = lon;
      double projectedY = lat;
      double projectedXOffsetY = lon ;
      double projectedYOffsetY = lat + 0.1;
      double latCorrection = 1;
      if (hasZoomableDiscRadius){
        latCorrection = cos((lat / 90) * (3.141592654/2));;
        if (latCorrection <0.01) latCorrection = 0.01;
      }
      double projectedXOffsetX = lon + 0.1 / latCorrection;
      double projectedYOffsetX = lat;
      bool projectionError = false;
      if(projectionRequired){
        if(imageWarper.reprojfromLatLon(projectedX,projectedY)!=0){
          projectionError = true;
          hasPrevLatLon = false;
        }
        if (hasZoomableDiscRadius){
          if(imageWarper.reprojfromLatLon(projectedXOffsetY,projectedYOffsetY)!=0){
            projectionError = true;
          }
          if(imageWarper.reprojfromLatLon(projectedXOffsetX,projectedYOffsetX)!=0){
            projectionError = true;
          }
          float dX = projectedXOffsetY - projectedX;
          float dY = projectedYOffsetY - projectedY;
          rotation = -atan2(dY,dX) + (3.141592654/2);

        }
      }
      
      if (hasZoomableDiscRadius){
        float yDistanceProjected = projectedYOffsetY - projectedY;
        // CDBDebug("yDistanceProjected = %f", yDistanceProjected);
        discRadius = ((dataSource->srvParams->Geo->dfBBOX[3] - dataSource->srvParams->Geo->dfBBOX[1])/yDistanceProjected) / 
          float(dataSource->srvParams->Geo->dWidth);
        discRadius = (discSize/5)/ discRadius;
        if (discRadius < 0.1) discRadius = 0.1;
        discRadiusY = discRadius;
        
        float xDistanceProjected = projectedXOffsetX - projectedX;
        discRadiusX = ((dataSource->srvParams->Geo->dfBBOX[2] - dataSource->srvParams->Geo->dfBBOX[0])/xDistanceProjected) / 
          float(dataSource->srvParams->Geo->dWidth);
        discRadiusX = (discSize/5)/ discRadiusX;
        if (discRadiusX < 0.1) discRadiusX = 0.1;       
      }
      
      if(projectionError == false){
        int dlon=int((projectedX-offsetX)/cellSizeX);
        int dlat=int((projectedY-offsetY)/cellSizeY);
  //      if(dlon>=0&&dlat>=0&&dlon<dataSource->dWidth&&dlat<dataSource->dHeight){ remove
        for(size_t d=0;d<nrDataObjects;d++){
          if(pointVar[d]!=NULL){
          float *sdata = ((float*)dataObjects[d]->cdfVariable->data);
          PointDVWithLatLon * lastPoint = NULL;
          
          /**
          * With string and char (text) data, the float value is set to NAN and character data is put in the paramlist
          */
          if(pointVar[d]->currentType==CDF_STRING){
            float v = NAN;
            //CDBDebug("pushing stationNr %d dateDimIndex %d,pPoint DIM %d",stationNr,dateDimIndex,pPoint);
            dataObjects[d]->points.push_back(PointDVWithLatLon(dlon,dlat,lon,lat,v));//,((const char**)pointVar[d]->data)[pPoint]));
            lastPoint = &(dataObjects[d]->points.back());
            const char * key = pointVar[d]->name.c_str();
            const char * description = key;
            try{
              description = (const char*)pointVar[d]->getAttribute("long_name")->data;
              
            }catch(int e){
            }
            /* Get the last pushed point from the array and push the character text data in the paramlist */
            const char *b = ((const char**)pointVar[d]->data)[pPoint];
            lastPoint->paramList.push_back(CKeyValue(key,description ,b));
            float a = 0;
            drawCircle(sdata,a,dataSource->dWidth,dataSource->dHeight,dlon-1,dlat,int(discRadius));
          }
          
          
          
          
          
          if(pointVar[d]->currentType==CDF_CHAR){
            float v = NAN;
            //CDBDebug("pushing stationNr %d dateDimIndex %d,pPoint DIM %d",stationNr,dateDimIndex,pPoint);
            dataObjects[d]->points.push_back(PointDVWithLatLon(dlon,dlat,lon,lat,v));//,((const char**)pointVar[d]->data)[pPoint]));
            lastPoint = &(dataObjects[d]->points.back());
            const char * key = pointVar[d]->name.c_str();
            const char * description = key;
            try{
              description = (const char*)pointVar[d]->getAttribute("long_name")->data;
              
            }catch(int e){
            }
            /* Get the last pushed point from the array and push the character text data in the paramlist */
            lastPoint->paramList.push_back(CKeyValue(key,description ,((const char**)pointVar[d]->data)[pPoint]));
            float a = 0;
            drawCircle(sdata,a,dataSource->dWidth,dataSource->dHeight,dlon-1,dlat,int(discRadius));
          }
          
          
          if(pointVar[d]->currentType==CDF_FLOAT){
            float val  = ((float*)pointVar[d]->data)[pPoint];
            
          
            if(val!=fill){
            
              //CDBDebug("P %d %d %f",dlon,dlat,val);

                dataObjects[d]->points.push_back(PointDVWithLatLon(dlon,dlat,lon,lat,val, rotation, discRadiusX, discRadiusY));//,id));
                lastPoint = &(dataObjects[d]->points.back());
                if(pointID!=NULL){
                
                  const char * key = pointID->name.c_str();
                  const char * description = key;
                  try{
                    description = (const char*)pointID->getAttribute("long_name")->data;
                  }catch(int e){
                  }
                  lastPoint->paramList.push_back(CKeyValue(key,description ,((const char**)pointID->data)[pGeo]));
                }
                if(doDrawCircle){
                  drawCircle(sdata,val,dataSource->dWidth,dataSource->dHeight,dlon-1,dlat,int(discRadius));
                }
                if(doDrawLinearInterpolated && roadIdData != NULL){
                  if(hasPrevLatLon){
                    float roadId = roadIdData[pPoint];
                    if(roadId == prevRoadId && val == val && prevVal == prevVal){
                        lineInterpolated(sdata,dataSource->dWidth,dataSource->dHeight,prevLon,prevLat,dlon,dlat,prevVal,val);
                    }
                    prevRoadId = roadId;
                  }
                }
                prevLon = dlon;
                prevLat = dlat;;
                prevVal=val;
                
                hasPrevLatLon = true;
              }else{
                hasPrevLatLon = false;
              }
              
              
            }
            
            if(hasTimeValuePerObs&& lastPoint!=NULL){
              
              const char * key = timeVarPerObs->name.c_str();
              const char * description = key;
              try{
                description = (const char*)timeVarPerObs->getAttribute("long_name")->data;
              }catch(int e){
              }
              //obsTimeData
              
              lastPoint->paramList.push_back(CKeyValue(key,description ,obsTime.dateToISOString(obsTime.getDate(obsTimeData[pPoint])).c_str()));
              
            }
          }
        }
        
//      } TODO remove if test for "clipping"
      }
    }
    
    #ifdef MEASURETIME
      StopWatch_Stop("Points added");
    #endif
    imageWarper.closereproj();
   
  }
  
  #ifdef CCONVERTADAGUCPOINT_DEBUG
  CDBDebug("/convertADAGUCPointData");
  #endif
  return 0;
}
