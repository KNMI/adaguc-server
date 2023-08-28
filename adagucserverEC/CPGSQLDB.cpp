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
#ifdef ADAGUC_USE_POSTGRESQL
#include "CPGSQLDB.h"
#include <fstream>
#include <sstream>
#include <algorithm>

const char *CPGSQLDB::className = "CPGSQLDB";
void CPGSQLDB::clearResult() {
  if (result != NULL) PQclear(result);
  result = NULL;
}

const char *CPGSQLDB::getError() { return LastErrorMsg; }

CPGSQLDB::CPGSQLDB() {
  connection = NULL;
  result = NULL;
  dConnected = 0;
}

CPGSQLDB::~CPGSQLDB() { close2(); }

int CPGSQLDB::close2() {

  if (dConnected == 1) {
    // CDBDebug("[DB DISCONNECT]");
    clearResult();
    PQfinish(connection);
  }
  dConnected = 0;
  return 0;
}

int CPGSQLDB::connect(const char *pszOptions) {
  LastErrorMsg[0] = '\0';
  if (dConnected == 1) return 0;
  // CDBDebug("[DB CONNECT]");
  connection = PQconnectdb(pszOptions);
  if (PQstatus(connection) == CONNECTION_BAD) {
    snprintf(szTemp, CPGSQLDB_MAX_STR_LEN, "Connection to database failed: %s", PQerrorMessage(connection));
    CDBError(szTemp);
    return 1;
  }
  dConnected = 1;
  return 0;
}

int CPGSQLDB::checkTable(const char *pszTableName, const char *pszColumns) {
  // returncodes:
  // 0 = no change
  // 1 = error
  // 2 = table created

  LastErrorMsg[0] = '\0';
  int i;
  if (dConnected == 0) {
    CDBError("checkTable: Not connected to DB");
    return 1;
  }
  char query_string[1024];
  snprintf(query_string, 1023, "select * from pg_tables where schemaname='public';");
  result = PQexec(connection, query_string);     /* send the query */
  if (PQresultStatus(result) != PGRES_TUPLES_OK) /* did the query fail? */
  {
    CDBError("checkTable: select from pg_tables failed");
    clearResult();
    return 1;
  }

  for (i = 0; i < PQntuples(result); i++) {
    char *pqval = PQgetvalue(result, i, 1);
    if (strncmp(pszTableName, pqval, strlen(pszTableName)) == 0 && strlen(pqval) == strlen(pszTableName)) {
      clearResult();
      // CDBDebug("Found: %s == %s",pqval,pszTableName);
      return 0;
    } else {
      // CDBDebug("Found: [%s] != [%s]",pqval,pszTableName);
    }
  }
  // No table exists yet
  if (i == PQntuples(result)) {
    clearResult();

    snprintf(query_string, 1023, "CREATE TABLE %s (%s)", pszTableName, pszColumns);

    result = PQexec(connection, query_string);      /* send the query */
    if (PQresultStatus(result) != PGRES_COMMAND_OK) /* did the query fail? */
    {

      snprintf(LastErrorMsg, CPGSQLDB_MAX_STR_LEN, "%s: %s (%s)", PQresStatus(PQresultStatus(result)), PQresultErrorMessage(result), query_string);

      // snprintf(szTemp,CPGSQLDB_MAX_STR_LEN,"checkTable: CREATE TABLE %s failed",pszTableName);
      // CDBError(LastErrorMsg);
      clearResult();
      return 1;
    }
    // Table created set status to 2
    clearResult();
    return 2;
  }

  clearResult();
  return 0;
}

int CPGSQLDB::query(const char *pszQuery) {
  LastErrorMsg[0] = '\0';
  if (dConnected == 0) {
    CDBError("query: Not connected to DB");
    return 1;
  }
  result = PQexec(connection, pszQuery);
  if (PQresultStatus(result) != PGRES_COMMAND_OK) /* did the query fail? */
  {
    snprintf(LastErrorMsg, CPGSQLDB_MAX_STR_LEN, "%s: %s (%s)", PQresStatus(PQresultStatus(result)), PQresultErrorMessage(result), pszQuery);
    // snprintf(szTemp,CPGSQLDB_MAX_STR_LEN,"query: [%s] failed: %s",pszQuery,PQerrorMessage(connection));
    // CDBError(szTemp);
    clearResult();
    return 1;
  }
  clearResult();
  return 0;
}
// CT::string* CPGSQLDB::query_select_deprecated(const char *pszQuery,int dColumn){
// //  CDBDebug("query_select %d %s",dColumn,pszQuery);
//   LastErrorMsg[0]='\0';
//   int i;
//   if(dConnected == 0){
//     CDBError("query_select: Not connected to DB");
//     return NULL;
//   }
//
//   result = PQexec(connection, pszQuery);
//
//   if (PQresultStatus(result) != PGRES_TUPLES_OK) // did the query fail?
//   {
//     //snprintf(szTemp,CPGSQLDB_MAX_STR_LEN,"query_select: %s failed",pszQuery);
//     //CDBError(szTemp);
//     clearResult();
//     return NULL;
//   }
//   int n=PQntuples(result);
//   CT::string *strings=new CT::string[n+1];
//   for(i=0;i<n;i++){
//     strings[i].copy(PQgetvalue(result, i, dColumn));
//     strings[i].count=n;
//   }
//   CT::CTlink<CT::string>(strings,n);
//   clearResult();
//   return strings;
// }
// CT::string* CPGSQLDB::query_select_deprecated(const char *pszQuery){
//   return query_select_deprecated(pszQuery,0);
// }

CDBStore::Store *CPGSQLDB::queryToStore(const char *pszQuery, bool throwException) {
  // CDBDebug("query_select %s",pszQuery);
  LastErrorMsg[0] = '\0';

  if (dConnected == 0) {
    if (throwException) {
      throw CDB_CONNECTION_ERROR;
    }
    CDBError("queryToStore: Not connected to DB");
    return NULL;
  }

  result = PQexec(connection, pszQuery);

  if (PQresultStatus(result) != PGRES_TUPLES_OK) // did the query fail?
  {
    // snprintf(szTemp,CPGSQLDB_MAX_STR_LEN,"query_select: %s failed",pszQuery);
    // CDBError(szTemp);
    clearResult();
    if (throwException) {
      throw CDB_QUERYFAILED;
    }
    return NULL;
  }
  size_t numCols = PQnfields(result);
  size_t numRows = PQntuples(result);
  if (numCols == 0) {
    clearResult();
    if (throwException) {
      throw CDB_NODATA;
    }
    return NULL;
  }
  CDBStore::ColumnModel *colModel = new CDBStore::ColumnModel(PQnfields(result));
  // colModel

  for (size_t colNumber = 0; colNumber < numCols; colNumber++) {
    colModel->setColumn(colNumber, PQfname(result, colNumber));
  }

  CDBStore::Store *store = new CDBStore::Store(colModel);

  for (size_t rowNumber = 0; rowNumber < numRows; rowNumber++) {
    CDBStore::Record *record = new CDBStore::Record(colModel);
    for (size_t colNumber = 0; colNumber < numCols; colNumber++) record->push(colNumber, PQgetvalue(result, rowNumber, colNumber));
    store->push(record);
  }

  clearResult();
  return store;
  ;
}

std::string CPGSQLDB::queryToString(const char *pszQuery, bool throwException) {
  LastErrorMsg[0] = '\0';

  if (dConnected == 0) {
    if (throwException) {
      throw CDB_CONNECTION_ERROR;
    }
    CDBError("queryToStore: Not connected to DB");
    return "";
  }
  CDBDebug("query was: %s", pszQuery);
  result = PQexec(connection, pszQuery);

  if (PQresultStatus(result) != PGRES_TUPLES_OK) // did the query fail?
  {
    clearResult();
    if (throwException) {
      throw CDB_QUERYFAILED;
    }
    return "wrong1";
  }

  size_t numCols = PQnfields(result);
  size_t numRows = PQntuples(result);
  CDBDebug("Number of columns: %zu\n", numCols);
  CDBDebug("Number of rows: %zu\n", numRows);

  if (numCols == 0) {
    clearResult();
    if (throwException) {
      throw CDB_NODATA;
    }
    return "wrong2";
  }

  std::string queryResults;

  for (size_t rowNumber = 0; rowNumber < numRows; rowNumber++) {
    for (size_t colNumber = 0; colNumber < numCols; colNumber++) {
      queryResults.append(PQgetvalue(result, rowNumber, colNumber));

      if (colNumber < numCols - 1) { // Append comma if not last column
        queryResults.append(",");
      }
    }

    if (rowNumber < numRows - 1) { // Append carriage return if not last row
      queryResults.append("\r\n");
    }
  }

  clearResult();
  CDBDebug("QueryResults %s", queryResults);
  return queryResults;
}

int CPGSQLDB::createTableWithCSVData(const char *pszTableName, const char *pszFileName) {
  // returncodes:
  // 0 = no change
  // 1 = error
  // 2 = table created

  CDBDebug("Table name: %s\n", pszTableName);
  CDBDebug("File name: %s\n", pszFileName);

  if (dConnected == 0) {
    CDBError("createTableWithCSVData: Not connected to DB");
    return 1;
  }

  std::ifstream infile(pszFileName);
  if (!infile) {
    CDBError("createTableWithCSVData: Could not open file");
    return 1;
  }

  char query_string[2048];
  snprintf(query_string, 2047, "CREATE TABLE IF NOT EXISTS %s (radar TEXT, datetime TEXT, height TEXT, csv_data TEXT, PRIMARY KEY (radar, datetime, height));", pszTableName);

  result = PQexec(connection, query_string);
  if (PQresultStatus(result) != PGRES_COMMAND_OK) {
    CDBError("createTableWithCSVData: CREATE TABLE failed");
    clearResult();
    return 1;
  }

  std::string line;
  while (std::getline(infile, line)) {
    std::stringstream ss(line);
    std::string discard, station, timestamp, height;

    // Get fields from the CSV line
    std::getline(ss, discard, ',');   // Discard the first field
    std::getline(ss, station, ',');   // Second field
    std::getline(ss, timestamp, ','); // Third field
    std::getline(ss, height, ',');    // Fourth field

    // Remove double quotes from station
    station.erase(std::remove_if(station.begin(), station.end(), [](char c) { return c == '\"'; }), station.end());

    std::string restOfLine;
    std::getline(ss, restOfLine, '\n'); // Get the rest of the line, if there is any

    // The whole line, minus the first field
    std::string newLine = station + "," + timestamp + "," + height + "," + restOfLine;

    CDBDebug("CSV DATA: %s", newLine.c_str());
    snprintf(query_string, 2047, "INSERT INTO %s (radar, datetime, height, csv_data) VALUES ('%s', '%s', '%s', '%s') ON CONFLICT DO NOTHING;", pszTableName, station.c_str(), timestamp.c_str(),
             height.c_str(), newLine.c_str());
    CDBDebug("query_string: %s", query_string);
    result = PQexec(connection, query_string);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
      CDBError("createTableWithCSVData: INSERT INTO TABLE failed");
      clearResult();
      return 1;
    }
  }
  infile.close();

  // Prepare and execute count query
  snprintf(query_string, 2047, "SELECT COUNT(*) FROM %s;", pszTableName);
  result = PQexec(connection, query_string);
  if (PQresultStatus(result) != PGRES_TUPLES_OK) {
    CDBError("createTableWithCSVData: COUNT query failed");
    clearResult();
    return 1;
  }

  // Fetch and print the count
  if (PQntuples(result) > 0) {
    int count = atoi(PQgetvalue(result, 0, 0));
    CDBDebug("Total tuples in %s: %d", pszTableName, count);
  }

  clearResult();
  return 2;
}
#endif