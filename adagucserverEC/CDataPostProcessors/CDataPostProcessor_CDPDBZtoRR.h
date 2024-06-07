#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_DBZtoRR_H
#define CDATAPOSTPROCESSOR_DBZtoRR_H

#define CDATAPOSTPROCESSOR_DBZtoRR_ID "dbztorr"

/**
 * Radar dbZ to Rain intensity algorithm
 */
class CDPDBZtoRR : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();
  float getRR(float dbZ);

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *data, size_t numItems);
};

#endif
