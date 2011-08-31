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
    size_t size = dataSource->dWidth*dataSource->dHeight;
    
    if(dataObject->dataType==CDF_CHAR)calcMinMax((char*)dataObject->data,size,dataObject);
    if(dataObject->dataType==CDF_BYTE)calcMinMax((char*)dataObject->data,size,dataObject);
    if(dataObject->dataType==CDF_UBYTE)calcMinMax((unsigned char*)dataObject->data,size,dataObject);
    if(dataObject->dataType==CDF_SHORT)calcMinMax((short*)dataObject->data,size,dataObject);
    if(dataObject->dataType==CDF_USHORT)calcMinMax((unsigned short*)dataObject->data,size,dataObject);
    if(dataObject->dataType==CDF_INT)calcMinMax((int*)dataObject->data,size,dataObject);
    if(dataObject->dataType==CDF_UINT)calcMinMax((unsigned int*)dataObject->data,size,dataObject);
    if(dataObject->dataType==CDF_FLOAT)calcMinMax((float*)dataObject->data,size,dataObject);
    if(dataObject->dataType==CDF_DOUBLE)calcMinMax((double*)dataObject->data,size,dataObject); 
    
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
  datasourceIndex=0;
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
}

int CDataSource::setCFGLayer(CServerParams *_srvParams,CServerConfig::XMLE_Configuration *_cfg,CServerConfig::XMLE_Layer * _cfgLayer,const char *_layerName,int layerIndex){
  srvParams=_srvParams;
  cfg=_cfg;
  cfgLayer=_cfgLayer;
//    numVariables = cfgLayer->Variable.size();
 
  datasourceIndex=layerIndex;
  for(size_t j=0;j<cfgLayer->Variable.size();j++){
    DataClass *data = new DataClass();
    data->variableName.copy(cfgLayer->Variable[j]->value.c_str());
    dataObject.push_back(data);
  }
  //Defaults to database
  dLayerType=CConfigReaderLayerTypeDataBase;
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
  CT::string layerUniqueName;
  if(_layerName==NULL){
    if(srvParams->makeUniqueLayerName(&layerUniqueName,cfgLayer)!=0)layerUniqueName="undefined";
    _layerName=layerUniqueName.c_str();
  }
  if(isalpha(_layerName[0])==0)layerName="ID_";else layerName="";
  
  layerName.concat(_layerName);
#ifdef CDATAREADER_DEBUG  
  CDBDebug("LayerName=\"%s\"",layerName.c_str());
#endif  

  //When a database table is not configured, generate a name automatically
  if( dLayerType!=CConfigReaderLayerTypeCascaded){
    if(cfgLayer->DataBaseTable.size()==0){
        CServerConfig::XMLE_DataBaseTable *dbtable=new CServerConfig::XMLE_DataBaseTable();
        cfgLayer->DataBaseTable.push_back(dbtable);
        //Create a table name based on the filepath and its filter.
        CT::string tableName=layerName.c_str();
        if(cfgLayer->FilePath[0]->attr.filter.c_str()==NULL){
          cfgLayer->FilePath[0]->attr.filter.copy("\\.nc");
        }
        srvParams->lookupTableName(&tableName,cfgLayer->FilePath[0]->value.c_str(),cfgLayer->FilePath[0]->attr.filter.c_str());
        srvParams->encodeTableName(&tableName);
        dbtable->value.copy(tableName.c_str());
    }
  }

  isConfigured=true;
  return 0;
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
  if(currentAnimationStep<0)return NULL;
  if(currentAnimationStep>=(int)timeSteps.size())return NULL;
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

