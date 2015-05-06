#ifndef CDBADAPTER_H
#define CDBADAPTER_H

#include <stdlib.h>
#include "CImageDataWriter.h"
#include "CServerParams.h"
#include "CDataSource.h"
#include "CDFObjectStore.h"
#include "CCDFStore.h"
#include "CDBStore.h"
#include "CDebugger.h"

class CDBAdapter{
public:
  CDBAdapter(){}
  virtual ~CDBAdapter() = 0;
  virtual int setConfig(CServerConfig::XMLE_Configuration *cfg) = 0;
  
  virtual CDBStore::Store *getReferenceTime(const char *netcdfDimName, const char *netcdfTimeDimName, const char *timeValue, const char *timeTableName, const char *tableName) = 0;
  virtual CDBStore::Store *getClosestReferenceTimeToSystemTime(const char *netcdfDimName, const char *tableName) = 0; 
  
  /**
    * Makes use of a lookup table to find the tablename belonging to the filter and path combinations.
    * @param path The path of the layer
    * @param filter The filter of the layer
    * @param dimension The dimension of the layer, can be NULL if not used.
    * @param dataSource The dataSource
    * @return Tablename on succes, throws integer exception on failure.
    */
  virtual CT::string getTableNameForPathFilterAndDimension(const char *path,const char *filter, const char * dimension,CDataSource *dataSource) = 0;

  virtual CDBStore::Store *getMin(const char *name, const char *table) = 0;
  virtual CDBStore::Store *getMax(const char *name, const char *table) = 0;
  virtual CDBStore::Store *getUniqueValuesOrderedByValue(const char *name, int limit, bool orderDescOrAsc,const char *table) = 0;
  virtual CDBStore::Store *getUniqueValuesOrderedByIndex(const char *name, int limit, bool orderDescOrAsc,const char *table) = 0;
  
  /*Dimension Info*/
  virtual CDBStore::Store *getDimensionInfoForLayerTableAndLayerName  (const char *layertable,const char *layername) = 0;
  virtual int              storeDimensionInfoForLayerTableAndLayerName(const char *layertable,const char *layername,const char *netcdfname,const char *ogcname,const char *units) = 0;
  
  
  virtual CDBStore::Store *getFilesAndIndicesForDimensions(CDataSource *dataSource,int limit) = 0;
  
  virtual int              autoUpdateAndScanDimensionTables(CDataSource *dataSource) = 0;
  
  
  virtual int              dropTable(const char *tablename) = 0;
  virtual int              createDimTableInt(const char *dimname,const char *tablename) = 0;
  virtual int              createDimTableReal(const char *dimname,const char *tablename) = 0;
  virtual int              createDimTableString(const char *dimname,const char *tablename) = 0;
  virtual int              createDimTableTimeStamp(const char *dimname,const char *tablename) = 0;
  virtual int              removeFilesWithChangedCreationDate(const char *tablename,const char *file,const char *creationDate) = 0;
  virtual int              checkIfFileIsInTable(const char *tablename,const char *filename) = 0;
  
  
  virtual int              setFileInt(const char *tablename,const char *file,int dimvalue,int dimindex,const char*filedate) = 0;
  virtual int              setFileReal(const char *tablename,const char *file,double dimvalue,int dimindex,const char*filedate) = 0;
  virtual int              setFileString(const char *tablename,const char *file,const char * dimvalue,int dimindex,const char*filedate) = 0;
  virtual int              setFileTimeStamp(const char *tablename,const char *file,const char *dimvalue,int dimindex,const char*filedate) = 0;
  
  /** First use setFile<type> as many times as you whish, second use addFilesToDataBase to make it final*/
  virtual int              addFilesToDataBase() = 0;
  
  
};



#endif

