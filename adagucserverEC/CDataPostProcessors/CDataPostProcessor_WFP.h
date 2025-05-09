#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_WFP_H
#define CDATAPOSTPROCESSOR_WFP_H
#include "./GenericDataWarper/CGenericDataWarper.h"
/**
 * WFP algorithm
 */

class CDPPWFP : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();

  CDF::Variable *cloneVariable(CDF::Variable *varToClone, const char *name, int size);

public:
  static CDataSource *getDataSource(CDataSource *dataSource, CT::string baseLayerName);
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) {
    CDBDebug("%s: CDATAPOSTPROCESSOR_METHOD_NOT_IMPLEMENTED", this->className);
    return CDATAPOSTPROCESSOR_METHOD_NOT_IMPLEMENTED;
  } // TODO: Still need to implement for timeseries
};

#endif