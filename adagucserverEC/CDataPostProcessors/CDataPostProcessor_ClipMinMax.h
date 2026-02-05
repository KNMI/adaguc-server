#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_CLIPMINMAX_H
#define CDATAPOSTPROCESSOR_CLIPMINMAX_H
/**
 * Clip min max algorithm
 */

class CDPPClipMinMax : public CDPPInterface {
public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) {
    CDBDebug("CDATAPOSTPROCESSOR_METHOD_NOT_IMPLEMENTED");
    return CDATAPOSTPROCESSOR_METHOD_NOT_IMPLEMENTED;
  } // TODO: Still need to implement for timeseries
};

#endif