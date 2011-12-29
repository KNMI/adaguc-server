#ifndef CDBFileScanner_H
#define CDBFileScanner_H

#include "CDebugger.h"
#include "CStopWatch.h"
#include "CDFObjectStore.h"
#include "CServerError.h"
#include "CDirReader.h"
#include "CPGSQLDB.h"
#include "CADAGUC_time.h"

class CDBFileScanner{
private:
  DEF_ERRORFUNCTION();
  static int createDBUpdateTables(CPGSQLDB *DB,CDataSource *dataSource,int &removeNonExistingFiles);
  static int DBLoopFiles(CPGSQLDB *DB,CDataSource *dataSource,int removeNonExistingFiles,CDirReader *dirReader);
 
public:
  static int searchFileNames(CDataSource *dataSource,CDirReader *dirReader,CT::string *tailPath);
  static int updatedb(const char *pszDBParams, CDataSource *dataSource,CT::string *tailPath,CT::string *_layerPathToScan);
  
};

#endif