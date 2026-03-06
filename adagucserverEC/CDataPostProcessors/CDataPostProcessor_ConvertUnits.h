#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_CONVERTUNITS_H
#define CDATAPOSTPROCESSOR_CONVERTUNITS_H

/**
 * Calculate CDPPConvertUnits algorithm
 */
class CDPPConvertUnits : public CDPPInterface {

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t);
};

#endif