/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2015-05-06
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
#ifdef ADAGUC_USE_SQLITE

#ifndef CDBADAPTERSQLLITE_H
#define CDBADAPTERSQLLITE_H

#include "CDBAdapter.h"
#include "CDebugger.h"
#include <set>
#include <sqlite3.h>

class CDBAdapterSQLLite : public CDBAdapter {
public:
  class CSQLLiteDB {
    DEF_ERRORFUNCTION();

  private:
    sqlite3 *db;
    char *zErrMsg;
    static int callback(void *NotUsed, int argc, char **argv, char **azColName);
    static int callbacknoresults(void *NotUsed, int argc, char **argv, char **azColName);
    static std::set<std::string> columnSet;
    static std::vector<std::string> columnNames;
    static std::vector<std::string> queryValues;
    static std::pair<std::set<std::string>::iterator, bool> ret;
    CT::string errorMessage;

  public:
    CSQLLiteDB();
    ~CSQLLiteDB();
    int close();
    int connect(const char *pszOptions);
    int query(const char *pszQuery);
    int checkTable(const char *pszTableName, const char *pszColumns);
    /**
     * Queries to a store
     * @param pszQuery The query to execute
     * @param throwException Throw an (int) exception with a CDB_ERROR code if something fails
     * @return CDB::Store containing the results. Returns NULL or throws exceptions when fails.
     */
    CDBStore::Store *queryToStore(const char *pszQuery, bool throwException);

    /**
     * Queries to a store
     * @param pszQuery The query to execute
     * @return CDB::Store containing the results. Returns NULL when fails.
     */
    CDBStore::Store *queryToStore(const char *pszQuery);

    const char *getError() { return errorMessage.c_str(); }
  };

private:
  DEF_ERRORFUNCTION();
  CSQLLiteDB *dataBaseConnection;
  CSQLLiteDB *getDataBaseConnection();

  CServerConfig::XMLE_Configuration *configurationObject;
  std::map<std::string, std::string> lookupTableNameCacheMap; // PathFilter gives tablename

  std::map<std::string, std::vector<std::string>> fileListPerTable;
  int createDimTableOfType(const char *dimname, const char *tablename, int type);

public:
  CDBAdapterSQLLite();
  ~CDBAdapterSQLLite();
  int setConfig(CServerConfig::XMLE_Configuration *cfg);

  CDBStore::Store *getReferenceTime(const char *netcdfDimName, const char *netcdfTimeDimName, const char *timeValue, const char *timeTableName, const char *tableName);
  CDBStore::Store *getClosestDataTimeToSystemTime(const char *netcdfDimName, const char *tableName);

  CT::string getTableNameForPathFilterAndDimension(const char *path, const char *filter, const char *dimension, CDataSource *dataSource);
  int autoUpdateAndScanDimensionTables(CDataSource *dataSource);
  CDBStore::Store *getMin(const char *name, const char *table);
  CDBStore::Store *getMax(const char *name, const char *table);
  CDBStore::Store *getBetween(const char *, const char *, const char *, const char *, int);
  CT::string getDimValueForFileName(const char *filename, const char *table);
  CDBStore::Store *getUniqueValuesOrderedByValue(const char *name, int limit, bool orderDescOrAsc, const char *table);
  CDBStore::Store *getUniqueValuesOrderedByIndex(const char *name, int limit, bool orderDescOrAsc, const char *table);
  CDBStore::Store *getFilesAndIndicesForDimensions(CDataSource *dataSource, int limit, bool raiseExceptionWhenOverLimit);

  // TODO implement this method!!!
  CDBStore::Store *getFilesForIndices(CDataSource *dataSource, size_t *start, size_t *count, ptrdiff_t *stride, int limit);

  CDBStore::Store *getDimensionInfoForLayerTableAndLayerName(const char *layertable, const char *layername);
  int storeDimensionInfoForLayerTableAndLayerName(const char *layertable, const char *layername, const char *netcdfname, const char *ogcname, const char *units);
  int removeDimensionInfoForLayerTableAndLayerName(const char *layertable, const char *layername);

  int dropTable(const char *tablename);
  int createDimTableInt(const char *dimname, const char *tablename);
  int createDimTableReal(const char *dimname, const char *tablename);
  int createDimTableString(const char *dimname, const char *tablename);
  int createDimTableTimeStamp(const char *dimname, const char *tablename);
  int checkIfFileIsInTable(const char *tablename, const char *filename);

  int removeFile(const char *tablename, const char *file);
  int removeFilesWithChangedCreationDate(const char *tablename, const char *file, const char *creationDate);
  int setFileInt(const char *tablename, const char *file, int dimvalue, int dimindex, const char *filedate, GeoOptions *geoOptions);
  int setFileReal(const char *tablename, const char *file, double dimvalue, int dimindex, const char *filedate, GeoOptions *geoOptions);
  int setFileString(const char *tablename, const char *file, const char *dimvalue, int dimindex, const char *filedate, GeoOptions *geoOptions);
  int setFileTimeStamp(const char *tablename, const char *file, const char *dimvalue, int dimindex, const char *filedate, GeoOptions *geoOptions);
  int addFilesToDataBase();
  int storeLayerMetadata(const char *datasetName, const char *layerName, const char *metadataKey, const char *metadatablob);
  CDBStore::Store *getLayerMetadataStore(const char *datasetName);
  int dropLayerFromLayerMetadataStore(const char *datasetName, const char *layerName);

  bool tryAdvisoryLock(size_t) { return true; };
  bool advisoryUnLock(size_t) { return true; }
};

#endif

#endif
