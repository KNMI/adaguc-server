/* 
 * Copyright (C) 2012, Royal Netherlands Meteorological Institute (KNMI)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or any 
 * later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Project    : ADAGUC
 *
 * initial programmer :  M.Plieger
 * initial date       :  20120610
 */
#ifndef CCDFDATAMODEL_H
#define CCDFDATAMODEL_H
#include <stdio.h>
#include <vector>
#include <iostream>
#include "CDebugger.h"
#include "CTypes.h"
#include <stdlib.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

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
    public:
      template <class T>
      int copy(T *destdata,void *sourcedata,CDFType sourcetype,size_t destinationOffset,size_t sourceOffset,size_t length){
        size_t dsto=destinationOffset;
        size_t srco=sourceOffset;
        if(sourcetype==CDF_STRING){
          //CDBError("Unable to copy CDF_STRING");
          return 1;
        }
        if(sourcetype==CDF_CHAR||sourcetype==CDF_BYTE)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((char*)sourcedata)[t+srco];}
        if(sourcetype==CDF_CHAR||sourcetype==CDF_UBYTE)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((unsigned char*)sourcedata)[t+srco];}
        if(sourcetype==CDF_SHORT)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((short*)sourcedata)[t+srco];}
        if(sourcetype==CDF_USHORT)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((unsigned short*)sourcedata)[t+srco];}
        if(sourcetype==CDF_INT)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((int*)sourcedata)[t+srco];}
        if(sourcetype==CDF_UINT)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((unsigned int*)sourcedata)[t+srco];}
        if(sourcetype==CDF_FLOAT)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((float*)sourcedata)[t+srco];}
        if(sourcetype==CDF_DOUBLE)for(size_t t=0;t<length;t++){destdata[t+dsto]=(T)((double*)sourcedata)[t+srco];}
        return 0;
      }
      template <class T>          
      int copy(T *destdata,void *sourcedata,CDFType sourcetype,size_t length){
        return copy(destdata,sourcedata,sourcetype,0,0,length);
      }
    int copy(void *destdata,void *sourcedata,CDFType sourcetype,size_t destinationOffset,size_t sourceOffset,size_t length){
      if(sourcetype==CDF_STRING){
        //CDBError("Unable to copy CDF_STRING");
        return 1;
      }
      switch(sourcetype){
        case CDF_CHAR:copy((char*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
        case CDF_BYTE:copy((char*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
        case CDF_UBYTE:copy((unsigned char*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
        case CDF_SHORT:copy((short*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
        case CDF_USHORT:copy((unsigned short*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
        case CDF_INT:copy((int*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
        case CDF_UINT:copy((unsigned int*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
        case CDF_FLOAT:copy((float*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
        case CDF_DOUBLE:copy((double*)destdata,sourcedata,sourcetype,destinationOffset,sourceOffset,length);break;
        default:return 1;
      }
      
        return 0;
          }          
          
  };
  /*dataCopier.copy function should do the job...*/
  static DataCopier dataCopier;

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
  
  
  class Attribute{
    public:
      void setName(const char *value){
        name.copy(value);
      }

      Attribute(){
        data=NULL;
        length=0;
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
        if(data!=NULL)freeData(&data);data=NULL;
      }
      CDFType type;
      CT::string name;
      size_t length;
      void *data;
      int setData(Attribute *attribute){
        this->setData(attribute->type,attribute->data,attribute->size());
        return 0;
      }
      int setData(CDFType type,const void *dataToSet,size_t dataLength){
        if(data!=NULL)freeData(&data);data=NULL;
        length=dataLength;
        this->type=type;
        allocateData(type,&data,length);
        if(type==CDF_CHAR||type==CDF_UBYTE||type==CDF_BYTE)memcpy(data,dataToSet,length);
        if(type==CDF_SHORT||type==CDF_USHORT)memcpy(data,dataToSet,length*sizeof(short));
        if(type==CDF_INT||type==CDF_UINT)memcpy(data,dataToSet,length*sizeof(int));
        if(type==CDF_FLOAT)memcpy(data,dataToSet,length*sizeof(float));
        if(type==CDF_DOUBLE){memcpy(data,dataToSet,length*sizeof(double));}
        return 0;
      }
      int setData(const char*dataToSet){
        if(data!=NULL)freeData(&data);data=NULL;
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
        dataCopier.copy(dataToGet,data,type,getlength);
        return getlength;
      }
      int getDataAsString(CT::string *out){
        out->copy("");
        if(type==CDF_CHAR||type==CDF_UBYTE||type==CDF_BYTE){out->copy((const char*)data,length);return 0;}
        if(type==CDF_INT||type==CDF_UINT)for(size_t n=0;n<length;n++){if(out->length()>0)out->concat(" ");out->printconcat("%d",((int*)data)[n]);}
        if(type==CDF_SHORT||type==CDF_USHORT)for(size_t n=0;n<length;n++){if(out->length()>0)out->concat(" ");out->printconcat("%ds",((short*)data)[n]);}
        if(type==CDF_FLOAT)for(size_t n=0;n<length;n++){if(out->length()>0)out->concat(" ");out->printconcat("%ff",((float*)data)[n]);}
        if(type==CDF_DOUBLE)for(size_t n=0;n<length;n++){if(out->length()>0)out->concat(" ");out->printconcat("%fdf",((double*)data)[n]);}
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
  class Dimension{
    
    public:
      Dimension(){
        isIterative=false;
        length=0;
      }
      CT::string name;
      size_t length;
      bool isIterative;
      int id;
      size_t getSize(){
        return length;
      }
      void setSize(size_t _length){
        length=_length;
      }
      void setName(const char *value){
        name.copy(value);
      }
      //Returns a new copy of this dimension
      Dimension *clone(){
        Dimension *newDim = new Dimension ();
        newDim->name=name.c_str();
        newDim->length=length;
        newDim->isIterative=isIterative;
        newDim->id=id;
        return newDim;
      }
  };
  
  
  class Variable{
    private:
      //Currently, aggregation along just 1 dimension is supported.
      class CDFObjectClass{
      public:
        void *cdfObjectPointer;
        int dimIndex;
        double dimValue;
      };
    std::vector<CDFObjectClass *> cdfObjectList;
    void *cdfReaderPointer;
    void *parentCDFObject;
    public:
      void setCDFReaderPointer(void *cdfReaderPointer){
        this->cdfReaderPointer=cdfReaderPointer;
      }
      void setParentCDFObject(void *parentCDFObject){
        this->parentCDFObject=parentCDFObject;
      }
      void * getParentCDFObject(){
        if(parentCDFObject==NULL){
          throw(CDF_E_VARHASNOPARENT);
        }
        return parentCDFObject;
      }
      void *getCDFObjectPointer(size_t *start,size_t *count){
        if(cdfObjectList.size()==0){
#ifdef CCDFDATAMODEL_DEBUG
          CDBDebug("returning getParentCDFObject");
#endif          
          return getParentCDFObject();
        }
        //Return the correct cdfReader According the given dims.
        size_t iterativeDimIndex;
        for(size_t j=0;j<dimensionlinks.size();j++){
          //CDBDebug("%d-> %d - %d - isIterative: %d", j, start[j],count[j],dimensionlinks[j]->isIterative);
          if(dimensionlinks[j]->isIterative){
            iterativeDimIndex=start[j];
            if(count[j]!=1){
              CDBError("Count %d instead of  1 is requested for iterative dimension %s",count[j],dimensionlinks[j]->name.c_str());
              throw(CDF_E_ERROR);
            }
            break;
          }
        }
#ifdef CCDFDATAMODEL_DEBUG        
        CDBDebug("Aggregating %d == %d",cdfObjectList[iterativeDimIndex]->dimIndex,iterativeDimIndex);
#endif
        return cdfObjectList[iterativeDimIndex]->cdfObjectPointer;
      }

      void setCDFObjectDim(Variable *sourceVar,const char *dimName);
      void freeData(){
        if(data==NULL)return;
#ifdef CCDFDATAMODEL_DEBUG                
        char typeName[255];getCDFDataTypeName(typeName,254,type);
        CDBDebug("Freeing %d elements of type %s for variable %s",getSize(),typeName,name.c_str());
#endif        
        if(data!=NULL){CDF::freeData(&data);data=NULL;}
        setSize(0);
      }
      
      int readData(CDFType type);
      int readData(bool applyScaleOffset);
      int readData(CDFType type,bool applyScaleOffset);
      int readData(CDFType type,size_t *_start,size_t *_count,ptrdiff_t *stride);
      
       template <class T>
      T getDataAt(int index){
        if(data==NULL){
          throw(CDF_E_VARHASNODATA);
        }
        T dataElement=((T*)data)[index];
        return dataElement;
      }
      
      /*template <class T>
      int getData(T *dataToGet,size_t getlength){
        if(data==NULL)return 0;
        if(getlength>getSize())getlength=getSize();
        dataCopier.copy(dataToGet,data,type,getlength);
        return getlength;
      }*/
      int getIterativeDimIndex(){
        for(size_t j=0;j<dimensionlinks.size();j++){if(dimensionlinks[j]->isIterative)return j;}
        return -1;
      }
      Dimension* getIterativeDim(){
        int i=getIterativeDimIndex();
        if(i==-1){
          throw(CDF_E_DIMNOTFOUND);
        }
        return dimensionlinks[i];
      }
      DEF_ERRORFUNCTION();
      Variable(){
        isDimension=false;
        data = NULL;
        currentSize=0;
        type=CDF_CHAR;
        cdfReaderPointer=NULL;
        parentCDFObject=NULL;
      }
      ~Variable(){
        
        
        for(size_t j=0;j<attributes.size();j++){if(attributes[j]!=NULL){delete attributes[j];attributes[j]=NULL;}}
        if(type==CDF_STRING){
          //CDBDebug("~Variable() %s %d",name.c_str(),getSize());
          for(size_t j=0;j<getSize();j++){
            //CDBDebug("%s\n",((const char**)data)[j]);
            free(((char**)data)[j]);
          }
        }
        if(data!=NULL){CDF::freeData(&data);data=NULL;}
        for(size_t j=0;j<cdfObjectList.size();j++){if(cdfObjectList[j]!=NULL){delete cdfObjectList[j];cdfObjectList[j]=NULL;}}
      }
      std::vector<Attribute *> attributes;
      std::vector<Dimension *> dimensionlinks;
      CDFType type;
      CT::string name;
      CT::string orgName;
      void setName(const char *value){
        name.copy(value);
        //TODO Implement this correctly in readvariabledata....
        if(orgName.length()==0)orgName.copy(value);
      }
      int id;
      size_t currentSize;
      void setSize(size_t size){
        currentSize = size;
      }
      size_t getSize(){
        return currentSize;
      }
      void *data;
      bool isDimension;
      
      Attribute * getAttribute(const char *name){
        for(size_t j=0;j<attributes.size();j++){
          if(attributes[j]->name.equals(name)){
            return attributes[j];
          }
        }
        throw(CDF_E_ATTNOTFOUND);
        return NULL;
      }
      Attribute * getAttributeNE(const char *name){try{return getAttribute(name);}catch(int e){return NULL;}}
      
      Dimension * getDimension(const char *name){
        for(size_t j=0;j<dimensionlinks.size();j++){
          if(dimensionlinks[j]->name.equals(name)){
            return dimensionlinks[j];
          }
        }
        throw(CDF_E_DIMNOTFOUND);
        return NULL;
      }
      Dimension * getDimensionNE(const char *name){try{return getDimension(name);}catch(int e){return NULL;}}

      int getDimensionIndexNE(const char *name){
        try{
          return getDimensionIndex(name);
        }catch(int e){}
        return -1;
      }
      
      int getDimensionIndex(const char *name){
        for(size_t j=0;j<dimensionlinks.size();j++){
          if(dimensionlinks[j]->name.equals(name)){
            return j;
          }
        }
       throw(CDF_E_DIMNOTFOUND);
      }
      
      
      int addAttribute(Attribute *attr){
        attributes.push_back(attr);
        return 0;
      }
      int removeAttribute(const char *name){
        for(size_t j=0;j<attributes.size();j++){
          if(attributes[j]->name.equals(name)){
            delete attributes[j];attributes[j]=NULL;
            attributes.erase(attributes.begin()+j);
          }
        }
        return 0;
      }
      
      int removeAttributes(){
        for(size_t j=0;j<attributes.size();j++){
          delete attributes[j];attributes[j]=NULL;
        }
        attributes.clear();
        return 0;
      }
      
      int setAttribute(const char *attrName,CDFType attrType,const void *attrData,size_t attrLen){
        Attribute *attr;
        try{
          attr=getAttribute(attrName);
        }catch(...){
          attr = new Attribute();
          attr->name.copy(attrName);
          addAttribute(attr);
        }
        attr->type=attrType;
        attr->setData(attrType,attrData,attrLen);
        return 0;
      }
      int setAttributeText(const char *attrName,const char *attrString,size_t strLen){
        size_t attrLen=strLen+1;
        char *attrData=new char[attrLen];
        memcpy(attrData,attrString,strLen);attrData[strLen]='\0';
        int retStat = setAttribute(attrName,CDF_CHAR,attrData,attrLen);
        delete[] attrData;
        return retStat;
      }
      int setAttributeText(const char *attrName,const char *attrString){
        size_t attrLen=strlen(attrString);
        char *attrData=new char[attrLen];
        memcpy(attrData,attrString,attrLen);
        int retStat = setAttribute(attrName,CDF_CHAR,attrData,attrLen);
        delete[] attrData;
        return retStat;
      }

      
      int setData(CDFType type,const void *dataToSet,size_t dataLength){
        if(data!=NULL)CDF::freeData(&data);data=NULL;
        currentSize=dataLength;
        this->type=type;
        allocateData(type,&data,currentSize);
        if(type==CDF_CHAR||type==CDF_UBYTE||type==CDF_BYTE)memcpy(data,dataToSet,currentSize);
        if(type==CDF_SHORT||type==CDF_USHORT)memcpy(data,dataToSet,currentSize*sizeof(short));
        if(type==CDF_INT||type==CDF_UINT)memcpy(data,dataToSet,currentSize*sizeof(int));
        if(type==CDF_FLOAT)memcpy(data,dataToSet,currentSize*sizeof(float));
        if(type==CDF_DOUBLE){memcpy(data,dataToSet,currentSize*sizeof(double));}
        return 0;
      }
  };

}
class CDFObject:public CDF::Variable{
  private:
    char *NCMLVarName;
    CDFType ncmlTypeToCDFType(const char *type){
      if(strncmp("String",type,6)==0)return CDF_CHAR;
      if(strncmp("byte",type,4)==0)return CDF_BYTE;
      if(strncmp("char",type,4)==0)return CDF_CHAR;
      if(strncmp("short",type,5)==0)return CDF_SHORT;
      if(strncmp("int",type,3)==0)return CDF_INT;
      if(strncmp("float",type,5)==0)return CDF_FLOAT;
      if(strncmp("double",type,6)==0)return CDF_DOUBLE;
      return CDF_DOUBLE;
    }
    void putNCMLAttributes(xmlNode * a_node){
      xmlNode *cur_node = NULL;
      for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE&&cur_node->name!=NULL){
            //Variable elements
            if(strncmp("variable",(char*)cur_node->name,8)==0){
              NCMLVarName=NULL;
              if(cur_node->properties->name!=NULL){
                if(cur_node->properties->children->content!=NULL){
                  xmlAttr*node=cur_node->properties;
                  char * pszOrgName=NULL,*pszName=NULL,*pszType=NULL;
                  while(node!=NULL){
                    if(strncmp("name",(char*)node->name,4)==0)
                      pszName=(char*)node->children->content;
                    if(strncmp("orgName",(char*)node->name,7)==0)
                      pszOrgName=(char*)node->children->content;
                    if(strncmp("type",(char*)node->name,4)==0)
                      pszType=(char*)node->children->content;
                    node=node->next;
                  }
                  //Rename a variable
                  if(pszOrgName!=NULL&&pszName!=NULL){
                    try{
                      CDF::Variable *var= getVariable(pszOrgName);
                      var->name.copy(pszName);
                    }catch(...){}
                  }
                  if(pszName!=NULL){
                    NCMLVarName=pszName;
                    CDF::Variable *var=NULL;
                    try{
                      var= getVariable(pszName);
                    }catch(...){
                      if(pszOrgName==NULL){
                        var = new CDF::Variable();
                        var->type=CDF_CHAR;
                        if(pszType!=NULL){
                          var->type=ncmlTypeToCDFType(pszType);
                        }
                        var->name.copy(pszName);
                        addVariable(var);
                      }
                    }                    
                  }
                }
              }
            }
            //Remove elements
            if(strncmp("remove",(char*)cur_node->name,6)==0){
              if(cur_node->properties->name!=NULL){
                xmlAttr*node=cur_node->properties;
                char * pszType=NULL,*pszName=NULL;
                while(node!=NULL){
                  if(strncmp("name",(char*)node->name,4)==0)
                    pszName=(char*)node->children->content;
                  if(strncmp("type",(char*)node->name,4)==0)
                    pszType=(char*)node->children->content;
                  node=node->next;
                }
                //Check what the parentname of this attribute is:
                const char *attributeParentVarName = NULL;
                if(cur_node->parent){
                  if(cur_node->parent->properties){
                    xmlAttr *tempnode = cur_node->parent->properties;
                    while(tempnode!=NULL){
                      if(strncmp("name",(char*)tempnode->name,4)==0){
                        attributeParentVarName=(char*)tempnode->children->content;
                        break;
                      }
                      tempnode=tempnode->next;
                    }
                  }
                }
                if(pszType!=NULL&&pszName!=NULL){
                  if(strncmp(pszType,"variable",8)==0){
                    removeVariable(pszName);
                  }
                  //Check wether we want to remove an attribute
                  if(strncmp(pszType,"attribute",9)==0){
                    if(attributeParentVarName!=NULL){
                      try{
                        CDF::Variable *var= getVariable(attributeParentVarName);
                        var->removeAttribute(pszName);
                      }catch(...){}
                    }else {
                      //Remove a global attribute
                      removeAttribute(pszName);
                    }
                  }
                }
              }
            }
            //Attribute elements
            if(strncmp("attribute",(char*)cur_node->name,9)==0){
              if(NCMLVarName==NULL)NCMLVarName=(char*)"NC_GLOBAL";
              if(NCMLVarName!=NULL){
                if(cur_node->properties->name!=NULL){
                  xmlAttr*node=cur_node->properties;
                  char * pszAttributeType=NULL,*pszAttributeName=NULL,*pszAttributeValue=NULL;
                  char * pszOrgName=NULL;
                  while(node!=NULL){
                    if(strncmp("name",(char*)node->name,4)==0)
                      pszAttributeName=(char*)node->children->content;
                    if(strncmp("type",(char*)node->name,4)==0)
                      pszAttributeType=(char*)node->children->content;
                    if(strncmp("value",(char*)node->name,5)==0)
                      pszAttributeValue=(char*)node->children->content;
                    if(strncmp("orgName",(char*)node->name,7)==0)
                      pszOrgName=(char*)node->children->content;
                    node=node->next;
                  }
                  if(pszAttributeName!=NULL){
                    //Rename an attribute
                    if(pszOrgName!=NULL){
                      try{
                        getVariable(NCMLVarName)->getAttribute(pszOrgName)->name.copy(pszAttributeName);
                      }catch(...){}
                    }else{
                      //Add an attribute
                      if(pszAttributeType!=NULL&&pszAttributeValue!=NULL){
                        CDF::Variable *var = NULL;
                        try{
                          var = getVariable(NCMLVarName);
                        }catch(...){
                          var = new CDF::Variable();
                          var->name.copy(NCMLVarName);
                          addVariable(var);
                        }
                        CDFType attrType = ncmlTypeToCDFType(pszAttributeType);
                        if(strncmp("String",pszAttributeType,6)==0){
                          var->setAttribute(pszAttributeName,
                                            attrType,
                                            pszAttributeValue,
                                            strlen(pszAttributeValue));
                        }else{
                          size_t attrLen=0;
                          CT::string t=pszAttributeValue;
                          CT::string *t2=t.splitToArray(",");
                          attrLen=t2->count;
                          double values[attrLen];
                          for(size_t attrN=0;attrN<attrLen;attrN++){
                            values[attrN]=atof(t2[attrN].c_str());
                            //CDBDebug("%f",values[attrN]);
                          }
                          delete[] t2;
                          //if(attrLen==3)exit(2);
                          
                          //double value=atof(pszAttributeValue);
                          CDF::Attribute *attr = new CDF::Attribute();
                          attr->name.copy(pszAttributeName);
                          var->addAttribute(attr);
                          attr->type=attrType;
                          CDF::allocateData(attrType,&attr->data,attrLen);
                          for(size_t attrN=0;attrN<attrLen;attrN++){
                            if(attrType==CDF_BYTE)((char*)attr->data)[attrN]=(char)values[attrN];
                            if(attrType==CDF_UBYTE)((unsigned char*)attr->data)[attrN]=(unsigned char)values[attrN];
                            if(attrType==CDF_CHAR)((char*)attr->data)[attrN]=(char)values[attrN];
                            if(attrType==CDF_SHORT)((short*)attr->data)[attrN]=(short)values[attrN];
                            if(attrType==CDF_USHORT)((unsigned short*)attr->data)[attrN]=(unsigned short)values[attrN];
                            if(attrType==CDF_INT)((int*)attr->data)[attrN]=(int)values[attrN];
                            if(attrType==CDF_UINT)((unsigned int*)attr->data)[attrN]=(unsigned int)values[attrN];
                            if(attrType==CDF_FLOAT)((float*)attr->data)[attrN]=(float)values[attrN];
                            if(attrType==CDF_DOUBLE)((double*)attr->data)[attrN]=(double)values[attrN];
                          }
                          attr->length=attrLen;
                        }
                      }
                    }
                  }
                }
              }
            }
        }
        putNCMLAttributes(cur_node->children);
      }
    }
  void *reader;
  public:
    DEF_ERRORFUNCTION();
    CDFObject(){
      name.copy("NC_GLOBAL");
      reader=NULL;
    }
    ~CDFObject();
    std::vector<CDF::Dimension *> dimensions;
    //std::vector<CDF::Attribute *> attributes;
    std::vector<CDF::Variable *> variables;
    CT::string name;
    int getVariableIndex(const char *name){
      if(strncmp("NC_GLOBAL",name,9)==0){
        throw(CDF_E_VARNOTFOUND);
      }
      for(size_t j=0;j<variables.size();j++){
        if(variables[j]->name.equals(name)){
          return j;
        }
      }
      throw(CDF_E_VARNOTFOUND);
    }
    int getVariableIndexNE(const char *name){
      try{
        return getVariableIndex(name);
      }catch(int e){}
      return -1;
    }
    
    int getDimensionIndex(const char *name){
      for(size_t j=0;j<dimensions.size();j++){
        if(dimensions[j]->name.equals(name)){
          return j;
        }
      }
      throw(CDF_E_DIMNOTFOUND);
    }
    int getDimensionIndexNE(const char *name){
      try{
        return getDimensionIndex(name);
      }catch(int e){}
      return -1;
    }
    
      
    
    CDF::Variable * getVariable(const char *name){
      if(strncmp("NC_GLOBAL",name,9)==0){
        return this;
      }
      for(size_t j=0;j<variables.size();j++){
        if(variables[j]->name.equals(name)){
          return variables[j];
        }
      }
      throw(CDF_E_VARNOTFOUND);
      return NULL;
    }
    CDF::Variable * getVariableNE(const char *name){
      try{return getVariable(name);}catch(int e){return NULL;}
    }
    int addVariable(CDF::Variable *var){
      var->id = variables.size();
      var->setParentCDFObject(this);
      variables.push_back(var);
      return 0;
    }
    int removeVariable(const char *name){
      for(size_t j=0;j<variables.size();j++){
        if(variables[j]->name.equals(name)){
          delete variables[j];variables[j]=NULL;
          variables.erase(variables.begin()+j);
        }
      }
      return 0;
    }
    int removeDimension(const char *name){
      for(size_t j=0;j<dimensions.size();j++){
        if(dimensions[j]->name.equals(name)){
          delete dimensions[j];dimensions[j]=NULL;
          dimensions.erase(dimensions.begin()+j);
        }
      }
      return 0;
    }
    int addDimension(CDF::Dimension *dim){
      dim->id = dimensions.size();
      dimensions.push_back(dim);
      return 0;
    }
    CDF::Dimension * getDimension(const char *name){
      for(size_t j=0;j<dimensions.size();j++){
        if(dimensions[j]->name.equals(name)){
          return dimensions[j];
        }
      }
      throw(CDF_E_DIMNOTFOUND);
      return NULL;
    }
    CDF::Dimension * getDimensionNE(const char *name){
      try{return getDimension(name);}catch(int e){return NULL;}
    }

    int applyNCMLFile(const char * ncmlFileName){
      //The following ncml features have been implemented:
      // add a variable with <variable name=... type=.../>
      // add a attribute with <attribute name=... type=.../>
      // remove a variable with <remove name="..." type="variable"/>
      // remove a attribute with <remove name="..." type="attribute"/>
      // rename a variable with <variable name="LavaFlow" orgName="TDCSO2" />
      // rename a attribute with <attribute name="LavaFlow" orgName="TDCSO2" />
      int errorRaised=0;
      // Read the XML file and put the attributes into the data model
      xmlDoc *doc = NULL;
      NCMLVarName =NULL;
      xmlNode *root_element = NULL;
      LIBXML_TEST_VERSION;
      doc = xmlReadFile(ncmlFileName, NULL, 0);
      if (doc == NULL) {
        CDBError("Could not parse file \"%s\"", ncmlFileName);
        return 1;
      }
      root_element = xmlDocGetRootElement(doc);
      putNCMLAttributes(root_element);
      xmlFreeDoc(doc);
      xmlCleanupParser();
      if(errorRaised==1)return 1;
      return 0;
    }
    int aggregateDim(CDFObject *sourceCDFObject,const char *dimName){
      CDF::Variable *srcVar;
      CDF::Variable *destVar;
      for(size_t v=0;v<sourceCDFObject->variables.size();v++){
          //try{srcVar=sourceCDFObject->getVariable("ctt");}catch(int e){CDBError("Variable not found.");throw(__LINE__);}
        srcVar=sourceCDFObject->variables[v];
        
        try{
          try{destVar=getVariable(srcVar->name.c_str());}catch(int e){CDBError("Variable %s not found.",srcVar->name.c_str());throw(__LINE__);}
          
          try{
            destVar->setCDFObjectDim(srcVar,dimName);
          }catch(int e){char msg[255];CDF::getErrorMessage(msg,254,e);CDBError(msg);throw(__LINE__);}
        }catch(int e){
          CDBError("Unable to setCDFObjectDim for variable %s",srcVar->name.c_str());
          return 1;
        }
        
      }
      
      return 0;
    }
    CT::string currentFile;
    int open(const char *fileName) ;
    int close();
    void clear();
    int attachCDFReader(void *reader);
    void *getCDFReader(){return reader;}
};
namespace CDF{
  void dump(CDFObject* cdfObject,CT::string* dumpString);
  void dump(CDF::Variable* cdfVariable,CT::string* dumpString);
  CT::string dump(CDFObject* cdfObject);
  
  void _dumpPrintAttributes(const char *variableName, std::vector<CDF::Attribute *>attributes,CT::string *dumpString);
};

class CDFReader{
  public:
    CDFReader(){}
    virtual ~CDFReader(){}
    CDFObject *cdfObject;
    virtual int open(const char *fileName) = 0;
    virtual int close() = 0;
    
    //These two function may only be used by the variable class itself (TODO create friend class, protected?).
    virtual int _readVariableData(CDF::Variable *var, CDFType type) = 0;
    //Allocates and reads the variable data
    virtual int _readVariableData(CDF::Variable *var,CDFType type,size_t *start,size_t *count,ptrdiff_t  *stride) = 0;
};

#endif

