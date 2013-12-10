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

#ifndef CCDFNETCDFIO_H
#define CCDFNETCDFIO_H

#include <stdio.h>
#include <vector>
#include <iostream>
#include <netcdf.h>
#include <math.h>
#include "CCDFDataModel.h"
#include "CDebugger.h"

//#define CCDFNETCDFIO_DEBUG

class CDFNetCDFReader :public CDFReader{
  private:
    static void ncError(int line, const char *className, const char * msg,int e);  
    CT::string fileName;
    CCDFWarper warper;
  CDFType typeConversion(nc_type type){
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
  DEF_ERRORFUNCTION();
  int status,root_id;
  int nDims,nVars,nRootAttributes,unlimDimIdP;    
  bool keepFileOpen;
  int readDimensions(){
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
  int readAttributes(std::vector<CDF::Attribute *> &attributes,int varID,int natt){
    
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
  int readVariables(){
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

  public:
    CDFNetCDFReader():CDFReader(){
      root_id=-1;
        keepFileOpen=false;
    }
    ~CDFNetCDFReader(){
      close();
    }
    
    void enableLonWarp(bool enableLonWarp){
      warper.enableLonWarp=enableLonWarp;
    }
    
    int open(const char *fileName){
      this->fileName=fileName;
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
      CDBDebug("opening %s",fileName);
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
      
      return 0;
    }
    int close(){
      
      if(root_id!=-1){
#ifdef CCDFNETCDFIO_DEBUG        
CDBDebug("closing");
#endif        
        nc_close(root_id);
      }
      root_id=-1;
      return 0;
    }
    

    
    int _readVariableData(CDF::Variable *var, CDFType type);
    
    int _readVariableData(CDF::Variable *var, CDFType type,size_t *start,size_t *count,ptrdiff_t *stride);
};


class CDFNetCDFWriter{
  private:
    static void ncError(int line, const char *className, const char * msg,int e);  
    nc_type NCtypeConversion(CDFType  type){
      if(type==CDF_BYTE)return NC_BYTE;
      if(type==CDF_UBYTE)return NC_UBYTE;
      if(type==CDF_CHAR)return NC_CHAR;
      if(type==CDF_SHORT)return NC_SHORT;
      if(type==CDF_USHORT)return NC_USHORT;
      if(type==CDF_INT)return NC_INT;
      if(type==CDF_UINT)return NC_UINT;
      if(type==CDF_FLOAT)return NC_FLOAT;
      if(type==CDF_DOUBLE)return NC_DOUBLE;
      if(type==CDF_STRING)return NC_STRING;
      return NC_DOUBLE;
    }
    CT::string NCtypeConversionToString(CDFType  type){
      CT::string r;
      r="NC_DOUBLE";
      if(type==CDF_BYTE)r="NC_BYTE";
      if(type==CDF_UBYTE)r="NC_UBYTE";
      if(type==CDF_CHAR)r="NC_CHAR";
      if(type==CDF_SHORT)r="NC_SHORT";
      if(type==CDF_USHORT)r="NC_USHORT";
      if(type==CDF_INT)r="NC_INT";
      if(type==CDF_UINT)r="NC_UINT";
      if(type==CDF_FLOAT)r="NC_FLOAT";
      if(type==CDF_DOUBLE)r="NC_DOUBLE";
      if(type==CDF_STRING)r="NC_STRING";
      return r;
    }
    bool writeData;
    bool readData;
    bool listNCCommands;
    CT::string NCCommands;
    const char *fileName;
 
    CCDFWarper warper;
    
    int shuffle       ;
    int deflate       ;
    int deflate_level ;
  public:
    std::vector<CDF::Dimension *> dimensions;
    CDFObject *cdfObject;
    
    DEF_ERRORFUNCTION();
    int root_id,status;
    int netcdfMode;
    CDFNetCDFWriter(CDFObject *cdfObject){
      this->cdfObject=cdfObject;
      writeData=true;
      readData=true;
      listNCCommands=false;
      netcdfMode=4;
      shuffle=0;
      deflate=1;
      deflate_level=2;
    }
    ~CDFNetCDFWriter(){
      for(size_t j=0;j<dimensions.size();j++){delete dimensions[j];}
    }
    void setNetCDFMode(int mode){
      if(mode!=3&&mode!=4){
        CDBError("Illegal netcdf mode %d: keeping mode ",mode,netcdfMode);
        return;
      }
      netcdfMode=mode;
    }
    void disableVariableWrite(){
      writeData=false;
    }
    void disableReadData(){
      readData=false;
    }
    void recordNCCommands(bool enable){
      listNCCommands=enable;
    }
    
    
    
    CT::string getNCCommands(){
      return NCCommands;
    }

    int write(const char *fileName){
      NCCommands="";
      if(listNCCommands){
        NCCommands.printconcat("int root_id;\n");
        NCCommands.printconcat("size_t start[NC_MAX_DIMS];\n");
        NCCommands.printconcat("size_t count[NC_MAX_DIMS];\n");
        NCCommands.printconcat("int dimIDArray[NC_MAX_DIMS];\n");
        NCCommands.printconcat("int shuffle=%d;\n",shuffle);
        NCCommands.printconcat("int deflate=%d;\n",deflate);
        NCCommands.printconcat("int deflate_level=%d;\n",deflate_level);
        NCCommands.printconcat("int numDims=%d;\n",0);
        NCCommands.printconcat("void *variable_data=NULL;\n");
        
        for(size_t j=0;j<cdfObject->dimensions.size();j++){
            NCCommands.printconcat("int dim_id_%d;\n",j);
        }
        for(size_t j=0;j<cdfObject->variables.size();j++){
            NCCommands.printconcat("int var_id_%d;\n",j);
        }
      }
      
      this->fileName=fileName;
      #ifdef CCDFNETCDFIO_DEBUG                        
        CDBDebug("Writing to file %s",fileName);
      #endif  
      if(netcdfMode>3){
        status = nc_create(fileName ,NC_NETCDF4|NC_CLOBBER , &root_id);
        if(listNCCommands){
          NCCommands.printconcat("nc_create(\"%s\" ,NC_NETCDF4|NC_CLOBBER , &root_id);\n",fileName);
        }
      }else{
        status = nc_create(fileName , NC_CLOBBER|NC_64BIT_OFFSET, &root_id);
        if(listNCCommands){
          NCCommands.printconcat("nc_create(\"%s\" ,NC_CLOBBER|NC_64BIT_OFFSET , &root_id);\n",fileName);
        }
      }
      if(status!=NC_NOERR){ncError(__LINE__,className,"nc_create: ",status);nc_close(root_id);root_id=-1;return 1;}
      status = _write();
      #ifdef CCDFNETCDFIO_DEBUG                        
        CDBDebug("Finished writing to file %s",fileName);
      #endif  

      nc_close(root_id);root_id=-1;
      if(listNCCommands){
        NCCommands.printconcat("nc_close(root_id);\n");
      }
      
      
      
      return status;
    }
    int _write(){
      #ifdef CCDFNETCDFIO_DEBUG                        
        CDBDebug("Writing global attributes");
      #endif  

      //Write global attributes
      for(size_t i=0;i<cdfObject->attributes.size();i++){
        status = nc_put_att(root_id, NC_GLOBAL, cdfObject->attributes[i]->name.c_str(),NCtypeConversion(cdfObject->attributes[i]->getType()),cdfObject->attributes[i]->length,cdfObject->attributes[i]->data);
        if(listNCCommands){
                    
                    void *data=cdfObject->attributes[i]->data;
                    size_t length=cdfObject->attributes[i]->length;
                    CDFType type=cdfObject->attributes[i]->getType();
                    if(type==CDF_CHAR||type==CDF_UBYTE||type==CDF_BYTE){
                      CT::string out="";
                      out.concat((const char*)data,length);
                      NCCommands.printconcat("nc_put_att(root_id, NC_GLOBAL, \"%s\",%s,%d,\"%s\");\n",cdfObject->attributes[i]->name.c_str(),NCtypeConversionToString(type).c_str(),cdfObject->attributes[i]->length,out.c_str());
                    }else{
                      if(type==CDF_INT||type==CDF_UINT){
                        NCCommands.printconcat("int attrData_%d[]={",i);
                        for(size_t n=0;n<length;n++){
                          NCCommands.printconcat("%d",((int*)data)[n]);
                          if(n<length-1){
                            NCCommands.printconcat(",");
                          }
                          NCCommands.printconcat("};\n");
                        }
                      }
                      
                      if(type==CDF_SHORT||type==CDF_USHORT){
                        NCCommands.printconcat("short attrData_%d[]={",i);
                        for(size_t n=0;n<length;n++){
                          NCCommands.printconcat("%d",((short*)data)[n]);
                          if(n<length-1){
                            NCCommands.printconcat(",");
                          }
                          NCCommands.printconcat("};\n");
                        }
                      }
                      
                            
                      if(type==CDF_FLOAT){
                        NCCommands.printconcat("float attrData_%d[]={",i);
                        for(size_t n=0;n<length;n++){
                          NCCommands.printconcat("%f",((float*)data)[n]);
                          if(n<length-1){
                            NCCommands.printconcat(",");
                          }
                          NCCommands.printconcat("};\n");
                        }
                      }

                      if(type==CDF_DOUBLE){
                        NCCommands.printconcat("float attrData_%d[]={",i);
                        for(size_t n=0;n<length;n++){
                          NCCommands.printconcat("%f",((double*)data)[n]);
                          if(n<length-1){
                            NCCommands.printconcat(",");
                          }
                          NCCommands.printconcat("};\n");
                        }
                      }
                      NCCommands.printconcat("nc_put_att(root_id, NC_GLOBAL, \"%s\",%s,%d,attrData_%d_%d);\n",cdfObject->attributes[i]->name.c_str(),NCtypeConversionToString(type).c_str(),cdfObject->attributes[i]->length,i);
                    }
                    

        }
        
        if(status!=NC_NOERR){
          char name[1023];
          CDF::getCDFDataTypeName(name,1000,cdfObject->attributes[i]->getType());
          CDBError("For attribute NC_GLOBAL::%s of type %s:",cdfObject->attributes[i]->name.c_str(),name);
          ncError(__LINE__,className,"nc_put_att: ",status);return 1;
        }
      }
      #ifdef CCDFNETCDFIO_DEBUG                        
        CDBDebug("Define dimensions");
      #endif  

      //Define dimensions
      for(size_t j=0;j<cdfObject->dimensions.size();j++){
        CDF::Dimension * dim = new CDF::Dimension();
        dim->setName(cdfObject->dimensions[j]->name.c_str());
        dim->length=cdfObject->dimensions[j]->length;
        status = nc_def_dim(root_id,dim->name.c_str() , dim->length, &dim->id);
        if(listNCCommands){
          NCCommands.printconcat("nc_def_dim(root_id,\"%s\" , %d, &dim_id_%d);\n",dim->name.c_str(),dim->length,j);
        } 
        if(status!=NC_NOERR){ncError(__LINE__,className,"nc_def_dim: ",status);delete dim;return 1;}
        dimensions.push_back(dim);
      }
   
    
      int nrVarsWritten=0;
      //Write the variables
      //First write the variables connected to dimensions and later the variables itself
      //writeDimsFirst==0: dimension variables
      //writeDimsFirst==1: variables
      for(int writeDimsFirst=0;writeDimsFirst<2;writeDimsFirst++){
        #ifdef CCDFNETCDFIO_DEBUG                        
          if(writeDimsFirst==0)CDBDebug("Write dimensions");
          if(writeDimsFirst==1)CDBDebug("Write variables");
        #endif  

        
        //Write all different variables.
        //nc_close(root_id);
        //status = nc_set_chunk_cache(0,0,0);
        //if(status!=NC_NOERR){ncError(__LINE__,className,"nc_set_chunk_cache: ",status);return 1;}
  
        //status = nc_open(fileName ,netcdfWriteMode|NC_WRITE, &root_id);
        //if(status!=NC_NOERR){ncError(__LINE__,className,"nc_create: ",status);nc_close(root_id);return 1;}
        for(size_t j=0;j<cdfObject->variables.size();j++){
        //for(size_t j=0;j<4;j++){          
          //Get the variable names with these dimensions 
          CDF::Variable *variable = cdfObject->variables[j];
          const char *name = variable->name.c_str();
          #ifdef CCDFNETCDFIO_DEBUG                        
            if(writeDimsFirst==0)CDBDebug("Writing %s",name);
          #endif  

          int numDims=variable->dimensionlinks.size();
          if((variable->isDimension==true &&writeDimsFirst==0)||
             (variable->isDimension==false&&writeDimsFirst==1)){
            {
              int dimIDS[numDims+1];
              int NCCommandID[numDims+1];
              //size_t chunkSizes[numDims+1];
              size_t totalVariableSize = 0;
              //Find dim and chunk info
              CT::string variableInfo(name);
              variableInfo.concat("\t(");
              for(int i=0;i<numDims;i++){
                for(size_t k=0;k<dimensions.size();k++){
                  if(dimensions[k]->name.equals(&variable->dimensionlinks[i]->name)){
                    dimIDS[i]=dimensions[k]->id;
                    NCCommandID[i]=k;
                    if(totalVariableSize==0)totalVariableSize=1;
                    //CDBDebug("EQUALS: %s %d",dimensions[k]->name.c_str(),dimIDS[i]);  
                    totalVariableSize*=dimensions[k]->length;
                    //chunkSizes[i]=dimensions[k]->length;
                    variableInfo.printconcat("%s=%d",dimensions[k]->name.c_str(),dimensions[k]->length);
                    if(i+1<numDims)variableInfo.concat(",");
                  }
                }
              }
              variableInfo.concat(")");
              int nc_var_id;
              status = nc_redef(root_id);
              if(listNCCommands){
                NCCommands.printconcat("nc_redef(root_id);\n");
              } 
              //if(status!=NC_NOERR){ncError(__LINE__,className,"nc_redef: ",status);return 1;}
              /*for(int j=0;j<numDims;j++){
                printf("%d\n",dimIDS[j]);
            }*/
              status = nc_def_var(root_id, name    ,NCtypeConversion(variable->currentType),numDims, dimIDS,&nc_var_id);
              if(status!=NC_NOERR){CDBError("Unable to define variable %s",name);ncError(__LINE__,className,"nc_def_var: ",status);return 1;}
              if(listNCCommands){
                
                NCCommands.printconcat("numDims=%d;\n",numDims);
                for(int k=0;k<numDims;k++){
                  NCCommands.printconcat("dimIDArray[%d]=dim_id_%d;\n",k,NCCommandID[k]);
                }
                NCCommands.printconcat("nc_def_var(root_id, \"%s\",%s,numDims, dimIDArray,&var_id_%d);\n",name,NCtypeConversionToString(variable->currentType).c_str(),j);
                
              } 

              //Set chunking and deflate options
              //chunkSizes[0]=1;
              if(netcdfMode>=4&&numDims>0&&1==1){
          
                //status = nc_def_var_chunking(root_id,nc_var_id,0 ,chunkSizes);
                //if(status!=NC_NOERR){ncError(__LINE__,className,"nc_def_var_chunking: ",status);return 1;}
                //status = nc_def_var_deflate(root_id,nc_var_id,shuffle ,deflate, deflate_level);
                //if(status!=NC_NOERR){ncError(__LINE__,className,"nc_def_var_deflate: ",status);return 1;}
                //if(listNCCommands){
                  //NCCommands.printconcat("nc_def_var_deflate(root_id,var_id_%d,shuffle ,deflate, deflate_level);\n",j);
                //} 
                
              }
              
              //copy data
              #ifdef CCDFNETCDFIO_DEBUG     
              CT::string message;
              message.print("%d/%d Copying data for variable %s: total %d bytes",
                            nrVarsWritten+1,cdfObject->variables.size(),variableInfo.c_str(),int(totalVariableSize)*CDF::getTypeSize(variable->getType()));
              CDBDebug("%s",message.c_str());
              #endif
              //Copy attributes for this specific variable
              for(size_t i=0;i<variable->attributes.size();i++){
                if(!variable->attributes[i]->name.equals("CLASS")){
                  nc_type type=NCtypeConversion(variable->attributes[i]->getType());
                  status = nc_put_att(root_id, nc_var_id, variable->attributes[i]->name.c_str(),
                                      type,variable->attributes[i]->length,
                                      variable->attributes[i]->data);
                  if(listNCCommands){
          
                    void *data=variable->attributes[i]->data;
                    size_t length=variable->attributes[i]->length;
                    if(type==CDF_CHAR||type==CDF_UBYTE||type==CDF_BYTE){
                      CT::string out="";
                      out.concat((const char*)data,length);
                      NCCommands.printconcat("nc_put_att(root_id, var_id_%d, \"%s\",%s,%d,\"%s\");\n",j,variable->attributes[i]->name.c_str(),NCtypeConversionToString(type).c_str(),variable->attributes[i]->length,out.c_str());
                    }else{
                      if(type==CDF_INT||type==CDF_UINT){
                        NCCommands.printconcat("int attrData_%d_%d[]={",j,i);
                        for(size_t n=0;n<length;n++){
                          NCCommands.printconcat("%d",((int*)data)[n]);
                          if(n<length-1){
                            NCCommands.printconcat(",");
                          }
                          NCCommands.printconcat("};\n");
                        }
                      }
                      
                      if(type==CDF_SHORT||type==CDF_USHORT){
                        NCCommands.printconcat("short attrData_%d_%d[]={",j,i);
                        for(size_t n=0;n<length;n++){
                          NCCommands.printconcat("%d",((short*)data)[n]);
                          if(n<length-1){
                            NCCommands.printconcat(",");
                          }
                          NCCommands.printconcat("};\n");
                        }
                      }
                      
                            
                      if(type==CDF_FLOAT){
                        NCCommands.printconcat("float attrData_%d_%d[]={",j,i);
                        for(size_t n=0;n<length;n++){
                          NCCommands.printconcat("%f",((float*)data)[n]);
                          if(n<length-1){
                            NCCommands.printconcat(",");
                          }
                          NCCommands.printconcat("};\n");
                        }
                      }

                      if(type==CDF_DOUBLE){
                        NCCommands.printconcat("float attrData_%d_%d[]={",j,i);
                        for(size_t n=0;n<length;n++){
                          NCCommands.printconcat("%f",((double*)data)[n]);
                          if(n<length-1){
                            NCCommands.printconcat(",");
                          }
                          NCCommands.printconcat("};\n");
                        }
                      }
                      NCCommands.printconcat("nc_put_att(root_id, var_id_%d, \"%s\",%s,%d,attrData_%d_%d);\n",j,variable->attributes[i]->name.c_str(),NCtypeConversionToString(type).c_str(),variable->attributes[i]->length,j,i);
                    }
                    
                  }
                  
                  if(status!=NC_NOERR){
                    char attrType[256],varType[256];
                    CDF::getCDFDataTypeName(attrType,255,variable->attributes[i]->getType());
                    CDF::getCDFDataTypeName(varType,255,variable->currentType);
                    CDBError("Trying to write attribute %s with type %s for variable %s with type %s\nnc_put_att: %s",
                            variable->attributes[i]->name.c_str(),attrType,variable->name.c_str(),varType,nc_strerror(status));return 1;
                  }
                }else {
                  //CDBDebug("Skipping attribute %s:%s",variable->name.c_str(),variable->attributes[i]->name.c_str());
                }
              }
              if((numDims>0&&writeData==true)||(variable->isDimension&&numDims==1)){
                bool needsDimIteration=false;
                int iterativeDimIndex=variable->getIterativeDimIndex();
                if(iterativeDimIndex!=-1)needsDimIteration=true;
                if(variable->isDimension)needsDimIteration=false;
                size_t start[NC_MAX_DIMS],count[NC_MAX_DIMS];
                for(size_t j=0;j<variable->dimensionlinks.size();j++){
                  start[j]=0;count[j]=variable->dimensionlinks[j]->getSize();
                }
                status = nc_enddef(root_id);
                if(status!=NC_NOERR){
                  CDBError("For variable %s:",variable->name.c_str());
                  ncError(__LINE__,className,"nc_enddef: ",status);return 1;
                }
                if(listNCCommands){
                  NCCommands.printconcat("nc_enddef(root_id);\n");
                } 
                
#ifdef CCDFNETCDFIO_DEBUG                
                 CDBDebug("--- Copying Variable %s. needsDimIteration = %d---",variable->name.c_str(),needsDimIteration);
#endif                 
                if(needsDimIteration==false){
                  int status = copyVar(variable,nc_var_id,start,count);
                  if(status!=0)return status;
                }else{
                  for(size_t id=0;id<variable->dimensionlinks[iterativeDimIndex]->getSize();id++){
                    #ifdef CCDFNETCDFIO_DEBUG    
                    CDBDebug("  %d/%d Copying Iterative dim '%s' with index %d for variable %s",nrVarsWritten+1,cdfObject->variables.size(),variable->dimensionlinks[iterativeDimIndex]->name.c_str(),id,variable->name.c_str());
                    #endif                 
                    start[iterativeDimIndex]=id;count[iterativeDimIndex]=1;
                    int status = copyVar(variable,nc_var_id,start,count);
                    if(status!=0)return status;
                  }
                }
                nc_sync(root_id);
                if(listNCCommands){
                  NCCommands.printconcat("nc_sync(root_id);\n");
                } 
                //CDBDebug("DONE!");
              }
              nrVarsWritten++;
            }
          }
        }
      }
      return 0;
    }
    int copyVar(CDF::Variable *variable,int nc_var_id,size_t *start, size_t *count){
      ptrdiff_t stride[NC_MAX_DIMS];
      for(size_t j=0;j<variable->dimensionlinks.size();j++)stride[j]=1;
          
         if(readData==true){
        /*
          CDFReader *cdfReader = (CDFReader *)variable->getCDFReaderPointer();
        if(cdfReader!=NULL){
          status = cdfReader->readVariableData(variable, variable->getType());
          if(status!=0){
            CDBError("Reading of variable %s failed",variable->name.c_str());
            return 1;
          }
        }*/
        //TODO should read iterative for iterative dims.

        
        status = variable->readData(variable->currentType,start,count,stride);
        if(status!=0){
          CDBError("Reading of variable %s failed",variable->name.c_str());
          return 1;
        }
        if(variable->data==NULL){
          CDBError("variable->data == NULL for variable %s",variable->name.c_str());
          return 1;
        }else{
          //CDBWarning("No cdfReaderPointer defined for variable %s",variable->name.c_str());
        }
#ifdef CCDFNETCDFIO_DEBUG                        
        CDBDebug("Variable %s read",variable->name.c_str());
#endif        
      
        //Apply longitude warping of the data
        //EG 0-360 to -180 till -180
        warper.warpLonData(variable);
      }
      if(status==0){
        if(variable->data==NULL){
          CDBError("variable->data==NULL for %s",variable->name.c_str());
          return 1;
        }
        //CDBDebug("Writing %d elements",variable->getSize());
#ifdef CCDFNETCDFIO_DEBUG                        
        for(size_t i=0;i<variable->dimensionlinks.size();i++){
          CDBDebug("Writing %s,%d: %d %d\t\t[%d]",variable->name.c_str(),i,start[i],count[i],variable->getSize());
        }
#endif        
        
       
        status = nc_put_vara(root_id,nc_var_id,start,count,variable->data);
        if(listNCCommands){
          NCCommands.printconcat("//");
          for(size_t j=0;j<variable->dimensionlinks.size();j++){
            NCCommands.printconcat("%s\t",variable->dimensionlinks[j]->name.c_str());
          }
           NCCommands.printconcat("\n");
          for(size_t j=0;j<variable->dimensionlinks.size();j++){
            NCCommands.printconcat("start[%d]=%d;\t",j,start[j]);
          }
          NCCommands.printconcat("\n");
          for(size_t j=0;j<variable->dimensionlinks.size();j++){
            NCCommands.printconcat("count[%d]=%d;\t",j,count[j]);
          }
          NCCommands.printconcat("\n");
          NCCommands.printconcat("//variable_data should be defined here\n");
          NCCommands.printconcat("//nc_put_vara(root_id,var_id_%d,start,count,variable_data);\n",nc_var_id);
        } 
        //printf("Fake put vara\n");
        if(status!=NC_NOERR){
          CDBError("For variable %s:",variable->name.c_str());
          ncError(__LINE__,className,"nc_put_var: ",status);return 1;}
      }
      //Free the variable data
      //if(variable->isDimension==false&&
        if(readData==true){
#ifdef CCDFNETCDFIO_DEBUG                        
        CDBDebug("Free variable %s",variable->name.c_str());
#endif        
        if(!variable->isDimension)variable->freeData();
      }
      return 0;
    }
};

#endif
