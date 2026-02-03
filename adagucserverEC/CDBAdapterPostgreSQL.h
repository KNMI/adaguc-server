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

#include <cstdlib>
#include "CImageDataWriter.h"
#include "CServerParams.h"
#include "CDataSource.h"
#include "CDFObjectStore.h"
#include "CCDFStore.h"
#include "CDBStore.h"
#include "CDebugger.h"

#include "Types/GeoParameters.h"
#include "CPGSQLDB.h"

#define TABLETYPE_TIMESTAMP 1
#define TABLETYPE_INT 2
#define TABLETYPE_DOUBLE 3
#define TABLETYPE_STRING 4

struct DimInfo {
  CT::string tableName;
  CT::string dataType;
};
typedef struct DimInfo DimInfo;

struct GeoOptions {
  double bbox[4];
  int indices[4];
  CT::string proj4;
  int level;
};

class CDBAdapterPostgreSQL {
private:
  DEF_ERRORFUNCTION();
  CPGSQLDB *dataBaseConnection;
  CPGSQLDB *getDataBaseConnection();
  CServerConfig::XMLE_Configuration *configurationObject;
  std::map<std::string, DimInfo> lookupTableNameCacheMap;
  std::map<std::string, std::vector<std::string>> fileListPerTable;

  CDBStore::Store *layerMetaDataStore = nullptr;

public:
  CDBAdapterPostgreSQL();
  ~CDBAdapterPostgreSQL();

  int createDimTableOfType(const char *dimname, const char *tablename, int type);
  int setConfig(CServerConfig::XMLE_Configuration *cfg);

  /**
   * getReferenceTime
   * This function is used to find the most recent forecast_reference_time for a given time.
   * returns a forecast_reference_time value where the difference is smallest between time and referencetime and where the difference between time-referencetime is larger than zero
   * @param netcdfReferenceTimeDimName
   * @param netcdfTimeDimName
   * @param timeValue
   * @param timeTableName
   * @param referenceTimeTableName
   * @return timeValue The value to find the most recent reference time for
   */

  CDBStore::Store *getReferenceTime(const char *netcdfDimName, const char *netcdfTimeDimName, const char *timeValue, const char *timeTableName, const char *tableName);

  /**
   * getClosestDataTimeToSystemTime
   * Returns the closest data time to current system time. Used when time="current" in combination with reference_time.
   * @param netcdfDimName
   * @param tableName
   * @return timeValue;
   */
  CDBStore::Store *getClosestDataTimeToSystemTime(const char *netcdfDimName, const char *tableName);

  CT::string getTableNameForPathFilterAndDimension(CDataSource *dataSource);

  /**
   * getTableNameForPathFilterAndDimension
   * Makes use of a lookup table to find the tablename belonging to the filter and path combinations.
   * @param path The path of the layer
   * @param filter The filter of the layer
   * @param dimension The dimension of the layer, can be NULL if not used.
   * @param dataSource The dataSource
   * @return Tablename on succes, throws integer exception on failure.
   */
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
  CDBStore::Store *getFilesAndIndicesForDimensions(CDataSource *dataSource, int limit, bool raiseExceptionWhenOverLimit);
  CDBStore::Store *getFilesForIndices(CDataSource *dataSource, size_t *start, size_t *count, ptrdiff_t *stride, int limit);

  CDBStore::Store *getDimensionInfoForLayerTableAndLayerName(const char *layertable, const char *layername);
  int storeDimensionInfoForLayerTableAndLayerName(const char *layertable, const char *layername, const char *netcdfname, const char *ogcname, const char *units);
  int removeDimensionInfoForLayerTableAndLayerName(const char *layertable, const char *layername);

  int dropTable(const char *tablename);
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
  f8box getExtent(CDataSource *dataSource);

  std::vector<CT::string> getTableNames(CDataSource *dataSource);
};

#endif
