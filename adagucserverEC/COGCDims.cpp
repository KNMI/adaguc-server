#include "COGCDims.h"
const char * CCDFDims::className = "CCDFDims";

void COGCDims::addValue(const char *value){
  for(size_t j=0;j<uniqueValues.size();j++){
    if(uniqueValues[j].equals(value))return;
  }
  uniqueValues.push_back(value);
  
}

CCDFDims::~CCDFDims (){
  for(size_t j=0;j<dimensions.size();j++){
    delete dimensions[j];
    dimensions[j]=NULL;
  }
}

void CCDFDims::addDimension(const char *name,const char *value,size_t index){
  //CDBDebug("Adddimension %s %s %d",name,value,index);
  for(size_t j=0;j<dimensions.size();j++){
    if(dimensions[j]->name.equals(name)){
      dimensions[j]->index=index;
      dimensions[j]->value=value;
      return;
    }
  }
  NetCDFDim *dim = new NetCDFDim;
  dim->name.copy(name);
  dim->value.copy(value);
  dim->index=index;
  dimensions.push_back(dim);
}
size_t CCDFDims::getDimensionIndex(const char *name){
  for(size_t j=0;j<dimensions.size();j++){
    if(dimensions[j]->name.equals(name)){
      return dimensions[j]->index;
    }
  }
  return 0;
}
size_t CCDFDims::getDimensionIndex(int j){
  if(j<0)return 0;
  if(size_t(j)>dimensions.size())return 0;
  return dimensions[j]->index;
}
const char *CCDFDims::getDimensionValue(int j){
  if(j<0)return 0;
  if(size_t(j)>dimensions.size())return 0;
  return dimensions[j]->value.c_str();
}
const char *CCDFDims::getDimensionName(int j){
  if(j<0)return 0;
  if(size_t(j)>dimensions.size())return 0;
  return dimensions[j]->name.c_str();
}