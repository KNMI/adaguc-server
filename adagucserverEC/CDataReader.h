#ifndef CDataReader_H
#define CDataReader_H
#include "CDataSource.h"
#include "CServerError.h"
#include "CDefinitions.h"
#include "CDirReader.h"
#include "CPGSQLDB.h"
#include "CADAGUC_time.h"
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"
#include "CCDFHDF5IO.h"
#include <math.h>
#include "CStopWatch.h"
#include "CDebugger.h"
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
       for(size_t j=0;j<fileNames.size();j++){
         delete fileNames[j];
         delete cdfObjects[j];
       }
       fileNames.clear();
       cdfObjects.clear();
    }
    static CDFObjectStore *getCDFObjectStore();
    CDFObject *getCDFObject(CDataSource *dataSource,const char *fileName,bool returnNew);
    
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
    CDFObject *getCDFObject(){
      return thisCDFObject;
    }
    int getTimeString(char * pszTime);
    int getTimeUnit(char * pszTime);
    const char *getFileName(){return FileName.c_str();}
    int open(CDataSource *dataSource,int mode,const char *cacheLocation);
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
