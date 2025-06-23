#ifndef GenericDataWarper_H
#define GenericDataWarper_H
#include <functional>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <proj.h>
#include <math.h>
#include <cfloat>
#include "CGeoParams.h"
#include "CImageWarper.h"
#include "CDebugger.h"
#include "CGenericDataWarperTools.h"

typedef unsigned char uchar;
typedef unsigned char ubyte;

struct GDWState {
  CDFType sourceDataType;
  void *sourceData;
  int sourceDataPX, sourceDataPY, sourceDataWidth, sourceDataHeight;
  double tileDx, tileDy;
  double dfNodataValue;
  bool hasNodataValue;
  bool useHalfCellOffset = false;
  int destDataWidth, destDataHeight, destX, destY;
  CDFType destinationDataType;
  void *destinationGrid = nullptr;
};

class GenericDataWarper {
private:
  DEF_ERRORFUNCTION();
  GDWState warperState;

public:
  template <typename T>
  int render(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, const std::function<void(int, int, T, GDWState &warperState)> &drawFunction);
};
#endif
