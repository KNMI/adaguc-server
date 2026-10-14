#include <CImageWarper.h>
#include <CDataSource.h>
#include <CDrawImage.h>

#ifndef CIMGRENDERFIELDVECTORS_H
#define CIMGRENDERFIELDVECTORS_H

struct CalculatedWindVector {
  int x, y;
  double dir, viewDirCorrection, strength;
  bool convertToKnots, flip;
};

f8component jacobianTransform(f8component speedVector, f8point gridCoordLL, f8point gridCoordUR, CImageWarper *warper, bool gridRelative);

int applyUVConversion(CImageWarper *warper, CDataSource *sourceImage, int *dPixelExtent, float *uValues, float *vValues);

#endif