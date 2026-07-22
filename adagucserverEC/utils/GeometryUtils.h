#include <vector>
#include "Types/GeoParameters.h"
#include <CDataSource.h>
#include <CImageWarper.h>

#ifndef CGEOMETRYUTILS_H
#define CGEOMETRYUTILS_H

f8point compute2DPolygonCentroid(const std::vector<f8point> &vertices);
f8point getCentroid(const float *polyX, const float *polyY, const int numPoints);
f8point getPixelCoordinateFromGetMapCoordinate(f8point in, CDataSource &dataSource);
void getPixelCoordinateListFromGetMapCoordinateListInPlace(std::vector<f8point> &in, CDataSource &dataSource);
f8point getGetMapCoordinateFromPixelCoordinate(f8point in, CDataSource &dataSource);
f8point getStrideFromGetMapLocation(CDataSource &dataSource, CImageWarper &warper, f8point pixelOffset);

#endif