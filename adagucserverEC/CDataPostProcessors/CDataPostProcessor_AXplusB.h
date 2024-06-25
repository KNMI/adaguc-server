#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_AXPLUSB_H
#define CDATAPOSTPROCESSOR_AXPLUSB_H

#define CDATAPOSTPROCESSOR_AXPLUSB_ID "ax+b"

/**
 * AX + B algorithm
 */
class CDPPAXplusB : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) {
    CDBDebug("%s: Done during reading of data", this->className);
    return 0;
  }
};

#endif
