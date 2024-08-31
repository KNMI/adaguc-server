#include "CDataPostProcessor.h"

#ifndef CDATAPOSTPROCESSOR_WFP_H
#define CDATAPOSTPROCESSOR_WFP_H
/**
 * WFP algorithm
 */

class CDPPWFP : public CDPPInterface {
private:
  DEF_ERRORFUNCTION();
  class Settings {
  public:
    size_t width;
    size_t height;
    float *WindSpeedWindparksOff;        // Grid to write TO, same grid as dest grid
    float *WindSpeedWindparksOnImproved; // Grid to write TO, same grid as dest grid
    float *windSpeedDifference;          // Grid to write TO, same grid as dest grid
    float *windSectors;                  // Grid with found windSectors
    float *destGridWindSpeed;
    float *destGridWindDirection;
    CDF::Variable *windSpeedDifferenceVariable; // Lookup table / source grid with the pre-calculated wind secors
    CDF::Variable *windSectorVariable;          // Lookup table / source grid with the different sectors/directions
    size_t dimWindSectorQuantile;               // Quantile dimension length in wind sector lookup table
    size_t dimWindSectorHeightLevel;            // Height dimension length in wind sector lookup table
    size_t dimWindSectorY;                      // Y dimension length  in wind sector lookup table
    size_t dimWindSectorX;                      // X dimension length in wind sector lookup table
  };

  static void drawFunction(int x, int y, float val, void *_settings, void *warperInstance);
  CDF::Variable *cloneVariable(CDF::Variable *varToClone, const char *name, int size);

public:
  static CDataSource *getDataSource(CDataSource *dataSource, CT::string baseLayerName);
  virtual const char *getId();
  virtual int isApplicable(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *proc, CDataSource *dataSource, int mode);
  virtual int execute(CServerConfig::XMLE_DataPostProc *, CDataSource *, int, double *, size_t) {
    CDBDebug("%s: CDATAPOSTPROCESSOR_METHOD_NOT_IMPLEMENTED", this->className);
    return CDATAPOSTPROCESSOR_METHOD_NOT_IMPLEMENTED;
  } // TODO: Still need to implement for timeseries
};

#endif