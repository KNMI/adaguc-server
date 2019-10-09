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

#ifndef CDBFileScanner_H
#define CDBFileScanner_H

#include "CDebugger.h"
#include "CStopWatch.h"
#include "CDataReader.h"
#include "CDFObjectStore.h"
#include "CServerError.h"
#include "CDirReader.h"

#define CDBFILESCANNER_RESCAN 1
#define CDBFILESCANNER_UPDATEDB 2
#define CDBFILESCANNER_CREATETILES 4
#define CDBFILESCANNER_DONTREMOVEDATAFROMDB 8
#define CDBFILESCANNER_IGNOREFILTER 16
#define CDBFILESCANNER_RECREATETABLES 32
#define CDBFILESCANNER_CLEANFILES 64

/**
 * Class which scans files and updates the database.
 */
class CDBFileScanner{
private:
  DEF_ERRORFUNCTION();
  static int createDBUpdateTables(CDataSource *dataSource,int &removeNonExistingFiles,std::vector <std::string> *fileList, bool recreateTable);
  static int DBLoopFiles(CDataSource *dataSource,int removeNonExistingFiles,std::vector <std::string> *fileList ,int scanFlags);
  static std::vector <CT::string> tableNamesDone;
  
  static void handleDirHasNewFile(std::string a){
  }
  
  static std::vector <std::string> filesToDeleteFromDB;
  static void handleFileFromDBIsMissing(std::string a){
    CDBDebug("DirReader is missing %s", a.c_str());
    filesToDeleteFromDB.push_back(a);
  }
 public:
  static bool isTableAlreadyScanned(CT::string *tableName);
  static void markTableDirty(CT::string *tableName);
  /**
   * Populates dirReader with files defined by the path, exp and tailPath parameters
   * @param path: This is usually the configured FilePath in the Layers configuration, can also be an OpenDAP URL
   * @param expr: The regular expression to reject/accept each individual file on. When NULL is provided, *.nc is used as default.
   * @param tailPath: The path under the root path to be scanned.
   * @returns dirReader: An initialised dirReader or NULL on error
   */
  static std::vector<std::string> searchFileNames(const char * path,CT::string expr,const char *tailPath);
  
  /**
   * Updates the database for a specified dataSource
   * @param dataSource: The datasource to update
   * @param tailPath: The path under the root path to be scanned.
   * @param _layerPathToScan: When matched with the datasources configured FilePath, the datasource is updated, otherwise it is ignored. 
   * This can be used to update a specific layer by defining its corresponding path. When all datasources are looped, only the datasource with matching paths are updated
   * When set to NULL, the datasource is updated always.
   * @param scanFlags Scan flags parameters, e.g. CDBFILESCANNER_RESCAN can be set as flag.
   */
  static int updatedb(CDataSource *dataSource,CT::string *tailPath,CT::string *_layerPathToScan,int scanFlags);
  
  static int createTiles(CDataSource *dataSource,int scanFlags);

  /**
    * 
    */
  static int cleanFiles(CDataSource *dataSource, int scanFlags) ;
  
};

#endif
