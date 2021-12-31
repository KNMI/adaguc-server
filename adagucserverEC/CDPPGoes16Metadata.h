#include "CDPPInterface.h"

#ifndef CDDPGOES16METADATA_H
#define CDDPGOES16METADATA_H
/**
 * Fix goes16 metadata
 */
class CDPPGoes16Metadata : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) { return 1; } // TODO: Still need to implement
};
#endif