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

#include "CPGSQLDB.h"
#include "traceTimings/traceTimings.h"
// #define CPGSQLDB_DEBUG_H
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
    clearResult();
    traceTimingsSpanStart(TraceTimingType::DBCLOSE);
    PQfinish(connection);
    traceTimingsSpanEnd(TraceTimingType::DBCLOSE);
  }
  dConnected = 0;
  return 0;
}

int CPGSQLDB::connect(const char *pszOptions) {
  LastErrorMsg[0] = '\0';
  if (dConnected == 1) return 0;
  // CDBDebug("[DB CONNECT]");
  traceTimingsSpanStart(TraceTimingType::DBCONNECT);
  connection = PQconnectdb(pszOptions);
  traceTimingsSpanEnd(TraceTimingType::DBCONNECT);
  if (PQstatus(connection) == CONNECTION_BAD) {
    snprintf(szTemp, CPGSQLDB_MAX_STR_LEN, "Connection to database failed: %s", PQerrorMessage(connection));
    CDBError(szTemp);
    return 1;
  }
  dConnected = 1;
  return 0;
}

int CPGSQLDB::_checkTable(const char *pszTableName, const char *pszColumns) {
  // returncodes:
  // 0 = no change
  // 1 = error
  // 2 = table created

  LastErrorMsg[0] = '\0';

  if (dConnected == 0) {
    CDBError("checkTable: Not connected to DB");
    return 1;
  }

  CT::string queryString;
  queryString.print("SELECT EXISTS (SELECT 1 FROM information_schema.tables WHERE table_name = '%s') AS table_existence;", pszTableName);
#ifdef CPGSQLDB_DEBUG_H
  CDBDebug("checkTable PQexec SELECT EXISTS  %s", pszTableName);
#endif
  result = PQexec(connection, queryString.c_str()); /* send the query */
  if (PQresultStatus(result) != PGRES_TUPLES_OK)    /* did the query fail? */
  {
    CDBError("checkTable: select from pg_tables failed");
    clearResult();
    return 1;
  }

  if (PQntuples(result) == 1 && PQgetvalue(result, 0, 0)[0] == 't') {
    clearResult();
    return 0;
  }
  clearResult();

  // No table exists yet
  queryString.print("CREATE TABLE %s (%s)", pszTableName, pszColumns);
#ifdef CPGSQLDB_DEBUG_H
  CDBDebug("checkTable PQexec CREATE TABLE %s", pszTableName);
#endif
  result = PQexec(connection, queryString.c_str()); /* send the query */
  if (PQresultStatus(result) != PGRES_COMMAND_OK)   /* did the query fail? */
  {

    snprintf(LastErrorMsg, CPGSQLDB_MAX_STR_LEN, "%s: %s (%s)", PQresStatus(PQresultStatus(result)), PQresultErrorMessage(result), queryString.c_str());

    // snprintf(szTemp,CPGSQLDB_MAX_STR_LEN,"checkTable: CREATE TABLE %s failed",pszTableName);
    // CDBError(LastErrorMsg);
    clearResult();
    return 1;
  }
  // Table created set status to 2
  clearResult();
  return 2;
}

int CPGSQLDB::query(const char *pszQuery) {
  LastErrorMsg[0] = '\0';
  if (dConnected == 0) {
    CDBError("query: Not connected to DB");
    return 1;
  }
#ifdef CPGSQLDB_DEBUG_H
  CDBDebug("query PQexec %s", pszQuery);
#endif
  traceTimingsSpanStart(TraceTimingType::DB);
  result = PQexec(connection, pszQuery);
  traceTimingsSpanEnd(TraceTimingType::DB);
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

CDBStore::Store *CPGSQLDB::_queryToStore(const char *pszQuery, bool throwException) {

  LastErrorMsg[0] = '\0';

  if (dConnected == 0) {
    if (throwException) {
      throw CDB_CONNECTION_ERROR;
    }
    CDBError("queryToStore: Not connected to DB");
    return NULL;
  }
#ifdef CPGSQLDB_DEBUG_H
  CDBDebug("queryToStore PQexec %s", pszQuery);
#endif
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
  CDBStore::ColumnModel *colModel = new CDBStore::ColumnModel();
  // colModel

  for (size_t colNumber = 0; colNumber < numCols; colNumber++) {
    colModel->push(PQfname(result, colNumber));
  }

  CDBStore::Store *store = new CDBStore::Store(colModel);

  for (size_t rowNumber = 0; rowNumber < numRows; rowNumber++) {
    CDBStore::Record record;
    record.setColumnModel(colModel);
    for (size_t colNumber = 0; colNumber < numCols; colNumber++) {
      record.push(PQgetvalue(result, rowNumber, colNumber));
    }
    store->push(record);
  }

  clearResult();

  return store;
}

CDBStore::Store *CPGSQLDB::queryToStore(const char *pszQuery, bool throwException) {
  try {
    traceTimingsSpanStart(TraceTimingType::DB);
    auto result = this->_queryToStore(pszQuery, throwException);
    traceTimingsSpanEnd(TraceTimingType::DB);
    return result;
  } catch (int ex) {
    traceTimingsSpanEnd(TraceTimingType::DB);
    throw(ex);
  }
}

int CPGSQLDB::checkTable(const char *pszTableName, const char *pszColumns) {
  traceTimingsSpanStart(TraceTimingType::DBCHECKTABLE);
  auto result = this->_checkTable(pszTableName, pszColumns);
  traceTimingsSpanEnd(TraceTimingType::DBCHECKTABLE);
  return result;
}