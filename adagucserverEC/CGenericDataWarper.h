#ifndef GenericDataWarper_H
#define GenericDataWarper_H
#include <math.h>
#include <stdlib.h>
#include <proj.h>
#include <math.h>
#include <cfloat>
#include "CGeoParams.h"
#include "CImageWarper.h"
#include "CDebugger.h"
#include "CGenericDataWarperTools.h"

struct GDWDrawFunctionState;
typedef void (*FPSetValueInDestination)(GDWDrawFunctionState *drawFunctionSettings);
typedef double (*FPGetValueFromSource)(int, int, GDWDrawFunctionState *);

struct GDWDrawFunctionState {
  CDFType sourceDataType;
  void *sourceData;
  int sourceDataPX, sourceDataPY, sourceDataWidth, sourceDataHeight;
  double tileDx, tileDy;

  FPSetValueInDestination setValueInDestinationFunction;
  FPGetValueFromSource getValueFromSourceFunction;

  double dfNodataValue;
  bool hasNodataValue;
  bool useHalfCellOffset = false;
  int destDataWidth, destDataHeight, destX, destY; // TODO
  CDFType destinationDataType;
  void *destinationGrid = nullptr; // TODO
};
template <typename T> int warpT(CImageWarper *warper, void *_sourceData, CDFType sourceDataType, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, GDWDrawFunctionState *drawFunctionSetting);

int warp(CImageWarper *warper, void *_sourceData, CDFType sourceDataType, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, GDWDrawFunctionState *drawFunctionSetting);

class GenericDataWarper {
private:
  DEF_ERRORFUNCTION();

public:
  template <typename T>

  int render(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
             void (*drawFunction)(int, int, T, void *drawFunctionSettings));

  // int renderI(CImageWarper *warper, void *_sourceData, CDFType sourceDataType, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings, void *drawFunction);
};
#endif
