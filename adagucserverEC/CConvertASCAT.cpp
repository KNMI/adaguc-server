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

#include "CConvertASCAT.h"
#include "CFillTriangle.h"
#include "CImageWarper.h"

//#define CCONVERTASCAT_DEBUG

const char *CConvertASCAT::className="CConvertASCAT";

/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertASCAT::convertASCATHeader( CDFObject *cdfObject ){
  //Check whether this is really an ascat file
  if(cdfObject->getDimensionNE("nXtrack")!=NULL&&cdfObject->getDimensionNE("nTimes")!=NULL){
    try{cdfObject->getDimension("nXtrack")->name="NUMCELLS";}catch(int e){}
    try{cdfObject->getDimension("nTimes")->name="NUMROWS";}catch(int e){}
  }
  
  try{
    cdfObject->getDimensionIgnoreCase("NUMROWS");
    cdfObject->getDimensionIgnoreCase("NUMCELLS");
  }catch(int e){
    return 1;
  }
  

  CDBDebug("Using CConvertASCAT.h");

  
  bool hasTimeData = false;
  
  //Is there a time variable
  CDF::Variable *origT = cdfObject->getVariableNE("time");
  if(origT!=NULL){
    hasTimeData=true;
    
    //Create a new time dimension for the new 2D fields.
    CDF::Dimension *dimT=new CDF::Dimension();
    dimT->name="time2D";
    dimT->setSize(1);
    cdfObject->addDimension(dimT);
    
    //Create a new time variable for the new 2D fields.
    CDF::Variable *varT = new CDF::Variable();
    varT->setType(CDF_DOUBLE);
    varT->name.copy(dimT->name.c_str());
    varT->setAttributeText("standard_name","time");
    varT->setAttributeText("long_name","time");
    varT->dimensionlinks.push_back(dimT);
    CDF::allocateData(CDF_DOUBLE,&varT->data,dimT->length);
    cdfObject->addVariable(varT);
    
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
          double firstTimeValue = tdata[0];
          size_t tsize = origT->getSize();
          if(hastfill==true){
            for(size_t j=0;j<tsize;j++){
              if(tdata[j]!=tfill){
                firstTimeValue = tdata[j];
              }
            }
          }
          #ifdef CCONVERTASCAT_DEBUG
          CDBDebug("firstTimeValue  = %f",firstTimeValue );
          #endif
          //Set the time data
          varT->setData(CDF_DOUBLE,&firstTimeValue,1);
        }
      }catch(int e){}
    }
  }
  
  //Standard bounding box of ascat data is worldwide
  double dfBBOX[]={-180,-90,180,90};
  
  //Default size of ascat 2dField is 2x2
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
  
  //Make a list of variables which will be available as 2D fields  
  CT::StackList<CT::string> varsToConvert;
  for(size_t v=0;v<cdfObject->variables.size();v++){
    CDF::Variable *var = cdfObject->variables[v];
    if(var->isDimension==false){
      if(!var->name.equals("time2D")&&!var->name.equals("time")&&!var->name.equals("lon")&&!var->name.equals("lat")&&!var->name.equals("longitude")&&!var->name.equals("latitude")){
        varsToConvert.add(CT::string(var->name.c_str()));
      }
    }
  }
  
  //Create the new 2D field variiables based on the swath variables
  for(size_t v=0;v<varsToConvert.size();v++){
    CDF::Variable *swathVar=cdfObject->getVariable(varsToConvert[v].c_str());
    if(swathVar->dimensionlinks.size()>=2)
    {
      #ifdef CCONVERTASCAT_DEBUG
      CDBDebug("Converting %s",swathVar->name.c_str());
      #endif
      
      CDF::Variable *new2DVar = new CDF::Variable();
      cdfObject->addVariable(new2DVar);
      
      //Assign X,Y,T dims 
      if(hasTimeData){
        CDF::Variable *newTimeVar=cdfObject->getVariableNE("time2D");             
        if(newTimeVar!=NULL){
          new2DVar->dimensionlinks.push_back(newTimeVar->dimensionlinks[0]);
        }
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
  }
  return 0;
}




/**
 * This function draws the virtual 2D variable into a new 2D field
 */
int CConvertASCAT::convertASCATData(CDataSource *dataSource,int mode){
 
  CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
  if(cdfObject->getDimensionNE("nXtrack")!=NULL&&cdfObject->getDimensionNE("nTimes")!=NULL){
    try{cdfObject->getDimension("nXtrack")->name="NUMCELLS";}catch(int e){}
    try{cdfObject->getDimension("nTimes")->name="NUMROWS";}catch(int e){}
  }
  
  
  try{
    cdfObject->getDimensionIgnoreCase("NUMROWS");
    cdfObject->getDimensionIgnoreCase("NUMCELLS");
  }catch(int e){
    return 1;
  }
 
  
  
  size_t nrDataObjects = dataSource->getNumDataObjects();
  if(nrDataObjects<=0)return 1;
   
  CDataSource::DataObject *dataObjects[nrDataObjects];
  for(size_t d=0;d<nrDataObjects;d++){
    dataObjects[d] = dataSource->getDataObject(d);
  }
  #ifdef CCONVERTASCAT_DEBUG
  
  CDBDebug("convertASCATData %s",dataObjects[0]->cdfVariable->name.c_str());
  #endif
  CDF::Variable *new2DVar[nrDataObjects];
  CDF::Variable *swathVar[nrDataObjects];
  
  for(size_t d=0;d<nrDataObjects;d++){
    new2DVar[d] = dataObjects[d]->cdfVariable;
    CT::string origSwathName=new2DVar[d]->name.c_str();
    origSwathName.concat("_backup");
    swathVar[d]=cdfObject->getVariableNE(origSwathName.c_str());
    if(swathVar[d]==NULL){
      CDBError("Unable to find orignal swath variable with name %s",origSwathName.c_str());
      return 1;
    }
  }
  
  
  CDF::Variable *swathLon;
  CDF::Variable *swathLat;
  
  try{
    swathLon = cdfObject->getVariable("lon");
    swathLat = cdfObject->getVariable("lat");
  }catch(int e){
    try{
      swathLon = cdfObject->getVariable("longitude");
      swathLat = cdfObject->getVariable("latitude");
    }catch(int e){
      CDBError("lat or lon variables not found");
    }
  }
  
  //Read original data first 
  for(size_t d=0;d<nrDataObjects;d++){
    swathVar[d]->readData(CDF_FLOAT,true);
    CDF::Attribute *fillValue = swathVar[d]->getAttributeNE("_FillValue");
    if(fillValue!=NULL){
      dataObjects[d]->hasNodataValue=true;
      fillValue->getData(&dataObjects[d]->dfNodataValue,1);
      #ifdef CCONVERTASCAT_DEBUG
      CDBDebug("_FillValue = %f",dataObjects[d]->dfNodataValue);
      #endif
      float f=dataObjects[d]->dfNodataValue;
      new2DVar[d]->getAttribute("_FillValue")->setData(CDF_FLOAT,&f,1);
    }else dataObjects[d]->hasNodataValue=false;
  }
  
  swathLon->readData(CDF_FLOAT,true);
  swathLat->readData(CDF_FLOAT,true);
  
  
  float fill = (float)dataObjects[0]->dfNodataValue;
  float min = fill;float max=fill;
  //Detect minimum and maximum values
  
  
  
  size_t l=swathVar[0]->getSize();
  for(size_t j=0;j<l;j++){
    float v=((float*)swathVar[0]->data)[j];
    if(v!=fill){
      if(min==fill)min=v;
      if(max==fill)max=v;
      if(v<min)min=v;
      if(v>max)max=v;
    }
  }
  #ifdef CCONVERTASCAT_DEBUG
  CDBDebug("Calculated min/max : %f %f",min,max);
  #endif
  
  
  
  //Set statistics
  if(dataSource->stretchMinMax){
    #ifdef CCONVERTASCAT_DEBUG
    CDBDebug("dataSource->stretchMinMax");
    #endif
    if(dataSource->statistics==NULL){
      #ifdef CCONVERTASCAT_DEBUG
      CDBDebug("Setting statistics: min/max : %f %f",min,max);
      #endif
      dataSource->statistics = new CDataSource::Statistics();
      dataSource->statistics->setMaximum(max);
      dataSource->statistics->setMinimum(min);
    }
  }
  
  //Make the width and height of the new 2D ascat field the same as the viewing window
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
  
  #ifdef CCONVERTASCAT_DEBUG
  CDBDebug("Datasource bbox:%f %f %f %f",dataSource->srvParams->Geo->dfBBOX[0],dataSource->srvParams->Geo->dfBBOX[1],dataSource->srvParams->Geo->dfBBOX[2],dataSource->srvParams->Geo->dfBBOX[3]);
  CDBDebug("Datasource width height %d %d",dataSource->dWidth,dataSource->dHeight);
  CDBDebug("L2 %d %d",dataSource->dWidth,dataSource->dHeight);
  #endif
  
  if(mode==CNETCDFREADER_MODE_OPEN_ALL){
    #ifdef CCONVERTASCAT_DEBUG
    CDBDebug("Drawing %s",new2DVar[0]->name.c_str());
    #endif
    
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
    
    size_t fieldSize = dataSource->dWidth*dataSource->dHeight;
    
    //Allocate and clear data
    for(size_t d=0;d<nrDataObjects;d++){
      new2DVar[d]->setSize(fieldSize);
      CDF::allocateData(new2DVar[d]->getType(),&(new2DVar[d]->data),fieldSize);
      for(size_t j=0;j<fieldSize;j++){
        ((float*)dataObjects[d]->cdfVariable->data)[j]=(float)dataObjects[d]->dfNodataValue;
      }
    }
    
    
    
    float *lonData=(float*)swathLon->data;
    float *latData=(float*)swathLat->data;
    
    int numRows=swathVar[0]->dimensionlinks[0]->getSize();
    int numCells=swathVar[0]->dimensionlinks[1]->getSize();
    #ifdef CCONVERTASCAT_DEBUG
    CDBDebug("numRows %d numCells %d",numRows,numCells);
    #endif
    
    
    
    
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
    if(projectionRequired){
      int status = imageWarper.initreproj(dataSource,dataSource->srvParams->Geo,&dataSource->srvParams->cfg->Projection);
      if(status !=0 ){
        CDBError("Unable to init projection");
        return 1;
      }
    }
    
    bool drawBilinear=false;
    CStyleConfiguration *styleConfiguration = dataSource->getStyle();
    if(styleConfiguration->styleCompositionName.indexOf("bilinear")>=0){
      drawBilinear=true;
    }

    for(int rowNr=0;rowNr<numRows;rowNr++){ 
      for(int cellNr=0;cellNr<numCells;cellNr++){
        int pSwath = cellNr+rowNr * numCells;
        int bo = (rowNr == 0 ?numCells:-numCells);
        double lons[4],lats[4];
        lons[0] = (float)lonData[pSwath];
        lons[1] = (float)lonData[pSwath+(cellNr==0?1:-1)];
        lons[2] = (float)lonData[pSwath+bo];
        lons[3] = (float)lonData[pSwath+bo+(cellNr==0?1:-1)];
        
        lats[0] = (float)latData[pSwath];
        lats[1] = (float)latData[pSwath+(cellNr==0?1:-1)];
        lats[2] = (float)latData[pSwath+bo];
        lats[3] = (float)latData[pSwath+bo+(cellNr==0?1:-1)];
        
        
        
        bool tileHasNoData = false;
        
        bool tileIsTooLarge=false;
        bool moveTile=false;
        
        for(int j=0;j<4;j++){
          if(lons[j]==fill){tileIsTooLarge=true;break;}
          if(lons[j]>185)moveTile=true;
        }
        if(tileIsTooLarge==false){
          float lon0 ;
          float lat0;
          for(int j=0;j<4;j++){
            if(moveTile==true)lons[j]-=360;
            if(lons[j]<-280)lons[j]+=360;
            if(j==0){
              lon0 =lons[0];
              lat0 =lats[0];
            }
            if(fabs(lon0-lons[j])>5)tileIsTooLarge=true;
            if(fabs(lat0-lats[j])>5)tileIsTooLarge=true;
          }
            int dlons[4],dlats[4];double rotation;
            for(size_t d=0;d<nrDataObjects;d++){
              float *sdata = ((float*)dataObjects[d]->cdfVariable->data);
              float *swathData = (float*)swathVar[d]->data;
              float vals[4];
              vals[0] = swathData[pSwath];

              if(drawBilinear){
                vals[1] = swathData[pSwath+(cellNr==0?1:-1)];
                vals[2] = swathData[pSwath+bo];
                vals[3] = swathData[pSwath+bo+(cellNr==0?1:-1)];
              }else{
                vals[1]=vals[0];
                vals[2]=vals[0];
                vals[3]=vals[0];
              }
              
              if(d==0){
                if(vals[0]==fill)tileHasNoData=true;
                if(vals[1]==fill)tileHasNoData=true;
                if(vals[2]==fill)tileHasNoData=true;
                if(vals[3]==fill)tileHasNoData=true;
              }
            
            rotation=0;
            if(tileHasNoData==false){
              double origLon=lons[0],origLat=lats[0];
              if(d==0){
                double latOffSetForRot=lats[0]-0.01;
                double lonOffSetForRot=lons[0];
                if(projectionRequired){
                  imageWarper.reprojfromLatLon(lonOffSetForRot,latOffSetForRot);
                }
                for(int j=0;j<4;j++){
                  if(projectionRequired){
                    if(imageWarper.reprojfromLatLon(lons[j],lats[j])!=0){tileHasNoData=true;break;}
                  }
                  dlons[j]=int((lons[j]-offsetX)/cellSizeX);
                  dlats[j]=int((lats[j]-offsetY)/cellSizeY);
                }
                if(projectionRequired){
                  double dy=lats[0] - latOffSetForRot;
                  double dx=lons[0] - lonOffSetForRot;
                  rotation= -(atan2(dy,dx)/(3.141592654))*180-90;
               
                }
                
               }
               if(tileHasNoData==false){
                  if(nrDataObjects==2){
                    if(dlons[0]>=0&&dlons[0]<dataSource->dWidth&&dlats[0]>0&&dlats[0]<dataSource->dHeight){
                      if(tileIsTooLarge==false){
                      //  if(d==1)vals[0]=0;
                        if(d==0){
                          //Wind direction in ascat has an oceanographic convention, for meteorological symbols it should be shifted 180 degrees.
                          rotation+=180;
                        }
                
                        float rad = 10;
                        dataObjects[d]->points.push_back(PointDVWithLatLon(dlons[0],dlats[0],origLon,origLat,vals[0],rotation, rad, rad));
                      }
                    }
                  }
            
                  if(tileHasNoData==false){
                    if(tileIsTooLarge==false){
                      fillQuadGouraud(sdata, vals, dataSource->dWidth,dataSource->dHeight, dlons,dlats);
                  }
                }
              }
            }
          }
        }
      }
    }
    imageWarper.closereproj();
  }
  #ifdef CCONVERTASCAT_DEBUG
  CDBDebug("/convertASCATData");
  #endif
  return 0;
}
