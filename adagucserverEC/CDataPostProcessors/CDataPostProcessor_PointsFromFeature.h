#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_PointsFromFeature_H
#define CDATAPOSTPROCESSOR_PointsFromFeature_H

#define CDATAPOSTPROCESSOR_PointsFromFeature_ID "pointsfromfeature"

class CDPPointsFromFeature : public CDPPInterface {
public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *data, size_t numItems);
};

#endif
