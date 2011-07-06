#include "CDataSource.h"
const char *CDataSource::className = "CDataSource";

/************************************/
/* CDataSource::CDataClass          */
/************************************/
CDataSource::DataClass::DataClass(){
  hasStatusFlag=false;
  cdfVariable = NULL;
  cdfObject=NULL;
}
CDataSource::DataClass::~DataClass(){
  for(size_t j=0;j<statusFlagList.size();j++){
    delete statusFlagList[j];
  }
}


const char *CDataSource::DataClass::getFlagMeaning(double value){
  for(size_t j=0;j<statusFlagList.size();j++){if(statusFlagList[j]->value==value){return statusFlagList[j]->meaning.c_str();}}
  return "no flag meaning";
}

/************************************/
/* CDataSource::Statistics          */
/************************************/
double CDataSource::Statistics::getMinimum(){
  return min;
}
double CDataSource::Statistics::getMaximum(){
  return max;
}

// TODO this currently works only for float data
int CDataSource::Statistics::calculate(CDataSource *dataSource){
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


/************************************/
/* CDataSource                      */
/************************************/

CDataSource::CDataSource(){
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
  cfgLayer = NULL;
  cfg=NULL;
 
}

CDataSource::~CDataSource(){
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
  //if(cdfObject!=NULL)delete cdfObject;cdfObject=NULL;(not owned by datasource)
}

void CDataSource::setCFGLayer(CServerParams *_srvParams,CServerConfig::XMLE_Configuration *_cfg,CServerConfig::XMLE_Layer * _cfgLayer,const char *_layerName){
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
  //A layername has to start with a letter (not numeric value);
  layerName="ID_";
  layerName.concat(_layerName);

  //When a database table is not configured, generate a name automatically
  if(cfgLayer->DataBaseTable.size()==0){
      CServerConfig::XMLE_DataBaseTable *dbtable=new CServerConfig::XMLE_DataBaseTable();
      cfgLayer->DataBaseTable.push_back(dbtable);
      //Create a table name based on the filepath and its filter.
      CT::string tableName=layerName.c_str();
      srvParams->lookupTableName(&tableName,cfgLayer->FilePath[0]->value.c_str(),cfgLayer->FilePath[0]->attr.filter.c_str());
      srvParams->encodeTableName(&tableName);
      dbtable->value.copy(tableName.c_str());
  }

  isConfigured=true;
}

void CDataSource::addTimeStep(const char * pszName,const char *pszTimeString){
  TimeStep * timeStep = new TimeStep();
  timeSteps.push_back(timeStep);
  timeStep->fileName.copy(pszName);
  timeStep->timeString.copy(pszTimeString);
  //timeStep->dims=dims;
  if(timeSteps.size()==1)setTimeStep(currentAnimationStep);
}

const char *CDataSource::getFileName(){
  if(currentAnimationStep<0)return "less than 0";
  if(currentAnimationStep>(int)timeSteps.size())return "more than timeSteps.size()";
  return timeSteps[currentAnimationStep]->fileName.c_str();
}

void CDataSource::setTimeStep(int timeStep){
  if(timeStep<0)return;
  if(timeStep>(int)timeSteps.size())return;
  currentAnimationStep=timeStep;
  //dOGCDimValues[0]=timeSteps[currentAnimationStep]->dims.getDimensionIndex("time");
}

int CDataSource::getCurrentTimeStep(){
  return currentAnimationStep;
}

size_t CDataSource::getDimensionIndex(const char *name){
  return timeSteps[currentAnimationStep]->dims.getDimensionIndex(name);
}

size_t CDataSource::getDimensionIndex(int i){
  return timeSteps[currentAnimationStep]->dims.getDimensionIndex(i);
}

int CDataSource::getNumTimeSteps(){
  return (int)timeSteps.size();
}

const char *CDataSource::getLayerName(){
  return layerName.c_str();
}

