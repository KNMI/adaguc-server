#ifndef CPGSQLDB_H
#define CPGSQLDB_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libpq-fe.h"                                   /* libpq header file */
#include "CDefinitions.h"
#include "CTypes.h"
#include "CDebugger.h"
class CPGSQLDB{
 // This class must be used within the same thread!
  private:
    DEF_ERRORFUNCTION();
    PGconn     *connection;
    PGresult   *result;
    char        szTemp[MAX_STR_LEN+1];
    char        LastErrorMsg[MAX_STR_LEN+1];
    int dConnected;
    void clearResult();
  public:

    const char *getError();
    CPGSQLDB();
    ~CPGSQLDB();
    int close();
    int connect(const char * pszOptions);
    int checkTable(const char * pszTableName,const char *pszColumns);
    int query(const char *pszQuery);
    CT::string* query_select(const char *pszQuery);
    CT::string* query_select(const char *pszQuery,int dColumn);
};
#endif

