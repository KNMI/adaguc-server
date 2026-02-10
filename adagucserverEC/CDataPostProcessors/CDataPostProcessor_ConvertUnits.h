#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_CONVERTUNITS_H
#define CDATAPOSTPROCESSOR_CONVERTUNITS_H

#define CDATAPOSTPROCESSOR_CONVERTUNITS_ID "convert_units"
#define CDATAPOSTPROCESSOR_TOKNOTS_ID "toknots"
#define CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_ID "windspeed_knots_to_ms"
#include <cmath>
#include <vector>
#include <cstddef>
/**
 * Calculate CDPPConvertUnits algorithm
 */
class CDPPConvertUnits : public CDPPInterface {

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t);
};

#endif