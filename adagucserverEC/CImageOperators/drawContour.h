#include <CDataSource.h>
#include <CDrawImage.h>

#ifndef DRAWCONTOURLINES_H
#define DRAWCONTOURLINES_H

// When useMultipleThreads is true, the distance field of large images (more than one megapixel) is filled with multiple threads. The result is the same.
void drawContour(float *sourceGrid, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration, bool useMultipleThreads = false);

#endif
