#include "CDataPostProcessor.h"
#include <cstdint>

#ifndef CDATAPOSTPROCESSOR_ADDDATAOBJECT_H
#define CDATAPOSTPROCESSOR_ADDDATAOBJECT_H

#define CDATAPOSTPROCESSOR_AddDataObject_ID "add_dataobject"
/**
 * Add new data object with a value
 */

class CDPPAddDataObject : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t);
};

#endif