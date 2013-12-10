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

#ifndef CDataReader_H
#define CDataReader_H
#include <math.h>
#include "CDebugger.h"
#include "CDataSource.h"
#include "CServerError.h"
#include "CDirReader.h"
#include "CPGSQLDB.h"
//#include "CADAGUC_time.h"
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"
#include "CCDFHDF5IO.h"
#include "CProj4ToCF.h"
#include "CStopWatch.h"
#include <sys/stat.h>
#include "CDBFileScanner.h"
#include "CDFObjectStore.h"
#include "CCache.h"

class CDataReader{
  private:
    DEF_ERRORFUNCTION();
    
  public:
    CDataReader(){}
    ~CDataReader(){}
  
    
    static int autoConfigureDimensions(CDataSource *dataSource);
    static int autoConfigureStyles(CDataSource *dataSource);
    
    /** 
     * Load a generic file header into the datasource. Usually the most recent file from a series is taken. The file header can for example be used to determine automatically the available dimensions.
     * @param dataSource
     * @return Zero on success.
     */
    static int justLoadAFileHeader(CDataSource *dataSource);
    
    /**
     * Returns a unique identifier for the current datasource. The identifier is made unique by it dimensions, so storing subsetted cache parts is possible.
     * @param dataSource
     * @param cacheName
     * @return Zero on success.
     */
    static int getCacheFileName(CDataSource *dataSource,CT::string *cacheName);
    
    static int getTimeDimIndex( CDFObject *cdfObject, CDF::Variable * var);
    
    static CDF::Variable *getTimeVariable( CDFObject *cdfObject, CDF::Variable * var);
    
    /**
     * Get current time string for datasource based on the current timestep
     * @param dataSource 
     * @param charachter array where an ISO8601 time string fits in
     * @return zero on success
     */
    int getTimeString(CDataSource *dataSource,char * pszTime);
    
    /**
     * Get time units for this datasource, throws exception of int when failed.
     * @param dataSource
     * @return time units for this datasource
     */
    CT::string getTimeUnit(CDataSource* dataSource);

    int open(CDataSource *dataSource,int mode,int x,int y);
    int open(CDataSource *dataSource, int x,int y);
    int open(CDataSource *dataSource, int mode);
    int parseDimensions(CDataSource *dataSource,int mode,int x,int y,CCache *cache);
    
    int close(){return 0;};
};

#endif

