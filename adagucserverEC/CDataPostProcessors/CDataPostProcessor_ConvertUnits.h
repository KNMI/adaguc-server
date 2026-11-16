#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_CONVERTUNITS_H
#define CDATAPOSTPROCESSOR_CONVERTUNITS_H

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

// Applies the layer's unit conversions (ax+b, convert_units, toknots, windspeed_knots_to_ms) in configured order to values that were just read.
// Stateless: does not look at units, from_units or variable attributes, so every read is converted exactly once. Nodata and NaN stay as they are.
void applyUnitConversions(const std::vector<CServerConfig::XMLE_DataPostProc> &procs, double noDataValue, double *data, size_t numValues);

#endif