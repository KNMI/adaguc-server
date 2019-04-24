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

#include "CStyleConfiguration.h"

#include "CGeoJSONData.h"

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
MinMax getMinMax(float *data, bool hasFillValue, double fillValue,size_t numElements);

/**
 * Returns minmax values for a variable
 * @param var The variable to retrieve the min max for.
 * @return minmax object
 */
MinMax getMinMax(CDF::Variable *var);



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
  bool dimsAreAutoConfigured;
private:
  
  CT::PointerList<CStyleConfiguration*> *_styles;
  CStyleConfiguration *_currentStyle;
  public:
    
  CStyleConfiguration * getStyle();
  
  void setStyle(CStyleConfiguration * style){
    CDBDebug("Setting styleconfiguration");
    _currentStyle = style;
  }
  

  class DataObject{
    public:
      DataObject();
      ~DataObject();
      bool hasStatusFlag,hasNodataValue,appliedScaleOffset,hasScaleOffset;
      double dfNodataValue,dfscale_factor,dfadd_offset;
      std::vector<StatusFlag*> statusFlagList;
      CDF::Variable *cdfVariable;
      CDFObject *cdfObject;
      CT::string overruledUnits;
      
      CT::string variableName;
      CT::string getUnits();
      void setUnits(CT::string units);
      std::vector<PointDVWithLatLon> points;
      std::map<int,CFeature> features;
      DataObject *clone();
  };
  
  class Statistics{
    public:
    template <class T>
    void calculate(size_t size,T*data,CDFType type,double dfNodataValue, bool hasNodataValue){
    T _min=(T)NAN,_max=(T)NAN;
    double _sum=0,_sumsquared=0;
    numSamples=0;
    T maxInf=(T)INFINITY;
    T minInf=(T)-INFINITY;
    
    bool checkInfinity = false;
    if(type==CDF_FLOAT||type==CDF_DOUBLE)checkInfinity=true;
    int firstDone=0;

    for(size_t p=0;p<size;p++){
      T v=data[p];
      if((((T)v)!=(T)dfNodataValue||(!hasNodataValue))&&v==v){
        if((checkInfinity&&v!=maxInf&&v!=minInf)||(!checkInfinity))
        {
          if(firstDone==0){
            _min=v;_max=v;
            firstDone=1;
          }else{
            if(v<_min)_min=v;
            if(v>_max)_max=v;
          }
          _sum+=v;
          _sumsquared+=(v*v);
          numSamples++;
        }
      }
    }
  avg=_sum/double(numSamples);
  stddev=sqrt((numSamples*_sumsquared-_sum*_sum)/(numSamples*(numSamples-1)));
  min=(double)_min;
  max=(double)_max;
}
  private:
     template <class T>
      void calcMinMax(size_t size,std::vector <DataObject *> *dataObject);
      double min,max,avg,stddev;
      size_t numSamples;
    public:
      Statistics(){
        min=0;
        max=0;
        avg=0;
        stddev=0;
        numSamples=0;
      }
      double getMinimum();
      double getMaximum();
      double getStdDev();
      double getAverage();
      void setMinimum(double min);
      void setMaximum(double max);
      size_t getNumSamples(){return numSamples;};
      int calculate(CDataSource *dataSource);
  };
  
  class TimeStep{
    public:
      CT::string fileName;   //Filename of the file to load
      CCDFDims   dims;//Dimension index in the corresponding name and file
  };
  int datasourceIndex;
  int currentAnimationStep;
  int threadNr;
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

//TODO KVP and metaDataItems can be moved out to GDAL datawriter  
   class KVP{
      public:
        KVP(const char *varname,const char *attrname,const char *value){
          this->varname = varname;
          this->attrname = attrname;
          this->value = value;
        }
        CT::string varname;
        CT::string attrname;
        CT::string value; 
    };
  
  std::vector<KVP> metaDataItems;
  

  //Configured?
  bool isConfigured;
  
  
  //Used for vectors and points
  bool formatConverterActive;
  
  //The index of the X and Y dimension in the variable dimensionlist (not the id's from the netcdf file)
  int dimXIndex;
  int dimYIndex;
  
  //The striding of the read 2D map
  int stride2DMap;
  
  
  // Lon transformation is used to swap datasets from 0-360 degrees to -180 till 180 degrees
  //Swap data from >180 degrees to domain of -180 till 180 in case of lat lon source data
  int useLonTransformation;
  double origBBOXLeft,origBBOXRight;
  int dOrigWidth;
  bool lonTransformDone;
  
  //Sometimes X and Y need to be swapped, this boolean indicates whether it should or not.
  bool swapXYDimensions;
  
  //X and Y variables of the 2D field
  CDF::Variable *varX;
  CDF::Variable *varY;
   
  
  
  //Numver of dims
  int dNetCDFNumDims;
  int dLayerType;
  CT::string layerName;
  
  //
  bool queryBBOX; //True: query on viewport
  double nativeViewPortBBOX[4];
  int queryLevel;

  //Current value index of the dim
  //int dOGCDimValues[MAX_DIMS];
  
  CServerParams *srvParams;

  //Link to the XML configuration
  CServerConfig::XMLE_Layer * cfgLayer;
  CServerConfig::XMLE_Configuration *cfg;
  
  CT::string featureSet;
  
  //Link to the root CDFObject, is owned by the datareader.
  
  CDataSource();
  ~CDataSource();
  static void readStatusFlags(CDF::Variable * var, std::vector<CDataSource::StatusFlag*> *statusFlagList);
  static const char *getFlagMeaning(std::vector<CDataSource::StatusFlag*> *statusFlagList,double value);
  static void getFlagMeaningHumanReadable(CT::string *flagMeaning ,std::vector<CDataSource::StatusFlag*> *statusFlagList,double value);
  //int autoCompleteDimensions(CPGSQLDB *dataBaseConnection);
  
  int setCFGLayer(CServerParams *_srvParams,CServerConfig::XMLE_Configuration *_cfg,CServerConfig::XMLE_Layer * _cfgLayer,const char *_layerName, int layerIndex);
  void addStep(const char * fileName, CCDFDims *dims);
  const char *getFileName();
  
  DataObject *getDataObject(const char *name){
   //Find out to which dataobject we need to write to
    for(size_t dataObjectNr = 0; dataObjectNr<dataObjects.size();dataObjectNr++){
      if(dataObjects[dataObjectNr]->cdfVariable->name.equals(name)){
        return dataObjects[dataObjectNr];
      }
    }
    CDBError("Unable to find dataobject: variable %s not found",name);
    throw (CEXCEPTION_NULLPOINTER);
  };
  
  DataObject *getDataObject(int j){
    
    if(int(dataObjects.size()) <= j){
      CDBError("No Data object witn nr %d (total %d) for animation step %d (total steps %d)",j,currentAnimationStep,dataObjects.size(),timeSteps.size());
      throw (CEXCEPTION_NULLPOINTER);
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
  CT::string getDimensionValue(int i);
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
  
  CDataSource * clone();
  
  /**
   * IMPORTANT
   */
  CT::PointerList<CStyleConfiguration*> *getStyleListForDataSource(CDataSource *dataSource);
  
  static void calculateScaleAndOffsetFromMinMax(float &scale, float &offset,float min,float max,float log);
  static CT::PointerList<CT::string*> *getLegendListForDataSource(CDataSource *dataSource, CServerConfig::XMLE_Style* style);
  static CT::PointerList<CT::string*> *getStyleNames(std::vector <CServerConfig::XMLE_Styles*> Styles);
  
  static CT::PointerList<CT::string*> *getRenderMethodListForDataSource(CDataSource *dataSource, CServerConfig::XMLE_Style* style);      
  static int  getServerLegendIndexByName(const char * legendName,std::vector <CServerConfig::XMLE_Legend*> serverLegends);
  static int  getServerStyleIndexByName(const char * styleName,std::vector <CServerConfig::XMLE_Style*> serverStyles);
  static int makeStyleConfig(CStyleConfiguration *styleConfig,CDataSource *dataSource);//,const char *styleName,const char *legendName,const char *renderMethod);
  //static void getStyleConfigurationByName(const char *styleName,CDataSource *dataSource);
  
  /**
   * Sets the style by name, can be a character string.
   * @param styleName The name of the style
   * returns zero on success.
   */
  int setStyle(const char *styleName);
  
};

#endif
