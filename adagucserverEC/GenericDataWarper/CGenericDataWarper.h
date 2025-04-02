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

struct GDWWarperState {
  void *sourceData;
  float *destinationGrid;
  int sourceDataPX, sourceDataPY, sourceDataWidth, sourceDataHeight;
  double tileDx, tileDy;
};

class CGenericDataWarper {
private:
  DEF_ERRORFUNCTION();

public:
  GDWWarperState warperState;
  bool useHalfCellOffset;
  CGenericDataWarper() {
    useHalfCellOffset = false;
    warperState.destinationGrid = nullptr;
  }

  template <typename T>
  int render(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
             void (*drawFunction)(int, int, T, void *drawFunctionSettings, void *genericDataWarper));
};
#endif
