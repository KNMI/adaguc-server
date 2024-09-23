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
#ifndef CDBADAPTERPOSTGRESQL_H
#define CDBADAPTERPOSTGRESQL_H
#include "CDBAdapter.h"
#include "CDebugger.h"
#include "CPGSQLDB.h"

struct DimInfo {
  CT::string tableName;
  CT::string dataType;
};
typedef struct DimInfo DimInfo;

class CDBAdapterPostgreSQL : public CDBAdapter {
private:
  DEF_ERRORFUNCTION();
  CPGSQLDB *dataBaseConnection;
  CPGSQLDB *getDataBaseConnection();
  CServerConfig::XMLE_Configuration *configurationObject;
  std::map<std::string, DimInfo> lookupTableNameCacheMap;
  std::map<std::string, std::vector<std::string>> fileListPerTable;
  int createDimTableOfType(const char *dimname, const char *tablename, int type);

  CDBStore::Store *layerMetaDataStore = nullptr;

public:
  CDBAdapterPostgreSQL();
  ~CDBAdapterPostgreSQL();
  int setConfig(CServerConfig::XMLE_Configuration *cfg);

  CDBStore::Store *getReferenceTime(const char *netcdfDimName, const char *netcdfTimeDimName, const char *timeValue, const char *timeTableName, const char *tableName);
  CDBStore::Store *getClosestDataTimeToSystemTime(const char *netcdfDimName, const char *tableName);

  CT::string getTableNameForPathFilterAndDimension(const char *path, const char *filter, const char *dimension, CDataSource *dataSource);
  std::map<CT::string, DimInfo> getTableNamesForPathFilterAndDimensions(const char *path, const char *filter, std::vector<CT::string> dimensions, CDataSource *dataSource);

  CT::string getLookupIdentifier(const char *path, const char *filter, const char *dimension);
  void assertLookupTableExists();
  void addToLookupTable(const char *path, const char *filter, CT::string dimensionName, CT::string tableName);
  CT::string generateRandomTableName();

  int autoUpdateAndScanDimensionTables(CDataSource *dataSource);
  CDBStore::Store *getMin(const char *name, const char *table);
  CDBStore::Store *getMax(const char *name, const char *table);
  CDBStore::Store *getBetween(const char *min, const char *max, const char *colname, const char *table, int limit);
  CT::string getDimValueForFileName(const char *filename, const char *table);
  CDBStore::Store *getUniqueValuesOrderedByValue(const char *name, int limit, bool orderDescOrAsc, const char *table);
  CDBStore::Store *getUniqueValuesOrderedByIndex(const char *name, int limit, bool orderDescOrAsc, const char *table);
  CDBStore::Store *getFilesAndIndicesForDimensions(CDataSource *dataSource, int limit);
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
  bool tryAdvisoryLock(size_t);
  bool advisoryUnLock(size_t);
};

#endif
