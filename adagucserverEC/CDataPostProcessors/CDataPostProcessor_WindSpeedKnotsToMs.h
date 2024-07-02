#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_H
#define CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_H

#define CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_ID "windspeed_knots_to_ms"

#define CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_NEWUNITS "m/s"

#define CDATAPOSTPROCESSOR_WINDSPEEDKTSTOMS_KTSTOMSFACTOR 1852. / 3600.

/**
 * Calculate CDPPWindspeedKnotsToMS algorithm
 */
class CDPPWindSpeedKnotsToMs : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();
  float getBeaufort(float speed);

public:
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t);
};

#endif