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
#include "CConvertGeoJSON.h"
const char *CDataSource::className = "CDataSource";

// #define CDATASOURCE_DEBUG
/************************************/
/* CDataSource::CDataObject          */
/************************************/
CDataSource::DataObject::DataObject(){
  hasStatusFlag=false;
  appliedScaleOffset = false;
  hasScaleOffset = false;
  cdfVariable = NULL;
  cdfObject=NULL;
  overruledUnits=NULL;
  dfadd_offset=0;
  dfscale_factor=1;
  std::vector <CPoint> points;
}
CDataSource::DataObject::~DataObject(){
  for(size_t j=0;j<statusFlagList.size();j++){
    delete statusFlagList[j];
  }

}

CDataSource::DataObject* CDataSource::DataObject::clone(){
  CDataSource::DataObject *nd = new CDataSource::DataObject();
  nd->hasStatusFlag=hasStatusFlag;
  nd->hasNodataValue=hasNodataValue;
  nd->appliedScaleOffset=appliedScaleOffset;
  nd->hasScaleOffset=hasScaleOffset;
  nd->dfNodataValue=dfNodataValue;
  nd->dfscale_factor=dfscale_factor;
  nd->dfadd_offset=dfadd_offset;
  nd->cdfVariable=cdfVariable;
  nd->cdfObject=cdfObject;
  nd->overruledUnits=overruledUnits;
  nd->variableName=variableName;
  
  
//   std::vector<StatusFlag*> statusFlagList;
//   std::vector<PointDVWithLatLon> points;
//   std::map<int,CFeature> features;
  return nd;
}

CT::string CDataSource::DataObject::getUnits() {
  if (overruledUnits.empty() && cdfVariable !=NULL ) {
    try{
      return cdfVariable->getAttribute("units")->getDataAsString();
    }catch(int e){}    
  } 
  return overruledUnits;
}

void CDataSource::DataObject::setUnits(CT::string units) {
  overruledUnits=units;
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


double CDataSource::Statistics::getStdDev(){
  return stddev;
}

double CDataSource::Statistics::getAverage(){
  return avg;
}


void CDataSource::Statistics::setMinimum(double min){
  this->min=min;
}
void CDataSource::Statistics::setMaximum(double max){
  this->max=max;
}

MinMax getMinMax(double *data, bool hasFillValue, double fillValue,size_t numElements){
  MinMax minMax;
  bool firstSet = false;
  for(size_t j=0;j<numElements;j++){
    double v=data[j];
    if(v==v){
      if((v!=fillValue)||(!hasFillValue)){
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

MinMax getMinMax(float *data, bool hasFillValue, double fillValue,size_t numElements){
  MinMax minMax;
  bool firstSet = false;
  for(size_t j=0;j<numElements;j++){
    float v=data[j];
    if(v==v){
      if((v!=fillValue)||(!hasFillValue)){
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

MinMax getMinMax(CDF::Variable *var){
  MinMax minMax;
  if(var!=NULL){
      if (var->getType()==CDF_FLOAT) {
   
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
      } else if (var->getType()==CDF_DOUBLE) {
   
	double *data = (double*)var->data;
	
	double scaleFactor=1,addOffset=0,fillValue = 0;
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
      }
    
  }else{
    //CDBError("getMinMax: Variable has not been set");
    throw __LINE__+100;
  }
  return minMax;
}


int CDataSource::Statistics::calculate(CDataSource *dataSource){
  //Get Min and Max
  //CDBDebug("calculate stat ");
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



      
template <class T>
void CDataSource::Statistics::calcMinMax(size_t size,std::vector <DataObject *> *dataObject){
#ifdef MEASURETIME
StopWatch_Stop("Start min/max calculation");
#endif
  if(dataObject->size()==1){
    T* data              = (T*)(*dataObject)[0]->cdfVariable->data;
    CDFType type         =(*dataObject)[0]->cdfVariable->getType();
    double dfNodataValue = (*dataObject)[0]->dfNodataValue;
    bool hasNodataValue  = (*dataObject)[0]->hasNodataValue;
    calculate(size,data,type,dfNodataValue,hasNodataValue);
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
/************************************/
/* CDataSource                      */
/************************************/

CDataSource::CDataSource(){
  stretchMinMax=false;
  stretchMinMaxDone = false;
  isConfigured=false;
  threadNr=-1;
  dimsAreAutoConfigured = false;
//   legendScale=1;
//   legendOffset=0;
//   legendLog=0.0f;
//   legendLowerRange=0;
//   legendUpperRange=0;
//   legendValueRange=0;
  //metaData=NULL;
  statistics=NULL;
  currentAnimationStep=0;
  srvParams=NULL;
  cfgLayer = NULL;
  cfg=NULL;
  datasourceIndex=0;
  formatConverterActive=false;
  useLonTransformation = -1;
  dOrigWidth = -1;
  lonTransformDone = false;
  swapXYDimensions = false;
  varX = NULL;
  varY = NULL;
  
  _styles = NULL;
  _currentStyle = NULL;
  
  queryBBOX = false;
  queryLevel = 0;
  featureSet=NULL;
}

CDataSource::~CDataSource(){
  //if(metaData!=NULL){delete[] metaData;metaData=NULL;}
    
  for(size_t d=0;d<dataObjects.size();d++){
    delete dataObjects[d];
    dataObjects[d]=NULL;
  }

  for(size_t j=0;j<timeSteps.size();j++){

    delete timeSteps[j];
    timeSteps[j]=NULL;
  }
  for(size_t j=0;j<requiredDims.size();j++)delete requiredDims[j];
  if(statistics!=NULL){delete statistics;};statistics=NULL;
  
  if(_styles != NULL){
    delete _styles;
    _styles = NULL;
  }
  
  if (featureSet.length()!=0) {
    CConvertGeoJSON::clearFeatureStore(featureSet);
    featureSet=NULL;
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
  
#ifdef CDATASOURCE_DEBUG  
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
  }else if(cfgLayer->attr.type.equals("baselayer")){
    dLayerType=CConfigReaderLayerTypeBaseLayer;
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
  
  if (!_srvParams->internalAutoResourceLocation.empty()) {
    headerFileName = _srvParams->internalAutoResourceLocation.c_str();
  }
 

  isConfigured=true;
  return 0;
}

void CDataSource::addStep(const char * fileName, CCDFDims *dims){
  //CDBDebug("addStep for %s",fileName);
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

CT::string CDataSource::getDimensionValue(int i){
  return timeSteps[currentAnimationStep]->dims.getDimensionValue(i);
}

int CDataSource::getNumTimeSteps(){
  return (int)timeSteps.size();
}

const char *CDataSource::getLayerName(){
  return layerName.c_str();
}


CCDFDims *CDataSource::getCDFDims(){
  if(currentAnimationStep>=int(timeSteps.size())){
    CDBError("Invalid step asked");
    return NULL;
  }
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




CT::string CDataSource::getDimensionValueForNameAndStep(const char *dimName,int dimStep){
  return timeSteps[dimStep]->dims.getDimensionValue(dimName);
}




/**
* Fills in the styleConfig object based on datasource,stylename, legendname and rendermethod
* 
* @param styleConfig
* 
* 
*/
int CDataSource::makeStyleConfig(CStyleConfiguration *styleConfig,CDataSource *dataSource){//,const char *styleName,const char *legendName,const char *renderMethod){
  //CT::string errorMessage;
  //CT::string renderMethodString = renderMethod;
//   CT::StackList<CT::string> sl = renderMethodString.splitToStack("/");
//   if(sl.size()==2){
//     renderMethodString.copy(&sl[0]);
//     //if(sl[1].equals("HQ")){CDBDebug("32bitmode");}
//   }

//   styleConfig->renderMethod = CDataSource::getRenderMethodFromString(&renderMethodString);
//   if(styleConfig->renderMethod == RM_UNDEFINED){errorMessage.print("rendermethod %s",renderMethod); }
//   styleConfig->styleIndex   = getServerStyleIndexByName(styleName,dataSource->cfg->Style);
//   //if(styleConfig->styleIndex == -1){errorMessage.print("styleIndex %s",styleName); }
//   styleConfig->legendIndex  = getServerLegendIndexByName(legendName,dataSource->cfg->Legend);
//   if(styleConfig->legendIndex == -1){errorMessage.print("legendIndex %s",legendName); }
//   
//   if(errorMessage.length()>0){
//     CDBError("Unable to configure style: %s is invalid",errorMessage.c_str());
//     return -1;
//   }
//   
  //Set defaults
  CStyleConfiguration * s = styleConfig;
  s->shadeInterval=0.0f;
  s->contourIntervalL=0.0f;
  s->contourIntervalH=0.0f;
  s->legendScale = 0.0f;
  s->legendOffset = 0.0f;
  s->legendLog = 0.0f;
  s->legendLowerRange = 0.0f;
  s->legendUpperRange = 0.0f;
  s->smoothingFilter = 0;
  s->hasLegendValueRange = false;
  
  
  float min =0.0f;
  float max=0.0f;
  s->minMaxSet = false;
  
  if(s->styleIndex!=-1){
    //Get info from style
    CServerConfig::XMLE_Style* style = dataSource->cfg->Style[s->styleIndex];
    s->styleConfig = style;
    if(style->Scale.size()>0)s->legendScale=parseFloat(style->Scale[0]->value.c_str());
    if(style->Offset.size()>0)s->legendOffset=parseFloat(style->Offset[0]->value.c_str());
    if(style->Log.size()>0)s->legendLog=parseFloat(style->Log[0]->value.c_str());
    
    if(style->ContourIntervalL.size()>0)s->contourIntervalL=parseFloat(style->ContourIntervalL[0]->value.c_str());
    if(style->ContourIntervalH.size()>0)s->contourIntervalH=parseFloat(style->ContourIntervalH[0]->value.c_str());
    s->shadeInterval=s->contourIntervalL;
    if(style->ShadeInterval.size()>0)s->shadeInterval=parseFloat(style->ShadeInterval[0]->value.c_str());
    if(style->SmoothingFilter.size()>0)s->smoothingFilter=parseInt(style->SmoothingFilter[0]->value.c_str());
    
    if(style->ValueRange.size()>0){
      s->hasLegendValueRange=true;
      s->legendLowerRange=parseFloat(style->ValueRange[0]->attr.min.c_str());
      s->legendUpperRange=parseFloat(style->ValueRange[0]->attr.max.c_str());
    }
    
    
    if(style->Min.size()>0){min=parseFloat(style->Min[0]->value.c_str());s->minMaxSet=true;}
    if(style->Max.size()>0){max=parseFloat(style->Max[0]->value.c_str());s->minMaxSet=true;}
    
    s->contourLines=&style->ContourLine;
    s->shadeIntervals=&style->ShadeInterval;
    s->symbolIntervals=&style->SymbolInterval;
    s->featureIntervals=&style->FeatureInterval;
    
    if(style->Legend.size()>0){
      if(style->Legend[0]->attr.tickinterval.empty()==false){
        styleConfig->legendTickInterval = parseDouble(style->Legend[0]->attr.tickinterval.c_str());
      }
      if(style->Legend[0]->attr.tickround.empty()==false){
        styleConfig->legendTickRound = parseDouble(style->Legend[0]->attr.tickround.c_str());
      }
      if(style->Legend[0]->attr.fixedclasses.equals("true")){
        styleConfig->legendHasFixedMinMax=true;
      }
    }
    
    
  }
  
  //Legend settings can always be overriden in the layer itself!
  CServerConfig::XMLE_Layer* layer = dataSource->cfgLayer;
  if(layer->Scale.size()>0)s->legendScale=parseFloat(layer->Scale[0]->value.c_str());
  if(layer->Offset.size()>0)s->legendOffset=parseFloat(layer->Offset[0]->value.c_str());
  if(layer->Log.size()>0)s->legendLog=parseFloat(layer->Log[0]->value.c_str());
  
  if(layer->ContourIntervalL.size()>0)s->contourIntervalL=parseFloat(layer->ContourIntervalL[0]->value.c_str());
  if(layer->ContourIntervalH.size()>0)s->contourIntervalH=parseFloat(layer->ContourIntervalH[0]->value.c_str());
  if(s->shadeInterval == 0.0f)s->shadeInterval = s->contourIntervalL;
  if(layer->ShadeInterval.size()>0)s->shadeInterval=parseFloat(layer->ShadeInterval[0]->value.c_str());
  if(layer->SmoothingFilter.size()>0)s->smoothingFilter=parseInt(layer->SmoothingFilter[0]->value.c_str());
  
  if(layer->ValueRange.size()>0){
    s->hasLegendValueRange=true;
    s->legendLowerRange=parseFloat(layer->ValueRange[0]->attr.min.c_str());
    s->legendUpperRange=parseFloat(layer->ValueRange[0]->attr.max.c_str());
  }
  
  if(layer->Min.size()>0){min=parseFloat(layer->Min[0]->value.c_str());s->minMaxSet=true;}
  if(layer->Max.size()>0){max=parseFloat(layer->Max[0]->value.c_str());s->minMaxSet=true;}

  if(layer->ContourLine.size()>0){
    s->contourLines=&layer->ContourLine;
  }
  if(layer->ShadeInterval.size()>0){
    s->shadeIntervals=&layer->ShadeInterval;
  }
  if (layer->FeatureInterval.size()>0){
    s->featureIntervals=&layer->FeatureInterval;
  }
  
  if(layer->Legend.size()>0){
    if(layer->Legend[0]->attr.tickinterval.empty()==false){
      styleConfig->legendTickInterval = parseDouble(layer->Legend[0]->attr.tickinterval.c_str());
    }
    if(layer->Legend[0]->attr.tickround.empty()==false){
      styleConfig->legendTickRound = parseDouble(layer->Legend[0]->attr.tickround.c_str());
    }
    if(layer->Legend[0]->attr.fixedclasses.equals("true")){
      styleConfig->legendHasFixedMinMax=true;
    }
  }
  
  //Min and max can again be overriden by WMS extension settings
  if( dataSource->srvParams->wmsExtensions.colorScaleRangeSet){
    s->minMaxSet=true;
    min=dataSource->srvParams->wmsExtensions.colorScaleRangeMin;
    max=dataSource->srvParams->wmsExtensions.colorScaleRangeMax;
  }
  //Log can again be overriden by WMS extension settings
  if(dataSource->srvParams->wmsExtensions.logScale){
    s->legendLog=10;
  }
      
  if(dataSource->srvParams->wmsExtensions.numColorBandsSet){
    float interval = (max-min)/dataSource->srvParams->wmsExtensions.numColorBands;
    s->shadeInterval=interval;
    s->contourIntervalL=interval;
    if(dataSource->srvParams->wmsExtensions.numColorBands>0&&dataSource->srvParams->wmsExtensions.numColorBands<300){
      s->legendTickInterval=int((max-min)/double(dataSource->srvParams->wmsExtensions.numColorBands)+0.5);
      //s->legendTickRound = s->legendTickInterval;//pow(10,(log10(s->legendTickInterval)-1));
      //if(s->legendTickRound > 0.1)s->legendTickRound =0.1;
      // 
    }
  }
        
  
  
  //When min and max are given, calculate the scale and offset according to min and max.
  if(s->minMaxSet){
    #ifdef CDATASOURCE_DEBUG          
    CDBDebug("Found min and max in layer configuration");
    #endif      
    calculateScaleAndOffsetFromMinMax(s->legendScale,s->legendOffset,min,max,s->legendLog);
    
    dataSource->stretchMinMax = false;
    //s->legendScale=240/(max-min);
    //s->legendOffset=min*(-s->legendScale);
  }
    
  //Some safety checks, we cannot create contourlines with negative values.
  /*if(s->contourIntervalL<=0.0f||s->contourIntervalH<=0.0f){
    if(s->renderMethod==contour||
      s->renderMethod==bilinearcontour||
      s->renderMethod==nearestcontour){
      s->renderMethod=nearest;
      }
  }*/
  CT::string styleDump;
  styleConfig->printStyleConfig(&styleDump);
//   #ifdef CDATASOURCE_DEBUG          
//  
//   CDBDebug("styleDump:\n%s",styleDump.c_str());
//   #endif
  return 0;
}


/**
* Returns a stringlist with all available legends for this datasource and chosen style.
* @param dataSource pointer to the datasource 
* @param style pointer to the style to find the legends for
* @return stringlist with the list of available legends.
*/


CT::PointerList<CT::string*> *CDataSource::getLegendListForDataSource(CDataSource *dataSource, CServerConfig::XMLE_Style* style){
  #ifdef CDATASOURCE_DEBUG          
  CDBDebug("getLegendListForDataSource");
  #endif
  if(dataSource->cfgLayer->Legend.size()>0){
    return CServerParams::getLegendNames(dataSource->cfgLayer->Legend);
  }else{
    if(style!=NULL){
      return CServerParams::getLegendNames(style->Legend);
    }
  }
//  CDBError("No legendlist for layer %s",dataSource->layerName.c_str());
  return NULL;
}


/**
* Returns a stringlist with all available rendermethods for this datasource and chosen style.
* @param dataSource pointer to the datasource 
* @param style pointer to the style to find the rendermethods for
* @return stringlist with the list of available rendermethods.
*/
CT::PointerList<CT::string*> *CDataSource::getRenderMethodListForDataSource(CDataSource *dataSource, CServerConfig::XMLE_Style* style){
  //List all the desired rendermethods
  CT::string renderMethodList;
  

  
  //rendermethods defined in the layers must prepend rendermethods defined in the style
  if(dataSource->cfgLayer->RenderMethod.size()>0){
   
    for(size_t j=0;j<dataSource->cfgLayer->RenderMethod.size();j++){
      if(renderMethodList.length()>0)renderMethodList.concat(",");
      renderMethodList.concat(dataSource->cfgLayer->RenderMethod[j]->value.c_str());
    }
  }
  
  if(style!=NULL){
    if(style->RenderMethod.size()>0){
      for(size_t j=0;j<style->RenderMethod.size();j++){
        if(renderMethodList.length()>0)renderMethodList.concat(",");
        renderMethodList.concat(style->RenderMethod[j]->value.c_str());
      }
    }
  }
//   CDBDebug("RendermethodListString = %s",renderMethodList.c_str());
  //If still no list of rendermethods is found, use the default list
  if(renderMethodList.length()==0){
    renderMethodList.copy("nearest");
  }
  
  CT::PointerList<CT::string*> * renderMethods = renderMethodList.splitToPointer(",");
//   if(dataSource!=NULL){
//     if(dataSource->getNumDataObjects()>0){
//       if(dataSource->getDataObject(0)->cdfObject!=NULL){
//         
//         try{
//           if(dataSource->getDataObject(0)->cdfObject->getAttribute("featureType")->toString().equals("timeSeries")||dataSource->getDataObject(0)->cdfObject->getAttribute("featureType")->toString().equals("point")){
//             renderMethods->insert(renderMethods->begin(),1,new CT::string("pointnearest"));
//           }
//         }catch(int e){
//         }
//       }
//     }
//   }
  return  renderMethods;
}

// /**
// * This function calls getStyleListForDataSource in mode (1).
// * 
// * @param dataSource pointer to the datasource to find the stylelist for
// * @return the stringlist with all possible stylenames. Pointer should be deleted with delete!
// */
// CT::PointerList<CT::string*> *CDataSource::getStyleListForDataSource(CDataSource *dataSource){
//   return getStyleListForDataSource(dataSource,NULL);
// }

/**
* Sets a new CStyleConfiguration in datasource which contains all settings for the corresponding styles. This function calls getStyleListForDataSource in mode(2).
* @param styleName
* @param serverCFG
*/
// void CDataSource::getStyleConfigurationByName(const char *styleName,CDataSource *dataSource){
//   //#ifdef CDATASOURCE_DEBUG    
//   CDBDebug("getStyleConfigurationByName for layer %s with name %s",dataSource->layerName.c_str(),styleName);
//   //#endif
//   CStyleConfiguration *styleConfiguration = dataSource->getStyle();
//   if( styleConfiguration != NULL){
//     CDBDebug("styleConfiguration already set");
//     return;
//   }
//   //CServerConfig::XMLE_Configuration *serverCFG = dataSource->cfg;
//   
//   
// //   styleConfiguration->reset();
// //   styleConfiguration->styleCompositionName=styleName;
//   CT::PointerList<CStyleConfiguration*> *styles = getStyleListForDataSource(dataSource);
//   CDBDebug("Found %d styles",styles->size());
//   styleConfiguration = styles->get(0);
//   for(size_t j=0;j<styles->size();j++){
//     if(styles->get(j)->styleCompositionName.equals(styleName)){
//        dataSource->setStyle(styles->get(j));
//        break;
//     }
//       //CDBDebug("getStyleConfigurationByName: %s == %s",styleName,styles->get(j)->styleCompositionName.c_str());
//   }
//   
//   delete styles;
// }



/**
* This function has two modes, return a string list (1) or (2) configure a CStyleConfiguration object.
*   (1) Returns a stringlist with all possible style names for a datasource, when styleConfig is set to NULL.
*   (2) When a styleConfig is provided, this function fills in the provided CStyleConfiguration object, 
*   the styleCompositionName needs to be set in advance (The stylename usually given in the request string)
* 
* @param dataSource pointer to the datasource to find the stylelist for
* @param styleConfig pointer to the CStyleConfiguration object to be filled in. 
* @return the stringlist with all possible stylenames
*/
CT::PointerList<CStyleConfiguration*> *CDataSource::getStyleListForDataSource(CDataSource *dataSource){
  
  #ifdef CDATASOURCE_DEBUG      
  CDBDebug("getStyleListForDataSource %s",dataSource->layerName.c_str());
#endif
  CT::PointerList<CStyleConfiguration*> *styleConfigurationList = new CT::PointerList<CStyleConfiguration*>();
  
  CServerConfig::XMLE_Configuration *serverCFG = dataSource->cfg;

  CT::PointerList<CT::string*> *renderMethods = NULL;
  CT::PointerList<CT::string*> *legendList = NULL;
  
  //Auto configure styles, if no legends or styles are defined
  if(dataSource->cfgLayer->Styles.size()==0&&dataSource->cfgLayer->Legend.size()==0){
    
    renderMethods = getRenderMethodListForDataSource(dataSource,NULL);
    if(renderMethods->size()>0){
      //For cascaded and rgba layers, no styles need to be defined
//       if((CStyleConfiguration::getRenderMethodFromString(renderMethods->get(0))&(RM_RGBA)){
//         CDBDebug("Using rendermethod %s",renderMethods->get(0)->c_str());
//         delete renderMethods ; 
//         
//         CStyleConfiguration * styleConfig = new CStyleConfiguration();
//         //CDBDebug("Setting rendermethod RM_RGBA");
//         styleConfig->styleTitle.copy("rgba");
//         styleConfig->styleAbstract.copy("rgba");
//         styleConfig->renderMethod = RM_RGBA;
//         styleConfig->styleCompositionName = "rgba";
//         styleConfigurationList->push_back(styleConfig);
//         return styleConfigurationList;
//         
//       }
      CAutoConfigure::autoConfigureStyles(dataSource);
    }
   
  }
  
  delete renderMethods ;  renderMethods  = NULL;
    
  CT::PointerList<CT::string*> *styleNames = getStyleNames(dataSource->cfgLayer->Styles);

  //We always skip the style "default" if there are more styles.
  size_t start=0;if(styleNames->size()>1)start=1;
  

  //Loop over the styles.
  try{
    //CDBDebug("There are %d styles to check",styleNames->size());
    for(size_t i=start;i<styleNames->size();i++){
      
      //Lookup the style index in the servers configuration
      int dStyleIndex=getServerStyleIndexByName(styleNames->get(i)->c_str(),serverCFG->Style);
      
      if(dStyleIndex==-1){
        
//         if(returnStringList){
//           
//           if(styleNames->get(i)->equals("default")==false){
//             CDBError("Style %s not found for layer %s",styleNames->get(i)->c_str(),dataSource->layerName.c_str());
//             delete styleNames;styleNames = NULL;
//             
//             return styleConfigurationList;
//           }else{
//            CDBDebug("Warning: Style [%s] not found (%d)",styleNames->get(i)->c_str(),styleNames->get(i)->equals("default"));
//             CT::string * styleName = new CT::string();
//             styleName->copy("default");
//            
//             styleConfigurationList->push_back(styleName);
//             delete styleNames;styleNames = NULL;
//             return styleConfigurationList;
//            continue;
//           }
//         }
      }
      
      
      
#ifdef CDATASOURCE_DEBUG      
      CDBDebug("dStyleIndex = %d",dStyleIndex);
#endif
      //TODO CHECK, why did we add this line?:      
      if(dStyleIndex!=-1)
      {
        
        CServerConfig::XMLE_Style* style = NULL;
        if(dStyleIndex!=-1)style=serverCFG->Style[dStyleIndex];
        
        
        
        renderMethods = getRenderMethodListForDataSource(dataSource,style);
        legendList = getLegendListForDataSource(dataSource,style);
        
        if(legendList==NULL){
          CDBError("No legends defined for layer %s",dataSource->layerName.c_str());
          delete styleNames;styleNames = NULL;
          
          delete renderMethods;renderMethods= NULL;
          ////if(styleConfig!=NULL){styleConfig->hasError=true;}
          return NULL;
        }
        
  //       if(legendList->size()==0){
  //         CDBError("Zero legends defined for layer %s",dataSource->layerName.c_str());
  //         delete styleNames;styleNames = NULL;
  //         delete styleConfigurationList;styleConfigurationList = NULL;
  //         delete renderMethods;renderMethods= NULL;
  //         if(styleConfig!=NULL){styleConfig->hasError=true;}
  //         return NULL;
  //       }if(renderMethods->size()==0){
  //         CDBError("Zero renderMethods defined for layer %s",dataSource->layerName.c_str());
  //         delete styleNames;styleNames = NULL;
  //         delete styleConfigurationList;styleConfigurationList = NULL;
  //         delete renderMethods;renderMethods= NULL;
  //         if(styleConfig!=NULL){styleConfig->hasError=true;}
  //         return NULL;
  //       }
      
   
        CT::string styleName;
        for(size_t l=0;l<legendList->size();l++){
          for(size_t r=0;r<renderMethods->size();r++){
            if(renderMethods->get(r)->length()>0){
              int dLegendIndex = getServerLegendIndexByName(legendList->get(l)->c_str(),dataSource->cfg->Legend);
              if(legendList->size()>1){
                styleName.print("%s_%s/%s",styleNames->get(i)->c_str(),legendList->get(l)->c_str(),renderMethods->get(r)->c_str());
              }else{
                styleName.print("%s/%s",styleNames->get(i)->c_str(),renderMethods->get(r)->c_str());
              }
              

              //CStyleConfiguration mode, try to find which stylename we want our CStyleConfiguration for.
            
                #ifdef CDATASOURCE_DEBUG    
              //CDBDebug("Matching '%s' == '%s'",styleName->c_str(),styleToSearchString.c_str());
                #endif
                
                //CDBDebug("Matching '%s' == '%s'",styleName.c_str(),styleToSearchString.c_str());
                
               // if(styleToSearchString.equals(&styleName)||isDefaultStyle==true){
                  CStyleConfiguration * styleConfig = new CStyleConfiguration();
                  styleConfigurationList->push_back(styleConfig);
                  styleConfig->styleCompositionName = styleName.c_str();
                  styleConfig->styleTitle = styleName.c_str();
                  //CDBDebug("FOUND");
                  // We found the correspondign legend/style and rendermethod corresponding with the requested stylename!
                  // Now fill in the CStyleConfiguration Object.
                  
                  styleConfig->renderMethod = CStyleConfiguration::getRenderMethodFromString(renderMethods->get(r));
                  styleConfig->styleIndex   = dStyleIndex;

                  styleConfig->legendIndex  = dLegendIndex;
                  
                  if(dLegendIndex == -1){
                    CDBError("Legend %s not found",legendList->get(l)->c_str());
                  }
                  
                  
                  if(style!=NULL){
                    
                    for(size_t j=0;j<style->NameMapping.size();j++){
                      
                      if(renderMethods->get(r)->equals(style->NameMapping[j]->attr.name.c_str())){
                        
                        
                        styleConfig->styleTitle.copy(style->NameMapping[j]->attr.title.c_str());
                        styleConfig->styleAbstract.copy(style->NameMapping[j]->attr.abstract.c_str());
                        break;
                      }
                    }
                  }
#ifdef CDATASOURCE_DEBUG
                  CDBDebug("Pushing %s with legendIndex %d and styleIndex %d",styleName.c_str(),dLegendIndex,dStyleIndex);
#endif
                  int status = makeStyleConfig(styleConfig,dataSource);//,styleNames->get(i)->c_str(),legendList->get(l)->c_str(),renderMethods->get(r)->c_str());
                  
                  if(status == -1){
                    styleConfig->hasError=true;
                  }
                  //Stop with iterating:
                 // throw(__LINE__);
                //}
                
             
              
              //if(returnStringList){
              
                
              //}
            }
          }
        }
      }
      
      delete legendList;legendList =NULL;
      delete renderMethods;renderMethods = NULL;
    }
    delete styleNames; styleNames = NULL;
    
    // We have been through the loop, but the styleConfig has not been created. This is an error.
//     if(styleConfig!=NULL){
//       CDBError("Unable to find style %s",styleToSearchString.c_str());
//       styleConfig->hasError=true;
//     }
  }catch(int e){

    delete legendList;
    delete renderMethods;
    delete styleNames;
    
  }
  
  if(styleConfigurationList->size()==0){
    CStyleConfiguration * styleConfig = new CStyleConfiguration();
    //CDBDebug("Setting rendermethod RM_NEAREST");
    styleConfig->styleTitle.copy("default");
    styleConfig->styleAbstract.copy("default");
    styleConfig->renderMethod = RM_NEAREST;
    styleConfig->styleCompositionName = "default";
    styleConfigurationList->push_back(styleConfig);
  }
        #ifdef CDATASOURCE_DEBUG      
  CDBDebug("/getStyleListForDataSource");
#endif

  return styleConfigurationList;
}


/**
* Returns a stringlist with all possible legends available for this Legend config object.
* This is usually a configured legend element in a layer, or a configured legend element in a style.
* @param Legend a XMLE_Legend object configured in a style or in a layer
* @return Pointer to a new stringlist with all possible legend names, must be deleted with delete. Is NULL on failure.
*/
/*
CT::PointerList<CT::string*> *CDataSource::getLegendNames(std::vector <CServerConfig::XMLE_Legend*> Legend){
  if(Legend.size()==0){CDBError("No legends defined");return NULL;}
  CT::PointerList<CT::string*> *stringList = new CT::PointerList<CT::string*>();
  
  for(size_t j=0;j<Legend.size();j++){
    CT::string legendValue=Legend[j]->value.c_str();
    CT::StackList<CT::string> l1=legendValue.splitToStack(",");
    for(size_t i=0;i<l1.size();i++){
      if(l1[i].length()>0){
        CT::string * val = new CT::string();
        stringList->push_back(val);
        val->copy(&l1[i]);
      }
    }
  }
  return stringList;
}
*/

/**
* Returns a stringlist with all possible styles available for this style config object.
* @param Style a pointer to XMLE_Style vector configured in a layer
* @return Pointer to a new stringlist with all possible style names, must be deleted with delete. Is NULL on failure.
*/
CT::PointerList<CT::string*> *CDataSource::getStyleNames(std::vector <CServerConfig::XMLE_Styles*> Styles){
  #ifdef CDATASOURCE_DEBUG          
  CDBDebug("getStyleNames");
  #endif
  CT::PointerList<CT::string*> *stringList = new CT::PointerList<CT::string*>();
  CT::string * val = new CT::string();
  stringList->push_back(val);
  val->copy("default");
  for(size_t j=0;j<Styles.size();j++){
    if(Styles[j]->value.empty()==false){
      CT::string StyleValue=Styles[j]->value.c_str();
      if(StyleValue.length()>0){
        CT::StackList<CT::string>  l1=StyleValue.splitToStack(",");
        for(size_t i=0;i<l1.size();i++){
          if(l1[i].length()>0){
            CT::string * val = new CT::string();
            stringList->push_back(val);
            val->copy(&l1[i]);
          }
        }
      }
    }
  }
  #ifdef CDATASOURCE_DEBUG          
  CDBDebug("/getStyleNames");
  #endif
  return stringList;
}



/**
* Retrieves the position of for the requested style name in the servers configured style elements.
* @param styleName The name of the style to locate
* @param serverStyles Pointer to the servers configured styles.
* @return The style index as integer, points to the position in the servers configured styles. Is -1 on failure.
*/
int  CDataSource::getServerStyleIndexByName(const char * styleName,std::vector <CServerConfig::XMLE_Style*> serverStyles){
  #ifdef CDATASOURCE_DEBUG          
  CDBDebug("getServerStyleIndexByName");
  #endif
  if(styleName==NULL){
    CDBError("No style name provided");
    return -1;
  }
  CT::string styleString = styleName;
  if(styleString.equals("default")||styleString.equals("default/HQ"))return -1;
  for(size_t j=0;j<serverStyles.size();j++){
    if(serverStyles[j]->attr.name.empty()==false){
      if(styleString.equals(serverStyles[j]->attr.name.c_str())){
  #ifdef CDATASOURCE_DEBUG          
  CDBDebug("/getServerStyleIndexByName");
  #endif

        return j;
      }
    }
  }
  CDBError("No style found with name [%s]",styleName);
  return -1;
}

/**
* Retrieves the position of for the requested legend name in the servers configured legend elements.
* @param legendName The name of the legend to locate
* @param serverLegends Pointer to the servers configured legends.
* @return The legend index as integer, points to the position in the servers configured legends. Is -1 on failure.
*/
int  CDataSource::getServerLegendIndexByName(const char * legendName,std::vector <CServerConfig::XMLE_Legend*> serverLegends){
  int dLegendIndex=-1;
  CT::string legendString = legendName;
  if(legendName==NULL)return -1;
  for(size_t j=0;j<serverLegends.size()&&dLegendIndex==-1;j++){
    if(legendString.equals(serverLegends[j]->attr.name.c_str())){
      dLegendIndex=j;
      break;
    }
  }
  return dLegendIndex;
}


/**
* 
*/
void CDataSource::calculateScaleAndOffsetFromMinMax(float &scale, float &offset,float min,float max,float log){
  if(log!=0.0f){
    //CDBDebug("LOG = %f",log);
    min=log10(min)/log10(log);
    max=log10(max)/log10(log);
  }
    
  scale=240/(max-min);
  offset=min*(-scale);
}

CStyleConfiguration *CDataSource::getStyle(){
  if(_currentStyle == NULL){
    if(_styles == NULL){
      _styles = getStyleListForDataSource(this);
    }
    if(_styles->size() == 0){
      CDBError("There are no styles available");
      return NULL;
    }        
    CT::string styleName="default";
    CT::string styles(srvParams->Styles.c_str());

    //TODO CHECK CDBDebug("Server Styles=%s",srvParam->Styles.c_str());
    CT::StackList<CT::string> layerstyles = styles.splitToStack(",");
    int layerIndex=datasourceIndex;
    if(layerstyles.size()!=0){
      //Make sure default layer index is within the right bounds.
      if(layerIndex<0)layerIndex=0;
      if(layerIndex>((int)layerstyles.size())-1)layerIndex=layerstyles.size()-1;
      styleName=layerstyles[layerIndex].c_str();
      if(styleName.length()==0){
        styleName.copy("default");
      }
    }

    _currentStyle = _styles->get(0);
    
    for(size_t j=0;j<_styles->size();j++){
      if(_styles->get(j)->styleCompositionName.equals(styleName)){
          _currentStyle=_styles->get(j);
          break;
      }
    }
    
    if(_currentStyle->styleIndex == -1){
      int status = makeStyleConfig(_currentStyle,this);
      if(status == -1){
        _currentStyle->hasError=true;
      }
    }
    if(_currentStyle->legendIndex == -1){
      CT::PointerList<CT::string*> *legendList = getLegendListForDataSource(this,NULL);
      if(legendList!=NULL){
        _currentStyle->legendIndex = getServerLegendIndexByName(legendList->get(0)->c_str(),this->cfg->Legend);
      }
      delete legendList;
    }
  }
  
  return _currentStyle;
}



int CDataSource::setStyle(const char *styleName){
  if(_styles == NULL){
    _styles = getStyleListForDataSource(this);
  }
  if(_styles->size() == 0){
    CDBError("There are no styles available");
    return 1;
  }
  
  _currentStyle = _styles->get(0);
  bool foundStyle = false;
  for(size_t j=0;j<_styles->size();j++){
    if(_styles->get(j)->styleCompositionName.equals(styleName)){
      
        _currentStyle=_styles->get(j);
        foundStyle = true;
        break;
    }
  }
  
  if(foundStyle == false){
    CDBWarning("Unable to find style %s. Available styles:",styleName);
    for(size_t j=0;j<_styles->size();j++){
        CDBWarning("  -%s",_styles->get(j)->styleCompositionName.c_str());
    }
  }
  
  if(_currentStyle->styleIndex == -1){
    int status = makeStyleConfig(_currentStyle,this);//,styleNames->get(i)->c_str(),legendList->get(l)->c_str(),renderMethods->get(r)->c_str());
    if(status == -1){
      _currentStyle->hasError=true;
    }
  }
  if(_currentStyle->legendIndex == -1){
    CT::PointerList<CT::string*> *legendList = getLegendListForDataSource(this,NULL);
    if(legendList!=NULL){
      _currentStyle->legendIndex = getServerLegendIndexByName(legendList->get(0)->c_str(),this->cfg->Legend);
    }
    delete legendList;
  }
  if(_currentStyle->hasError)return 1;
  return 0;
};


   /*
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
  
  if(_styles != NULL){
    delete _styles;
    _styles = NULL;
  }
  
  if (featureSet.length()!=0) {
    CConvertGeoJSON::clearFeatureStore(featureSet);
    featureSet=NULL;
  }*/

CDataSource *CDataSource::clone(){
 
  CDataSource *d = new CDataSource();
  d->_currentStyle = _currentStyle;
  d->datasourceIndex=datasourceIndex;
  d->currentAnimationStep = currentAnimationStep;
  
  
  /* Copy timesteps */
  for(size_t j=0;j<timeSteps.size();j++){
     //CDBDebug("addStep for %s",fileName);
    TimeStep * timeStep = new TimeStep();
    d->timeSteps.push_back(timeStep);
    timeStep->fileName.copy(timeSteps[j]->fileName.c_str());
    timeStep->dims.copy(&timeSteps[j]->dims);
  }

  
  /* Copy dataObjects */
  for(size_t j=0;j<dataObjects.size();j++){
    d->dataObjects.push_back(dataObjects[j]->clone());
  }
    
  d->stretchMinMax=stretchMinMax;
  d->stretchMinMaxDone=stretchMinMaxDone;
  
  /* Copy requireddims */
  for(size_t j=0;j<requiredDims.size();j++){
    COGCDims *ogcDim = new COGCDims();
    d->requiredDims.push_back(ogcDim);
    ogcDim->name = requiredDims[j]->name;
    ogcDim->value = requiredDims[j]->value;
    ogcDim->netCDFDimName = requiredDims[j]->netCDFDimName;
    for(size_t i=0;i<requiredDims[j]->uniqueValues.size();i++){
      ogcDim->uniqueValues.push_back(requiredDims[j]->uniqueValues[i].c_str());
    }
    ogcDim->isATimeDimension = requiredDims[j]->isATimeDimension;
  }

  for(size_t j=0;j<4;j++){
    d->dfBBOX[j]=dfBBOX[j];
    d->nativeViewPortBBOX[j]=nativeViewPortBBOX[j];
  }
  
  d->dfCellSizeX = dfCellSizeX;
  d->dfCellSizeY = dfCellSizeY;
  d->dWidth = dWidth;
  d->dHeight = dHeight;
  d->nativeEPSG=nativeEPSG;
  d->nativeProj4=nativeProj4;
  d->isConfigured=isConfigured;
  d->formatConverterActive=formatConverterActive;
  d->dimXIndex = dimXIndex;
  d->dimYIndex = dimYIndex;
  d->stride2DMap = stride2DMap;
  d->useLonTransformation = useLonTransformation;
  d->origBBOXLeft = origBBOXLeft;
  d->origBBOXRight = origBBOXRight;
  d->dOrigWidth = dOrigWidth;
  d->lonTransformDone = lonTransformDone;
  d->swapXYDimensions = swapXYDimensions;
  d->varX = varX;
  d->varY = varY;
  d->dNetCDFNumDims = dNetCDFNumDims;
  d->dLayerType = dLayerType;
  d->layerName = layerName;
  d->queryBBOX = queryBBOX;
  d->queryLevel = queryLevel;
  d->srvParams = srvParams;
  d->cfgLayer = cfgLayer;
  d->cfg = cfg;
  d->featureSet = featureSet;
  
  
  

  return d;
}
