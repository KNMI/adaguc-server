#include "CDataPostProcessor.h"
#include <cstddef>

#ifndef CDATAPOSTPROCESSOR_CDDPFILTERDATAOBJECTS_H
#define CDATAPOSTPROCESSOR_CDDPFILTERDATAOBJECTS_H
/**
 * CDATAPOSTPROCESSOR_CDDPFILTERDATAOBJECTS_H algorithm
 */

#define CDATAPOSTPROCESSOR_CDDPFILTERDATAOBJECTS_ID "filter_dataobjects"

class CDDPFilterDataObjects : public CDPPInterface {
private:
public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) { return 0; }
};

#endif