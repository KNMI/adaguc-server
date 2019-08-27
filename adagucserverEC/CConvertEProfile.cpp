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

#include "CConvertEProfile.h"
#include "CFillTriangle.h"
#include "CImageWarper.h"
#include <set>
// #define CCONVERTEPROFILE_DEBUG
// #define CCONVERTEPROFILE_DEBUG
const char *CConvertEProfile::className="CConvertEProfile";


/**
 * Checks if the format of this file corresponds to the ADAGUC Profile format.
 */
bool isADAGUCProfileFormat(CDFObject *cdfObject) {
  try{
    cdfObject->getVariable("range");
    cdfObject->getDimension("range");

    if( cdfObject->getAttribute("featureType")->toString().equalsIgnoreCase("profile")==false) {

      // The file is not a profile according to the format standards, check if it adheres to the deprecated format:
      if( cdfObject->getAttribute("source")->toString().startsWith("CHM")==false){
        return false;
      }
      if( cdfObject->getAttribute("serlom")->toString().startsWith("TUB")==false){
        return false;
      }
    }
  } catch(int e){
    return false;
  }

  return true;
}

/*
 * Checks if the format of this file corresponds to the deprecated ADAGUC EProfile format.
 */
bool isDeprecatedADAGUCEProfileFormat(CDFObject *cdfObject) {
  try{
    cdfObject->getVariable("range");
    cdfObject->getDimension("range");

    if( cdfObject->getAttribute("source")->toString().startsWith("CHM")==false){
      return false;
    }
    if( cdfObject->getAttribute("serlom")->toString().startsWith("TUB")==false){
      return false;
    }
  } catch(int e){
    return false;
  }

  return true;
}

/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertEProfile::convertEProfileHeader( CDFObject *cdfObject,CServerParams *srvParams){
  //Check whether this is really a profile file
  if(!isADAGUCProfileFormat(cdfObject) && !isDeprecatedADAGUCEProfileFormat(cdfObject)) {
    return 1;
  }
//   CDBDebug("Using CConvertEProfile.h");
  
  cdfObject->setAttributeText("ADAGUC_PROFILE","true");

  //Standard bounding box of adaguc data is worldwide
  CDF::Variable *pointLon;
  CDF::Variable *pointLat;
  
  try{
    pointLon = cdfObject->getVariable("longitude");
    pointLat = cdfObject->getVariable("latitude");
  }catch(int e){
    CDBError("lat or lon variables not found");
    return 1;
  }
  
  
  #ifdef CCONVERTEPROFILE_DEBUG
    StopWatch_Stop("EPROFILE LIDAR DATA");
  #endif
  pointLon->readData(CDF_FLOAT,true);
  pointLat->readData(CDF_FLOAT,true);
  
  #ifdef CCONVERTEPROFILE_DEBUG
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
  #ifdef CCONVERTEPROFILE_DEBUG
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
    #ifdef CCONVERTEPROFILE_DEBUG
      StopWatch_Stop("Need to add varX and varY");
    #endif
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
    
     
    #ifdef CCONVERTEPROFILE_DEBUG
      StopWatch_Stop("Added varX and varY");
    #endif
  }
  
  CDF::Variable *timev = cdfObject->getVariable("time");
  CDF::Dimension *timed = cdfObject->getDimension("time");
  
  timev->name="time_obs";
  timed->name="time_obs";
  
  #ifdef CCONVERTEPROFILE_DEBUG
    StopWatch_Stop("Start Reading dates ");
  #endif  
  timev->readData(CDF_DOUBLE);
  double*timeData=((double*)timev->data);
  #ifdef CCONVERTEPROFILE_DEBUG
    StopWatch_Stop("Finished Reading dates ");
  #endif  
  
     
  double currentTime = -1;
  std::set<double> datesToAdd;
  
  //The startdate of the file will be used in time_file
  
//   #ifdef CCONVERTEPROFILE_DEBUG
//     StopWatch_Stop("Creating CTIME");
//   #endif  
//   CTime obsTime;
// 
//   #ifdef CCONVERTEPROFILE_DEBUG
//     StopWatch_Stop("Initializing CTIME");
//   #endif  
//   if(obsTime.init(timev)!=0){
//     return 1;
//   }
  CTime * obsTime = CTime::GetCTimeInstance(timev);
  
  if (obsTime == NULL){
    CDBError("Unable to get CTime instance");
    return 1;
  }
  
  #ifdef CCONVERTEPROFILE_DEBUG
      StopWatch_Stop("Inserting dates %d", timev->getSize());
  #endif    
  for(size_t j=0;j<timev->getSize();j++){
    double inTime = timeData[j];
    double outTime = obsTime->quantizeTimeToISO8601(inTime, "PT5M", "low");
    if(outTime > currentTime){
      datesToAdd.insert(outTime);
      currentTime = outTime;
    }
  }
  
  
  #ifdef CCONVERTEPROFILE_DEBUG
      StopWatch_Stop("Inserted dates");
  #endif
  // CDBDebug("Set time Size = %d",datesToAdd.size());
 
  CDF::Dimension *dimT=new CDF::Dimension();
  dimT->name="time";
  dimT->setSize(datesToAdd.size());
  cdfObject->addDimension(dimT);
  CDF::Variable * varT = new CDF::Variable();
  varT->setType(CDF_DOUBLE);
  varT->name.copy("time");
  varT->setAttributeText("standard_name","time");
  varT->setAttributeText("units",timev->getAttribute("units")->toString().c_str());
  varT->isDimension=true;
  varT->dimensionlinks.push_back(dimT);
  cdfObject->addVariable(varT);
  CDF::allocateData(CDF_DOUBLE,&varT->data,dimT->length);
  
  #ifdef CCONVERTEPROFILE_DEBUG
      StopWatch_Stop("Allocated time data");
  #endif
  std::set<double>::iterator it;
  size_t counter = 0;
  for (it=datesToAdd.begin(); it!=datesToAdd.end(); ++it){
    ((double*)varT->data)[counter]=*it;
    counter++;
  }
  
  
  

  
  #ifdef CCONVERTEPROFILE_DEBUG
    StopWatch_Stop("2D Coordinate dimensions created");
  #endif
  
  //Make a list of variables which will be available as 2D fields  
  CT::StackList<CT::string> varsToConvert;
  for(size_t v=0;v<cdfObject->variables.size();v++){
    CDF::Variable *var = cdfObject->variables[v];
    if(var->isDimension==false){
      if(var->getType()!=CDF_STRING)
      {
        if(!var->name.equals("time2D")&&
          !var->name.equals("time")&&
          !var->name.equals("lon")&&
          !var->name.equals("lat")&&
          !var->name.equals("altitude")&&
          !var->name.equals("longitude")&&
          !var->name.equals("latitude")&&
          !var->name.equals("x")&&
          !var->name.equals("y")&&
          !var->name.equals("lat_bnds")&&
          !var->name.equals("lon_bnds")&&
          !var->name.equals("custom")&&
          !var->name.equals("projection")&&
          !var->name.equals("product")&&
          !var->name.equals("iso_dataset")&&
          !var->name.equals("tile_properties")
        ){
          bool added = false;
          if(var->dimensionlinks.size()==2){
            // Check if this is a profile variable which we added.
            if(var->dimensionlinks[0]->name.equals("time_obs") && var->dimensionlinks[1]->name.equals("range")){
              varsToConvert.add(CT::string(var->name.c_str()));
              added=true;
            }
          }
          if(added == false){
            var->setAttributeText("ADAGUC_SKIP","true");
          }
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
    
    #ifdef CCONVERTEPROFILE_DEBUG
    StopWatch_Stop("Converting %s",pointVar->name.c_str());
    #endif
    
    CDF::Variable *new2DVar = new CDF::Variable();
    cdfObject->addVariable(new2DVar);
   

   
    
    //Assign X,Y,T dims 
    if(pointVar->dimensionlinks.size() >=2){
         new2DVar->dimensionlinks.push_back( dimT);//pointVar->dimensionlinks[0]);//time
         //new2DVar->dimensionlinks.push_back( pointVar->dimensionlinks[1]);//range
    }
    
    //new2DVar->dimensionlinks.push_back( pointVar->dimensionlinks[0]);
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
    new2DVar->setAttributeText("ADAGUC_PROFILE","true");
    
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
  
  #ifdef CCONVERTEPROFILE_DEBUG
    StopWatch_Stop("Header done");
  #endif

 
  return 0;
}




/**
 * This function draws the virtual 2D variable into a new 2D field
 */
int CConvertEProfile::convertEProfileData(CDataSource *dataSource,int mode){
  #ifdef CCONVERTEPROFILE_DEBUG
  CDBDebug("convertEProfileData");
  #endif
  
  CDFObject *cdfObject0 = dataSource->getDataObject(0)->cdfObject;
  if(!isADAGUCProfileFormat(cdfObject0) && !isDeprecatedADAGUCEProfileFormat(cdfObject0)) {
    return 1;
  }

  CDBDebug("THIS IS PROFILE DATA");
  
  #ifdef CCONVERTEPROFILE_DEBUG
    StopWatch_Stop("Reading data");
  #endif
    
  CDF::Variable *pointLon;
  CDF::Variable *pointLat;
  
  try{
    pointLon = cdfObject0->getVariable("longitude");
    pointLat = cdfObject0->getVariable("latitude");
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
      CDBError("Unable to find orignal swath variable with name %s",origSwathName.c_str());
      return 1;
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
//   int stationDimIndexInCoord = 0;
  int numStations = 1;

  
  #ifdef CCONVERTEPROFILE_DEBUG
    CDBDebug("numStations %d ",numStations);
    CDBDebug("numDims %d ",numDims);
  #endif
  
  
/*  
  if(pointLon->dimensionlinks.size()>=2){
    #ifdef CCONVERTEPROFILE_DEBUG
    CDBDebug("Dimension dependant locations");
    #endif
    pointLon->freeData();
    pointLat->freeData();
    pointLon->readData(CDF_FLOAT,start,count,stride,true);
    pointLat->readData(CDF_FLOAT,start,count,stride,true);
  }else{
    #ifdef CCONVERTEPROFILE_DEBUG
    CDBDebug("NON Dimension dependant location");
    #endif*/
    pointLon->readData(CDF_FLOAT,true);
    pointLat->readData(CDF_FLOAT,true);
//   }

    
  #ifdef CCONVERTEPROFILE_DEBUG
    StopWatch_Stop("Lat and lon read");
  #endif

//   CT::string data = CDF::dump(cdfObject0);
//   
//   CDBDebug("%s",data.c_str());
//   
  
  for(size_t d=0;d<nrDataObjects;d++){
     /*Second read actual variables*/
    int rangeDimIndexInVariable = -1;
    
    for(size_t j=0;j<pointVar[d]->dimensionlinks.size();j++){
      if(pointVar[d]->dimensionlinks[j]->name.equals("range")){
        rangeDimIndexInVariable = j;
        break;
      }
    }
  
     /*Set dimension indices*/
    for(size_t j=0;j<pointVar[d]->dimensionlinks.size();j++){
      if(rangeDimIndexInVariable == int(j)){
        start[j] = 0;
        count[j] = pointVar[d]->dimensionlinks[j]->getSize();
        stride[j]=1;
      }else{
        start[j] = dataSource->getDimensionIndex(pointVar[d]->dimensionlinks[j]->name.c_str());
        count[j] = 1;
        stride[j]=1;
      }
    }
      

    
    if(pointVar[d]->nativeType!=CDF_STRING&&pointVar[d]->nativeType!=CDF_CHAR){
      #ifdef CCONVERTEPROFILE_DEBUG
      CDBDebug("Reading FLOAT %s", pointVar[d]->name.c_str());
      #endif
      pointVar[d]->freeData();
      
//       for(size_t j=0;j<pointVar[d]->dimensionlinks.size();j++){
//         CDBDebug("%d %s [%d:%d:%d]",j,pointVar[d]->dimensionlinks[j]->name.c_str(),start[j],count[j],stride[j]);
//       }
      
      
      pointVar[d]->readData(CDF_FLOAT,start,count,stride,true);
      
      //TODO Use altitude to correct range height
      
      
    }else{

      if(pointVar[d]->nativeType==CDF_CHAR){
        //Compatibilty function for LCW data, reading strings stored as fixed arrays of CDF_CHAR
        start[1]=0;
        count[1]=pointVar[d]->dimensionlinks[1]->getSize();
        CT::string data[count[0]];
        pointVar[d]->readData(CDF_CHAR,start,count,stride,false);
        for(size_t j=0;j<count[0];j++){
          data[j].copy(((char*)pointVar[d]->data+j*count[1]),count[1]-1);
        }
        pointVar[d]->freeData();
        
        #ifdef CCONVERTEPROFILE_DEBUG
        CDBDebug("Reading CDF_CHAR array");
        for(size_t j=0;j<numDims;j++){
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
       #ifdef CCONVERTEPROFILE_DEBUG
        CDBDebug("Reading CDF_STRING array");
        for(size_t j=0;j<numDims;j++){
          CDBDebug("CDF_STRING %d: %s %d till %d ",j,pointVar[d]->dimensionlinks[j]->name.c_str(),start[j],count[j]);
        }
        #endif
        pointVar[d]->freeData();
        pointVar[d]->readData(CDF_STRING,start,count,stride,false);
      }
    }
    //pointVar[d]->readData(CDF_FLOAT,true);
  }
  
  #ifdef CCONVERTEPROFILE_DEBUG
    StopWatch_Stop("Variables read");
  #endif


  for(size_t d=0;d<nrDataObjects;d++){
  
    CDF::Attribute *fillValue = new2DVar[d]->getAttributeNE("_FillValue");
    if(fillValue!=NULL){
      dataObjects[d]->hasNodataValue=true;
      fillValue->getData(&dataObjects[d]->dfNodataValue,1);
      #ifdef CCONVERTEPROFILE_DEBUG
      CDBDebug("_FillValue = %f",dataObjects[d]->dfNodataValue);
      #endif
    }
  }
  
  #ifdef CCONVERTEPROFILE_DEBUG
    StopWatch_Stop("FillValues set");
  #endif
  
  //Detect minimum and maximum values
  float fill = (float)dataObjects[0]->dfNodataValue;
 
  
  //Set statistics
  if(dataSource->stretchMinMax) 
  {
    if(dataSource->stretchMinMaxDone == false){
    #ifdef CCONVERTEPROFILE_DEBUG
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

      
      #ifdef CCONVERTEPROFILE_DEBUG
        StopWatch_Stop("Min max calculated");
      #endif

      #ifdef CCONVERTEPROFILE_DEBUG
      CDBDebug("Calculated min/max : %f %f",min,max);
      #endif
      #ifdef CCONVERTEPROFILE_DEBUG
      CDBDebug("Setting statistics: min/max : %f %f",min,max);
      #endif
      dataSource->statistics = new CDataSource::Statistics();
      dataSource->statistics->setMaximum(max);
      dataSource->statistics->setMinimum(min);
    }
     
    }

  }
  
  
  
  #ifdef CCONVERTEPROFILE_DEBUG
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
   
    
    #ifdef CCONVERTEPROFILE_DEBUG
    for(size_t d=0;d<nrDataObjects;d++){
      CDBDebug("Drawing %s",new2DVar[d]->name.c_str());
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
    
    #ifdef CCONVERTEPROFILE_DEBUG
      StopWatch_Stop("Dimensions set");
    #endif

    
    //Allocate 2D field
    for(size_t d=0;d<nrDataObjects;d++){
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
    
    
    #ifdef CCONVERTEPROFILE_DEBUG
      StopWatch_Stop("2D Field allocated");
    #endif
      
    float *lonData=(float*)pointLon->data;
    float *latData=(float*)pointLat->data;
    
   
    
  
    

  
    CImageWarper imageWarper;
    bool projectionRequired=false;
    if(dataSource->srvParams->Geo->CRS.length()>0){
      projectionRequired=true;
      for(size_t d=0;d<nrDataObjects;d++){
        new2DVar[d]->setAttributeText("grid_mapping","customgridprojection");
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
   
    
    #ifdef CCONVERTEPROFILE_DEBUG
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
      pointID->readData(CDF_STRING);
    }
    
    //Read dates for obs
//     bool hasTimeValuePerObs = false;
    
    
    CDBDebug("CTIME");
//     double *obsTimeData = NULL;
    CDF::Variable * timeVarPerObs =  cdfObject0->getVariableNE("time");
      if(timeVarPerObs!= NULL){
      //if(timeVarPerObs->isDimension == true){
        if(timeVarPerObs->dimensionlinks[0]->getSize(),pointVar[0]->dimensionlinks[0]->getSize()){
          CDF::Attribute* timeStringAttr = timeVarPerObs->getAttributeNE("units");
          if(timeStringAttr !=NULL){
              if(timeStringAttr -> data != NULL){
              CTime * obsTime = CTime::GetCTimeInstance(timeVarPerObs);
              if(obsTime!= NULL){
                
//                 hasTimeValuePerObs = true;
                timeVarPerObs->readData(CDF_DOUBLE,start,count,stride,true);
//                 obsTimeData = (double*)timeVarPerObs->data;
              }
            }
          }
        }
      //}
    }
    
    #ifdef CCONVERTEPROFILE_DEBUG
    CDBDebug("Date numStations = %d",numStations);
    #endif
    

   
    for(int stationNr=0;stationNr<numStations;stationNr++){ 
      int pPoint = stationNr+0;//dateDimIndex;//*numStations;
      int pGeo = stationNr;
//       //CDBDebug("stationNr %d dateDimIndex %d,pPoint DIM %d",stationNr,dateDimIndex,pPoint);
   
      double lon = (double)lonData[pGeo];
      double lat = (double)latData[pGeo];
      double projectedX = lon;
      double projectedY = lat;
      
      if(projectionRequired)imageWarper.reprojfromLatLon(projectedX,projectedY);
      int dlon=int((projectedX-offsetX)/cellSizeX);
      int dlat=int((projectedY-offsetY)/cellSizeY);
      if(dlon>=0&&dlat>=0&&dlon<dataSource->dWidth&&dlat<dataSource->dHeight){
      for(size_t d=0;d<nrDataObjects;d++){
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
          drawCircle(sdata,a,dataSource->dWidth,dataSource->dHeight,dlon-1,dlat,8);
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
          drawCircle(sdata,a,dataSource->dWidth,dataSource->dHeight,dlon-1,dlat,8);
        }
        
        
        if(pointVar[d]->currentType==CDF_FLOAT){
          float val  = ((float*)pointVar[d]->data)[pPoint];
        
          
          if(val!=fill){
           
            //CDBDebug("P %d %d %f",dlon,dlat,val);
            
              dataObjects[d]->points.push_back(PointDVWithLatLon(dlon,dlat,lon,lat,val));//,id));
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
              drawCircle(sdata,val,dataSource->dWidth,dataSource->dHeight,dlon-1,dlat,8);
            }        
            
            
          }
          
//           if(hasTimeValuePerObs&& lastPoint!=NULL){
//              
//             const char * key = timeVarPerObs->name.c_str();
//             const char * description = key;
//             try{
//               description = (const char*)timeVarPerObs->getAttribute("long_name")->data;
//             }catch(int e){
//             }
//             //obsTimeData
//              
//             lastPoint->paramList.push_back(CKeyValue(key,description ,obsTime.dateToISOString(obsTime.getDate(obsTimeData[pPoint])).c_str()));
//              
//           }
          
        }
      }
    }
    
    #ifdef CCONVERTEPROFILE_DEBUG
      StopWatch_Stop("Points added");
    #endif
    imageWarper.closereproj();
   
  }
  
  #ifdef CCONVERTEPROFILE_DEBUG
  CDBDebug("/convertEProfileData");
  #endif
  return 0;
}
