#ifndef GenericDataWarper_H
#define GenericDataWarper_H
#include <functional>
#include <iostream>
#include <math.h>
#include <stdlib.h>
#include <proj.h>
#include <math.h>
#include <cfloat>
#include "Types/GeoParameters.h"
#include "CImageWarper.h"
#include "CDebugger.h"
#include "CGenericDataWarperTools.h"

typedef unsigned char uchar;
typedef unsigned char ubyte;

struct GDWState {
  void *sourceData;
  int sourceDataPX, sourceDataPY, sourceDataWidth, sourceDataHeight;
  double tileDx, tileDy;
  double dfNodataValue;
  bool hasNodataValue;
  int destDataWidth, destDataHeight;
  int destX, destY;
};

struct GDWArgs {
  CImageWarper *warper;
  void *sourceData;
  GeoParameters sourceGeoParams;
  GeoParameters destGeoParams;
};

class ProjectionGrid {
public:
  double *px = nullptr;
  double *py = nullptr;
  bool *skip = nullptr;
  void initSize(size_t dataSize) {
    px = new double[dataSize];
    py = new double[dataSize];
    skip = new bool[dataSize];
  }
  ~ProjectionGrid() {
    CDBDebug("Destructed ProjectionGrid");
    delete[] px;
    delete[] py;
    delete[] skip;
  }
};

class GenericDataWarper {
private:
  DEF_ERRORFUNCTION();
  ProjectionGrid *projectionGrid = nullptr;

public:
  GenericDataWarper() { CDBDebug("NEW GenericDataWarper"); }
  ~GenericDataWarper() {
    CDBDebug("Destruct GenericDataWarper");
    delete projectionGrid;
    projectionGrid = nullptr;
  };
  bool useHalfCellOffset = false;
  template <typename T>
  int render(CImageWarper *warper, void *_sourceData, GeoParameters sourceGeoParams, GeoParameters destGeoParams, const std::function<void(int, int, T, GDWState &warperState)> &drawFunction);

  template <typename T> int render(GDWArgs &args, const std::function<void(int, int, T, GDWState &warperState)> &drawFunction) {
    return render(args.warper, args.sourceData, args.sourceGeoParams, args.destGeoParams, drawFunction);
  }
};
#endif
