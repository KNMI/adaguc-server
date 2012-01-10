#ifndef CDFObjectStore_H
#define CDFObjectStore_H

#include "CDebugger.h"
#include "CStopWatch.h"
#include "CDataSource.h"
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"
#include "CCDFHDF5IO.h"

//Datasource can share multiple cdfObjects
//A cdfObject is allways opened using a dataSource path/filter combo
// When a CDFObject is already opened
class CDFObjectStore{
private:
  std::vector <CT::string*> fileNames;
  std::vector <CDFObject*> cdfObjects;
  std::vector <CDFReader*> cdfReaders;
  static CDFReader *getCDFReader(CDataSource *dataSource);
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
/*extern CDFObjectStore cdfObjectStore;
CDFObjectStore cdfObjectStore;
CDFReader *getCDFReader(CDataSource *dataSource);
CDFObject *getCDFObject(CDataSource *dataSource,const char *fileName);
CDFObjectStore *getCDFObjectStore();
CDFObject *deleteCDFObject(CDFObject **cdfObject);
void clear();*/

#endif
