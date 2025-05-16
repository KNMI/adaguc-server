#include "CDataPostProcessor.h"
#include <cstddef>

#ifndef CDATAPOSTPROCESSOR_CDDPMETADATA_VARIABLE_H
#define CDATAPOSTPROCESSOR_CDDPMETADATA_VARIABLE_H
/**
 * CDATAPOSTPROCESSOR_CDDPMETADATA_VARIABLE_H algorithm
 */

#define CDATAPOSTPROCESSOR_CDDPMETADATA_VARIABLE_ID "METADATA_VARIABLE"

class CDDPMetadataVariable : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) { return 0; }
};

#endif