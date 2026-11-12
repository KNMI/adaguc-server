#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_HighLowAreaDetector_H
#define CDATAPOSTPROCESSOR_HighLowAreaDetector_H

#define CDATAPOSTPROCESSOR_HighLowAreaDetector_ID "highlowareadetector"

/**
 * High/Low pressure area detector
 */
class CDPPHighLowAreaDetector : public CDPPInterface {
public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *data, size_t numItems);
};

#endif
