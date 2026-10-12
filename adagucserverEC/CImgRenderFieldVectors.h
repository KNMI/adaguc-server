#include <Types/GeoParameters.h>

#ifndef CIMGRENDERFIELDVECTORS_H
#define CIMGRENDERFIELDVECTORS_H

class CImageWarper;
class CDataSource;

f8component jacobianTransform(f8component speedVector, f8point gridCoordLL, f8point gridCoordUR, CImageWarper *warper, bool gridRelative);

int applyUVConversion(CImageWarper *warper, CDataSource *sourceImage, int *dPixelExtent, float *uValues, float *vValues);

#endif