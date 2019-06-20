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

#ifndef CDFObjectStore_H
#define CDFObjectStore_H

#include "CDebugger.h"
#include "CStopWatch.h"
#include "CDataSource.h"
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"
#include "CCDFHDF5IO.h"
#include "CCDFGeoJSONIO.h"
#include "CCDFPNGIO.h"

#include "CCache.h"

//Datasource can share multiple cdfObjects
//A cdfObject is allways opened using a dataSource path/filter combo
// When a CDFObject is already opened
class CDFObjectStore{
private:
  std::vector <CT::string*> fileNames;
  std::vector <CDFObject*> cdfObjects;
  std::vector <CDFReader*> cdfReaders;
  
  /**
   * Get a CDFReader based on information in the datasource. In the Layer element this can be configured with <DataReader>HDF5</DataReader>
   * @param dataSource The configured datasource or NULL pointer. NULL pointer defaults to a NetCDF/OPeNDAP reader
   */
  static CDFReader *getCDFReader(CDataSource *dataSource,const char *fileName);
  
  /**
   * Get a CDFReader based on fileName information, currently based on extension.
   * @param fileName The fileName
   * @return The CDFReader
   */
  static CDFReader *getCDFReader(const char *fileName);
  
  
  
  CDFObject* getCDFObject(CDataSource *dataSource,CServerParams *srvParams,const char *fileName,bool plain);
  
  DEF_ERRORFUNCTION();
public:
  ~CDFObjectStore(){
    clear();
  }
  void deleteCDFObject(CDFObject **cdfObject);
  void deleteCDFObject(const char *fileName);
  /**
   * Gets the current allocated object store
   */
  static CDFObjectStore *getCDFObjectStore();
  
  /**
   * Get a CDFObject based with opened and configured CDF reader for a filename/OPeNDAP url and a dataSource.
   * @param dataSource The configured datasource or NULL pointer. NULL pointer defaults to a NetCDF/OPeNDAP reader
   * @param fileName The filename to read.
   */
  CDFObject *getCDFObject(CDataSource *dataSource,const char *fileName);
  
  CDFObject *getCDFObjectHeader(CDataSource *dataSource, CServerParams *srvParams,const char *fileName);
  CDFObject *getCDFObjectHeaderPlain(CDataSource *dataSource, CServerParams *srvParams,const char *fileName);
  static CT::StackList<CT::string> getListOfVisualizableVariables(CDFObject *cdfObject);
  
  /**
   * Returns how many objects are openend in this store
   */
  int getNumberOfOpenObjects();
  /**
   * Returns how many objects can be openend in this store
   */
  int getMaxNumberOfOpenObjects();
  
  /**
   * Clean the CDFObject store and throw away all readers and objects
   */
  void clear();
};


#endif
