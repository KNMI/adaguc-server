#ifndef CIMGRENDERFIELDVECTORS_H
#define CIMGRENDERFIELDVECTORS_H

#include <Types/GeoParameters.h>

class CImageWarper;
class CDataSource;

f8component jacobianTransform(f8component speedVector, f8point gridPoint, double gridYStep, CImageWarper *warper, bool gridRelative);

int applyUVConversion(CImageWarper *warper, CDataSource *sourceImage, int *dPixelExtent, float *uValues, float *vValues);

#endif