#ifndef GDW_FINDPIXELEXTENT_UTILS_H
#define GDW_FINDPIXELEXTENT_UTILS_H

#include "CGeoParams.h"

int gdwFindPixelExtent(int *PXExtentBasedOnSource, CGeoParams &sourceGeoParams, CGeoParams &destGeoParams, CImageWarper *warper);

#endif
