#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_BEAUFORT_H
#define CDATAPOSTPROCESSOR_BEAUFORT_H

/**
 * Calculate Beaufort algorithm
 */
class CDPPBeaufort : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();
  float getBeaufort(float speed);

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t);
};

#endif