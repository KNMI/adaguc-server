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

#ifndef CCDFTYPES_DEBUG
#define CCDFTYPES_DEBUG

#include <stdio.h>
#include <stddef.h>
#include <vector>
#include <iostream>
#include <stdlib.h>

#include "CTypes.h"



//#define CCDFDATAMODEL_DEBUG
//CDF: Common Data Format


/* Types supported by CDF */
typedef int CDFType;
#define CDF_NONE    0  /* Unknown */
#define CDF_BYTE    1  /* signed 1 byte integer */
#define CDF_CHAR    2  /* ISO/ASCII character */
#define CDF_SHORT   3  /* signed 2 byte integer */
#define CDF_INT     4  /* signed 4 byte integer */
#define CDF_FLOAT   5  /* single precision floating point number */
#define CDF_DOUBLE  6  /* double precision floating point number */
#define CDF_UBYTE   7  /* unsigned 1 byte int */
#define CDF_USHORT  8  /* unsigned 2-byte int */
#define CDF_UINT    9  /* unsigned 4-byte int */
#define CDF_STRING  10 /* variable string */
#define CDF_UNKNOWN 11 /* Unknown type, using CDF_DOUBLE */
#define CDF_INT64   12 /* signed 8 byte integer */
#define CDF_UINT64  13 /* unsigned 8 byte integer */



/* Possible error codes, thrown by CDF */
typedef int CDFError;
#define CDF_E_NONE              0 /* Unknown */
#define CDF_E_ERROR             1 /* Unknown */
#define CDF_E_VARNOTFOUND       2 /* Variable not found */
#define CDF_E_DIMNOTFOUND       3 /* Dimension not found */
#define CDF_E_ATTNOTFOUND       4 /* Attribute not found */
#define CDF_E_NRDIMSNOTEQUAL    5
#define CDF_E_VARHASNOPARENT    6 /*Variable has no parent CDFObject*/
#define CDF_E_VARHASNODATA      7/*Variable has no data*/

/*#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ", " TOSTRING(__LINE__)
#define EXCEPTION(x) "Exception in "AT": " x*/


namespace CDF{
  //Allocates data for an array, provide type, the empty array and length
  // Data must be freed by using free()
  int allocateData(CDFType type,void **p,size_t length);
  int freeData(void **p);
  
  
  
  
  
  //Copies data from one array to another and performs type conversion
  //Destdata must be a pointer to an empty array with non-void type
  class DataCopier{
    private:
      
    template <typename T>
    static int _copy(T *destdata,void *sourcedata,CDFType sourcetype,size_t destinationOffset,size_t sourceOffset,size_t length){
      
      size_t dsto=destinationOffset;
      size_t srco=sourceOffset;
      if(sourcetype==CDF_STRING){
        //CDBError("Unable to copy CDF_STRING");
        return 1;
      }
      CT::string t = typeid(T).name();
      if(t.equals(typeid(void).name())){
        return 1;
      }
      
      
      if(sourcetype==CDF_CHAR||sourcetype==CDF_BYTE)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((char*)sourcedata)[t+srco];}
      if(sourcetype==CDF_CHAR||sourcetype==CDF_UBYTE)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((unsigned char*)sourcedata)[t+srco];}
      if(sourcetype==CDF_SHORT)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((short*)sourcedata)[t+srco];}
      if(sourcetype==CDF_USHORT)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((unsigned short*)sourcedata)[t+srco];}
      if(sourcetype==CDF_INT)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((int*)sourcedata)[t+srco];}
      if(sourcetype==CDF_UINT)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((unsigned int*)sourcedata)[t+srco];}
      if(sourcetype==CDF_INT64)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((long*)sourcedata)[t+srco];}
      if(sourcetype==CDF_UINT64)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((unsigned long*)sourcedata)[t+srco];}
      if(sourcetype==CDF_FLOAT)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((float*)sourcedata)[t+srco];}
      if(sourcetype==CDF_DOUBLE)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((double*)sourcedata)[t+srco];}
      return 0;
    }
    
   
public:
    template <class T>          
    static int copy(T *destdata,void *sourcedata,CDFType sourcetype,size_t length){
      return _copy(destdata,sourcedata,sourcetype,0,0,length);
    }
     template <class T>    
    static void _fill(T *data,double value,size_t size){
      for(size_t j=0;j<size;j++){
        data[j]=value;
      }
    }
    


    //Deprectated?
    //static int copy(void *destdata,void *sourcedata,CDFType sourcetype,size_t destinationOffset,size_t sourceOffset,size_t length);
  
    static int copy(void *destdata,CDFType destType,void *sourcedata,CDFType sourcetype,size_t destinationOffset,size_t sourceOffset,size_t length);
    
  };
  
  int fill(void *destdata,CDFType destType,double value,size_t size);
  
  /*dataCopier.copy function should do the job...*/
 // static DataCopier dataCopier;

  /*Puts the CDF name of the type in the string array with name (CDF_FLOAT, CDF_INT, etc...)*/
  void getCDFDataTypeName(char *name,const size_t maxlen,const int type);
  
  /*Puts the C name of the type in the string array with name (float, int, etc...)*/
  void getCDataTypeName(char *name,const size_t maxlen,const int type);
  
  /*Puts the error code as string in the string array based on the error code number*/
  void getErrorMessage(char *errorMessage,const size_t maxlen,const int errorCode);
  void getErrorMessage(CT::string *errorMessage,const int errorCode);
  
  /**
   * Static function which converts an exception into a readable message
   * @param int The value of catched exception 
   * @return CT::string with the readable message
   */
  CT::string getErrorMessage(int errorCode);
  
  
  /**
   * returns the type name as string
   * @param type The CDF type
   * @return string with the name
   */
  CT::string getCDFDataTypeName(const int type);
  
  /*Returns the number of bytes needed for a single element of this datatype*/
  int getTypeSize(CDFType type);

};



#endif
