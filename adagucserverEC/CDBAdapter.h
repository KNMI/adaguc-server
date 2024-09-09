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

/**
 * Interface for several database implementations. Currently for CDBAdapterPostgreSQL and CDBAdapterSQLLite
 */
class CDBAdapter {
public:
  CDBAdapter() {}
  virtual ~CDBAdapter() {};

  class GeoOptions {
  public:
    double bbox[4];
    int indices[4];
    CT::string proj4;
    int level;
  };

  virtual int setConfig(CServerConfig::XMLE_Configuration *cfg) = 0;

  /**
   * getReferenceTime
   * This function is used to find the most recent forecast_reference_time for a given time.
   * returns a forecast_reference_time value where the difference is smallest between time and referencetime and where the difference between time-referencetime is larger than zero
   * @param netcdfReferenceTimeDimName
   * @param netcdfTimeDimName
   * @param timeValue
   * @param timeTableName
   * @param referenceTimeTableName
   * @return timeValue The value to find the most recent reference time for
   */
  virtual CDBStore::Store *getReferenceTime(const char *netcdfReferenceTimeDimName, const char *netcdfTimeDimName, const char *timeValue, const char *timeTableName,
                                            const char *referenceTimeTableName) = 0;

  /**
   * getClosestDataTimeToSystemTime
   * Returns the closest data time to current system time. Used when time="current" in combination with reference_time.
   * @param netcdfDimName
   * @param tableName
   * @return timeValue;
   */
  virtual CDBStore::Store *getClosestDataTimeToSystemTime(const char *netcdfDimName, const char *tableName) = 0;

  /**
   * getTableNameForPathFilterAndDimension
   * Makes use of a lookup table to find the tablename belonging to the filter and path combinations.
   * @param path The path of the layer
   * @param filter The filter of the layer
   * @param dimension The dimension of the layer, can be NULL if not used.
   * @param dataSource The dataSource
   * @return Tablename on succes, throws integer exception on failure.
   */
  virtual CT::string getTableNameForPathFilterAndDimension(const char *path, const char *filter, const char *dimension, CDataSource *dataSource) = 0;

  virtual CDBStore::Store *getMin(const char *name, const char *table) = 0;
  virtual CDBStore::Store *getMax(const char *name, const char *table) = 0;
  virtual CDBStore::Store *getBetween(const char *min, const char *max, const char *colname, const char *table, int limit) = 0;
  virtual CT::string getDimValueForFileName(const char *filename, const char *table) = 0;
  virtual CDBStore::Store *getUniqueValuesOrderedByValue(const char *name, int limit, bool orderDescOrAsc, const char *table) = 0;
  virtual CDBStore::Store *getUniqueValuesOrderedByIndex(const char *name, int limit, bool orderDescOrAsc, const char *table) = 0;

  /*Dimension Info*/
  virtual CDBStore::Store *getDimensionInfoForLayerTableAndLayerName(const char *layertable, const char *layername) = 0;
  virtual int storeDimensionInfoForLayerTableAndLayerName(const char *layertable, const char *layername, const char *netcdfname, const char *ogcname, const char *units) = 0;
  virtual int removeDimensionInfoForLayerTableAndLayerName(const char *layertable, const char *layername) = 0;

  virtual CDBStore::Store *getFilesAndIndicesForDimensions(CDataSource *dataSource, int limit) = 0;
  virtual CDBStore::Store *getFilesForIndices(CDataSource *dataSource, size_t *start, size_t *count, ptrdiff_t *stride, int limit) = 0;

  virtual int autoUpdateAndScanDimensionTables(CDataSource *dataSource) = 0;

  virtual int dropTable(const char *tablename) = 0;
  virtual int createDimTableInt(const char *dimname, const char *tablename) = 0;
  virtual int createDimTableReal(const char *dimname, const char *tablename) = 0;
  virtual int createDimTableString(const char *dimname, const char *tablename) = 0;
  virtual int createDimTableTimeStamp(const char *dimname, const char *tablename) = 0;
  virtual int removeFile(const char *tablename, const char *file) = 0;
  virtual int removeFilesWithChangedCreationDate(const char *tablename, const char *file, const char *creationDate) = 0;

  virtual int checkIfFileIsInTable(const char *tablename, const char *filename) = 0;

  virtual int setFileInt(const char *tablename, const char *file, int dimvalue, int dimindex, const char *filedate, GeoOptions *geoOptions) = 0;
  virtual int setFileReal(const char *tablename, const char *file, double dimvalue, int dimindex, const char *filedate, GeoOptions *geoOptions) = 0;
  virtual int setFileString(const char *tablename, const char *file, const char *dimvalue, int dimindex, const char *filedate, GeoOptions *geoOptions) = 0;
  virtual int setFileTimeStamp(const char *tablename, const char *file, const char *dimvalue, int dimindex, const char *filedate, GeoOptions *geoOptions) = 0;

  /** First use setFile<type> as many times as you whish, second use addFilesToDataBase to make it final*/
  virtual int addFilesToDataBase() = 0;

  virtual int storeLayerMetadata(const char *datasetName, const char *layerName, const char *metadataKey, const char *metadatablob) = 0;
  virtual CT::string getLayerMetadata(const char *datasetName, const char *layerName, const char *metadataKey) = 0;
};

#endif
