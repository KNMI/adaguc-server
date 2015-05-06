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

#ifndef CPGSQLDB_H
#define CPGSQLDB_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpq-fe.h"                                   /* libpq header file */
#include "CTypes.h"
#include "CDebugger.h"
#include "CDBStore.h"
#define CPGSQLDB_MAX_STR_LEN 8191



class CPGSQLDB{
 // This class must be used within the same thread!
  private:
    DEF_ERRORFUNCTION();
    PGconn     *connection;
    PGresult   *result;
    char        szTemp[CPGSQLDB_MAX_STR_LEN+1];
    char        LastErrorMsg[CPGSQLDB_MAX_STR_LEN+1];
    int dConnected;
    void clearResult();
    
  public:

    const char *getError();
    CPGSQLDB();
    ~CPGSQLDB();
    int close2();
    int connect(const char * pszOptions);
    int checkTable(const char * pszTableName,const char *pszColumns);
    int query(const char *pszQuery);
//     CT::string* query_select_deprecated(const char *pszQuery);
//     CT::string* query_select_deprecated(const char *pszQuery,int dColumn);
    /**
      * Queries to a store
      * @param pszQuery The query to execute
      * @param throwException Throw an (int) exception with a CDB_ERROR code if something fails
      * @return CDB::Store containing the results. Returns NULL or throws exceptions when fails.
      */
    CDBStore::Store* queryToStore(const char *pszQuery,bool throwException);
    
    /**
      * Queries to a store
      * @param pszQuery The query to execute
      * @return CDB::Store containing the results. Returns NULL when fails.
      */
    CDBStore::Store* queryToStore(const char *pszQuery){
      return queryToStore(pszQuery,false);
    }
};
#endif

