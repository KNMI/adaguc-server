/******************************************************************************
 * 
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
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

#include <stdio.h>
#include <vector>
#include <iostream>
#include <CTypes.h>
#include "CDebugger.h" 
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"
#include "CCDFHDF5IO.h"
#include "CCDFGeoJSONIO.h"
#include "CCDFCSVReader.h"

DEF_ERRORMAIN();

int main(int argCount,char **argVars){
    CDFReader*cdfReader = NULL;
    CDFObject *cdfObject = NULL;
    if(argCount<=2){
      printf("anydump [-h] [-json] [-v <variable name>] [-ncml <ncml file>] file\n");
      printf("File can be any of the ADAGUC supported files, like NetCDF, HDF5, PNG, GeoJSON, CSV\n");
      printf("  [-h]                  Header information in ncdump format.\n");
      printf("  [-json]               Header information in ADAGUC json format\n");
      printf("  [-v <variable name>]  Dump specific variable\n");
      printf("  [-ncml <ncml file>]   Dump with ncml file\n");
      return 0;
    }
    
    
    CT::string variableName, ncmlFile;
    bool dumpHeader = false;
    bool dumpAsJSON = false;

    for(int j=0;j< argCount; j++) {
      CT::string cmdType = argVars[j];
      if(cmdType.equals("-h")) dumpHeader = true;
      if(cmdType.equals("-json")) dumpAsJSON = true;
      if(cmdType.equals("-v")) {
        if(j + 1 >= argCount){
          CDBError("Not enough arguments, please specify the variable");
          return 1;
        }
        variableName = argVars[j + 1];
      }
      if(cmdType.equals("-ncml")) {
        if(j + 1 >= argCount){
          CDBError("Not enough arguments, please specify the ncml file");
          return 1;
        }
        ncmlFile = argVars[j + 1];
      }
    }
    
    CT::string inputFile=argVars[argCount-1];//"/nobackup/users/plieger/projects/msgcpp/oud/meteosat9.fl.geo.h5";
    
    
    
    int status = 0;
    try{
      cdfObject=new CDFObject();
      if(inputFile.endsWith(".nc")){
        cdfReader = new CDFNetCDFReader() ;
      }
      if(inputFile.endsWith(".h5")){
        cdfReader = new CDFHDF5Reader();
      }
      if(inputFile.endsWith(".geojson")){
        cdfReader = new CDFGeoJSONReader();
      }
      if(inputFile.endsWith(".csv")){
        cdfReader = new CDFCSVReader();
      }
      if (cdfReader == NULL){
        CDBError("Unrecognized extension");
        throw __LINE__;
      }
      cdfObject->attachCDFReader(cdfReader);
      status = cdfReader->open(inputFile.c_str());
      if(status != 0){CDBError("Unable to read file %s",inputFile.c_str());throw(__LINE__);}
      
      if (!ncmlFile.empty()) {
        CDBDebug("Applying NCML [%s]", ncmlFile.c_str());
        cdfObject->applyNCMLFile(ncmlFile.c_str());
      }

      if(dumpHeader && !dumpAsJSON) {
        CT::string dumpString = CDF::dump(cdfObject);
        printf("%s\n",dumpString.c_str());
      }
      if(dumpAsJSON) {
        CT::string dumpString = CDF::dumpAsJSON(cdfObject);
        printf("%s\n",dumpString.c_str());
      }

      if(!variableName.empty()) {
        CDF::Variable *var = cdfObject->getVariableNE(variableName.c_str());
        if(var == NULL){
          CDBError("Variable not found");
          throw(__LINE__);
        }
        
        printf("// Data dump for variable [%s]:\n", var->name.c_str());
        CT::string dumpString = CDF::dump(var);
        printf("%s\n",dumpString.c_str());
        
        bool isString = var->getNativeType()==CDF_STRING;
        
        if(!isString){
          var->readData(var->getNativeType());
          printf("[");
          for(size_t j=0;j<var->getSize();j++){
            if(var->dimensionlinks.size()>0){
              size_t firstDimSize = var->dimensionlinks[var->dimensionlinks.size()-1]->getSize();
              if(j%firstDimSize==0&&j!=0){
                printf("]\n[");
              }
            }
            if (var->getNativeType() == CDF_CHAR){
              printf("%c",((char*)var->data)[j]);
            } if (var->getNativeType() == CDF_SHORT || var->getNativeType() == CDF_USHORT){
              printf("%d",((short*)var->data)[j]);
            } if (var->getNativeType() == CDF_INT || var->getNativeType() == CDF_UINT){
              printf("%d",((int*)var->data)[j]);
            } if (var->getNativeType() == CDF_INT64 || var->getNativeType() == CDF_UINT64){
              printf("%ld",((long*)var->data)[j]);
            } else if (var->getNativeType() == CDF_FLOAT){
              printf("%g, ",((float*)var->data)[j]);
            } else if (var->getNativeType() == CDF_DOUBLE){
              printf("%g, ",((double*)var->data)[j]);
            }
          }
          printf("]\n");
        }else{
          
          var->readData(CDF_STRING);
          printf("[");
          
          for(size_t j=0;j<var->getSize();j++){
            if(var->dimensionlinks.size()>0){
              size_t firstDimSize = var->dimensionlinks[var->dimensionlinks.size()-1]->getSize();
              if(j%firstDimSize==0&&j!=0){
                printf("]\n[");
              }
            }
            printf("\"%s\",\n ",((char**)var->data)[j]);
          }
          printf("]\n");
        }
        
        
      }
      
      
      
  
      delete cdfReader;cdfReader=NULL;
      delete cdfObject;cdfObject=NULL;
    }
    catch(int e){
      delete cdfReader;cdfReader=NULL;
      delete cdfObject;cdfObject=NULL;
    }

  CTime::cleanInstances();
  return 0;
}
