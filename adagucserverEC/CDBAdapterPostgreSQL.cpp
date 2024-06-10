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
#include "CDBAdapterPostgreSQL.h"

#include <set>
#include <map>
#include "CDebugger.h"

const char *CDBAdapterPostgreSQL::className = "CDBAdapterPostgreSQL";
// #define CDBAdapterPostgreSQL_DEBUG
#define CDBAdapterPostgreSQL_PATHFILTERTABLELOOKUP "pathfiltertablelookup_v2_22_0"
// #define MEASURETIME

CDBAdapterPostgreSQL::CDBAdapterPostgreSQL() {
#ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("CDBAdapterPostgreSQL()");
#endif
  dataBaseConnection = NULL;
}

CDBAdapterPostgreSQL::~CDBAdapterPostgreSQL() {
#ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("~CDBAdapterPostgreSQL()");
#endif
  if (dataBaseConnection != NULL) {
    dataBaseConnection->close2();
  }
  delete dataBaseConnection;
  dataBaseConnection = NULL;
}

CPGSQLDB *CDBAdapterPostgreSQL::getDataBaseConnection() {
  if (dataBaseConnection == NULL) {
#ifdef MEASURETIME
    StopWatch_Stop(">CDBAdapterPostgreSQL::getDataBaseConnection");
#endif
    dataBaseConnection = new CPGSQLDB();
    int status = dataBaseConnection->connect(configurationObject->DataBase[0]->attr.parameters.c_str());
    if (status != 0) {
      CDBError("Unable to connect to DB");
      return NULL;
    }

    assertLookupTableExists();

#ifdef MEASURETIME
    StopWatch_Stop("<CDBAdapterPostgreSQL::getDataBaseConnection");
#endif
  }
  return dataBaseConnection;
}

int CDBAdapterPostgreSQL::setConfig(CServerConfig::XMLE_Configuration *cfg) {
  configurationObject = cfg;
  return 0;
}

CT::string CDBAdapterPostgreSQL::getDimValueForFileName(const char *filename, const char *table) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getDimValueForFileName");
#endif
  CPGSQLDB *DB = getDataBaseConnection();
  if (DB == NULL) {
    return "";
  }

  CT::string query;
  query.print("select * from %s where path = '%s' limit 1", table, filename);
  CDBStore::Store *store = DB->queryToStore(query.c_str());
  if (store == NULL || store->size() == 0) {
    setExceptionType(InvalidDimensionValue);
    CDBError("Invalid filename value for  %s", filename);
    CDBError("query failed");
    return "";
  }
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getDimValueForFileName");
#endif
  CT::string dimValue = store->getRecord(0)->get(1);
  delete store;
  return dimValue;
};

CDBStore::Store *CDBAdapterPostgreSQL::getMax(const char *name, const char *table) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getMax");
#endif
  CPGSQLDB *DB = getDataBaseConnection();
  if (DB == NULL) {
    return NULL;
  }

  CT::string query;
  query.print("select max(%s) from %s", name, table);
  CDBStore::Store *maxStore = DB->queryToStore(query.c_str());
  if (maxStore == NULL) {
    setExceptionType(InvalidDimensionValue);
    CDBError("Invalid dimension value for  %s", name);
    CDBError("query failed");
    return NULL;
  }
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getMax");
#endif
  return maxStore;
};

CDBStore::Store *CDBAdapterPostgreSQL::getMin(const char *name, const char *table) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getMin");
#endif
  CPGSQLDB *DB = getDataBaseConnection();
  if (DB == NULL) {
    return NULL;
  }
  CT::string query;

  query.print("select min(%s) from %s", name, table);
  CDBStore::Store *maxStore = DB->queryToStore(query.c_str());
  if (maxStore == NULL) {
    setExceptionType(InvalidDimensionValue);
    CDBError("Invalid dimension value for  %s", name);
    CDBError("query failed");
    return NULL;
  }
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getMin");
#endif
  return maxStore;
};

CDBStore::Store *CDBAdapterPostgreSQL::getBetween(const char *min, const char *max, const char *colname, const char *table, int limit) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getBetween");
#endif
  CPGSQLDB *DB = getDataBaseConnection();
  if (DB == NULL) {
    return NULL;
  }

  CT::string query;
  query.print("select path, %s FROM %s WHERE %s between '%s' and '%s' order by %s asc limit %d ", colname, table, colname, min, max, colname, limit);

  CDBStore::Store *maxStore = DB->queryToStore(query.c_str());
  if (maxStore == NULL) {
    setExceptionType(InvalidDimensionValue);
    CDBError("Invalid dimension value for  %s", colname);
    CDBError("query failed");
    return NULL;
  }
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getBetween");
#endif
  return maxStore;
};

CDBStore::Store *CDBAdapterPostgreSQL::getUniqueValuesOrderedByValue(const char *name, int limit, bool orderDescOrAsc, const char *table) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getUniqueValuesOrderedByValue");
#endif
  CPGSQLDB *DB = getDataBaseConnection();
  if (DB == NULL) {
    return NULL;
  }

  CT::string query;
  query.print("select %s from %s group by %s order by %s %s", name, table, name, name, orderDescOrAsc ? "asc" : "desc");
  if (limit > 0) {
    query.printconcat(" limit %d", limit);
  }
  CDBStore::Store *store = DB->queryToStore(query.c_str());
  if (store == NULL) {
    CDBDebug("Query %s failed", query.c_str());
  }
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getUniqueValuesOrderedByValue");
#endif
  return store;
}

CDBStore::Store *CDBAdapterPostgreSQL::getUniqueValuesOrderedByIndex(const char *name, int limit, bool, const char *table) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getUniqueValuesOrderedByIndex");
#endif
  CPGSQLDB *DB = getDataBaseConnection();
  if (DB == NULL) {
    return NULL;
  }

  CT::string query;
  query.print("select distinct %s,dim%s from %s order by dim%s,%s", name, name, table, name, name);
  if (limit > 0) {
    query.printconcat(" limit %d", limit);
  }
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getUniqueValuesOrderedByIndex");
#endif
  return DB->queryToStore(query.c_str());
}

CDBStore::Store *CDBAdapterPostgreSQL::getReferenceTime(const char *netcdfReferenceTimeDimName, const char *netcdfTimeDimName, const char *timeValue, const char *timeTableName,
                                                        const char *referenceTimeTableName) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getReferenceTime");
#endif
  CPGSQLDB *DB = getDataBaseConnection();
  if (DB == NULL) {
    return NULL;
  }
  CT::string query;

  query.print("select max(forecast_reference_time) from (select %s,%s as age from ( select * from %s)a0 ,( select * from %s where %s = '%s')a1 where a0.path = a1.path )a0", netcdfReferenceTimeDimName,
              netcdfTimeDimName, referenceTimeTableName, timeTableName, netcdfTimeDimName, timeValue);

#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getReferenceTime");
#endif
  return DB->queryToStore(query.c_str());
};

CDBStore::Store *CDBAdapterPostgreSQL::getClosestDataTimeToSystemTime(const char *netcdfDimName, const char *tableName) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getClosestDataTimeToSystemTime");
#endif
  CPGSQLDB *DB = getDataBaseConnection();
  if (DB == NULL) {
    return NULL;
  }
  CT::string query;

  query.print("SELECT %s,abs(EXTRACT(EPOCH FROM (to_timestamp(%s) - now()))) as t from %s order by t asc limit 1", netcdfDimName, netcdfDimName, tableName);

#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getClosestDataTimeToSystemTime");
#endif
  return DB->queryToStore(query.c_str());
};

CDBStore::Store *CDBAdapterPostgreSQL::getFilesForIndices(CDataSource *dataSource, size_t *start, size_t *count, ptrdiff_t *, int) {
#ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("getFilesForIndices");
#endif
  CPGSQLDB *DB = getDataBaseConnection();
  if (DB == NULL) {
    return NULL;
  }

  CT::string queryOrderedDESC;
  CT::string query;
  queryOrderedDESC.print("select a0.path");
  for (size_t i = 0; i < dataSource->requiredDims.size(); i++) {
    queryOrderedDESC.printconcat(",%s,dim%s", dataSource->requiredDims[i]->netCDFDimName.c_str(), dataSource->requiredDims[i]->netCDFDimName.c_str());
  }

  queryOrderedDESC.concat(" from ");

#ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("%s", queryOrderedDESC.c_str());
#endif

  // Find all tables belonging to given path, filter, and dimensions
  std::vector<CT::string> dims;
  for (const auto &dim : dataSource->requiredDims) {
    CT::string dimString(dim->netCDFDimName);
    dimString.toLowerCaseSelf();
    dims.push_back(dimString);
  }
  std::map<CT::string, DimInfo> mapping =
      getTableNamesForPathFilterAndDimensions(dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), dims, dataSource);

  // Compose the query
  for (size_t i = 0; i < dataSource->requiredDims.size(); i++) {
    CT::string netCDFDimName(&dataSource->requiredDims[i]->netCDFDimName);
    CT::string tableName = mapping[netCDFDimName].tableName;

    CT::string subQuery;
    subQuery.print("(select path,dim%s,%s from %s ", netCDFDimName.c_str(), netCDFDimName.c_str(), tableName.c_str());

    // subQuery.printconcat("where dim%s = %d ",netCDFDimName.c_str(),start[i]);
    subQuery.printconcat("ORDER BY %s ASC limit %d offset %d)a%d ", netCDFDimName.c_str(), count[i], start[i], i);
    if (i < dataSource->requiredDims.size() - 1) subQuery.concat(",");
    queryOrderedDESC.concat(&subQuery);
  }

#ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("%s", queryOrderedDESC.c_str());
#endif
  // Join by path
  if (dataSource->requiredDims.size() > 1) {
    queryOrderedDESC.concat(" where a0.path=a1.path");
    for (size_t i = 2; i < dataSource->requiredDims.size(); i++) {
      queryOrderedDESC.printconcat(" and a0.path=a%d.path", i);
    }
  }
#ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("%s", queryOrderedDESC.c_str());
#endif

  query.print("select distinct * from (%s)T order by ", queryOrderedDESC.c_str());
  query.concat(&dataSource->requiredDims[0]->netCDFDimName);
  for (size_t i = 1; i < dataSource->requiredDims.size(); i++) {
    query.printconcat(",%s", dataSource->requiredDims[i]->netCDFDimName.c_str());
  }

  // Execute the query

  // writeLogFile3(query.c_str());
  // writeLogFile3("\n");
// values_path = DB.query_select(query.c_str(),0);
#ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("%s", query.c_str());
#endif

  CDBStore::Store *store = NULL;
  try {
    store = DB->queryToStore(query.c_str(), true);
  } catch (int e) {
    if ((CServerParams::checkDataRestriction() & SHOW_QUERYINFO) == false) query.copy("hidden");
    setExceptionType(InvalidDimensionValue);
    CDBError("Invalid dimension value for layer %s", dataSource->cfgLayer->Name[0]->value.c_str());
    CDBDebug("Query failed with code %d (%s)", e, query.c_str());
    return NULL;
  }
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getFilesForIndices");
#endif
  return store;
}

CDBStore::Store *CDBAdapterPostgreSQL::getFilesAndIndicesForDimensions(CDataSource *dataSource, int limit) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getFilesAndIndicesForDimensions");
#endif
  CPGSQLDB *DB = getDataBaseConnection();
  if (DB == NULL) {
    return NULL;
  }

  if (dataSource->requiredDims.size() == 0) {
    CDBError("Unable to do getFilesAndIndicesForDimensions, this datasource has no dimensions: dataSource->requiredDims.size()==0");
    return NULL;
  }

  // Find all tables belonging to given path, filter and dimensions
  std::vector<CT::string> dims;
  for (const auto &dim : dataSource->requiredDims) {
    CT::string dimString(dim->netCDFDimName);
    dimString.toLowerCaseSelf();
    dims.push_back(dimString);
  }
  std::map<CT::string, DimInfo> mapping = getTableNamesForPathFilterAndDimensions(dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), dims, dataSource);

  // Create a mapping for filtering where key=dimension name, value=dimension value
  // FIXME: merge with other mapping?
  std::map<CT::string, CT::string *> dimMap;
  for (const auto &dim : dataSource->requiredDims) {
    CT::string queryParams(&dim->value);
    // FIXME: should support filtering on multiple dim values when passing a list,of,values
    CT::string *sDims = queryParams.splitToArray("/"); // Split up by slashes (and put into sDims)
    // It is allowed to pass time as a range: start/end. If we do this, we assume to given value is datetime string

    dimMap[dim->netCDFDimName.c_str()] = sDims;
  }

  // Build query to find combination of file paths and dimensions, by filtering on dimensions. Query has following form:
  //    SELECT DISTINCT t1.path, <dimension 1 name>, dim<dimension 1 name>, ... FROM <table> t1
  //    INNER JOIN <table for dimension n> tn ON t1.path = tn.path ...
  //    WHERE <adaguc tiling query> AND <dimension 1 filter> ...
  //    ORDER BY <dimensions> LIMIT <limit>

  CT::string query;
  query.print("SELECT DISTINCT t1.path");
  for (const auto &dim : dims) {
    query.printconcat(", %s, dim%s", dim.c_str(), dim.c_str());
  }

  int i = 0;
  for (const auto &[key, dimInfo] : mapping) {
    if (i == 0) {
      query.printconcat(" FROM %s t1 ", dimInfo.tableName.c_str());
    } else {
      query.printconcat("INNER JOIN %s t%d ON t1.path = t%d.path ", dimInfo.tableName.c_str(), i + 1, i + 1);
    }
    i++;
  }

  // Filter on tiling bounding box (or not)
  query.concat("WHERE ");
  if (dataSource->queryBBOX) {
    query.printconcat("t1.adaguctilinglevel = %d and minx >= %f and maxx <= %f and miny >= %f and maxy <= %f ", dataSource->queryLevel, dataSource->nativeViewPortBBOX[0],
                      dataSource->nativeViewPortBBOX[2], dataSource->nativeViewPortBBOX[1], dataSource->nativeViewPortBBOX[3]);
  } else {
    query.concat("t1.adaguctilinglevel != -1 ");
  }

  // Filter on the requested dimensions
  for (const auto &[dimNameString, dimInfo] : mapping) {
    CT::string *dimVals = dimMap[dimNameString];
    const char *dimName = dimNameString.c_str();
    const char *tableName = dimInfo.tableName.c_str();
    CT::string dimDataType = dimInfo.dataType;

    if (dimVals->count == 1) {
      if (dimVals[0].equals("*")) continue;

      const char *dimVal = dimVals[0].c_str();

      // If dimension value is a number, find closest value.
      if (dimDataType.equals("numeric")) {
        query.printconcat("AND ABS(%s - %s) = (SELECT MIN(ABS(%s - %s)) FROM %s) ", dimVal, dimName, dimVal, dimName, tableName);
      } else {
        query.printconcat("AND %s = '%s' ", dimName, dimVal);
      }
    } else {
      // Find value within range of dimVals[0] and dimVals[1]
      // Get closest lowest value to this requested one, or if request value is way below get earliest value:
      query.printconcat("AND %s >= (SELECT MAX(%s) FROM %s WHERE %s <= '%s' OR %s = (SELECT MIN(%s) FROM %s)) AND %s <= '%s' ", dimName, dimName, tableName, dimName, dimVals[0].c_str(), dimName,
                        dimName, tableName, dimName, dimVals[1].c_str());
    }
    delete[] dimVals;
  }

  // Order and limit query
  query.printconcat("ORDER BY %s", dataSource->requiredDims[0]->netCDFDimName.c_str());
  for (size_t i = 1; i < dataSource->requiredDims.size(); i++) {
    query.printconcat(", %s", dataSource->requiredDims[i]->netCDFDimName.c_str());
  }
  query.printconcat(" LIMIT %d", limit);

  // Execute the query
  CDBStore::Store *store = NULL;
  try {
    store = DB->queryToStore(query.c_str(), true);
  } catch (int e) {
    if ((CServerParams::checkDataRestriction() & SHOW_QUERYINFO) == false) query.copy("hidden");
    setExceptionType(InvalidDimensionValue);
    CDBDebug("Query failed with code %d (%s)", e, query.c_str());
    return NULL;
  }
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getFilesAndIndicesForDimensions");
#endif
  return store;
}

int CDBAdapterPostgreSQL::autoUpdateAndScanDimensionTables(CDataSource *dataSource) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::autoUpdateAndScanDimensionTables");
#endif
  CServerParams *srvParams = dataSource->srvParams;
  CServerConfig::XMLE_Layer *cfgLayer = dataSource->cfgLayer;
  CPGSQLDB *dataBaseConnection = getDataBaseConnection();
  if (dataBaseConnection == NULL) {
    return -1;
  }

#ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("[checkDimTables]");
#endif
  bool tableNotFound = false;
  bool fileNeedsUpdate = false;
  CT::string dimName;

  // Find all tables belonging to given path, filter, and dimensions
  std::vector<CT::string> dims;
  for (const auto &dim : cfgLayer->Dimension) {
    CT::string dimString(dim->attr.name);
    dimString.toLowerCaseSelf();
    dims.push_back(dimString);
  }
  std::map<CT::string, DimInfo> mapping = getTableNamesForPathFilterAndDimensions(cfgLayer->FilePath[0]->value.c_str(), cfgLayer->FilePath[0]->attr.filter.c_str(), dims, dataSource);

  for (const auto &[dimName, dimInfo] : mapping) {
    CT::string query;
    query.print("select path,filedate,%s from %s limit 1", dimName.c_str(), dimInfo.tableName.c_str());
    CDBStore::Store *store = dataBaseConnection->queryToStore(query.c_str());
    if (store == NULL) {
      tableNotFound = true;
      CDBDebug("No table found for dimension %s", dimName.c_str());
    }

    if (tableNotFound == false) {
      if (srvParams->isAutoLocalFileResourceEnabled() == true) {
        try {
          CT::string databaseTime = store->getRecord(0)->get(1);
          if (databaseTime.length() < 20) {
            databaseTime.concat("Z");
          }
          databaseTime.setChar(10, 'T');

          CT::string fileDate = CDirReader::getFileDate(store->getRecord(0)->get(0)->c_str());

          if (databaseTime.equals(fileDate) == false) {
            CDBDebug("Table was found, %s ~ %s : %d", fileDate.c_str(), databaseTime.c_str(), databaseTime.equals(fileDate));
            fileNeedsUpdate = true;
          }

        } catch (int e) {
          CDBDebug("Unable to get filedate from database, error: %s", CDBStore::getErrorMessage(e));
          fileNeedsUpdate = true;
        }
      }
    }

    delete store;
    if (tableNotFound || fileNeedsUpdate) break;
  }

  if (fileNeedsUpdate == true) {
    // Recreate table
    if (srvParams->isAutoLocalFileResourceEnabled() == true) {
      for (auto &[dimName, dimInfo] : mapping) {
        CDBFileScanner::markTableDirty(&dimInfo.tableName);
        // CDBDebug("Dropping old table (if exists)",tableName.c_str());
        CT::string query;
        query.print("drop table %s", dimInfo.tableName.c_str());
        CDBDebug("Try to %s for %s", query.c_str(), dimName.c_str());
        dataBaseConnection->query(query.c_str());
      }
      tableNotFound = true;
    }
  }

  if (tableNotFound) {
    if (srvParams->isAutoLocalFileResourceEnabled() == true) {

      CDBDebug("Updating database");
      int status = CDBFileScanner::updatedb(dataSource, NULL, NULL, 0);
      if (status != 0) {
        CDBError("Could not update db for: %s", cfgLayer->Name[0]->value.c_str());
        return 2;
      }
    } else {
      CDBDebug("No table found for dimension %s and autoresource is disabled", dimName.c_str());
      return 1;
    }
  }
#ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("[/checkDimTables]");
#endif
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::autoUpdateAndScanDimensionTables");
#endif
  return 0;
}

CT::string CDBAdapterPostgreSQL::getLookupIdentifier(const char *path, const char *filter, const char *dimension) {
  CT::string lookupIdentifier = "lookuptable/";
  lookupIdentifier.printconcat("%s/%s/%s", path, filter, dimension);
  return lookupIdentifier;
}

void CDBAdapterPostgreSQL::assertLookupTableExists() {
  CDBDebug("Asserting that lookup table exists");

  CPGSQLDB *DB = getDataBaseConnection();
  if (DB == NULL) {
    CDBError("Unable to connect to DB");
    throw(1);
  }

  // Should check only once if lookup table exists
  CT::string tableColumns("path varchar (511), filter varchar (511), dimension varchar (511), tablename varchar (63), dimension_datatype varchar (31), UNIQUE (path,filter,dimension) ");
  int status = DB->checkTable(CDBAdapterPostgreSQL_PATHFILTERTABLELOOKUP, tableColumns.c_str());
  if (status == 1) {
    CDBError("FAIL: Table %s could not be created: %s", CDBAdapterPostgreSQL_PATHFILTERTABLELOOKUP, tableColumns.c_str());
    CDBError("Error: %s", DB->getError());
    throw(1);
  }
}

void CDBAdapterPostgreSQL::addToLookupTable(const char *path, const char *filter, CT::string dimensionName, CT::string tableName) {
  CPGSQLDB *DB = getDataBaseConnection();
  if (DB == NULL) {
    CDBError("Unable to connect to DB");
    throw(1);
  }

  // Determine data type of dimension
  CT::string query;
  query.print("SELECT data_type FROM information_schema.columns WHERE table_name='%s' and column_name='%s';", tableName.c_str(), dimensionName.c_str());
  CDBStore::Store *dataType = DB->queryToStore(query.c_str(), true);
  if (dataType == NULL) {
    CDBError("Failed adding to lookup table. Could not determine data type of table_name: %s and column_name: %s", tableName.c_str(), dimensionName.c_str());
    throw(1);
  }

  CT::string dimDataType = "";
  if (dataType->getSize() != 0) {
    dimDataType = dataType->getRecord(0)->get("data_type");
  }

  query.print("INSERT INTO %s values (E'P_%s', E'F_%s', E'%s', E'%s', E'%s')", CDBAdapterPostgreSQL_PATHFILTERTABLELOOKUP, path, filter, dimensionName.c_str(), tableName.c_str(), dimDataType.c_str());
  int status = DB->query(query.c_str());
  if (status != 0) {
    CDBError("Unable to insert records in lookup table: \"%s\"", query.c_str());
    throw(1);
  }
}

std::map<CT::string, DimInfo> CDBAdapterPostgreSQL::getTableNamesForPathFilterAndDimensions(const char *path, const char *filter, std::vector<CT::string> dimensions, CDataSource *dataSource) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getTableNamesForPathFilterAndDimensions");
#endif
  /*
    Given a path to a netcdf file, a filter, and a vector of requested dimensions, return a mapping of dimension name -> dimension table

    This information can be fetched from multiple areas, which means this function does the following:
    - Use hardcoded database table name if it appears in the configuration
    - Else, check if the requested dimension(s) appear(s) in the `lookupTableNameCache`
    - If no mapping found for all requested dimensions, check if the requested dimensions are in the sql lookup table
    - If the mapping is not complete, fill the lookuptable
  */

  // FIXME: Use struct instead of std::pair to prevent .first and .second? I just want to store a mapping of dimName -> (tableName, dimDataType)
  // FIXME: Make sure dimDataType also gets stored in lookupCache
  // FIXME: Make sure dimDataType can get fetched in other cases as well
  // FIXME: We assume dim is always lowercase. Should we manually lowercase everything in `dimensions`?
  // FIXME: Support netcdf without dimension, dimension=NULL, or dimension=""

  std::map<CT::string, DimInfo> mapping;

  // If config has a hardcoded db table name for layer, use it
  if (dataSource->cfgLayer->DataBaseTable.size() == 1) {
    CT::string tableName = dataSource->cfgLayer->DataBaseTable[0]->value.c_str();

    for (auto &dim : dimensions) {
      dataSource->srvParams->makeCorrectTableName(&tableName, &dim);
      mapping[dim] = {tableName, ""};
    }
#ifdef MEASURETIME
    StopWatch_Stop("<CDBAdapterPostgreSQL::getTableNamesForPathFilterAndDimensions");
#endif
    return mapping;
  }

  // Check in lookupTableNameCache
  bool done = true;
  for (const auto &dim : dimensions) {
    CT::string lookupIdentifier = getLookupIdentifier(path, filter, dim.c_str());
    std::map<std::string, DimInfo>::iterator it = lookupTableNameCacheMap.find(lookupIdentifier.c_str());

    if (it != lookupTableNameCacheMap.end()) {
      mapping[dim].tableName = (*it).second.tableName.c_str();
      mapping[dim].dataType = (*it).second.dataType;
    } else {
      mapping[dim].tableName = "";
      done = false;
    }
  }

  // We've found all requested dimensions in the lookupTableNameCache. No need to update the lookup table, we're done!
  if (done) {
#ifdef MEASURETIME
    StopWatch_Stop("<CDBAdapterPostgreSQL::getTableNamesForPathFilterAndDimensions");
#endif
    return mapping;
  }

  CPGSQLDB *DB = getDataBaseConnection();
  if (DB == NULL) {
    CDBError("Unable to connect to DB");
    throw(1);
  }


  // Query the lookup table once for the requested dimension(s)
  CT::string dimList;
  for (const auto &dim : dimensions) {
    dimList.printconcat("E'%s'", dim.c_str());
    if (&dim != &dimensions.back()) dimList += ", ";
  }
  CT::string query;
  query.print("SELECT tablename, dimension, dimension_datatype FROM %s WHERE path=E'P_%s' AND filter=E'F_%s' AND dimension IN (%s)", CDBAdapterPostgreSQL_PATHFILTERTABLELOOKUP, path, filter, dimList.c_str());
  CDBStore::Store *tableDimStore = DB->queryToStore(query.c_str());

  for (size_t i = 0; i < tableDimStore->size(); i++) {
    CT::string dim = tableDimStore->getRecord(i)->get("dimension");

    if (mapping[dim].tableName.length() == 0) {
      mapping[dim].tableName = tableDimStore->getRecord(i)->get("tablename");
      mapping[dim].dataType = tableDimStore->getRecord(i)->get("dimension_datatype");

      // Found tablename in SQL lookup table, also add to the lookupTableNameCacheMap
      CT::string lookupIdentifier = getLookupIdentifier(path, filter, dim.c_str());
      DimInfo d = {mapping[dim].tableName, mapping[dim].dataType};
      lookupTableNameCacheMap[lookupIdentifier.c_str()] = d;
    }
  }

  // Found all tablenames for requested dimensions in lookup table
  int found = tableDimStore->size();
  delete tableDimStore;
  if (found == mapping.size()) {
#ifdef MEASURETIME
    StopWatch_Stop("<CDBAdapterPostgreSQL::getTableNamesForPathFilterAndDimensions");
#endif
    return mapping;
  }

  // Check if any tables still missing?

  // We're missing tables for the requested dimensions. Create the missing tables, and add to cache and lookup table
  for (auto &m : mapping) {
    // for (const auto &[key, value] : mapping) {
    if (!m.second.tableName.empty()) continue;

    // Generate a new random table name
    CT::string tableName;
    tableName.print("t%s_%s", CTime::currentDateTime().c_str(), CServerParams::randomString(20).c_str());
    tableName.replaceSelf(":", "");
    tableName.replaceSelf("-", "");
    tableName.replaceSelf("Z", "");
    tableName.toLowerCaseSelf();

    addToLookupTable(path, filter, m.first.c_str(), tableName.c_str());
    CT::string lookupIdentifier = getLookupIdentifier(path, filter, m.first.c_str());
    DimInfo d = {tableName, m.second.dataType};
    lookupTableNameCacheMap[lookupIdentifier.c_str()] = d;
    mapping[m.first.c_str()] = d;

    // CDBDebug("@@@@@@3 Pushing tablename=%s dim_datatype=%s with id=%s", tableName.c_str(), m.second.dataType.c_str(), lookupIdentifier.c_str());
  }

#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getTableNamesForPathFilterAndDimensions");
#endif
  return mapping;
}

CT::string CDBAdapterPostgreSQL::getTableNameForPathFilterAndDimension(const char *path, const char *filter, const char *dimension, CDataSource *dataSource) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getTableNameForPathFilterAndDimension");
#endif

#if 1
  CT::string dimString(dimension);
  dimString.toLowerCaseSelf();
  std::vector<CT::string> dims { dimString };

  std::map<CT::string, DimInfo> mapping = getTableNamesForPathFilterAndDimensions(path, filter, dims, dataSource);
  CDBDebug("Using getTableNamesForPathFilterAndDimensions, found mapping %s -> %s", dimension, mapping[dimString].tableName.c_str());
  return mapping[dimString].tableName;
#else
  if (dataSource->cfgLayer->DataBaseTable.size() == 1) {
    CT::string tableName = "";

    tableName.concat(dataSource->cfgLayer->DataBaseTable[0]->value.c_str());
    CT::string dimName = "";
    if (dimension != NULL) {
      dimName = dimension;
    }

    dataSource->srvParams->makeCorrectTableName(&tableName, &dimName);
#ifdef MEASURETIME
    StopWatch_Stop("<CDBAdapterPostgreSQL::getTableNameForPathFilterAndDimension");
#endif
    return tableName;
  }

  CT::string identifier = "lookuptable/";
  identifier.concat(path);
  identifier.concat("/");
  identifier.concat(filter);
  if (dimension != NULL) {
    identifier.concat("/");
    identifier.concat(dimension);
  }
  CT::string tableName;

  std::map<std::string, DimInfo>::iterator it = lookupTableNameCacheMap.find(identifier.c_str());
  if (it != lookupTableNameCacheMap.end()) {
    tableName = (*it).second.tableName.c_str();
// CDBDebug("Returning tablename %s from map",tableName.c_str());
#ifdef MEASURETIME
    StopWatch_Stop("<CDBAdapterPostgreSQL::getTableNameForPathFilterAndDimension");
#endif
    return tableName;
  }

  // This makes use of a lookup table to find the tablename belonging to the filter and path combinations.
  // Database collumns: path filter tablename

  CT::string filterString = "F_";
  filterString.concat(filter);
  CT::string pathString = "P_";
  pathString.concat(path);
  CT::string dimString = "";
  if (dimension != NULL) {
    dimString.concat(dimension);
    dimString.toLowerCaseSelf();
  }

  // CDBDebug("lookupTableName %s",identifier.c_str());

  CT::string lookupTableName = CDBAdapterPostgreSQL_PATHFILTERTABLELOOKUP;

  // TODO CRUCIAL setting for fast perfomance on large datasets, add Unique to enable building fast lookup indexes.
  CT::string tableColumns("path varchar (511), filter varchar (511), dimension varchar (511), tablename varchar (63), dimension_datatype varchar (31), UNIQUE (path,filter,dimension) ");
  // CT::string tableColumns("path varchar (511), filter varchar (511), dimension varchar (511), tablename varchar (63)");
  CT::string mvRecordQuery;
  int status;
  CPGSQLDB *DB = getDataBaseConnection();
  if (DB == NULL) {
    CDBError("Unable to connect to DB");
    throw(1);
  }

  CT::string dimDataType;
  try {

    status = DB->checkTable(lookupTableName.c_str(), tableColumns.c_str());
    // if(status == 0){CDBDebug("OK: Table %s is available",lookupTableName.c_str());}
    if (status == 1) {
      CDBError("FAIL: Table %s could not be created: %s", lookupTableName.c_str(), tableColumns.c_str());
      CDBError("Error: %s", DB->getError());
      throw(1);
    }
    // if(status == 2){CDBDebug("OK: Table %s is created",lookupTableName.c_str());  }

    // Check wether a records exists with this path and filter combination.

    bool lookupTableIsAvailable = false;

    if (dimString.length() > 1) {
      mvRecordQuery.print("SELECT * FROM %s where path=E'%s' and filter=E'%s' and dimension=E'%s'", lookupTableName.c_str(), pathString.c_str(), filterString.c_str(), dimString.c_str());
    } else {
      mvRecordQuery.print("SELECT * FROM %s where path=E'%s' and filter=E'%s'", lookupTableName.c_str(), pathString.c_str(), filterString.c_str());
    }

    CDBStore::Store *rec = DB->queryToStore(mvRecordQuery.c_str());
    if (rec == NULL) {
      CDBError("Unable to select records: \"%s\"", mvRecordQuery.c_str());
      throw(1);
    }

    if (rec->getSize() > 0) {
      // FIXME: use column name instead of column order
      tableName.copy(rec->getRecord(0)->get(3));
      dimDataType.copy(rec->getRecord(0)->get(4));
      if (tableName.length() > 0) {
        lookupTableIsAvailable = true;
      }
    }

    delete rec;

    // Add a new lookuptable with an unique id.
    if (lookupTableIsAvailable == false) {

      CT::string randomTableString = "t";
      randomTableString.concat(CTime::currentDateTime());
      randomTableString.concat("_");
      randomTableString.concat(CServerParams::randomString(20));
      randomTableString.replaceSelf(":", "");
      randomTableString.replaceSelf("-", "");
      randomTableString.replaceSelf("Z", "");

      tableName.copy(randomTableString.c_str());
      tableName.toLowerCaseSelf();

      // mvRecordQuery.print("INSERT INTO %s values (E'%s',E'%s',E'%s',E'%s')", lookupTableName.c_str(), pathString.c_str(), filterString.c_str(), dimString.c_str(), tableName.c_str());
      addToLookupTable(path, filter, dimString.c_str(), tableName.c_str());
      // CDBDebug("%s",mvRecordQuery.c_str());
      // status = DB->query(mvRecordQuery.c_str());
      // if (status != 0) {
      //   CDBError("Unable to insert records: \"%s\"", mvRecordQuery.c_str());
      //   throw(1);
      // }
    }
    // Close the database
  } catch (int e) {
    throw(e);
  }

  if (tableName.length() > 0) {
    // CDBDebug("Pushing %s with id %s",tableName.c_str(),identifier.c_str());
    DimInfo d = {tableName, dimDataType};
    lookupTableNameCacheMap[identifier.c_str()] = d;
  }

  if (tableName.length() <= 0) {
    CDBError("Unable to generate lookup table name for %s", identifier.c_str());
    throw(1);
  }
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getTableNameForPathFilterAndDimension");
#endif
  return tableName;
#endif
}

CDBStore::Store *CDBAdapterPostgreSQL::getDimensionInfoForLayerTableAndLayerName(const char *layertable, const char *layername) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::getDimensionInfoForLayerTableAndLayerName");
#endif
  CPGSQLDB *dataBaseConnection = getDataBaseConnection();
  if (dataBaseConnection == NULL) {
    return NULL;
  }

  CT::string query;
  query.print("SELECT * FROM autoconfigure_dimensions where layerid=E'%s_%s'", layertable, layername);
  CDBStore::Store *store = dataBaseConnection->queryToStore(query.c_str());
  if (store == NULL) {
    CDBDebug("No dimension info stored for %s", layername);
  }
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::getDimensionInfoForLayerTableAndLayerName");
#endif
  return store;
}

int CDBAdapterPostgreSQL::storeDimensionInfoForLayerTableAndLayerName(const char *layertable, const char *layername, const char *netcdfname, const char *ogcname, const char *units) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::storeDimensionInfoForLayerTableAndLayerName");
#endif
  CPGSQLDB *dataBaseConnection = getDataBaseConnection();
  if (dataBaseConnection == NULL) {
    return -1;
  }

  CT::string query;
  CT::string tableColumns("layerid varchar (255), ncname varchar (255), ogcname varchar (255), units varchar (255)");

  int status = dataBaseConnection->checkTable("autoconfigure_dimensions", tableColumns.c_str());
  if (status == 1) {
    CDBError("\nFAIL: Table autoconfigure_dimensions could not be created: %s", tableColumns.c_str());
    throw(__LINE__);
  }

  query.print("INSERT INTO autoconfigure_dimensions values (E'%s_%s',E'%s',E'%s',E'%s')", layertable, layername, netcdfname, ogcname, units);
  status = dataBaseConnection->query(query.c_str());
  if (status != 0) {
    CDBError("Unable to insert records: \"%s\"", query.c_str());
    throw(__LINE__);
  }
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::storeDimensionInfoForLayerTableAndLayerName");
#endif
  return 0;
}

int CDBAdapterPostgreSQL::removeDimensionInfoForLayerTableAndLayerName(const char *layertable, const char *) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::removeDimensionInfoForLayerTableAndLayerName");
#endif
  CPGSQLDB *dataBaseConnection = getDataBaseConnection();
  if (dataBaseConnection == NULL) {
    return -1;
  }
  CT::string query;
  query.print("delete FROM autoconfigure_dimensions where layerid like E'%s%'", layertable);
  int status = dataBaseConnection->query(query.c_str());
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::removeDimensionInfoForLayerTableAndLayerName");
#endif
  return status;
}

int CDBAdapterPostgreSQL::dropTable(const char *tablename) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::dropTable");
#endif

  CPGSQLDB *dataBaseConnection = getDataBaseConnection();
  if (dataBaseConnection == NULL) {
    return -1;
  }

  CT::string query;
  query.print("drop table %s", tablename);
  if (dataBaseConnection->query(query.c_str()) != 0) {
    CDBError("Query %s failed", query.c_str());
    return 1;
  }
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::dropTable");
#endif
  return 0;
}

int CDBAdapterPostgreSQL::createDimTableOfType(const char *dimname, const char *tablename, int type) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::createDimTableOfType");
#endif

  CPGSQLDB *dataBaseConnection = getDataBaseConnection();
  if (dataBaseConnection == NULL) {
    return -1;
  }

  CT::string tableColumns("path varchar (511)");
  // 12345678901234567890
  // 0000-00-00T00:00:00Z
  if (type == 3) tableColumns.printconcat(", %s varchar (20), dim%s int", dimname, dimname);
  if (type == 2) tableColumns.printconcat(", %s varchar (64), dim%s int", dimname, dimname);
  if (type == 1) tableColumns.printconcat(", %s numeric (10, 1), dim%s int", dimname, dimname);
  if (type == 0) tableColumns.printconcat(", %s int, dim%s int", dimname, dimname);
  tableColumns.printconcat(", filedate timestamp");

  // New since 2016-02-15 projection information and level
  tableColumns.printconcat(", adaguctilinglevel int");
  // tableColumns.printconcat(", crs varchar (511)");
  tableColumns.printconcat(", minx numeric, miny numeric, maxx numeric, maxy numeric");
  tableColumns.printconcat(", startx int, starty int, countx int, county int");

  tableColumns.printconcat(", PRIMARY KEY (path, %s)", dimname);

  // CDBDebug("tableColumns = %s",tableColumns.c_str());
  int status = dataBaseConnection->checkTable(tablename, tableColumns.c_str());
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::createDimTableOfType");
#endif
  if (status == 2) {
    CDBDebug("New table created: Set indexes");
    CT::string query;
    int status = 0;
    /* Create index on dimension */
    query.print("CREATE INDEX idxdim%s on %s (%s)", tablename, tablename, dimname);
    status = dataBaseConnection->query(query.c_str());
    if (status != 0) {
      CDBDebug("Warning: Unable to create index [%s]", query.c_str());
    }

    //     /* Create index on filepath */
    //     query.print("CREATE INDEX INDEXFOR%s on %s (%s)", "path", tablename, "path"); status = dataBaseConnection->query(query.c_str()); if(status!=0) { CDBError("Unable to create index [%s]",
    //     query.c_str());throw(__LINE__); }
    //
    //     /* Create index on minx etc */
    //     query.print("CREATE INDEX INDEXFOR%s on %s (%s)", "minx", tablename, "minx"); status = dataBaseConnection->query(query.c_str()); if(status!=0) { CDBError("Unable to create index [%s]",
    //     query.c_str());throw(__LINE__); } query.print("CREATE INDEX INDEXFOR%s on %s (%s)", "miny", tablename, "miny"); status = dataBaseConnection->query(query.c_str()); if(status!=0) {
    //     CDBError("Unable to create index [%s]", query.c_str());throw(__LINE__); } query.print("CREATE INDEX INDEXFOR%s on %s (%s)", "maxx", tablename, "maxx"); status =
    //     dataBaseConnection->query(query.c_str()); if(status!=0) { CDBError("Unable to create index [%s]", query.c_str());throw(__LINE__); } query.print("CREATE INDEX INDEXFOR%s on %s (%s)", "maxy",
    //     tablename, "maxy"); status = dataBaseConnection->query(query.c_str()); if(status!=0) { CDBError("Unable to create index [%s]", query.c_str());throw(__LINE__); } query.print("CREATE INDEX
    //     INDEXFOR%s on %s (%s)", "adaguctilinglevel", tablename, "adaguctilinglevel"); status = dataBaseConnection->query(query.c_str()); if(status!=0) { CDBError("Unable to create index [%s]",
    //     query.c_str());throw(__LINE__); }

    // Fill the dimension_data_type column in the pathfilterlookup table
    query.print("SELECT data_type FROM information_schema.columns WHERE table_name='%s' and column_name='%s';", tablename, dimname);
    CDBStore::Store *dataTypeStore = dataBaseConnection->queryToStore(query.c_str(), true);
    if (dataTypeStore == NULL || dataTypeStore->getSize() != 1) {
      // FIXME: exceptions?
      CDBError("stuk???");
      throw(1);
    }
    CT::string dimDataType = dataTypeStore->getRecord(0)->get("data_type");
    delete dataTypeStore;
    query.print("UPDATE %s SET dimension_datatype='%s' WHERE tablename='%s'", CDBAdapterPostgreSQL_PATHFILTERTABLELOOKUP, dimDataType.c_str(), tablename);
    status = dataBaseConnection->query(query.c_str());
    if (status != 0) {
      CDBError("stuk???");
      throw(1);
    }
  }
  return status;
}

int CDBAdapterPostgreSQL::createDimTableInt(const char *dimname, const char *tablename) { return createDimTableOfType(dimname, tablename, 0); }

int CDBAdapterPostgreSQL::createDimTableReal(const char *dimname, const char *tablename) { return createDimTableOfType(dimname, tablename, 1); }

int CDBAdapterPostgreSQL::createDimTableString(const char *dimname, const char *tablename) { return createDimTableOfType(dimname, tablename, 2); }

int CDBAdapterPostgreSQL::createDimTableTimeStamp(const char *dimname, const char *tablename) { return createDimTableOfType(dimname, tablename, 3); }

int CDBAdapterPostgreSQL::checkIfFileIsInTable(const char *tablename, const char *filename) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::checkIfFileIsInTable");
#endif
  int fileIsOK = 1;
  CPGSQLDB *dataBaseConnection = getDataBaseConnection();
  if (dataBaseConnection == NULL) {
    return -1;
  }

  CT::string query;
  query.print("select path from %s where path = '%s' limit 1", tablename, filename);
  CDBStore::Store *pathValues = dataBaseConnection->queryToStore(query.c_str());
  if (pathValues == NULL) {
    CDBError("Query failed");
    throw(__LINE__);
  }
  if (pathValues->getSize() == 1) {
    fileIsOK = 0;
  } else {
    fileIsOK = 1;
  }
  delete pathValues;
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::checkIfFileIsInTable");
#endif
  return fileIsOK;
}

int CDBAdapterPostgreSQL::removeFile(const char *tablename, const char *file) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::removeFile");
#endif
  CPGSQLDB *dataBaseConnection = getDataBaseConnection();
  if (dataBaseConnection == NULL) {
    return -1;
  }

  CT::string query;
  query.print("delete from %s where path = '%s'", tablename, file);
  int status = dataBaseConnection->query(query.c_str());
  if (status != 0) throw(__LINE__);
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::removeFile");
#endif
  return 0;
}

int CDBAdapterPostgreSQL::removeFilesWithChangedCreationDate(const char *tablename, const char *file, const char *creationDate) {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::removeFilesWithChangedCreationDate");
#endif
  CPGSQLDB *dataBaseConnection = getDataBaseConnection();
  if (dataBaseConnection == NULL) {
    return -1;
  }

  CT::string query;
  query.print("delete from %s where path = '%s' and (filedate != '%s' or filedate is NULL)", tablename, file, creationDate);
  int status = dataBaseConnection->query(query.c_str());
  if (status != 0) {
    // CDBError("removeFilesWithChangedCreationDate exception");
    throw(__LINE__);
  }
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::removeFilesWithChangedCreationDate");
#endif
  return 0;
}

int CDBAdapterPostgreSQL::setFileInt(const char *tablename, const char *file, int dimvalue, int dimindex, const char *filedate, GeoOptions *geoOptions) {
  CT::string values;
  values.print("('%s',%d,'%d','%s','%d','%f','%f','%f','%f','%d','%d','%d','%d')", file, dimvalue, dimindex, filedate, geoOptions->level, geoOptions->bbox[0], geoOptions->bbox[1], geoOptions->bbox[2],
               geoOptions->bbox[3], geoOptions->indices[0], geoOptions->indices[1], geoOptions->indices[2], geoOptions->indices[3]);
#ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("Adding INT %s", values.c_str());
#endif
  fileListPerTable[tablename].push_back(values.c_str());
  return 0;
}
int CDBAdapterPostgreSQL::setFileReal(const char *tablename, const char *file, double dimvalue, int dimindex, const char *filedate, GeoOptions *geoOptions) {
  CT::string values;
  values.print("('%s',%f,'%d','%s','%d','%f','%f','%f','%f','%d','%d','%d','%d')", file, dimvalue, dimindex, filedate, geoOptions->level, geoOptions->bbox[0], geoOptions->bbox[1], geoOptions->bbox[2],
               geoOptions->bbox[3], geoOptions->indices[0], geoOptions->indices[1], geoOptions->indices[2], geoOptions->indices[3]);
#ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("Adding REAL %s", values.c_str());
#endif
  fileListPerTable[tablename].push_back(values.c_str());
  return 0;
}
int CDBAdapterPostgreSQL::setFileString(const char *tablename, const char *file, const char *dimvalue, int dimindex, const char *filedate, GeoOptions *geoOptions) {
  CT::string values;
  values.print("('%s','%s','%d','%s','%d','%f','%f','%f','%f','%d','%d','%d','%d')", file, dimvalue, dimindex, filedate, geoOptions->level, geoOptions->bbox[0], geoOptions->bbox[1],
               geoOptions->bbox[2], geoOptions->bbox[3], geoOptions->indices[0], geoOptions->indices[1], geoOptions->indices[2], geoOptions->indices[3]);
#ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("Adding STRING %s", values.c_str());
#endif
  fileListPerTable[tablename].push_back(values.c_str());
  return 0;
}
int CDBAdapterPostgreSQL::setFileTimeStamp(const char *tablename, const char *file, const char *dimvalue, int dimindex, const char *filedate, GeoOptions *geoOptions) {
  CT::string values;
  values.print("('%s','%s','%d','%s','%d','%f','%f','%f','%f','%d','%d','%d','%d')", file, dimvalue, dimindex, filedate, geoOptions->level, geoOptions->bbox[0], geoOptions->bbox[1],
               geoOptions->bbox[2], geoOptions->bbox[3], geoOptions->indices[0], geoOptions->indices[1], geoOptions->indices[2], geoOptions->indices[3]);
#ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("Adding TIMESTAMP %s", values.c_str());
#endif
  fileListPerTable[tablename].push_back(values.c_str());
  return 0;
}
int CDBAdapterPostgreSQL::addFilesToDataBase() {
#ifdef MEASURETIME
  StopWatch_Stop(">CDBAdapterPostgreSQL::addFilesToDataBase");
#endif
#ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("Adding files to database");
#endif
  CPGSQLDB *dataBaseConnection = getDataBaseConnection();
  if (dataBaseConnection == NULL) {
    return -1;
  }

  CT::string multiInsert = "";
  for (std::map<std::string, std::vector<std::string>>::iterator it = fileListPerTable.begin(); it != fileListPerTable.end(); ++it) {
#ifdef CDBAdapterPostgreSQL_DEBUG
    CDBDebug("Updating table %s with %d records", it->first.c_str(), (it->second.size()));
#endif
    size_t maxIters = 50;
    if (it->second.size() > 0) {
      size_t rowNumber = 0;
      do {
        multiInsert.print("INSERT into %s VALUES ", it->first.c_str());
        for (size_t j = 0; j < maxIters; j++) {
          if (j > 0) multiInsert.concat(",");
          multiInsert.concat(it->second[rowNumber].c_str());
          rowNumber++;
          if (rowNumber >= it->second.size()) break;
        }
        // CDBDebug("Inserting %d bytes ",multiInsert.length());
        int status = dataBaseConnection->query(multiInsert.c_str());
        if (status != 0) {
          CDBError("Query failed [%s]:", dataBaseConnection->getError());
          throw(__LINE__);
        }
      } while (rowNumber < it->second.size());
    }
    it->second.clear();
  }
#ifdef CDBAdapterPostgreSQL_DEBUG
  CDBDebug("clearing arrays");
#endif
  fileListPerTable.clear();
#ifdef MEASURETIME
  StopWatch_Stop("<CDBAdapterPostgreSQL::addFilesToDataBase");
#endif
  return 0;
}
