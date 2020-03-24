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

#ifndef CCDFVARIABLE_H
#define CCDFVARIABLE_H

#include "CCDFTypes.h"
#include "CCDFAttribute.h"
#include "CCDFDimension.h"
 //#define CCDFDATAMODEL_DEBUG
#include "CDebugger_H2.h"
namespace CDF{
  class Variable{
    
    public:
      CDFType nativeType;
      CDFType currentType;
      CT::string name;
      CT::string orgName;
      std::vector<Attribute *> attributes;
      std::vector<Dimension *> dimensionlinks;
      int id;
      size_t currentSize;
      void *data;
      bool isDimension;
      
  
      //Currently, aggregation along just 1 dimension is supported.
      class CDFObjectClass{
      public:
        void *cdfObjectPointer;
        int dimIndex;
        CT::string dimValue;
      };  private:
    std::vector<CDFObjectClass *> cdfObjectList;
    void *cdfReaderPointer;
    void *parentCDFObject;
    bool hasCustomReader;

    public:
      class CustomReader{
        public:
          virtual ~CustomReader(){}
          virtual int readData(CDF::Variable *thisVar,size_t *start,size_t *count,ptrdiff_t *stride) = 0;
      };
    class CustomMemoryReader:public CDF::Variable::CustomReader{
      public:
        ~CustomMemoryReader(){
          
        }
        int readData(CDF::Variable *thisVar,size_t *start,size_t *count,ptrdiff_t *stride){
          int size = 1;
          for(size_t j=0;j<thisVar->dimensionlinks.size();j++){
            size*=int((float(count[j])/float(stride[j]))+0.5);
          }
          thisVar->setSize(size);
          CDF::allocateData(thisVar->getType(),&thisVar->data,size);
          //CDF::fill(thisVar->data, thisVar->getType(),12345,size);//Should be done by followup code.
          return 0;
        }
    };
    static CustomMemoryReader*CustomMemoryReaderInstance;
  private:
        CustomReader * customReader;
        bool _isString;
  public:
      void setCustomReader(CustomReader *customReader){
        hasCustomReader=true;
        this->customReader = customReader;
      };

   
      
      void setCDFReaderPointer(void *cdfReaderPointer){
        this->cdfReaderPointer=cdfReaderPointer;
      }
      void setParentCDFObject(void *parentCDFObject){
        this->parentCDFObject=parentCDFObject;
      }
      void * getParentCDFObject() const {
        if(parentCDFObject==NULL){
          throw(CDF_E_VARHASNOPARENT);
        }
        return parentCDFObject;
      }
      void *getCDFObjectClassPointer(size_t *start,size_t *count){
        if(cdfObjectList.size()==0){
#ifdef CCDFDATAMODEL_DEBUG
          CDBDebug("returning getParentCDFObject because cdfObjectList");
#endif          
          return getParentCDFObject();
        }
        if(start == NULL || count == NULL){
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
        if(iterativeDimIndex>=cdfObjectList.size()){
          CDBError("Wrong index %d, list size is %d",iterativeDimIndex,cdfObjectList.size());
        }
        return cdfObjectList[iterativeDimIndex];//->cdfObjectPointer;
      }

      void setCDFObjectDim(Variable *sourceVar,const char *dimName);
      
      void allocateData(size_t size){
        if(data!=NULL){CDF::freeData(&data);};data=NULL;
        CDF::allocateData(currentType,&data,size);
        setSize(size);
      }
      
      void freeData(){
        if(data==NULL)return;
#ifdef CCDFDATAMODEL_DEBUG                
        char typeName[255];getCDFDataTypeName(typeName,254,getType());
        CDBDebug("Freeing %d elements of type %s for variable %s",getSize(),typeName,name.c_str());
#endif        
        if(data!=NULL){CDF::freeData(&data);data=NULL;}
        setSize(0);
      }
      
      int readData(CDFType type);
      int readData(bool applyScaleOffset);
      int readData(CDFType type,bool applyScaleOffset);
      int readData(CDFType type,size_t *_start,size_t *_count,ptrdiff_t *stride);
      int readData(CDFType type,size_t *_start,size_t *_count,ptrdiff_t *stride,bool applyScaleOffset);
      
      
       template <class T>
      T getDataAt(int index){
        if(data==NULL){
          throw(CDF_E_VARHASNODATA);
        }
        T dataElement = 0;
        if(currentType == CDF_CHAR)dataElement=(T)((char*)data)[index];
        if(currentType == CDF_BYTE)dataElement=(T)((char*)data)[index];
        if(currentType == CDF_UBYTE)dataElement=(T)((unsigned char*)data)[index];
        if(currentType == CDF_SHORT)dataElement=(T)((short*)data)[index];
        if(currentType == CDF_USHORT)dataElement=(T)((ushort*)data)[index];
        if(currentType == CDF_INT)dataElement=(T)((int*)data)[index];
        if(currentType == CDF_UINT)dataElement=(T)((unsigned int*)data)[index];
        if(currentType == CDF_INT64)dataElement=(T)((long*)data)[index];
        if(currentType == CDF_UINT64)dataElement=(T)((unsigned long*)data)[index];
        if(currentType == CDF_FLOAT)dataElement=(T)((float*)data)[index];
        if(currentType == CDF_DOUBLE)dataElement=(T)((double*)data)[index];
        
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
      Variable(const char *name, CDFType type, CDF::Dimension *dims[], int numdims, bool isCoordinateVariable){
        isDimension=false;
        data = NULL;
        currentSize=0;
        currentType=CDF_NONE;
        nativeType=CDF_NONE;
        cdfReaderPointer=NULL;
        parentCDFObject=NULL;
        hasCustomReader = false;
        _isString = false;
        //CDBDebug("Variable");
        setName(name);
        setType(type);
        //CDBDebug("Iterating dims[%d]",numdims);
        for(int j=0;j<numdims;j++){
          //CDBDebug("Iterating dims %s",dims[j]->getName().c_str());
          dimensionlinks.push_back(dims[j]);
        }
        isDimension = isCoordinateVariable;
        //CDBDebug("done");
      }
      Variable(){
        isDimension=false;
        data = NULL;
        currentSize=0;
        currentType=CDF_NONE;
        nativeType=CDF_NONE;
        cdfReaderPointer=NULL;
        parentCDFObject=NULL;
        hasCustomReader = false;
        _isString = false;
      }
      ~Variable(){
        
        
        for(size_t j=0;j<attributes.size();j++){if(attributes[j]!=NULL){delete attributes[j];attributes[j]=NULL;}}
        if(currentType==CDF_STRING){
          //CDBDebug("~Variable() %s %d",name.c_str(),getSize());
          for(size_t j=0;j<getSize();j++){
            //CDBDebug("%s\n",((const char**)data)[j]);
            free(((char**)data)[j]);
            ((char**)data)[j]=NULL;
          }
        }
        if(data!=NULL){CDF::freeData(&data);data=NULL;}
        for(size_t j=0;j<cdfObjectList.size();j++){if(cdfObjectList[j]!=NULL){delete cdfObjectList[j];cdfObjectList[j]=NULL;}}
      }
    
      CDFType getType(){
        return currentType;
      };
      CDFType getNativeType(){
        return nativeType;
      };
      void setType(CDFType type){
        currentType = type;
        if(nativeType == CDF_NONE)nativeType =type;
      };
      
      
      bool isString(){
        return _isString;
      }
      bool isString(bool isString){
        _isString=isString;
        return _isString;
      }
      
     
      void setName(const char *value){
        name.copy(value);
        //TODO Implement this correctly in readvariabledata....
        if(orgName.length()==0)orgName.copy(value);
      }

      void setSize(size_t size){
        currentSize = size;
      }
      size_t getSize(){
        return currentSize;
      }

      
      Attribute * getAttribute(const char *name) const {
        for(size_t j=0;j<attributes.size();j++){
          if(attributes[j]->name.equals(name)){
            return attributes[j];
          }
        }
        throw(CDF_E_ATTNOTFOUND);
        return NULL;
      }
      Attribute * getAttributeNE(const char *name) const {try{return getAttribute(name);}catch(int e){return NULL;}}
      
      /**
       * Returns the dimension for given name. Throws error code  when something goes wrong
       * @param name The name of the dimension to look for
       * @return Pointer to the dimension
       */
      Dimension* getDimension(const char *name){
        for(size_t j=0;j<dimensionlinks.size();j++){
          if(dimensionlinks[j]->name.equals(name)){
            return dimensionlinks[j];
          }
        }
        throw(CDF_E_DIMNOTFOUND);
        return NULL;
      }
      
      Dimension* getDimensionIgnoreCase(const char *name){
        for(size_t j=0;j<dimensionlinks.size();j++){
          if(dimensionlinks[j]->name.equalsIgnoreCase(name)){
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
      
      template <class T>
      int setAttribute(const char *attrName,CDFType attrType,T data){ 
        Attribute *attr;
        try{
          attr=getAttribute(attrName);
        }catch(...){
          attr = new Attribute();
          attr->name.copy(attrName);
          addAttribute(attr);
        }
        attr->type=attrType;
        attr->setData(attrType,data);
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
        if(data!=NULL){CDF::freeData(&data);};data=NULL;
        currentSize=dataLength;
        this->currentType=type;
        this->nativeType=type;
        CDF::allocateData(type,&data,currentSize);
        if(type==CDF_CHAR||type==CDF_UBYTE||type==CDF_BYTE)memcpy(data,dataToSet,currentSize);
        if(type==CDF_SHORT||type==CDF_USHORT)memcpy(data,dataToSet,currentSize*sizeof(short));
        if(type==CDF_INT||type==CDF_UINT)memcpy(data,dataToSet,currentSize*sizeof(int));
        if(type==CDF_INT64||type==CDF_UINT64)memcpy(data,dataToSet,currentSize*sizeof(long));
        if(type==CDF_FLOAT)memcpy(data,dataToSet,currentSize*sizeof(float));
        if(type==CDF_DOUBLE){memcpy(data,dataToSet,currentSize*sizeof(double));}
        return 0;
      }
      
       //Returns a new copy of this variable
      /*Variable *clone(){
        Variable *newVar = new Variable ();

        
        
        
        
        
        
        
      newVar->nativeType = nativeType;
      newVar->currentType;
      newVar->name;
      newVar->orgName;
      std::vector<Attribute *> attributes;
      std::vector<Dimension *> dimensionlinks;
      int id;
      size_t currentSize;
      void *data;
      bool isDimension;
      
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
    bool hasCustomReader;
        return newVar;
      }*/
  };
}
#endif
