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

#ifndef CDATASOURCE_H
#define CDATASOURCE_H
#include <math.h>
#include "CXMLSerializerInterface.h"
#include "CServerParams.h"
#include "CServerConfig_CPPXSD.h"
#include "CDebugger.h"
#include "Definitions.h"
#include "CTypes.h"
#include "CCDFDataModel.h"
#include "COGCDims.h"
#include "CStopWatch.h"
#include "CPGSQLDB.h"
#include "CStyleConfiguration.h"


/**
 * Class which holds min and max values.
 * isSet indicates whether the values have been set or not.
 */
class MinMax{
public:
  MinMax(){
    isSet = false;
  }
  bool isSet;
  double min,max;
};


/**
 * Returns minmax values for a data array
 * throws integer if no min max are found
 * @param data The data array in double format
 * @param hasFillValue Is there a nodata value
 * @param fillValue the Nodata value
 * @param numElements The length of the data array
 * @return minmax object
 */
static MinMax getMinMax(float *data, bool hasFillValue, double fillValue,size_t numElements){
  MinMax minMax;
  bool firstSet = false;
  for(size_t j=0;j<numElements;j++){
    float v=data[j];
    if(v==v){
      if(v!=fillValue||(!hasFillValue)){
        if(firstSet == false){
          firstSet = true;
          minMax.min=v;
          minMax.max=v;
          minMax.isSet=true;
        }
        if(v<minMax.min)minMax.min=v;
        if(v>minMax.max)minMax.max=v;
      }
    }
  }
  if(minMax.isSet==false){
    throw __LINE__+100;
  }
  return minMax;
}      


/**
 * Returns minmax values for a variable
 * @param var The variable to retrieve the min max for.
 * @return minmax object
 */
static MinMax getMinMax(CDF::Variable *var){
  MinMax minMax;
  if(var!=NULL){
 
      float *data = (float*)var->data;
      
      float scaleFactor=1,addOffset=0,fillValue = 0;
      bool hasFillValue = false;
      
      try{
        var->getAttribute("scale_factor")->getData(&scaleFactor,1);
      }catch(int e){}
      try{
        var->getAttribute("add_offset")->getData(&addOffset,1);
      }catch(int e){}
      try{
        var->getAttribute("_FillValue")->getData(&fillValue,1);
        hasFillValue = true;
      }catch(int e){}
      
      size_t lsize= var->getSize();
      
      //Apply scale and offset
      if(scaleFactor!=1||addOffset!=0){
        for(size_t j=0;j<lsize;j++){
          data[j]=data[j]*scaleFactor+addOffset;
        }
        fillValue=fillValue*scaleFactor+addOffset;
      }
      

      minMax = getMinMax(data,hasFillValue,fillValue,lsize);
    
  }else{
    //CDBError("getMinMax: Variable has not been set");
    throw __LINE__+100;
  }
  return minMax;
}



/**
 * This class represents data to be used further in the server. Specific  metadata and data is filled in by CDataReader
 * This class is used for both image drawing (WMS) and data output (WCS)
 */
class CDataSource{
  private:
   DEF_ERRORFUNCTION();
 
   
  public:
  class StatusFlag{
    public:
      CT::string meaning;
      double value;
  };
  
  CStyleConfiguration *styleConfiguration;

  class DataObject{
    public:
      DataObject();
      ~DataObject();
      bool hasStatusFlag,hasNodataValue,appliedScaleOffset,hasScaleOffset;
      double dfNodataValue,dfscale_factor,dfadd_offset;
      std::vector<StatusFlag*> statusFlagList;
      CDF::Variable *cdfVariable;
      CDFObject *cdfObject;
      
      CT::string variableName,units;
      std::vector<PointDVWithLatLon> points;
  };
  
  class Statistics{
    private:
      template <class T>
      void calcMinMax(size_t size,std::vector <DataObject *> *dataObject){
#ifdef MEASURETIME
  StopWatch_Stop("Start min/max calculation");
#endif
      if(dataObject->size()==1){
        T* data = (T*)(*dataObject)[0]->cdfVariable->data;
        
        CDFType type=(*dataObject)[0]->cdfVariable->getType();
        
        //CDBDebug("nodataval %f",(T)dataObject[0]->dfNodataValue);
        
        T _min=(T)0.0f,_max=(T)1.0f;
        
        T maxInf=(T)INFINITY;
        T minInf=(T)-INFINITY;
        
        bool checkInfinity = false;
        if(type==CDF_FLOAT||type==CDF_DOUBLE)checkInfinity=true;
        
        //CDBDebug("MINMAX %f %f %s",(double)maxInf,(double)minInf,CDF::getCDFDataTypeName(type).c_str());
        int firstDone=0;
        for(size_t p=0;p<size;p++){
          
          T v=data[p];

          //CDBDebug("Value %d =  %f",p,(double)v);
          if((((T)v)!=(T)(*dataObject)[0]->dfNodataValue||(!(*dataObject)[0]->hasNodataValue))&&v==v){
            if((checkInfinity&&v!=maxInf&&v!=minInf)||(!checkInfinity))
            {
            
              if(firstDone==0){
                _min=v;_max=v;
                firstDone=1;
              }else{
                if(v<_min)_min=v;
                if(v>_max)_max=v;
              }
            }
          }
        }
        min=(double)_min;
        max=(double)_max;
      }
      //Wind vector min max calculation
      if(dataObject->size()==2){
         T* dataU = (T*)(*dataObject)[0]->cdfVariable->data;
         T* dataV = (T*)(*dataObject)[1]->cdfVariable->data;
      //CDBDebug("nodataval %f",(T)dataObject->dfNodataValue);
        T _min=(T)0.0f,_max=(T)0.0f;
        int firstDone=0;
        T s =0;
        for(size_t p=0;p<size;p++){
          
          T u=dataU[p];
          T v=dataV[p];
          
          if(((((T)v)!=(T)(*dataObject)[0]->dfNodataValue||(!(*dataObject)[0]->hasNodataValue))&&v==v)&&
            ((((T)u)!=(T)(*dataObject)[0]->dfNodataValue||(!(*dataObject)[0]->hasNodataValue))&&u==u)){
            s=(T)hypot(u,v);
            if(firstDone==0){
              _min=s;_max=s;
              firstDone=1;
            }else{
              
              if(s<_min)_min=s;
              if(s>_max)_max=s;
            }
          }
        }
        min=(double)_min;
        max=(double)_max;
      }
#ifdef MEASURETIME
  StopWatch_Stop("Finished min/max calculation");
#endif
      }
      double min,max;
    public:
      Statistics(){
        min=0;
        max=0;
      }
      double getMinimum();
      double getMaximum();
      void setMinimum(double min);
      void setMaximum(double max);
      int calculate(CDataSource *dataSource);      // TODO this currently works only for float data
  };
  
  class TimeStep{
    public:
   
      CT::string fileName;   //Filename of the file to load
      //CT::string timeString; //String of the current time
      CCDFDims   dims;//Dimension index in the corresponding name and file
  
  };
  int datasourceIndex;
  int currentAnimationStep;
  
  /**
   * The amount of steps in this datasource
   */
  std::vector <TimeStep*> timeSteps;
  
  /**
   * Returns the value for a certain dimension and step
   */
  CT::string getDimensionValueForNameAndStep(const char *dimName,int dimStep);
  
  
  std::vector <DataObject *> dataObjects;
  
  bool stretchMinMax,stretchMinMaxDone;
  
  /**
   * The required dimensions for this datasource
   */
  std::vector <COGCDims*> requiredDims;
  Statistics *statistics; // is NULL when not available
  //The actual dataset data (can have multiple variables)
  
  //source image parameters
  double dfBBOX[4],dfCellSizeX,dfCellSizeY;
  int dWidth,dHeight;
  CT::string nativeEPSG;
  CT::string nativeProj4;

  //Used for scaling the legend to the palette range of 0-240
  /*float legendScale,legendOffset,legendLog,legendLowerRange,legendUpperRange;
  bool legendValueRange;
  CT::string styleName;*/

  //Number of metadata items
  size_t nrMetadataItems;
  CT::string *metaData;

  //Configured?
  bool isConfigured;
  
  
  //Used for vectors and points
  bool level2CompatMode;
  
  //The index of the X and Y dimension in the variable dimensionlist (not the id's from the netcdf file)
  int dimXIndex;
  int dimYIndex;
  
  //The striding of the read 2D map
  int stride2DMap;
  
  
  // Lon transformation is used to swap datasets from 0-360 degrees to -180 till 180 degrees
  //Swap data from >180 degrees to domain of -180 till 180 in case of lat lon source data
  int useLonTransformation;
  
  //Sometimes X and Y need to be swapped, this boolean indicates whether it should or not.
  bool swapXYDimensions;
  
  //X and Y variables of the 2D field
  CDF::Variable *varX;
  CDF::Variable *varY;
   
  
  
  //Numver of dims
  int dNetCDFNumDims;
  int dLayerType;
  CT::string layerName;

  //Current value index of the dim
  //int dOGCDimValues[MAX_DIMS];
  
  CServerParams *srvParams;

  //Link to the XML configuration
  CServerConfig::XMLE_Layer * cfgLayer;
  CServerConfig::XMLE_Configuration *cfg;
  
  //Link to the root CDFObject, is owned by the datareader.
  
  
  CDataSource();
  ~CDataSource();
  static void readStatusFlags(CDF::Variable * var, std::vector<CDataSource::StatusFlag*> *statusFlagList);
  static const char *getFlagMeaning(std::vector<CDataSource::StatusFlag*> *statusFlagList,double value);
  static void getFlagMeaningHumanReadable(CT::string *flagMeaning ,std::vector<CDataSource::StatusFlag*> *statusFlagList,double value);
  //int autoCompleteDimensions(CPGSQLDB *dataBaseConnection);
  int checkDimTables(CPGSQLDB *dataBaseConnection);
  
  
  int setCFGLayer(CServerParams *_srvParams,CServerConfig::XMLE_Configuration *_cfg,CServerConfig::XMLE_Layer * _cfgLayer,const char *_layerName, int layerIndex);
  void addStep(const char * fileName, CCDFDims *dims);
  const char *getFileName();
  
  DataObject *getDataObject(int j){
    
    if(int(dataObjects.size()) <= j){
      CDBError("No data objects for animation step %d of %d",currentAnimationStep,timeSteps.size());
      return NULL;
    }

    DataObject *d = dataObjects[j];
    //CDBDebug("getDataObject %d %d",currentAnimationStep,j);
    return d;
  }
  
  std::vector <DataObject *> *getDataObjectsVector(){
     return &(dataObjects);
  }
  
  size_t getNumDataObjects(){
    return dataObjects.size();
  }
  void eraseDataObject(int j){
    delete dataObjects[j];
    dataObjects.erase(dataObjects.begin()+j);
  }
  void setTimeStep(int timeStep);
  int getCurrentTimeStep();
  size_t getDimensionIndex(const char *name);
  size_t getDimensionIndex(int i);
  CCDFDims *getCDFDims();
  int getNumTimeSteps();
  const char *getLayerName();
  
  
  int attachCDFObject(CDFObject *cdfObject){
    if(cdfObject==NULL){
      CDBError("cdfObject==NULL");
      return 1;
    }
    if(isConfigured==false){
      CDBError("Datasource %s is not configured",cfgLayer->Name[0]->value.c_str());
      return 1;
    }
    if(getNumDataObjects()<=0){
      CDBError("No variables found for datasource %s",cfgLayer->Name[0]->value.c_str());
      return 1;
    }
  
    for(size_t varNr=0;varNr<getNumDataObjects();varNr++){
      getDataObject(varNr)->cdfObject = cdfObject;
      getDataObject(varNr)->cdfVariable = cdfObject->getVariableNE(getDataObject(varNr)->variableName.c_str());
      if(getDataObject(varNr)->cdfVariable==NULL){
        CDBError("attachCDFObject: variable \"%s\" does not exist",getDataObject(varNr)->variableName.c_str());
        return 1;
      }
    }
    return 0;
  }
  void detachCDFObject(){
    for(size_t j=0;j<getNumDataObjects();j++){
      getDataObject(j)->cdfVariable = NULL;
      getDataObject(j)->cdfObject = NULL;
    }
  }
  
};

#endif
