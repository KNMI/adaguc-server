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

#include "CCDFObject.h"
#include "CCDFReader.h"

const char *CDFObject::className="CDFObject";

CDFObject::~CDFObject(){
  clear();
}

int CDFObject::attachCDFReader(void *reader){
  CDFReader *r=(CDFReader*)reader;
  r->cdfObject=this;
  this->reader=r;
  return 0;
}
void CDFObject::clear(){
  for(size_t j=0;j<dimensions.size();j++){delete dimensions[j];dimensions[j]=NULL;}
  for(size_t j=0;j<variables.size();j++){delete variables[j];variables[j]=NULL;}
}

int CDFObject::open(const char *fileName){
  //CDBDebug("Opening file %s (current =%s)",fileName,currentFile.c_str());
  if(currentFile.equals(fileName)){
    //CDBDebug("OK: Current file is already open");
    return 0;
  }
  CDFReader *r=(CDFReader*)reader;
   if(r==NULL){
    CDBError("No reader attached");return 1;
  }
  clear();
  currentFile.copy(fileName);
  return r->open(fileName);
}

int CDFObject::close(){
  if(reader==NULL){
    CDBError("No reader attached");return 1;
  }
  //CDBDebug("Closing reader");
  CDFReader *r=(CDFReader*)reader;
  return r->close();
  reader = NULL;
}
