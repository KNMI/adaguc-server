/******************************************************************************
 * 
 * Project:  Generic common data format
 * Purpose:  Generic Data model to read netcdf and hdf5
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

#include "CCDFNetCDFIO.h"

const char *CDFNetCDFReader::className="NetCDFReader";
const char *CDFNetCDFWriter::className="NetCDFWriter";
const char *CCDFWarper::className="CCDFWarper";


#define CCDFNETCDFIO_DEBUG_OPEN

CDFNetCDFReader::CDFNetCDFReader():CDFReader(){
  #ifdef CCDFNETCDFIO_DEBUG        
  CDBDebug("New CDFNetCDFReader");
  #endif
  root_id=-1;
    keepFileOpen=false;
    
}
CDFNetCDFReader::~CDFNetCDFReader(){
  close();
}

void CDFNetCDFReader::enableLonWarp(bool enableLonWarp){
  warper.enableLonWarp=enableLonWarp;
}
void CDFNetCDFReader::ncError(int line, const char *className, const char * msg,int e){
  if(e==NC_NOERR)return;
  char szTemp[1024];
  snprintf(szTemp,1023,"[E: %s, %d in class %s] %s: %s\n",__FILE__,line,className,msg,nc_strerror(e));
  printErrorStream(szTemp);
  
}

int CDFNetCDFReader::_readVariableData(CDF::Variable *var, CDFType type){
  return _readVariableData(var,type,NULL,NULL,NULL);
}

int CDFNetCDFReader::_readVariableData(CDF::Variable *var, CDFType type,size_t *start,size_t *count,ptrdiff_t *stride){

  if(var->dimensionlinks.size()==0){
    CDBError("No dimensions specified for variable %s",var->name.c_str());
    return 1;
  }
  
  if(cdfCache!=NULL){
    //CDBDebug("Looking into cache %s of type %s",var->name.c_str(),CDF::getCDFDataTypeName(type).c_str());
    int cacheStatus = cdfCache->readVariableData(var, type,start,count,stride,false);
    if(cacheStatus == 0) {return 0;}
  }
    
  if(root_id==-1){
    #ifdef CCDFNETCDFIO_DEBUG_OPEN        
    CDBDebug("NC_OPEN re-opening %s for %s",fileName.c_str(),var->name.c_str());
    #endif      
    
    status = nc_open(fileName.c_str(),NC_NOWRITE,&root_id);
    if(status!=NC_NOERR){ncError(__LINE__,className,"nc_open: ",status);return 1;}
  }
  #ifdef CCDFNETCDFIO_DEBUG        
  CDBDebug("reading %s from file %s",var->name.c_str(),fileName.c_str());
  #endif
  //CDBDebug("readVariableData");
  //It is essential that the variable nows which reader can be used to read the data
  //var->cdfReaderPointer=(void*)this;
  var->freeData();
  
 
  
  
  bool useStartCount = false;
  bool useStriding=false;
  
  if(start!=NULL&&count!=NULL&&stride!=NULL){
    useStartCount = true;
  }
  
  size_t totalVariableSize = 1;
  
  if(useStartCount == false){
    for(size_t i=0;i<var->dimensionlinks.size();i++){
      totalVariableSize*=var->dimensionlinks[i]->length;
    }
  }
  
  if(useStartCount == true){
    for(size_t i=0;i<var->dimensionlinks.size();i++){
      totalVariableSize*=count[i];//stride[i];
      if(stride[i]!=1){
        useStriding=true;
      }
      #ifdef CCDFNETCDFIO_DEBUG        
      CDBDebug("%s: [%d %d %d]",var->dimensionlinks[i]->name.c_str(),start[i],count[i],stride[i]);
      #endif
    }
  }
  //CDBDebug("totalVariableSize = %d",totalVariableSize);
  
  #ifdef CCDFNETCDFIO_DEBUG        
  CDBDebug("Setting variable size to %d",totalVariableSize);
  #endif
  var->setSize(totalVariableSize);
  
  #ifdef CCDFNETCDFIO_DEBUG        
  CDBDebug("Allocating data for variable %s, type: %s, size: %d",var->name.c_str(),CDF::getCDFDataTypeName(var->currentType).c_str(),var->getSize());
  #endif
  CDF::allocateData(type,&var->data,var->getSize());
  
   
  if(type==CDF_STRING){
    status = nc_get_var_string(root_id,var->id,(char**)var->data);
    if(status!=NC_NOERR){
      char typeName[254];
      CDF::getCDFDataTypeName(typeName,255,var->currentType);
      CDBError("Problem with variable %s of type %s (requested %s):",var->name.c_str(),typeName,CDF::getCDFDataTypeName(type).c_str());
      ncError(__LINE__,className,"nc_get_var: ",status);
      return 1;
    }
    
    if(cdfCache!=NULL){
      CDBDebug("Putting into cache %s",var->name.c_str());
      int cacheStatus = cdfCache->readVariableData(var, type,start,count,stride,true);
      if(cacheStatus == 0) return 0;
    }  
    return 0;
  }
  
  //Data is requested with another type than requested. We will perform type conversion in the following piece of code.
  if(type!=var->nativeType){
    void * voidData = NULL;
    #ifdef CCDFNETCDFIO_DEBUG        
    CDBDebug("Allocating data for temp data");
    #endif
    
    CDF::allocateData(var->nativeType,&voidData,var->getSize());
    var->setType(type);  
    
    #ifdef CCDFNETCDFIO_DEBUG        
    CDBDebug("Allocated %d elements",var->getSize());
    #endif
    
    
    if(useStartCount == true){
      if(useStriding){
        status = nc_get_vars(root_id,var->id,start,count,stride,voidData);
        if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_vars (typeconversion): ",status);}
      }else{
        status = nc_get_vara(root_id,var->id,start,count,voidData);
        if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_vara (typeconversion): ",status);}
      }
    }else{
      status = nc_get_var(root_id,var->id,voidData);
      if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_var (typeconversion): ",status);}
    }
    
    if(status!=NC_NOERR){
      char typeName[254];
      CDF::getCDFDataTypeName(typeName,255,var->currentType);
      CDBError("Problem with variable %s of type %s (requested %s):",var->name.c_str(),typeName,CDF::getCDFDataTypeName(type).c_str());
      ncError(__LINE__,className,"nc_get_var: ",status);
      return 1;
    }

    #ifdef CCDFNETCDFIO_DEBUG        
    CDBDebug("Copying %d elements from type %s to %s",var->getSize(),CDF::getCDFDataTypeName(var->nativeType).c_str(),CDF::getCDFDataTypeName(type).c_str());
    #endif
    
    
    CDF::DataCopier::copy(var->data,type,voidData,var->nativeType,0,0,var->getSize());
    
    #ifdef CCDFNETCDFIO_DEBUG        
    CDBDebug("Freeing temporary data object");
    #endif
    CDF::freeData(&voidData);
    
    //End of reading data and performing type conversion
  }
  
  if(type == var->nativeType){
     if(useStartCount){
       if(useStriding){
        status = nc_get_vars(root_id,var->id,start,count,stride,var->data);
        if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_vars (native): ",status);}
      }else{
        status = nc_get_vara(root_id,var->id,start,count,var->data);
        if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_vara (native): ",status);}
      }
    }else{
       status = nc_get_var(root_id,var->id,var->data);
      if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_var (native): ",status);}
    }
    if(status!=NC_NOERR){
      char typeName[254];
      CDF::getCDFDataTypeName(typeName,255,var->currentType);
      CDBError("Problem with variable %s of type %s (requested %s):",var->name.c_str(),typeName,CDF::getCDFDataTypeName(type).c_str());
      ncError(__LINE__,className,"nc_get_var: ",status);
      return 1;
    }
    //End of reading data natively.
  }
  
  
  
  
  warper.warpLonData(var);

  if(cdfCache!=NULL){
    #ifdef CCDFNETCDFIO_DEBUG        
    CDBDebug("Putting into cache %s",var->name.c_str());
    #endif
    int cacheStatus = cdfCache->readVariableData(var, type,start,count,stride,true);
    if(cacheStatus == 0) return 0;
  }  
  
  #ifdef CCDFNETCDFIO_DEBUG        
  CDBDebug("Ready.");
  #endif
  return 0;
}


int CDFNetCDFReader::readDimensions(){
  char name[NC_MAX_NAME+1];
  size_t length;
  for(int j=0;j<nDims;j++){
    status = nc_inq_dim(root_id,j,name,&length);
    if(status!=NC_NOERR){CDBError("For %s: ",name);ncError(__LINE__,className,"nc_inq_dim: ",status);return 1;}
    try{
      CDF::Dimension * existingDim = cdfObject->getDimension(name);
    //Only add non existing variables;
      if(existingDim->length!=length){
        CDBError("Previously dimensions size for dim %s is not the same as new definition",name);
        return 1;
      }
    }catch(...){
      CDF::Dimension * dim = new CDF::Dimension();
      dim->id=j;
      dim->setName(name);
      dim->length=length;
      cdfObject->dimensions.push_back(dim);
    }
  }
  return 0;
}

int CDFNetCDFReader::readAttributes(std::vector<CDF::Attribute *> &attributes,int varID,int natt){
  char name[NC_MAX_NAME+1];
  nc_type type;
  size_t length;
  for(int i=0;i<natt;i++){
    status = nc_inq_attname(root_id,varID,i,name);
    if(status!=NC_NOERR){ncError(__LINE__,className,"nc_inq_attname: ",status);return 1;}
    //Only add non existing attributes;
    int attributeExists=false;
    for(size_t k=0;k<attributes.size();k++){if(attributes[k]->name.equals(name)){attributeExists=true;break;}}
    if(attributeExists==false){
      status = nc_inq_att(root_id,varID,name,&type,&length);
      if(status!=NC_NOERR){ncError(__LINE__,className,"nc_inq_att: ",status);return 1;}
      CDF::Attribute *attr = new CDF::Attribute();
      attr->setName(name);
      attr->type=typeConversion(type);
      attr->length=length;
      CDF::allocateData(attr->getType(),&attr->data,attr->length+1);
      status = nc_get_att(root_id,varID,name,attr->data);
      if(type==NC_CHAR)((char*)attr->data)[attr->length]='\0';
      if(status!=NC_NOERR){ncError(__LINE__,className,"nc_get_att: ",status);return 1;}
      attributes.push_back(attr);
    }
  }
  return 0;
}

int CDFNetCDFReader::readVariables(){
  char name[NC_MAX_NAME+1];
  nc_type type;
  int ndims;
  int natt;
  int dimids[NC_MAX_VAR_DIMS];
  bool isDimension;
  for(int j=0;j<nVars;j++){
    status = nc_inq_var(root_id,j,name,&type,&ndims,dimids,&natt);
    if(status!=NC_NOERR){ncError(__LINE__,className,"nc_inq_var: ",status);return 1;}
    //Only add non existing variables...
    //CDF::Variable* existingVariable = NULL;
    try{
      //existingVariable=
      cdfObject->getVariable(name);
    }catch(...){
      //printf("%s\n",name);
      CDF::Variable * var = new CDF::Variable();
      isDimension = false;
        //Is this a dimension:
      for(size_t i=0;i<cdfObject->dimensions.size();i++){if(cdfObject->dimensions[i]->name.equals(name)){isDimension=true;break;}}
        //Dimension links:
      for(int k=0;k<ndims;k++){
        for(size_t i=0;i<cdfObject->dimensions.size();i++){
          if(cdfObject->dimensions[i]->id==dimids[k]){
            var->dimensionlinks.push_back(cdfObject->dimensions[i]);
            break;
          }
        }
      }
      var->setType(typeConversion(type));
      var->nativeType=typeConversion(type);
      var->setName(name);
      var->id=j;
      var->setParentCDFObject(cdfObject);
      var->isDimension=isDimension;
        //Attributes:
      status = readAttributes(var->attributes,j,natt);if(status!=0)return 1;
      cdfObject->variables.push_back(var);
        //printf("%d: %s %d\n",j,var->name.c_str(),isDimension);
      //It is essential that the variable nows which reader can be used to read the data
      var->setCDFReaderPointer((void*)this);
    }
  }
  return 0;
}

CDFType CDFNetCDFReader::typeConversion(nc_type type){
  if(type==NC_BYTE)return CDF_BYTE;
  if(type==NC_UBYTE)return CDF_UBYTE;
  if(type==NC_CHAR)return CDF_CHAR;
  if(type==NC_SHORT)return CDF_SHORT;
  if(type==NC_USHORT)return CDF_USHORT;
  if(type==NC_INT)return CDF_INT;
  if(type==NC_UINT)return CDF_UINT;
  if(type==NC_FLOAT)return CDF_FLOAT;
  if(type==NC_DOUBLE)return CDF_DOUBLE;
  if(type==NC_STRING)return CDF_STRING;
  return CDF_DOUBLE;
}

int CDFNetCDFReader::open(const char *fileName){

  if(cdfObject == NULL){
    CDBError("No CDFObject defined, use CDFObject::attachCDFReader(CDFNetCDFReader*). Please note that this function should be called by CDFObject open routines.");
    return 1;
  }
  this->fileName=fileName;
  
  if(cdfCache!=NULL){
    int cacheStatus = cdfCache->open(fileName,cdfObject,false);
    if(cacheStatus == 0) {
      //CDBDebug("Succesfully opened from cache for file %s",fileName);
      return 0;
    }
  }
  
  // Check type sizes 
  if(sizeof(char)  !=1){CDBError("The size of char is unequeal to 8 Bits");return 1;}
  if(sizeof(short) !=2){CDBError("The size of short is unequeal to 16 Bits");return 1;}
  if(sizeof(int)   !=4){CDBError("The size of int is unequeal to 32 Bits");return 1;}
  if(sizeof(float) !=4){CDBError("The size of float is unequeal to 32 Bits");return 1;}
  if(sizeof(double)!=8){CDBError("The size of double is unequeal to 64 Bits");return 1;}
  
 
  
  
  //Set cache size
  //status = nc_set_chunk_cache(55353600*10,2000,0.75);
  //if(status!=NC_NOERR){ncError(__LINE__,className,"nc_set_chunk_cache: ",status);return 1;}
  //status = nc_set_chunk_cache(0,0,0);
  //if(status!=NC_NOERR){ncError(__LINE__,className,"nc_set_chunk_cache: ",status);return 1;}
  #ifdef CCDFNETCDFIO_DEBUG_OPEN        
  CDBDebug("NC_OPEN opening %s",fileName);
  #endif      
  status = nc_open(fileName,NC_NOWRITE,&root_id);
  if(status!=NC_NOERR){ncError(__LINE__,className,"nc_open: ",status);return 1;}
/*#ifdef MEASURETIME
      StopWatch_Stop("CDFNetCDFReader open file\n");
#endif*/
  status = nc_inq(root_id,&nDims,&nVars,&nRootAttributes,&unlimDimIdP);
  if(status!=NC_NOERR){ncError(__LINE__,className,"nc_inq: ",status);return 1;}
/*#ifdef MEASURETIME
      StopWatch_Stop("NC_INQ");
#endif*/

  status = readDimensions();if(status!=0)return 1;
/*#ifdef MEASURETIME
      StopWatch_Stop("readDim");
#endif*/
  
  status = readVariables();if(status!=0)return 1;
/*#ifdef MEASURETIME
      StopWatch_Stop("readVar");
#endif*/
  
  status = readAttributes(cdfObject->attributes,NC_GLOBAL,nRootAttributes);if(status!=0)return 1;
/*#ifdef MEASURETIME
      StopWatch_Stop("readAttr");
#endif*/
  if(cdfCache!=NULL){
    int cacheStatus = cdfCache->open(fileName,cdfObject,true);
    if(cacheStatus == 0) return 0;
  }
  return 0;
}


int CDFNetCDFReader::close(){
  if(root_id!=-1){
#ifdef CCDFNETCDFIO_DEBUG        
CDBDebug("CLOSING %s", fileName.c_str());
#endif        
    nc_close(root_id);
  }
  root_id=-1;
  return 0;
}

void CDFNetCDFWriter::ncError(int line, const char *className, const char * msg,int e){
  if(e==NC_NOERR)return;
  char szTemp[1024];
  snprintf(szTemp,1023,"[E: %s, %d in class %s] %s: %s\n",__FILE__,line,className,msg,nc_strerror(e));
  printErrorStream(szTemp);
}



