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

#include "CDataSource.h"
#include "CDBFileScanner.h"
const char *CDataSource::className = "CDataSource";

/************************************/
/* CDataSource::CDataObject          */
/************************************/
CDataSource::DataObject::DataObject(){
  hasStatusFlag=false;
  appliedScaleOffset = false;
  hasScaleOffset = false;
  cdfVariable = NULL;
  cdfObject=NULL;
  dfadd_offset=0;
  dfscale_factor=1;
  std::vector <CPoint> points;
}
CDataSource::DataObject::~DataObject(){
  for(size_t j=0;j<statusFlagList.size();j++){
    delete statusFlagList[j];
  }
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

void CDataSource::Statistics::setMinimum(double min){
  this->min=min;
}
void CDataSource::Statistics::setMaximum(double max){
  this->max=max;
}

// TODO this currently works only for float data
int CDataSource::Statistics::calculate(CDataSource *dataSource){
  //Get Min and Max
  CDBDebug("calculate stat ");
  CDataSource::DataObject *dataObject = dataSource->getDataObject(0);
  if(dataObject->cdfVariable->data!=NULL){
    size_t size = dataObject->cdfVariable->getSize();//dataSource->dWidth*dataSource->dHeight;
    
    if(dataObject->cdfVariable->getType()==CDF_CHAR)calcMinMax<char>(size,dataSource->getDataObjectsVector());
    if(dataObject->cdfVariable->getType()==CDF_BYTE)calcMinMax<char>(size,dataSource->getDataObjectsVector());
    if(dataObject->cdfVariable->getType()==CDF_UBYTE)calcMinMax<unsigned char>(size,dataSource->getDataObjectsVector());
    if(dataObject->cdfVariable->getType()==CDF_SHORT)calcMinMax<short>(size,dataSource->getDataObjectsVector());
    if(dataObject->cdfVariable->getType()==CDF_USHORT)calcMinMax<unsigned short>(size,dataSource->getDataObjectsVector());
    if(dataObject->cdfVariable->getType()==CDF_INT)calcMinMax<int>(size,dataSource->getDataObjectsVector());
    if(dataObject->cdfVariable->getType()==CDF_UINT)calcMinMax<unsigned int>(size,dataSource->getDataObjectsVector());
    if(dataObject->cdfVariable->getType()==CDF_FLOAT)calcMinMax<float>(size,dataSource->getDataObjectsVector());
    if(dataObject->cdfVariable->getType()==CDF_DOUBLE)calcMinMax<double>(size,dataSource->getDataObjectsVector()); 
    
  }
  return 0;
}


/************************************/
/* CDataSource                      */
/************************************/

CDataSource::CDataSource(){
  stretchMinMax=false;
  stretchMinMaxDone = false;
  isConfigured=false;
//   legendScale=1;
//   legendOffset=0;
//   legendLog=0.0f;
//   legendLowerRange=0;
//   legendUpperRange=0;
//   legendValueRange=0;
  metaData=NULL;
  statistics=NULL;
  currentAnimationStep=0;
  srvParams=NULL;
  cfgLayer = NULL;
  cfg=NULL;
  datasourceIndex=0;
  level2CompatMode=false;
  useLonTransformation = -1;
  swapXYDimensions = false;
  varX = NULL;
  varY = NULL;
  
 // CDBDebug("C");
  styleConfiguration = new CStyleConfiguration ();
  
}

CDataSource::~CDataSource(){
  if(metaData!=NULL){delete[] metaData;metaData=NULL;}
    
  for(size_t d=0;d<dataObjects.size();d++){
    delete dataObjects[d];
    dataObjects[d]=NULL;
  }

  for(size_t j=0;j<timeSteps.size();j++){

    delete timeSteps[j];
    timeSteps[j]=NULL;
  }
  for(size_t j=0;j<requiredDims.size();j++)delete requiredDims[j];
  if(statistics!=NULL)delete statistics;statistics=NULL;
  
  //CDBDebug("D");
  if(styleConfiguration!=NULL){
    delete styleConfiguration;
    styleConfiguration = NULL;
  }
}

int CDataSource::setCFGLayer(CServerParams *_srvParams,CServerConfig::XMLE_Configuration *_cfg,CServerConfig::XMLE_Layer * _cfgLayer,const char *_layerName,int layerIndex){
  srvParams=_srvParams;
  cfg=_cfg;
  cfgLayer=_cfgLayer;
//    numVariables = cfgLayer->Variable.size();
// CDBDebug("Configure layer ");
  datasourceIndex=layerIndex;
  for(size_t j=0;j<cfgLayer->Variable.size();j++){
    DataObject *newDataObject = new DataObject();
    newDataObject->variableName.copy(cfgLayer->Variable[j]->value.c_str());
    getDataObjectsVector()->push_back(newDataObject);
   }
  //Set the layername
  CT::string layerUniqueName;
  if(_layerName==NULL){
    if(srvParams->makeUniqueLayerName(&layerUniqueName,cfgLayer)!=0)layerUniqueName="undefined";
    _layerName=layerUniqueName.c_str();
    
  }
  
  //A layername has to start with a letter (not numeric value);
  if(isalpha(_layerName[0])==0)layerName="ID_";else layerName="";
  layerName.concat(_layerName);
  
#ifdef CDATAREADER_DEBUG  
  CDBDebug("LayerName=\"%s\"",layerName.c_str());
#endif  
  //Defaults to database
  dLayerType=CConfigReaderLayerTypeDataBase;
  if(cfgLayer->attr.type.equals("database")){
    dLayerType=CConfigReaderLayerTypeDataBase;
  }else if(cfgLayer->attr.type.equals("styled")){
    dLayerType=CConfigReaderLayerTypeStyled;
  }else if(cfgLayer->attr.type.equals("cascaded")){
    dLayerType=CConfigReaderLayerTypeCascaded;
  }else if(cfgLayer->attr.type.equals("image")){
    dLayerType=CConfigReaderLayerTypeCascaded;
  }else if(cfgLayer->attr.type.equals("grid")){
    dLayerType=CConfigReaderLayerTypeCascaded;
  }else if(cfgLayer->attr.type.equals("autoscan")){
    dLayerType=CConfigReaderLayerTypeUnknown;
  }else if(cfgLayer->attr.type.empty()==false){
    if(strlen(cfgLayer->attr.type.c_str())>0){
      dLayerType=CConfigReaderLayerTypeUnknown;
      CDBError("Unknown layer type for layer %s",layerName.c_str());
      return 1;
    }
  }
  //CDBDebug("cfgLayer->attr.type %s %d",cfgLayer->attr.type.c_str(),dLayerType);
  //Deprecated
  if(cfgLayer->attr.type.equals("file")){
    dLayerType=CConfigReaderLayerTypeDataBase;//CConfigReaderLayerTypeFile;
  }
  
  
  //When a database table is not configured, generate a name automatically
  /*if( dLayerType!=CConfigReaderLayerTypeCascaded){
    if(cfgLayer->DataBaseTable.size()==0){
        CServerConfig::XMLE_DataBaseTable *dbtable=new CServerConfig::XMLE_DataBaseTable();
        cfgLayer->DataBaseTable.push_back(dbtable);
        //Create a table name based on the filepath and its filter.
        CT::string tableName=layerName.c_str();
        if(cfgLayer->FilePath[0]->attr.filter.empty()){
          cfgLayer->FilePath[0]->attr.filter.copy("\\.nc");
        }
        srvParams->lookupTableName(&tableName,cfgLayer->FilePath[0]->value.c_str(),cfgLayer->FilePath[0]->attr.filter.c_str());
        srvParams->encodeTableName(&tableName);
        dbtable->value.copy(tableName.c_str());
    }
  }*/

  isConfigured=true;
  return 0;
}

void CDataSource::addStep(const char * fileName, CCDFDims *dims){
  
  TimeStep * timeStep = new TimeStep();
  timeSteps.push_back(timeStep);
  currentAnimationStep = timeSteps.size()-1;
//  CDBDebug("Adding dataobject for timestep %d",currentAnimationStep);
  timeStep->fileName.copy(fileName);
  if(dims!=NULL){
    timeStep->dims.copy(dims);
  }
//   for(size_t j=0;j<cfgLayer->Variable.size();j++){
//     DataObject *newDataObject = new DataObject();
//     newDataObject->variableName.copy(cfgLayer->Variable[j]->value.c_str());
//    
//     getDataObjectsVector()->push_back(newDataObject);
//     
//   }
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


CCDFDims *CDataSource::getCDFDims(){
  return &timeSteps[currentAnimationStep]->dims;
}

void CDataSource::readStatusFlags(CDF::Variable * var, std::vector<CDataSource::StatusFlag*> *statusFlagList){
  for(size_t i=0;i<statusFlagList->size();i++)delete (*statusFlagList)[i];
  statusFlagList->clear();
  if(var!=NULL){
    CDF::Attribute *attr_flag_meanings=var->getAttributeNE("flag_meanings");
    //We might have status flag, check if all mandatory attributes are set!
    if(attr_flag_meanings!=NULL){
      CDF::Attribute *attr_flag_values=var->getAttributeNE("flag_values");
      if(attr_flag_values==NULL){
        attr_flag_values=var->getAttributeNE("flag_masks");
      }
      if(attr_flag_values!=NULL){
        CT::string flag_meanings;
        attr_flag_meanings->getDataAsString(&flag_meanings);
        CT::string *flagStrings=flag_meanings.splitToArray(" ");
        size_t nrOfFlagMeanings=flagStrings->count;
        if(nrOfFlagMeanings>0){
          size_t nrOfFlagValues=attr_flag_values->length;
          //Check we have an equal number of flagmeanings and flagvalues
          //nrOfFlagValues=54;
          
          if(nrOfFlagMeanings==nrOfFlagValues){
            //hasStatusFlag=true;
            double dfFlagValues[nrOfFlagMeanings+1];
            attr_flag_values->getData(dfFlagValues,attr_flag_values->length);
            for(size_t j=0;j<nrOfFlagMeanings;j++){
              CDataSource::StatusFlag * statusFlag = new CDataSource::StatusFlag;
              statusFlagList->push_back(statusFlag);
              statusFlag->meaning.copy(flagStrings[j].c_str());
              //statusFlag->meaning.replaceSelf("_"," ");
              statusFlag->value=dfFlagValues[j];
            }
          }else {CDBError("ReadStatusFlags: nrOfFlagMeanings!=nrOfFlagValues, %d!=%d",nrOfFlagMeanings,nrOfFlagValues);}
        }else {CDBError("ReadStatusFlags: flag_meanings: nrOfFlagMeanings = 0");}
        delete[] flagStrings;
      }else {CDBError("ReadStatusFlags: flag_meanings found, but no flag_values attribute found");}
    }
  }
}

const char *CDataSource::getFlagMeaning( std::vector<CDataSource::StatusFlag*> *statusFlagList,double value){
  for(size_t j=0;j<statusFlagList->size();j++){if((*statusFlagList)[j]->value==value){return (*statusFlagList)[j]->meaning.c_str();}}
  return "no_flag_meaning";
}

void CDataSource::getFlagMeaningHumanReadable( CT::string *flagMeaning,std::vector<CDataSource::StatusFlag*> *statusFlagList,double value){
  flagMeaning->copy(getFlagMeaning(statusFlagList,value));
  flagMeaning->replaceSelf("_"," ");
}


int  CDataSource::checkDimTables(CPGSQLDB *dataBaseConnection){
  CCache::Lock lock;
  CT::string identifier = "checkDimTables";  identifier.concat(cfgLayer->FilePath[0]->value.c_str());  identifier.concat("/");  identifier.concat(cfgLayer->FilePath[0]->attr.filter.c_str());  
  CT::string cacheDirectory = srvParams->cfg->TempDir[0]->attr.value.c_str();
  //srvParams->getCacheDirectory(&cacheDirectory);
  if(cacheDirectory.length()>0){
    lock.claim(cacheDirectory.c_str(),identifier.c_str(),srvParams->isAutoResourceEnabled());
  }
  
  #ifdef CDATASOURCE_DEBUG
  CDBDebug("[checkDimTables]");
  #endif
  bool tableNotFound=false;
  bool fileNeedsUpdate = false;
  CT::string dimName;
  for(size_t i=0;i<cfgLayer->Dimension.size();i++){
    dimName=cfgLayer->Dimension[i]->attr.name.c_str();
    
    CT::string tableName;
    try{
      tableName = srvParams->lookupTableName(cfgLayer->FilePath[0]->value.c_str(),cfgLayer->FilePath[0]->attr.filter.c_str(), dimName.c_str());
    }catch(int e){
      CDBError("Unable to create tableName from '%s' '%s' '%s'",cfgLayer->FilePath[0]->value.c_str(),cfgLayer->FilePath[0]->attr.filter.c_str(), dimName.c_str());
      return 1;
    }
    
    CT::string query;
    query.print("select path,filedate,%s from %s limit 1",dimName.c_str(),tableName.c_str());
    CDB::Store *store = dataBaseConnection->queryToStore(query.c_str());
    if(store==NULL){
      tableNotFound=true;
      CDBDebug("No table found for dimension %s",dimName.c_str());
    }
    
    if(tableNotFound == false){
      if(srvParams->isAutoLocalFileResourceEnabled()==true){
        try{
          CT::string databaseTime = store->getRecord(0)->get(1);if(databaseTime.length()<20){databaseTime.concat("Z");}databaseTime.setChar(10,'T');
          
          CT::string fileDate = srvParams->getFileDate(store->getRecord(0)->get(0)->c_str());
          
          
          
          if(databaseTime.equals(fileDate)==false){
            //CDBDebug("Table was found, %s ~ %s : %d",fileDate.c_str(),databaseTime.c_str(),databaseTime.equals(fileDate));
            fileNeedsUpdate = true;
          }
          
        }catch(int e){
          CDBDebug("Unable to get filedate from database, error: %s",CDB::getErrorMessage(e));
          fileNeedsUpdate = true;
        }
        
          
      }
    }
    
    delete store;
    if(tableNotFound||fileNeedsUpdate)break;
  }
  
  
  
  if(fileNeedsUpdate == true){
    if(srvParams->isAutoLocalFileResourceEnabled()==true){
      for(size_t i=0;i<cfgLayer->Dimension.size();i++){
        dimName=cfgLayer->Dimension[i]->attr.name.c_str();
      
        CT::string tableName;
        try{
          tableName = srvParams->lookupTableName(cfgLayer->FilePath[0]->value.c_str(),cfgLayer->FilePath[0]->attr.filter.c_str(), dimName.c_str());
        }catch(int e){
          CDBError("Unable to create tableName from '%s' '%s' '%s'",cfgLayer->FilePath[0]->value.c_str(),cfgLayer->FilePath[0]->attr.filter.c_str(), dimName.c_str());
          return 1;
        }
        CDBFileScanner::markTableDirty(&tableName);
        //CDBDebug("Dropping old table (if exists)",tableName.c_str());
        CT::string query ;
        query.print("drop table %s",tableName.c_str());
        CDBDebug("Try to %s for %s",query.c_str(),dimName.c_str());
        dataBaseConnection->query(query.c_str());
      }
      tableNotFound = true;
    }
  }
 
  
  
  
  if(tableNotFound){
    if(srvParams->isAutoLocalFileResourceEnabled()==true){

      CDBDebug("Updating database");
      int status = CDBFileScanner::updatedb(srvParams->cfg->DataBase[0]->attr.parameters.c_str(),this,NULL,NULL);
      if(status !=0){CDBError("Could not update db for: %s",cfgLayer->Name[0]->value.c_str());return 2;}
    }else{
      CDBDebug("No table found for dimension %s and autoresource is disabled",dimName.c_str());
      return 1;
    }
  }
  #ifdef CDATASOURCE_DEBUG
  CDBDebug("[/checkDimTables]");
  #endif
  lock.release();
  return 0;
}

CT::string CDataSource::getDimensionValueForNameAndStep(const char *dimName,int dimStep){
  return timeSteps[dimStep]->dims.getDimensionValue(dimName);
}
