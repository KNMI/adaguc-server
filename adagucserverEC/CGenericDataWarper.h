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
  void *sourceData;     // The source datagrid, has the same datatype as the template T
  bool hasNodataValue;  // Wether the source data grid has a nodata value
  double dfNodataValue; // No data value of the source grid, in double type. Can be casted to T
  int sourceDataPX;     // Which source X index is sampled from the source grid
  int sourceDataPY;     // Which source Y index is sampled for the source grid.
  int sourceDataWidth;  // The width of the sourcedata grid
  int sourceDataHeight; // The height of the source data grid
  int destDataWidth;    // The width of the destination grid
  int destDataHeight;   // The height of the destination grid
  double tileDx;        // The relative X sample position from the source grid cell from 0 to 1. Can be used for bilinear interpolation
  double tileDy;        // The relative y sample position
  int destX;            // The target X index in the target grid
  int destY;            // The target Y index in the target grid.
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
