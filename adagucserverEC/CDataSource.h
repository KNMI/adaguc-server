#ifndef CDataSource_H
#define CDataSource_H
#include "CXMLSerializerInterface.h"
#include "CServerParams.h"
#include "CServerConfig_CPPXSD.h"
#include "CDebugger.h"
#include "Definitions.h"
#include "CTypes.h"
#include "CCDFDataModel.h"
#include "COGCDims.h"

/**
 * This class represents data to be used further in the server. Specific  metadata and data is filled in by CDataReader
 * This class is used for both image drawing (WMS) and raw data output (WCS)
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
    
  //Data class
  class DataClass{
    public:
      DataClass(){
        hasStatusFlag=false;
      }
      ~DataClass(){
        for(size_t j=0;j<statusFlagList.size();j++){
          delete statusFlagList[j];
        }
      }
      
      //Handle status flags properly
      bool hasStatusFlag;
      std::vector<StatusFlag*> statusFlagList;
      const char *getFlagMeaning(double value){
        for(size_t j=0;j<statusFlagList.size();j++){if(statusFlagList[j]->value==value){return statusFlagList[j]->meaning.c_str();}}
        return "no flag meaning";
      }
      
      void *data;
      CDFObject *cdfObject;
      CDF::Variable *cdfVariable;
      CDFType dataType;
      double dfNodataValue;
      bool hasNodataValue;
      CT::string variableName;
      CT::string units;
      bool scaleOffsetIsApplied;
      double dfscale_factor;
      double dfadd_offset;
  };
  class Statistics{
    private:
      double min,max;
    public:
      double getMinimum(){
        return min;
      }
      double getMaximum(){
        return max;
      }
      
      // TODO this currently works only for float data
      int calculate(CDataSource *dataSource){
        //Get Min and Max
        CDataSource::DataClass *dataObject = dataSource->dataObject[0];
        if(dataObject->data!=NULL){
          size_t s = dataSource->dWidth*dataSource->dHeight;
          float _min=0.0f,_max=0.0f;
          if(dataObject->dataType==CDF_FLOAT){
            float *data=(float*)dataObject->data;
            int firstDone=0;
        
            for(size_t p=0;p<s;p++){
              float v=data[p];
              if(v!=(float)dataObject->dfNodataValue||(!dataObject->hasNodataValue)){
                if(firstDone==0){
                  _min=v;_max=v;
                  firstDone=1;
                }else{
              //CDBDebug("%d=%f",p,v);
                  if(v<_min)_min=v;
                  if(v>_max)_max=v;
                }
              }
            }
            min=_min;
            max=_max;
          }
        }
        return 0;
      }
  };
  class TimeStep{
    public:
      CT::string fileName;   //Filename of the file to load
      CT::string timeString; //String of the current time
      CCDFDims   dims;//Dimension index in the corresponding name and file

  };
  int currentAnimationStep;
  std::vector <TimeStep*> timeSteps;
  bool stretchMinMax;
  std::vector <COGCDims*> requiredDims;
  Statistics *statistics;
  //The actual dataset data (can have multiple variables)
  std::vector <DataClass *> dataObject;
  //source image parameters
  double dfBBOX[4],dfCellSizeX,dfCellSizeY;
  int dWidth,dHeight;
  CT::string nativeEPSG;
  CT::string nativeProj4;

  //Used for scaling the legend to the palette range of 0-240
  float legendScale,legendOffset,legendLog,legendLowerRange,legendUpperRange;
  bool legendValueRange;

  //Number of metadata items
  size_t nrMetadataItems;
  CT::string *metaData;

  //Configured?
  bool isConfigured;

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
  CDataSource(){
    stretchMinMax=false;
    isConfigured=false;
    legendScale=1;
    legendOffset=0;
    legendLog=0.0f;
    legendLowerRange=0;
    legendUpperRange=0;
    legendValueRange=0;
    metaData=NULL;
    statistics=NULL;
    currentAnimationStep=0;
    srvParams=NULL;
    }
  ~CDataSource(){
    if(metaData!=NULL){delete[] metaData;metaData=NULL;}
    for(size_t j=0;j<dataObject.size();j++){
      delete dataObject[j];
      dataObject[j]=NULL;
    }
    for(size_t j=0;j<timeSteps.size();j++){
      delete timeSteps[j];
      timeSteps[j]=NULL;
    }
    for(size_t j=0;j<requiredDims.size();j++)delete requiredDims[j];
    if(statistics!=NULL)delete statistics;statistics=NULL;
  }
  void setCFGLayer(CServerParams *_srvParams,CServerConfig::XMLE_Configuration *_cfg,CServerConfig::XMLE_Layer * _cfgLayer,const char *_layerName){
    srvParams=_srvParams;
    cfg=_cfg;
    cfgLayer=_cfgLayer;
//    numVariables = cfgLayer->Variable.size();
    
    for(size_t j=0;j<cfgLayer->Variable.size();j++){
      DataClass *data = new DataClass();
      data->variableName.copy(cfgLayer->Variable[j]->value.c_str());
      dataObject.push_back(data);
    }
    if(cfgLayer->attr.type.equals("database")){
      dLayerType=CConfigReaderLayerTypeDataBase;
    }
    if(cfgLayer->attr.type.equals("styled")){
      dLayerType=CConfigReaderLayerTypeStyled;
    }
    if(cfgLayer->attr.type.equals("cascaded")){
      dLayerType=CConfigReaderLayerTypeCascaded;
    }
    
    //Deprecated
    if(cfgLayer->attr.type.equals("file")){
      dLayerType=CConfigReaderLayerTypeDataBase;//CConfigReaderLayerTypeFile;
    }
    //Set the layername
    layerName.copy(_layerName);
    isConfigured=true;
  }
  void addTimeStep(const char * pszName,const char *pszTimeString){
    TimeStep * timeStep = new TimeStep();
    timeSteps.push_back(timeStep);
    timeStep->fileName.copy(pszName);
    timeStep->timeString.copy(pszTimeString);
    //timeStep->dims=dims;
    if(timeSteps.size()==1)setTimeStep(currentAnimationStep);
  }
  const char *getFileName(){
    if(currentAnimationStep<0)return "less than 0";
    if(currentAnimationStep>(int)timeSteps.size())return "more than timeSteps.size()";
    return timeSteps[currentAnimationStep]->fileName.c_str();
  }
  void setTimeStep(int timeStep){
    if(timeStep<0)return;
    if(timeStep>(int)timeSteps.size())return;
    currentAnimationStep=timeStep;
    //dOGCDimValues[0]=timeSteps[currentAnimationStep]->dims.getDimensionIndex("time");
  }
  int getCurrentTimeStep(){
    return currentAnimationStep;
  }
  size_t getDimensionIndex(const char *name){
    return timeSteps[currentAnimationStep]->dims.getDimensionIndex(name);
  }
  size_t getDimensionIndex(int i){
    return timeSteps[currentAnimationStep]->dims.getDimensionIndex(i);
  }
  int getNumTimeSteps(){
    return (int)timeSteps.size();
  }
  const char *getLayerName(){
    return layerName.c_str();
  }
};

#endif
