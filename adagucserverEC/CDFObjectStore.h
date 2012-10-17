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
  
  /**
   * Get a CDFReader based on information in the datasource. In the Layer element this can be configured with <DataReader>HDF5</DataReader>
   * @param dataSource The configured datasource or NULL pointer. NULL pointer defaults to a NetCDF/OPeNDAP reader
   */
  static CDFReader *getCDFReader(CDataSource *dataSource);
  
  /**
   * Get a CDFReader based on fileName information, currently based on extension.
   * @param fileName The fileName
   * @return The CDFReader
   */
  static CDFReader *getCDFReader(const char *fileName);
  DEF_ERRORFUNCTION();
public:
  ~CDFObjectStore(){
    clear();
  }
  
  /**
   * Gets the current allocated object store
   */
  static CDFObjectStore *getCDFObjectStore();
  
  /**
   * Get a CDFObject based with opened and configured CDF reader for a filename/OPeNDAP url and a dataSource.
   * @param dataSource The configured datasource or NULL pointer. NULL pointer defaults to a NetCDF/OPeNDAP reader
   * @param fileName The filename to read.
   */
  CDFObject *getCDFObject(CDataSource *dataSource,const char *fileName);
  CDFObject *deleteCDFObject(CDFObject **cdfObject);
  
  /**
   * Clean the CDFObject store and throw away all readers and objects
   */
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
