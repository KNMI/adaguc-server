#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_WFP_H
#define CDATAPOSTPROCESSOR_WFP_H
/**
 * WFP algorithm
 */

class CDPPWFP : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) { return 1; } // TODO: Still need to implement
};

#endif