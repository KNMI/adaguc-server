#ifndef CDataReader_H
#define CDataReader_H
#include <math.h>
#include "CDebugger.h"
#include "CDataSource.h"
#include "CServerError.h"
#include "CDirReader.h"
#include "CPGSQLDB.h"
#include "CADAGUC_time.h"
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"
#include "CCDFHDF5IO.h"
#include "CProj4ToCF.h"
#include "CStopWatch.h"
#include <sys/stat.h>
#include "CDBFileScanner.h"
#include "CDFObjectStore.h"

class CDataReader{
  private:
    DEF_ERRORFUNCTION();
    CDataSource *dataSource;
    //CDFReader * cdfReader;
    CDFObject *thisCDFObject;

    CT::string FileName;
    
  public:
    CDataReader(){}
    ~CDataReader(){close();}
  
    
    static int autoConfigureDimensions(CDataSource *dataSource,bool tryToFindInterval);
    static int autoConfigureStyles(CDataSource *dataSource);
    static int justLoadAFileHeader(CDataSource *dataSource);
    static int getCacheFileName(CDataSource *dataSource,CT::string *cacheName);
    static int getCacheFileName(CServerParams *srvParams,CT::string *uniqueIDFor2DField,const char *fileName);
    static int getTimeDimIndex( CDFObject *cdfObject, CDF::Variable * var);
    static CDF::Variable *getTimeVariable( CDFObject *cdfObject, CDF::Variable * var);
    void dump(CT::string *dumpString){
      CDF::dump(thisCDFObject,dumpString);
    }
    
    int getTimeString(char * pszTime);
    CDF::Variable *getTimeVariable();
    int getTimeUnit(char * pszTime);
    const char *getFileName(){return FileName.c_str();}
    int open(CDataSource *dataSource,int mode);
    int close();
};

#endif
