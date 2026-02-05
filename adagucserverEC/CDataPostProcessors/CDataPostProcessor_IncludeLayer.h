#ifndef CDATAPOSTPROCESSOR_INCLUDELAYER_H
#define CDATAPOSTPROCESSOR_INCLUDELAYER_H

#include "CGenericDataWarper.h"
#include "CDataPostProcessor.h"

/**
 * IncludeLayer algorithm
 */
class CDPPIncludeLayer : public CDPPInterface {
private:
  class CDPPIncludeLayerSettings {
  public:
    size_t width;
    size_t height;
    void *data;
  };

  template <class T> static void drawFunction(int x, int y, T val, GDWState &, CDPPIncludeLayerSettings &settings) {
    if (x >= 0 && y >= 0 && x < (int)settings.width && y < (int)settings.height) {
      ((T *)settings.data)[x + y * settings.width] = val;
    }
  };
  CDataSource *getDataSource(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource);
  int setDimsForNewDataSource(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, CDataSource *dataSourceToInclude);

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) {
    CDBDebug("CDATAPOSTPROCESSOR_METHOD_NOT_IMPLEMENTED ");
    return CDATAPOSTPROCESSOR_METHOD_NOT_IMPLEMENTED;
  } // TODO: Still need to implement for timeseries
};
#endif