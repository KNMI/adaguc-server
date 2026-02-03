#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_SOLARTERMINATOR_H
#define CDATAPOSTPROCESSOR_SOLARTERMINATOR_H
/**
 * SolarTerminator algorithm
 */

class CDPPSolarTerminator : public CDPPInterface {
private:
public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) { return 1; } // TODO: Still need to implement
};

#endif