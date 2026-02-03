/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2013-06-01
 *
 ******************************************************************************
 *
 * Copyright 2013, Royal Netherlands Meteorological Institute (KNMI)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

// http://datagenetics.com/blog/august32013/index.html

#include "CImgWarpNearestNeighbour.h"
#include <limits>
#include "CFillTriangle.h"

void CImgWarpNearestNeighbour::drawTriangleBil(CDrawImage *drawImage, float *destField, int *xP, int *yP, float *values) {

  // Sort the vertices in Y direction
  int W = drawImage->geoParams.width;
  int H = drawImage->geoParams.height;
  if (xP[0] < 0 && xP[1] < 0 && xP[2] < 0) return;
  if (xP[0] >= W && xP[1] >= W && xP[2] >= W) return;
  if (yP[0] < 0 && yP[1] < 0 && yP[2] < 0) return;
  if (yP[0] >= H && yP[1] >= H && yP[2] >= H) return;

  unsigned int lower;
  unsigned int middle;
  unsigned int upper;

  if (yP[0] < yP[1]) {
    if (yP[0] < yP[2]) {
      lower = 0;
      if (yP[1] < yP[2]) {
        middle = 1;
        upper = 2;
      } else {
        middle = 2;
        upper = 1;
      }
    } else {
      middle = 0;
      lower = 2;
      upper = 1;
    }
  } else {
    if (yP[1] < yP[2]) {
      lower = 1;
      if (yP[0] < yP[2]) {
        middle = 0;
        upper = 2;
      } else {
        middle = 2;
        upper = 0;
      }
    } else {
      middle = 1;
      lower = 2;
      upper = 0;
    }
  }

  int X1 = xP[lower];
  int X2 = xP[middle];
  int X3 = xP[upper];
  int Y1 = yP[lower];
  int Y2 = yP[middle];
  int Y3 = yP[upper];

  if (Y1 == Y3) return;

  if (Y2 == Y1 && Y3 == Y2) return;

  // x*stride+(y*stride)*(dataWidth*stride)

  double VX1 = values[lower];
  double VX2 = values[middle];
  double VX3 = values[upper];

  double rcl = double(X3 - X1) / double(Y3 - Y1);
  double rcvxl = (VX3 - VX1) / double(Y3 - Y1);

  if (Y2 != Y1 && Y1 < H && Y2 > 0) {
    double rca = double(X2 - X1) / double(Y2 - Y1);
    double rcvxa = (VX2 - VX1) / double(Y2 - Y1);

    int sy = (Y1 < 0) ? 0 : Y1;
    int ey = (Y2 > H) ? H : Y2;

    for (int y = sy; y < ey; y++) {
      int xL = (int)(rcl * double(y - Y1) + X1);
      int xA = (int)(rca * double(y - Y1) + X1);
      double vxL = rcvxl * double(y - Y1) + VX1;
      double vxA = rcvxa * double(y - Y1) + VX1;

      int x1, x2;
      double vx1, vx2;
      if (xL < xA) {
        x1 = xL;
        x2 = xA;
        vx1 = vxL;
        vx2 = vxA;
      } else {
        x2 = xL;
        x1 = xA;
        vx1 = vxA;
        vx2 = vxL;
      }
      if (x1 < W && x2 > 0) {
        int sx = (x1 < 0) ? 0 : x1;
        int ex = (x2 > W) ? W : x2;
        double rcxvx = double(vx2 - vx1) / double(x2 - x1);

        for (int x = sx; x < ex; x++) {
          double vx = rcxvx * double(x - x1) + vx1;
          drawImage->setPixelIndexed(x, y, vx);
          destField[x + y * W] = vx;
        }
      }
    }
  }

  if (Y3 != Y2 && Y2 < H && Y3 > 0) {
    double rcb = double(X3 - X2) / double(Y3 - Y2);
    double rcvxb = (VX3 - VX2) / double(Y3 - Y2);

    int sy = (Y2 < 0) ? 0 : Y2;
    int ey = (Y3 > H) ? H : Y3;

    for (int y = sy; y < ey; y++) {

      int xL = (int)(rcl * double(y - Y1) + X1);
      int xB = (int)(rcb * double(y - Y2) + X2);
      double vxL = rcvxl * double(y - Y1) + VX1;
      double vxB = rcvxb * double(y - Y2) + VX2;

      int x1, x2;
      double vx1, vx2;
      if (xL < xB) {
        x1 = xL;
        x2 = xB;
        vx1 = vxL;
        vx2 = vxB;
      } else {
        x2 = xL;
        x1 = xB;
        vx1 = vxB;
        vx2 = vxL;
      }
      if (x1 < W && x2 > 0) {
        int sx = (x1 < 0) ? 0 : x1;
        int ex = (x2 > W) ? W : x2;
        double rcxvx = double(vx2 - vx1) / double(x2 - x1);

        for (int x = sx; x < ex; x++) {
          double vx = rcxvx * double(x - x1) + vx1;
          drawImage->setPixelIndexed(x, y, vx);
          destField[x + y * W] = vx;
        }
      }
    }
  }
}

void CImgWarpNearestNeighbour::drawTriangle(CDrawImage *drawImage, int *xP, int *yP, int &value) {
  // Sort the vertices in Y direction
  int W = drawImage->geoParams.width;
  int H = drawImage->geoParams.height;
  if (xP[0] < 0 && xP[1] < 0 && xP[2] < 0) return;
  if (xP[0] >= W && xP[1] >= W && xP[2] >= W) return;
  if (yP[0] < 0 && yP[1] < 0 && yP[2] < 0) return;
  if (yP[0] >= H && yP[1] >= H && yP[2] >= H) return;

  unsigned short lower;
  unsigned short middle;
  unsigned short upper;

  if (yP[0] < yP[1]) {
    if (yP[0] < yP[2]) {
      lower = 0;
      if (yP[1] < yP[2]) {
        middle = 1;
        upper = 2;
      } else {
        middle = 2;
        upper = 1;
      }
    } else {
      middle = 0;
      lower = 2;
      upper = 1;
    }
  } else {
    if (yP[1] < yP[2]) {
      lower = 1;
      if (yP[0] < yP[2]) {
        middle = 0;
        upper = 2;
      } else {
        middle = 2;
        upper = 0;
      }
    } else {
      middle = 1;
      lower = 2;
      upper = 0;
    }
  }

  int X1 = xP[lower];
  int X2 = xP[middle];
  int X3 = xP[upper];
  int Y1 = yP[lower];
  int Y2 = yP[middle];
  int Y3 = yP[upper];

  if (Y1 == Y3) {
    return;
  }

  if (Y2 == Y1 && Y3 == Y2) {
    return;
  }

  float rcl = float(X3 - X1) / float(Y3 - Y1);

  if (Y2 != Y1 && Y1 < H && Y2 > 0) {

    float rca = float(X2 - X1) / float(Y2 - Y1);

    short sy = (Y1 < 0) ? 0 : Y1;
    short ey = (Y2 > H) ? H : Y2;

    for (short y = sy; y < ey; y++) {
      short xL = (short)(rcl * float(y - Y1) + X1);
      short xA = (short)(rca * float(y - Y1) + X1);

      short x1, x2;

      if (xL < xA) {
        x1 = xL;
        x2 = xA;
      } else {
        x2 = xL;
        x1 = xA;
      }
      if (x1 < W && x2 > 0) {
        short sx = (x1 < 0) ? 0 : x1;
        short ex = (x2 > W) ? W : x2; //<0?0:x2;
        for (short x = sx; x < ex; x++) {
          drawImage->setPixelIndexed(x, y, value);
        }
      }
    }
  }

  if (Y3 != Y2 && Y2 < H && Y3 > 0) {
    float rcb = float(X3 - X2) / float(Y3 - Y2);

    short sy = (Y2 < 0) ? 0 : Y2;
    short ey = (Y3 > H) ? H : Y3;

    for (short y = sy; y < ey; y++) {
      short xL = (short)(rcl * float(y - Y1) + X1);
      short xB = (short)(rcb * float(y - Y2) + X2);

      short x1, x2;

      if (xL < xB) {
        x1 = xL;
        x2 = xB;
      } else {
        x2 = xL;
        x1 = xB;
      }

      if (x1 < W && x2 > 0) {
        short sx = (x1 < 0) ? 0 : x1;
        short ex = (x2 > W) ? W : x2; //<0?0:x2;
        for (short x = sx; x < ex; x++) {
          drawImage->setPixelIndexed(x, y, value);
        }
      }
    }
  }
}
