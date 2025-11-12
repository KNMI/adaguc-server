#ifndef GDW_FINDPIXELEXTENT_UTILS_H
#define GDW_FINDPIXELEXTENT_UTILS_H

#include "Types/GeoParameters.h"

int gdwFindPixelExtent(int *PXExtentBasedOnSource, GeoParameters &sourceGeoParams, GeoParameters &destGeoParams, CImageWarper *warper);

#endif
