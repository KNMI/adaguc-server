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

#include "CCDFDataModel.h"

void CDF::_dumpPrintAttributes(const char *variableName, std::vector<CDF::Attribute *>attributes,CT::string *dumpString){
  //print attributes:
  for(size_t a=0;a<attributes.size();a++){
    CDF::Attribute *attr=attributes[a];
    if(attr->type!=CDF_STRING){
      dumpString->printconcat("\t\t%s:%s =",variableName,attr->name.c_str());
    }else{
      dumpString->printconcat("\t\tstring %s:%s =",variableName,attr->name.c_str());
    }
    
    //print data
    if(attr->type==CDF_CHAR){
      char *data = new char[attr->length+1];
      memcpy(data,attr->data,attr->length);
      data[attr->length]='\0';
      dumpString->printconcat(" \"%s\"",data);
      //if(attr->type==CDF_UBYTE)dumpString->printconcat(" \"%s\"",data);
      //if(attr->type==CDF_BYTE)dumpString->printconcat(" \"%s\"",data);
      delete[] data;
    }
    
    if(attr->type==CDF_BYTE)for(size_t n=0;n<attr->length;n++)dumpString->printconcat(" %db",((char*)attr->data)[n]);
    if(attr->type==CDF_UBYTE)for(size_t n=0;n<attr->length;n++)dumpString->printconcat(" %uub",((unsigned char*)attr->data)[n]);
    if(attr->type==CDF_INT)for(size_t n=0;n<attr->length;n++)dumpString->printconcat(" %d",((int*)attr->data)[n]);
    if(attr->type==CDF_UINT)for(size_t n=0;n<attr->length;n++)dumpString->printconcat(" %u",((unsigned int*)attr->data)[n]);
    if(attr->type==CDF_INT64)for(size_t n=0;n<attr->length;n++)dumpString->printconcat(" %ldll",((long*)attr->data)[n]);
    if(attr->type==CDF_UINT64)for(size_t n=0;n<attr->length;n++)dumpString->printconcat(" %luul",((unsigned long*)attr->data)[n]);
    if(attr->type==CDF_SHORT)for(size_t n=0;n<attr->length;n++)dumpString->printconcat(" %d",((short*)attr->data)[n]);
    if(attr->type==CDF_USHORT)for(size_t n=0;n<attr->length;n++)dumpString->printconcat(" %u",((unsigned short*)attr->data)[n]);
    if(attr->type==CDF_FLOAT)for(size_t n=0;n<attr->length;n++)dumpString->printconcat(" %ff",((float*)attr->data)[n]);
    if(attr->type==CDF_DOUBLE)for(size_t n=0;n<attr->length;n++)dumpString->printconcat(" %fdf",((double*)attr->data)[n]);
    if(attr->type==CDF_STRING)for(size_t n=0;n<attr->length;n++)dumpString->printconcat(" \"%s\"",((char**)attr->data)[n]);
    dumpString->printconcat(" ;\n");
  }
}

void CDF::dump(CDF::Variable* cdfVariable,CT::string* dumpString){
  char temp[1024];
  char dataTypeName[20];
  CDF::getCDataTypeName(dataTypeName,19,cdfVariable->getNativeType());
    snprintf(temp,1023,"\t%s %s",dataTypeName,cdfVariable->name.c_str());
    dumpString->printconcat("%s",temp);
    if(cdfVariable->dimensionlinks.size()>0){
      dumpString->printconcat("(");
      for(size_t i=0;i<cdfVariable->dimensionlinks.size();i++){
        if(i>0&&i<cdfVariable->dimensionlinks.size())dumpString->printconcat(", ");
        dumpString->printconcat("%s",cdfVariable->dimensionlinks[i]->name.c_str());
      }
      dumpString->printconcat(")");
    }
    dumpString->printconcat(" ;\n");
  _dumpPrintAttributes(cdfVariable->name.c_str(),cdfVariable->attributes,dumpString);
}

CT::string CDF::dump(CDFObject* cdfObject){
  CT::string d;
  dump(cdfObject,&d);
  return d;
}

void CDF::dump(CDFObject* cdfObject,CT::string* dumpString){
  //print dimensions:
  char temp[1024];
  char dataTypeName[20];
  
  dumpString->copy("CCDFDataModel {\ndimensions:\n");
  
  for(size_t j=0;j<cdfObject->dimensions.size();j++){
    snprintf(temp,1023,"%s",cdfObject->dimensions[j]->name.c_str());
    dumpString->printconcat("\t%s = %d ;\n",temp,int(cdfObject->dimensions[j]->length));
    
  }
  dumpString->printconcat("variables:\n");
  for(size_t j=0;j<cdfObject->variables.size();j++){
    {
      CDF::getCDataTypeName(dataTypeName,19,cdfObject->variables[j]->getNativeType());
      snprintf(temp,1023,"\t%s %s",dataTypeName,cdfObject->variables[j]->name.c_str());
      dumpString->printconcat("%s",temp);
      if(cdfObject->variables[j]->dimensionlinks.size()>0){
        dumpString->printconcat("(");
        for(size_t i=0;i<cdfObject->variables[j]->dimensionlinks.size();i++){
          if(i>0&&i<cdfObject->variables[j]->dimensionlinks.size())dumpString->printconcat(", ");
          dumpString->printconcat("%s",cdfObject->variables[j]->dimensionlinks[i]->name.c_str());
        }
        dumpString->printconcat(")");
      }
      dumpString->printconcat(" ;\n");
      //print attributes:
      _dumpPrintAttributes(cdfObject->variables[j]->name.c_str(),
                           cdfObject->variables[j]->attributes,
                           dumpString);
    }
  }
  //print GLOBAL attributes:
  dumpString->concat("\n// global attributes:\n");
  _dumpPrintAttributes("",cdfObject->attributes,dumpString);
  dumpString->concat("}\n");
}
