#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_OPERATOR_H
#define CDATAPOSTPROCESSOR_OPERATOR_H
/**
 * Operator algorithm
 */

class CDPPOperator : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();
  float getElement(void *data, CDFType dataType, size_t index);

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