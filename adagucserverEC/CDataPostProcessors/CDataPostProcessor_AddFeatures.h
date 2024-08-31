#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_ADDFEATURES_H
#define CDATAPOSTPROCESSOR_ADDFEATURES_H

#define CDATAPOSTPROCESSOR_ADDFEATURES_ID "addfeatures"

/**
 * AddFeature from a GEOJSON shape provider
 */
class CDPPAddFeatures : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();
  //     float addFeature(float speed);
public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) {
    CDBDebug("%s: CDATAPOSTPROCESSOR_METHOD_NOT_IMPLEMENTED", this->className);
    return CDATAPOSTPROCESSOR_METHOD_NOT_IMPLEMENTED;
  } // TODO: Still need to implement for timeseries
};
#endif
