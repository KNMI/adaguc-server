#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_CLIPMINMAX_H
#define CDATAPOSTPROCESSOR_CLIPMINMAX_H
/**
 * Clip min max algorithm
 */

class CDPPClipMinMax : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) { return 1; } // TODO: Still need to implement
};

#endif