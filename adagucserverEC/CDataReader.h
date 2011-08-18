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
#include "CStopWatch.h"



//Datasource can share multiple cdfObjects
//A cdfObject is allways opened using a sourceImage path/filter combo
// When a CDFObject is already opened
class CDFObjectStore{
  private:
    std::vector <CT::string*> fileNames;
    std::vector <CDFObject*> cdfObjects;

    DEF_ERRORFUNCTION();
  public:
    ~CDFObjectStore(){
      clear();
    }
    
    static CDFObjectStore *getCDFObjectStore();
    CDFObject *getCDFObject(CDataSource *dataSource,const char *fileName);
    CDFObject *deleteCDFObject(CDFObject **cdfObject);
    void clear();
};

class CDataReader{
  private:
    DEF_ERRORFUNCTION();
    CDataSource *sourceImage;
    //CDFReader * cdfReader;
    CDFObject *thisCDFObject;

    CT::string FileName;
    
  public:
    CDataReader(){}
    ~CDataReader(){close();}
  
    static CDFReader *getCDFReader(CDataSource *sourceImage);
    static int autoConfigureDimensions(CDataSource *dataSource,bool tryToFindInterval);
    static int autoConfigureStyles(CDataSource *dataSource);
    static int justLoadAFileHeader(CDataSource *dataSource);
    static int getCacheFileName(CDataSource *dataSource,CT::string *cacheName);
    static int getCacheFileName(CServerParams *srvParams,CT::string *uniqueIDFor2DField,const char *fileName);
  //  CDFObject *getCDFObject(){
    //  return thisCDFObject;
    //}
    void dump(CT::string *dumpString){
      CDF::dump(thisCDFObject,dumpString);
    }
    int getTimeString(char * pszTime);
    int getTimeUnit(char * pszTime);
    const char *getFileName(){return FileName.c_str();}
    int open(CDataSource *dataSource,int mode);
    int close();
};

class CDBFileScanner{
  private:
    DEF_ERRORFUNCTION();
    static int createDBUpdateTables(CPGSQLDB *DB,CDataSource *sourceImage,int &removeNonExistingFiles);
    static int DBLoopFiles(CPGSQLDB *DB,CDataSource *sourceImage,int removeNonExistingFiles,CDirReader *dirReader);
    
    
  public:
    static int searchFileNames(CDataSource *sourceImage,CDirReader *dirReader,CT::string *tailPath);
    static int updatedb(const char *pszDBParams, CDataSource *dataSource,CT::string *tailPath,CT::string *_layerPathToScan);
 
};
#endif
