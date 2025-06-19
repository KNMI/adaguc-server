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

f8component jacobianTransform(f8component speedVector, f8point gridCoordUL, f8point gridCoordLR, CImageWarper *warper, bool gridRelative);

int applyUVConversion(CImageWarper *warper, CDataSource *sourceImage, int *dPixelExtent, float *uValues, float *vValues);

std::vector<CalculatedWindVector> calculateBarbsAndVectorsAndSpeedFromUVComponents(CImageWarper *warper, CDataSource *sourceImage, CDrawImage *drawImage, bool enableShade, bool enableContour,
                                                                                   bool enableBarb, bool drawMap, bool enableVector, bool drawGridVectors, int *dPixelExtent, float *uValueData,
                                                                                   float *vValueData, int *dpDestX, int *dpDestY);

#endif