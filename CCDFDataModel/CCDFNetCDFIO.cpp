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

void CDFNetCDFReader::ncError(int line, const char *className, const char * msg,int e){
  if(e==NC_NOERR)return;
  char szTemp[1024];
  snprintf(szTemp,1023,"[E: %s, %d in class %s] %s: %s\n",__FILE__,line,className,msg,nc_strerror(e));
  printErrorStream(szTemp);
  
}

int CDFNetCDFReader::_readVariableData(CDF::Variable *var, CDFType type){
  return _readVariableData(var,type,NULL,NULL,NULL);
  //CDBDebug("reading from file %s",fileName.c_str());
/*#ifdef CCDFNETCDFIO_DEBUG   
  char typeName[254];
  CDF::getCDFDataTypeName(typeName,255,var->getType());
  CDBDebug("readVariableData %s of type %s",var->name.c_str(),typeName);
#endif      
  //It is essential that the variable nows which reader can be used to read the data
  //var->setCDFReaderPointer((void*)this);
  if(var->dimensionlinks.size()==0){
    CDBError("No dimensions specified for variable %s",var->name.c_str());
    return 1;
  }
  if(root_id==-1){
    #ifdef CCDFNETCDFIO_DEBUG_OPEN        
    CDBDebug("opening %s",fileName.c_str());
    #endif      
    status = nc_open(fileName.c_str(),NC_NOWRITE,&root_id);
    if(status!=NC_NOERR){ncError(__LINE__,className,"nc_open: ",status);return 1;}
  }
  var->freeData();
  size_t totalVariableSize = 1;
  for(size_t i=0;i<var->dimensionlinks.size();i++){
    totalVariableSize*=var->dimensionlinks[i]->length;
  }
#ifdef CCDFNETCDFIO_DEBUG                     
  for(size_t i=0;i<var->dimensionlinks.size();i++){
    CDBDebug("Reading %s,%d: %d",var->name.c_str(),i,var->dimensionlinks[i]->length);
  }
  CDBDebug("Calculated size = %d",totalVariableSize);
#endif      
  var->setSize(totalVariableSize);
  CDF::allocateData(type,&var->data,var->getSize());
  
  //printf("%s\n",var->name.c_str());
  //if(var->name.equals("azidiff"))return 0;
  if(type==CDF_BYTE||type==CDF_UBYTE)status = nc_get_var_ubyte(root_id,var->id,(unsigned char*)var->data);
  else if(type==CDF_CHAR)status = nc_get_var_text(root_id,var->id,(char*)var->data);
  else if(type==CDF_SHORT)status = nc_get_var_short(root_id,var->id,(short*)var->data);
  else if(type==CDF_USHORT)status = nc_get_var_ushort(root_id,var->id,(unsigned short*)var->data);
  else if(type==CDF_INT)status = nc_get_var_int(root_id,var->id,(int*)var->data);
  else if(type==CDF_UINT)status = nc_get_var_uint(root_id,var->id,(unsigned int*)var->data);
  else if(type==CDF_FLOAT)status = nc_get_var_float(root_id,var->id,(float*)var->data);
  else if(type==CDF_DOUBLE)status = nc_get_var_double(root_id,var->id,(double*)var->data);
  else if(type==CDF_STRING){
  
    status = nc_get_var_string(root_id,var->id,(char**)var->data);
  }
  else {
    CDBError("Unable to determine netcdf type\n");
    return 1;
  }
  if(status!=NC_NOERR){
    char typeName[254];
    CDF::getCDFDataTypeName(typeName,255,var->currentType);
    CDBError("Problem with variable %s of type %s (requested %s):",var->name.c_str(),typeName,CDF::getCDFDataTypeName(type).c_str());
    ncError(__LINE__,className,"nc_get_var: ",status);
    return 1;
    
  }
    warper.warpLonData(var);
  return 0;*/
}

int CDFNetCDFReader::_readVariableData(CDF::Variable *var, CDFType type,size_t *start,size_t *count,ptrdiff_t *stride){
  
  if(var->dimensionlinks.size()==0){
    CDBError("No dimensions specified for variable %s",var->name.c_str());
    return 1;
  }
  
  if(root_id==-1){
    #ifdef CCDFNETCDFIO_DEBUG_OPEN        
    CDBDebug("opening %s",fileName.c_str());
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

  #ifdef CCDFNETCDFIO_DEBUG        
  CDBDebug("Ready.");
  #endif
  return 0;
}

void CDFNetCDFWriter::ncError(int line, const char *className, const char * msg,int e){
  if(e==NC_NOERR)return;
  char szTemp[1024];
  snprintf(szTemp,1023,"[E: %s, %d in class %s] %s: %s\n",__FILE__,line,className,msg,nc_strerror(e));
  printErrorStream(szTemp);
}