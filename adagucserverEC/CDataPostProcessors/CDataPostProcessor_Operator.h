#include "CDataPostProcessor.h"
#include <cstdint>

#ifndef CDATAPOSTPROCESSOR_OPERATOR_H
#define CDATAPOSTPROCESSOR_OPERATOR_H
/**
 * Operator algorithm
 */

class CDPPOperator : public CDPPInterface {
private:
  float getElement(void *data, CDFType dataType, size_t index);

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t); // TODO: Still need to implement for timeseries
};

#endif