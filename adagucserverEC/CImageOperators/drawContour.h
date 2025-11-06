#include <CDataSource.h>
#include <CDrawImage.h>

#ifndef DRAWCONTOURLINES_H
#define DRAWCONTOURLINES_H

/*
Search window for xdir and ydir:
      -1  0  1  (x)
  -1   6  5  4
   0   7  X  3
   1   0  1  2
  (y)
            0  1  2  3  4  5  6  7 */
const int xdir[8] = {-1, 0, 1, 1, 1, 0, -1, -1};
const int ydir[8] = {1, 1, 1, 0, -1, -1, -1, 0};
/*                0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 */
const int xdirOuter[16] = {-2, -1, 0, 1, 2, 2, 2, 2, 2, 1, 0, -1, -2, -2, -2, -2};
const int ydirOuter[16] = {2, 2, 2, 2, 2, 1, 0, -1, -2, -2, -2, -2, -2, -1, 0, 1};

#define MAX_LINE_SEGMENTS 1000

void drawContour(double *sourceGrid, CDataSource *dataSource, CDrawImage *drawImage);

#endif
