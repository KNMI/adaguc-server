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

#include "CCDFTypes.h"

//#include "CDebugger.h"
//#ifdef MEMLEAKCHECK
//extern Tracer NewTrace;
//#define MEMLEAKCHECK
//#endif

//DEF_ERRORMAIN()

int CDF::getTypeSize(CDFType type){
  if(type == CDF_CHAR || type == CDF_UBYTE || type == CDF_BYTE)return 1;
  if(type == CDF_SHORT || type == CDF_USHORT)return 2;
  if(type == CDF_INT || type == CDF_UINT)return 4;
  if(type == CDF_INT64 || type == CDF_UINT64)return 8;
  if(type == CDF_FLOAT)return 4;
  if(type == CDF_DOUBLE)return 8;
  if(type == CDF_UNKNOWN)return 8;
  if(type == CDF_STRING)return sizeof(char*);
  return 0;
}

int CDF::freeData(void **p){
  #ifdef CCDFTYPES_MEMLEAKCHECK
  if (Tracer::Ready)
    NewTrace.Remove (*p);
  #endif
  free(*p);
  *p=NULL;
  return 0;
}


//Data must be freed with freeData()
int CDF::allocateData(CDFType type,void **p,size_t length){

  if((*p)!=NULL){freeData(p);};(*p)=NULL;

  size_t typeSize= getTypeSize(type);
  if(typeSize==0){
    //CDBError("In CDF::allocateData: Unknown type");
    return 1;
  }
  *p = malloc(length*typeSize);
  
  if(*p==NULL){
    //CDBError("In CDF::allocateData: Unable to allocate %d elements",length);
    return 1;
  }
  
  #ifdef CCDFTYPES_MEMLEAKCHECK
  if (Tracer::Ready)NewTrace.Add (*p, __FILE__, __LINE__);
  #endif

  if(type == CDF_STRING){
    for(size_t j=0;j<length;j++){
      ((char**)*p)[j]=NULL;
    }
  }
  return 0;
}

void CDF::getCDFDataTypeName(char *name,const size_t maxlen,const int type){
  snprintf(name,maxlen,"CDF_UNDEFINED");
  if(type==CDF_NONE  )snprintf(name,maxlen,"CDF_NONE");
  if(type==CDF_BYTE  )snprintf(name,maxlen,"CDF_BYTE");
  if(type==CDF_CHAR  )snprintf(name,maxlen,"CDF_CHAR");
  if(type==CDF_SHORT )snprintf(name,maxlen,"CDF_SHORT");
  if(type==CDF_INT   )snprintf(name,maxlen,"CDF_INT");
  if(type==CDF_INT64 )snprintf(name,maxlen,"CDF_INT64");
  if(type==CDF_FLOAT )snprintf(name,maxlen,"CDF_FLOAT");
  if(type==CDF_DOUBLE)snprintf(name,maxlen,"CDF_DOUBLE");
  if(type==CDF_UNKNOWN)snprintf(name,maxlen,"CDF_UNKNOWN");
  if(type==CDF_UBYTE )snprintf(name,maxlen,"CDF_UBYTE");
  if(type==CDF_USHORT)snprintf(name,maxlen,"CDF_USHORT");
  if(type==CDF_UINT  )snprintf(name,maxlen,"CDF_UINT");
  if(type==CDF_UINT64  )snprintf(name,maxlen,"CDF_UINT64");
  if(type==CDF_STRING)snprintf(name,maxlen,"CDF_STRING");
}

void CDF::getCDataTypeName(char *name,const size_t maxlen,const int type){
  snprintf(name,maxlen,"CDF_UNDEFINED");
  if(type==CDF_NONE  )snprintf(name,maxlen,"none");
  if(type==CDF_BYTE  )snprintf(name,maxlen,"uchar");
  if(type==CDF_CHAR  )snprintf(name,maxlen,"char");
  if(type==CDF_SHORT )snprintf(name,maxlen,"short");
  if(type==CDF_INT   )snprintf(name,maxlen,"int");
  if(type==CDF_INT64 )snprintf(name,maxlen,"long");
  if(type==CDF_FLOAT )snprintf(name,maxlen,"float");
  if(type==CDF_DOUBLE)snprintf(name,maxlen,"double");
  if(type==CDF_UBYTE )snprintf(name,maxlen,"ubyte");
  if(type==CDF_USHORT)snprintf(name,maxlen,"ushort");
  if(type==CDF_UINT  )snprintf(name,maxlen,"uint");
  if(type==CDF_UINT64)snprintf(name,maxlen,"ulong");
  if(type==CDF_STRING)snprintf(name,maxlen,"char*");
}

void CDF::getErrorMessage(char *errorMessage,const size_t maxlen,const int errorCode){
   snprintf(errorMessage,maxlen,"CDF_E_UNDEFINED");
   if(errorCode==CDF_E_NONE)snprintf(errorMessage,maxlen,"CDF_E_NONE");
   if(errorCode==CDF_E_DIMNOTFOUND)snprintf(errorMessage,maxlen,"CDF_E_DIMNOTFOUND");
   if(errorCode==CDF_E_ATTNOTFOUND)snprintf(errorMessage,maxlen,"CDF_E_ATTNOTFOUND");
   if(errorCode==CDF_E_VARNOTFOUND)snprintf(errorMessage,maxlen,"CDF_E_VARNOTFOUND");
   if(errorCode==CDF_E_NRDIMSNOTEQUAL)snprintf(errorMessage,maxlen,"CDF_E_NRDIMSNOTEQUAL");
   if(errorCode==CDF_E_VARHASNOPARENT)snprintf(errorMessage,maxlen,"CDF_E_VARHASNOPARENT");
   if(errorCode==CDF_E_VARHASNODATA)snprintf(errorMessage,maxlen,"CDF_E_VARHASNODATA");
   
}


CT::string CDF::getCDFDataTypeName(const int type){
  char data[100];
  getCDFDataTypeName(data,99,type);
  CT::string d=data;
  return d;
}
void CDF::getErrorMessage(CT::string *errorMessage,const int errorCode){
  char msg[1024];
  getErrorMessage(msg,1023,errorCode);
  errorMessage->copy(msg);    
}

CT::string CDF::getErrorMessage(int errorCode){
  CT::string errorMessage;
  getErrorMessage(&errorMessage,errorCode);
  return errorMessage;
}

int CDF::DataCopier::copy(void *destdata,CDFType destType,void *sourcedata,CDFType sourcetype,size_t destinationOffset,size_t sourceOffset,size_t length){
  if(sourcetype==CDF_STRING||destType==CDF_STRING){
    if(sourcetype==CDF_STRING&&destType==CDF_STRING){
      for(size_t t=0;t<length;t++){
        const char*sourceValue = ((char**)sourcedata)[t+sourceOffset];
        if(sourceValue!=NULL){
          size_t strlength =strlen(sourceValue);
          ((char**)destdata)[t+destinationOffset]=(char*)malloc(strlength+1);
          strncpy(((char**)destdata)[t+destinationOffset],sourceValue,strlength);
          ((char**)destdata)[t+destinationOffset][strlength]=0;
        }
      }
      return 0;
    }
    return 1;
  }
  switch(destType){
    
    case CDF_CHAR:_copy((char*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
    case CDF_BYTE:_copy((char*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
    case CDF_UBYTE:_copy((unsigned char*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
    case CDF_SHORT:_copy((short*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
    case CDF_USHORT:_copy((unsigned short*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
    case CDF_INT:_copy((int*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
    case CDF_UINT:_copy((unsigned int*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
    case CDF_INT64:_copy((long*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
    case CDF_UINT64:_copy((unsigned long*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
    case CDF_FLOAT:_copy((float*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
    case CDF_DOUBLE:_copy((double*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
    default:return 1;
  }
  return 0;
}
  
  
int CDF::fill(void *destdata,CDFType destType,double value,size_t size){
  if(destType==CDF_STRING){
    for(size_t j=0;j<size;j++){
      free(((char**)destdata)[j]);
      ((char**)destdata)[j]=NULL;
    }
    return 0;
  }
  switch(destType){
    case CDF_CHAR:DataCopier::_fill((char*)destdata,value,size);break;
    case CDF_BYTE:DataCopier::_fill((char*)destdata,value,size);break;
    case CDF_UBYTE:DataCopier::_fill((unsigned char*)destdata,value,size);break;
    case CDF_SHORT:DataCopier::_fill((short*)destdata,value,size);break;
    case CDF_USHORT:DataCopier::_fill((unsigned short*)destdata,value,size);break;
    case CDF_INT:DataCopier::_fill((int*)destdata,value,size);break;
    case CDF_UINT:DataCopier::_fill((unsigned int*)destdata,value,size);break;
    case CDF_INT64:DataCopier::_fill((long*)destdata,value,size);break;
    case CDF_UINT64:DataCopier::_fill((unsigned long*)destdata,value,size);break;
    case CDF_FLOAT:DataCopier::_fill((float*)destdata,value,size);break;
    case CDF_DOUBLE:DataCopier::_fill((double*)destdata,value,size);break;
    default:return 1;
  }
  return 0;
}
