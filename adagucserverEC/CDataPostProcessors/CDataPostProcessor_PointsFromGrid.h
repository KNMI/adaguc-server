#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_PointsFromGrid_H
#define CDATAPOSTPROCESSOR_PointsFromGrid_H

#define CDATAPOSTPROCESSOR_PointsFromGrid_ID "pointsfromgrid"

/**
 * Radar dbZ to Rain intensity algorithm
 */
class CDPPointsFromGrid : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *data, size_t numItems);
};

#endif
