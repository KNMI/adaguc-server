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
  template <class T, typename std::enable_if<std::is_same<T, double>::value || std::is_same<T, float>::value>::type * = nullptr>
  int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, T *data, size_t numItems);

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode, double *data, size_t numItems);
};

#endif
