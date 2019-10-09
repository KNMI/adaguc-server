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

#ifndef CCDFATTRIBUTE_H
#define CCDFATTRIBUTE_H

#include "CCDFTypes.h"
namespace CDF{  
  class Attribute{
    public:
      void setName(const char *value){
        name.copy(value);
      }

      Attribute(){
        data=NULL;
        length=0;
      }
      
      Attribute(Attribute*att){
        data=NULL;
        length=0;

        name.copy(&att->name);
        setData(att);
      }
      
      
      Attribute(const char *attrName,const char *attrString){
        data=NULL;
        length=0;
        name.copy(attrName);
        setData(CDF_CHAR,attrString,strlen(attrString));
      }
      
      Attribute(const char *attrName,CDFType type,const void *dataToSet,size_t dataLength){
        data=NULL;
        length=0;
        name.copy(attrName);
        setData(type,dataToSet,dataLength);
      }
      
      ~Attribute(){
        if(data!=NULL){freeData(&data);};data=NULL;
      }
      CDFType type;
      CT::string name;
      size_t length;
      void *data;
      
      CDFType getType(){
        return type;
      }
      
      int setData(Attribute *attribute){
        this->setData(attribute->type,attribute->data,attribute->size());
        return 0;
      }
      int setData(CDFType type,const void *dataToSet,size_t dataLength){
        if(data!=NULL){freeData(&data);};data=NULL;
        length=dataLength;
        this->type=type;
        allocateData(type,&data,length);
        if(type==CDF_CHAR||type==CDF_UBYTE||type==CDF_BYTE)memcpy(data,dataToSet,length);
        if(type==CDF_SHORT||type==CDF_USHORT)memcpy(data,dataToSet,length*sizeof(short));
        if(type==CDF_INT||type==CDF_UINT)memcpy(data,dataToSet,length*sizeof(int));
        if(type==CDF_INT64||type==CDF_UINT64)memcpy(data,dataToSet,length*sizeof(long));
        if(type==CDF_FLOAT)memcpy(data,dataToSet,length*sizeof(float));
        if(type==CDF_DOUBLE){memcpy(data,dataToSet,length*sizeof(double));}
        return 0;
      }
      
      /**
       * Sets one element of data
       */
      template <class T>
      int setData(CDFType type,T data){
        if(type==CDF_CHAR||type==CDF_BYTE){char d=data; setData(type,&d,1);}
        if(type==CDF_UBYTE){unsigned char d=data; setData(type,&d,1);}
        if(type==CDF_SHORT){short d=data; setData(type,&d,1);}
        if(type==CDF_USHORT){unsigned short d=data; setData(type,&d,1);}
        if(type==CDF_INT){int d=data; setData(type,&d,1);}
        if(type==CDF_UINT){unsigned int d=data; setData(type,&d,1);}
        if(type==CDF_INT64){long d=data; setData(type,&d,1);}
        if(type==CDF_UINT64){unsigned long d=data; setData(type,&d,1);}
        if(type==CDF_FLOAT){float d=data; setData(type,&d,1);}
        if(type==CDF_DOUBLE){double d=data; setData(type,&d,1);}
        return 0;
      }
      
      
      int setData(const char*dataToSet){
        if(data!=NULL){freeData(&data);};data=NULL;
        length=strlen(dataToSet);
        this->type=CDF_CHAR;
        allocateData(type,&data,length+1);
        if(type==CDF_CHAR){
          memcpy(data,dataToSet,length);//TODO support other data types as well
          ((char*)data)[length]='\0';
        }
        return 0;
      }
      template <class T>
      int getData(T *dataToGet,size_t getlength){
        if(data==NULL)return 0;
        if(getlength>length)getlength=length;
        CDF::DataCopier::copy(dataToGet,data,type,getlength);
        return getlength;
      }
      int getDataAsString(CT::string *out){
        out->copy("");
        if(type==CDF_CHAR){
          out->copy((const char*)data,length);
          int a = strlen(out->c_str());
          out->setSize(a);
          return 0;
        }
        if(type==CDF_BYTE)for(size_t n=0;n<length;n++){if(out->length()>0)out->concat(" ");out->printconcat("%d",((char*)data)[n]);}
        if(type==CDF_UBYTE)for(size_t n=0;n<length;n++){if(out->length()>0)out->concat(" ");out->printconcat("%u",((unsigned char*)data)[n]);}
        
        if(type==CDF_INT)for(size_t n=0;n<length;n++){if(out->length()>0)out->concat(" ");out->printconcat("%d",((int*)data)[n]);}
        if(type==CDF_UINT)for(size_t n=0;n<length;n++){if(out->length()>0)out->concat(" ");out->printconcat("%u",((unsigned int*)data)[n]);}
        
        if(type==CDF_INT64)for(size_t n=0;n<length;n++){if(out->length()>0)out->concat(" ");out->printconcat("%ld",((long*)data)[n]);}
        if(type==CDF_UINT64)for(size_t n=0;n<length;n++){if(out->length()>0)out->concat(" ");out->printconcat("%lu",((unsigned long*)data)[n]);}

        if(type==CDF_SHORT)for(size_t n=0;n<length;n++){if(out->length()>0)out->concat(" ");out->printconcat("%d",((short*)data)[n]);}
        if(type==CDF_USHORT)for(size_t n=0;n<length;n++){if(out->length()>0)out->concat(" ");out->printconcat("%u",((unsigned short*)data)[n]);}
        
        if(type==CDF_FLOAT)for(size_t n=0;n<length;n++){if(out->length()>0)out->concat(" ");out->printconcat("%f",((float*)data)[n]);}
        if(type==CDF_DOUBLE)for(size_t n=0;n<length;n++){if(out->length()>0)out->concat(" ");out->printconcat("%f",((double*)data)[n]);}
        if(type==CDF_STRING)for(size_t n=0;n<length;n++){if(out->length()>0)out->concat(" ");out->printconcat("%s",((char**)data)[n]);}
        return 0;
      }
      
      CT::string toString(){
        CT::string out = "";
        getDataAsString(&out);
        return out;
      }
      
      CT::string getDataAsString(){
        return toString();
      }
      
      size_t size(){
        return length;
      }
  };
}
#endif
