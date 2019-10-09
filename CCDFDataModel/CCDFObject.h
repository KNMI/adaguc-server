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

#ifndef CCDFOBJECT_H
#define CCDFOBJECT_H


#include "CCDFVariable.h"
#include "CDebugger_H2.h"


class CDFObject:public CDF::Variable{
  public:
    CDFType ncmlTypeToCDFType(const char *type);
  private:
    
    void putNCMLAttributes(void * a_node);
      
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
    
      
    /**
      * Returns the variable for given name. Throws error code  when something goes wrong
      * @param name The name of the dimension to look for
      * @return The variable pointer
      */    
    CDF::Variable *getVariable(const char *name){
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
    
   CDF::Variable *getVariableIgnoreCase(const char *name){
      if(strncmp("NC_GLOBAL",name,9)==0){
        return this;
      }
      for(size_t j=0;j<variables.size();j++){
        if(variables[j]->name.equalsIgnoreCase(name)){
          return variables[j];
        }
      }
      throw(CDF_E_VARNOTFOUND);
      return NULL;
    }
    
    CDF::Variable * getVariableNE(const char *name){
      try{return getVariable(name);}catch(int e){return NULL;}
    }
    CDF::Variable* addVariable(CDF::Variable *var){
      var->id = variables.size();
      var->setParentCDFObject(this);
      variables.push_back(var);
      return var;
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
    CDF::Dimension* addDimension(CDF::Dimension *dim){
      dim->id = dimensions.size();
      dimensions.push_back(dim);
      return dim;
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
    
    CDF::Dimension * getDimensionIgnoreCase(const char *name){
      for(size_t j=0;j<dimensions.size();j++){
        if(dimensions[j]->name.equalsIgnoreCase(name)){
          return dimensions[j];
        }
      }
      throw(CDF_E_DIMNOTFOUND);
      return NULL;
    }
    
    CDF::Dimension * getDimensionNE(const char *name){
      try{return getDimension(name);}catch(int e){return NULL;}
    }

    int applyNCMLFile(const char * ncmlFileName);
    int aggregateDim(CDFObject *sourceCDFObject,const char *dimName){
      CDF::Variable *srcVar;
      CDF::Variable *destVar;
      for(size_t v=0;v<sourceCDFObject->variables.size();v++){
          //try{srcVar=sourceCDFObject->getVariable("ctt");}catch(int e){CDBError("Variable not found.");throw(__LINE__);}
        srcVar=sourceCDFObject->variables[v];
        //if(getVariableNE(srcVar->name.c_str())!=NULL)
        {
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


#endif
