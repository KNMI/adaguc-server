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

#include "CGenericDataWarper.h"
#include "GenericDataWarper/gdwDrawTriangle.h"
#include "GenericDataWarper/gdwFindPixelExtent.h"
#include "utils/projectionUtils.h"

// #define GenericDataWarper_DEBUG

template <typename T>
int GenericDataWarper::render(CImageWarper *warper, void *_sourceData, GeoParameters &sourceGeoParams, GeoParameters &destGeoParams,
                              const std::function<void(int, int, T, GDWState &warperState)> &drawFunction) {

  warperState.sourceData = _sourceData;
  warperState.destDataWidth = destGeoParams.width;
  warperState.destDataHeight = destGeoParams.height;
  warperState.sourceDataWidth = sourceGeoParams.width;
  warperState.sourceDataHeight = sourceGeoParams.height;

  if (debug) {
    CDBDebug("render");
  }

  int imageHeight = destGeoParams.height;
  int imageWidth = destGeoParams.width;

  // Reproj back and forth sourceGeoParams boundingbox
  double y1 = sourceGeoParams.bbox.bottom;
  double y2 = sourceGeoParams.bbox.top;
  double x1 = sourceGeoParams.bbox.left;
  double x2 = sourceGeoParams.bbox.right;

  if (debug) {
    CDBDebug("sourceGeoParams.dfBBOX %f, %f, %f, %f", sourceGeoParams.bbox.left, sourceGeoParams.bbox.bottom, sourceGeoParams.bbox.right, sourceGeoParams.bbox.top);
    CDBDebug("destGeoParams.dfBBOX %f, %f, %f, %f", destGeoParams.bbox.left, destGeoParams.bbox.bottom, destGeoParams.bbox.right, destGeoParams.bbox.top);
  }
  if (y2 < y1) {
    if (y1 > -360 && y2 < 360 && x1 > -720 && x2 < 720) {
      if (isLonLatProjection(&sourceGeoParams.crs) == false) {
        double checkBBOX[4];
        sourceGeoParams.bbox.toArray(checkBBOX);

        // CDBDebug("Current BBOX:  %f %f %f %f",sourceGeoParams.bbox.left,sourceGeoParams.bbox.bottom,sourceGeoParams.bbox.right,sourceGeoParams.bbox.top);
        bool hasError = false;
        if (warper->reprojpoint_inv(checkBBOX[0], checkBBOX[1]) != 0) hasError = true;
        if (warper->reprojpoint(checkBBOX[0], checkBBOX[1]) != 0) hasError = true;

        if (warper->reprojpoint_inv(checkBBOX[2], checkBBOX[3]) != 0) hasError = true;
        if (warper->reprojpoint(checkBBOX[2], checkBBOX[3]) != 0) hasError = true;

        if (hasError == false) {
          sourceGeoParams.bbox = checkBBOX;
        }
      }
    }
  }

  double dfSourceExtW = (sourceGeoParams.bbox.right - sourceGeoParams.bbox.left);
  double dfSourceExtH = (sourceGeoParams.bbox.top - sourceGeoParams.bbox.bottom);
  double dfSourceW = double(sourceGeoParams.width);
  double dfSourceH = double(sourceGeoParams.height);

  double dfDestW = double(destGeoParams.width);
  double dfDestH = double(destGeoParams.height);

  double dfSourcedExtW = dfSourceExtW / (dfSourceW);
  double dfSourcedExtH = dfSourceExtH / (dfSourceH);
  double dfSourceOrigX = sourceGeoParams.bbox.left;
  double dfSourceOrigY = sourceGeoParams.bbox.bottom;

  double dfDestExtW = destGeoParams.bbox.right - destGeoParams.bbox.left;
  double dfDestExtH = destGeoParams.bbox.bottom - destGeoParams.bbox.top;
  double multiDestX = double(imageWidth) / dfDestExtW;

  double multiDestY = double(imageHeight) / dfDestExtH;

  double dfDestOrigX = destGeoParams.bbox.left; //-0.5/multiDestX;;
  double dfDestOrigY = destGeoParams.bbox.top;  //+0.5/multiDestY;;;

  // Determine source BBOX of based on destination grid
  if (debug) {
    CDBDebug("Creating px extent");
  }

  warperState.sourceDataWidth = sourceGeoParams.width;
  warperState.sourceDataHeight = sourceGeoParams.height;

  int PXExtentBasedOnSource[4];

  PXExtentBasedOnSource[0] = 0;
  PXExtentBasedOnSource[1] = 0;
  PXExtentBasedOnSource[2] = warperState.sourceDataWidth;
  PXExtentBasedOnSource[3] = warperState.sourceDataHeight;

  if (PXExtentBasedOnSource[2] - PXExtentBasedOnSource[0] <= 0) return 0;
  if (PXExtentBasedOnSource[3] - PXExtentBasedOnSource[1] <= 0) return 0;
  int dataWidth = PXExtentBasedOnSource[2] - PXExtentBasedOnSource[0];
  int dataHeight = PXExtentBasedOnSource[3] - PXExtentBasedOnSource[1];

  /* When geographical map projections are equal, just do a simple linear transformation */
  if (warper->isProjectionRequired() == false) {

    // Obtain pixelextent to avoid looping over all source grid cells which will never be used in the destination grid
    i4box pixelspan;
    pixelspan = PXExtentBasedOnSource;
    auto source = sourceGeoParams.bbox;
    auto dest = destGeoParams.bbox;
    f8point span = source.span();
    i4point wh = {.x = sourceGeoParams.width, .y = sourceGeoParams.height};
    f8box newbox = {
        .left = (dest.left - source.left) / span.x, .bottom = (dest.bottom - source.bottom) / span.y, .right = (dest.right - source.left) / span.x, .top = (dest.top - source.bottom) / span.y};
    i4box newpixelspan = {.left = (int)round(newbox.left * wh.x), .bottom = (int)round(newbox.bottom * wh.y), .right = (int)round(newbox.right * wh.x), .top = (int)round(newbox.top * wh.y)};
    newpixelspan.sort();
    newpixelspan.clip({.left = 0, .bottom = 0, .right = wh.x, .top = wh.y});

    pixelspan = newpixelspan;
    // CDBDebug("newpixelspan %d %d %d %d", pixelspan2.left, pixelspan2.bottom, pixelspan2.right, pixelspan2.top);
    // CDBDebug("pixelspan %d %d %d %d", pixelspan.left, pixelspan.bottom, pixelspan.right, pixelspan.top);

    for (int y = pixelspan.bottom; y < pixelspan.top; y++) {
      for (int x = pixelspan.left; x < pixelspan.right; x++) {
        double dfx = x;
        double dfy = y;
        int sx1 = roundedLinearTransform(dfx, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW);
        int sx2 = roundedLinearTransform(dfx + 1, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW);
        int sy1 = roundedLinearTransform(dfy, dfSourceH, dfSourceExtH, dfSourceOrigY, dfDestOrigY, dfDestExtH, dfDestH);
        int sy2 = roundedLinearTransform(dfy + 1, dfSourceH, dfSourceExtH, dfSourceOrigY, dfDestOrigY, dfDestExtH, dfDestH);
        bool skip = false;
        int sxw = floor(fabs(sx2 - sx1)) + 1;
        int syh = floor(fabs(sy2 - sy1)) + 1;
        // CDBDebug("%d %d %d, %d %d %d %d", x, y, sx1, sy1, sx2, sy2);
        if (sx1 < -sxw && sx2 < -sxw) skip = true;
        if (sy1 < -syh && sy2 < -syh) skip = true;
        if (sx1 >= destGeoParams.width + sxw && sx2 >= destGeoParams.width + sxw) skip = true;
        if (sy1 >= destGeoParams.height + syh && sy2 >= destGeoParams.height + syh) skip = true;
        //

        if (!skip) {
          warperState.sourceDataPX = x;
          warperState.sourceDataPY = sourceGeoParams.height - 1 - y;
          T value = ((T *)warperState.sourceData)[warperState.sourceDataPX + (warperState.sourceDataPY) * sourceGeoParams.width];
          int lx1, lx2, ly1, ly2;
          if (sx1 > sx2) {
            lx2 = sx1;
            lx1 = sx2;
          } else {
            lx2 = sx2;
            lx1 = sx1;
          }
          if (sy1 > sy2) {
            ly2 = sy1;
            ly1 = sy2;
          } else {
            ly2 = sy2;
            ly1 = sy1;
          }
          if (ly2 == ly1) ly2++;
          if (lx2 == lx1) lx2++;
          for (int sjy = ly1; sjy < ly2; sjy++) {
            for (int sjx = lx1; sjx < lx2; sjx++) {
              warperState.tileDy = (sjy - ly1) / double(ly2 - ly1);
              warperState.tileDx = (sjx - lx1) / double(lx2 - lx1);
              warperState.destX = sjx;
              warperState.destY = sjy;
              // CDBDebug("%d %d %d %d  %f", sjx, sjy, warperState.sourceDataPX, warperState.sourceDataPY, value);
              drawFunction(sjx, sjy, value, warperState);
            }
          }
        }
      }
    }
    return 0;
  }
  if (debug) {
    CDBDebug("warp is required");
  }

  bool useStridingProjection = false;
  int projStrideFactor = 16;
  if (dataWidth * dataHeight > 1000 * 1000) {
    useStridingProjection = true;
  }

  size_t dataSize = (dataWidth + 1) * (dataHeight + 1);

  double *px = new double[dataSize];
  double *py = new double[dataSize];
  bool *skip = new bool[dataSize];

  if (!useStridingProjection) {
    for (int y = 0; y < dataHeight + 1; y++) {
      for (int x = 0; x < dataWidth + 1; x++) {
        size_t p = x + y * (dataWidth + 1);
        double valX = dfSourcedExtW * (x + PXExtentBasedOnSource[0]) + dfSourceOrigX;
        double valY = dfSourcedExtH * (y + PXExtentBasedOnSource[1]) + dfSourceOrigY;
        px[p] = valX;
        py[p] = valY;
        skip[p] = false;
      }
    }
    if (warper->isProjectionRequired()) {
      if (proj_trans_generic(warper->projSourceToDest, PJ_FWD, px, sizeof(double), dataSize, py, sizeof(double), dataSize, nullptr, 0, 0, nullptr, 0, 0) != dataSize) {
        CDBDebug("Unable to do pj_transform");
      }
    }

  } else {

    size_t dataWidthStrided = dataWidth / projStrideFactor + projStrideFactor;
    size_t dataHeightStrided = dataHeight / projStrideFactor + projStrideFactor;
    size_t dataSizeStrided = (dataWidthStrided) * (dataHeightStrided);

    double *pxStrided = new double[dataSizeStrided];
    double *pyStrided = new double[dataSizeStrided];

    /* TODO faster init */
    for (int y = 0; y < dataHeight + 1; y++) {
      for (int x = 0; x < dataWidth + 1; x++) {
        size_t p = x + y * (dataWidth + 1);
        px[p] = DBL_MAX;
        py[p] = DBL_MAX;
        skip[p] = false;
      }
    }
    for (size_t y = 0; y < dataHeightStrided; y++) {
      for (size_t x = 0; x < dataWidthStrided; x++) {
        size_t pS = x + y * dataWidthStrided;

        double valX = dfSourcedExtW * (x * projStrideFactor + PXExtentBasedOnSource[0]) + dfSourceOrigX;
        double valY = dfSourcedExtH * (y * projStrideFactor + PXExtentBasedOnSource[1]) + dfSourceOrigY;
        pxStrided[pS] = valX;
        pyStrided[pS] = valY;
      }
    }

    if (warper->isProjectionRequired()) {
      if (proj_trans_generic(warper->projSourceToDest, PJ_FWD, pxStrided, sizeof(double), dataSizeStrided, pyStrided, sizeof(double), dataSizeStrided, nullptr, 0, 0, nullptr, 0, 0) !=
          dataSizeStrided) {
        CDBDebug("Unable to do pj_transform");
      }
    }
    for (int y = 0; y < dataHeight + 1; y++) {
      for (int x = 0; x < dataWidth + 1; x++) {
        size_t p = x + y * (dataWidth + 1);
        size_t pS = (x / projStrideFactor) + (y / projStrideFactor) * (dataWidthStrided);
        double sX = double(x % projStrideFactor) / double(projStrideFactor);
        double sY = double(y % projStrideFactor) / double(projStrideFactor);
        double x1 = pxStrided[pS] * (1 - sX) + pxStrided[pS + 1] * sX;
        double x2 = pxStrided[pS + dataWidthStrided] * (1 - sX) + pxStrided[pS + 1 + dataWidthStrided] * sX;
        px[p] = x1 * (1 - sY) + x2 * sY;
        double y1 = pyStrided[pS] * (1 - sY) + pyStrided[pS + dataWidthStrided] * sY;
        double y2 = pyStrided[pS + 1] * (1 - sY) + pyStrided[pS + dataWidthStrided + 1] * sY;
        py[p] = y1 * (1 - sX) + y2 * sX;
      }
    }
    delete[] pyStrided;
    delete[] pxStrided;
  }
  if (debug) {
    CDBDebug("Reprojection done");
  }

  for (size_t j = 0; j < dataSize; j++) {
    if (!(px[j] > -DBL_MAX && px[j] < DBL_MAX)) skip[j] = true;
  }

  double avgDX = 0;
  double avgDY = 0;
  // double prevquadX[0],prevquadX[1];

  double pLengthD = 0;

  bool isMercator = isMercatorProjection(&destGeoParams.crs);
  bool isLonLatOrMercatorProjection = isLonLatProjection(&destGeoParams.crs) == true || isMercator;

  double sphereWidth = 360;
  if (isMercator) {
    sphereWidth = 40000000;
  }

  // dataWidth should be +1 because we use a 1 pixel larger grid for the interpolation.
  int offs1 = 0;
  int offs2 = 1;
  int offs3 = dataWidth + 1 + 1;
  int offs4 = dataWidth + 1 + 0;

  CDBDebug("start looping");
  for (int y = 0; y < dataHeight; y = y + 1) {
    for (int x = 0; x < dataWidth; x = x + 1) {
      size_t p = x + y * (dataWidth + 1);
      if (skip[p + offs1] == false && skip[p + offs2] == false && skip[p + offs3] == false && skip[p + offs4] == false) {
        bool doDraw = true;
        // Order for the quad corners is:
        //  quadX[0] -- quadX[1]
        //   |      |
        //  quadX[3] -- quadX[2]

        double quadX[4] = {px[p + offs1], px[p + offs2], px[p + offs3], px[p + offs4]};
        double quadY[4] = {py[p + offs1], py[p + offs2], py[p + offs3], py[p + offs4]};

        if (isLonLatOrMercatorProjection) {
          double lonMin = std::min(quadX[0], std::min(quadX[1], std::min(quadX[2], quadX[3])));
          double lonMax = std::max(quadX[0], std::max(quadX[1], std::max(quadX[2], quadX[3])));
          if (lonMax - lonMin >= sphereWidth * 0.9) {
            double lonMiddle = (lonMin + lonMax) / 2.0;
            if (lonMiddle > 0) {
              quadX[0] += sphereWidth;
              quadX[1] += sphereWidth;
              quadX[2] += sphereWidth;
              quadX[3] += sphereWidth;
            } else {
              quadX[0] -= sphereWidth;
              quadX[1] -= sphereWidth;
              quadX[2] -= sphereWidth;
              quadX[3] -= sphereWidth;
            }
          }
        }

        quadX[0] = (quadX[0] - dfDestOrigX) * multiDestX;
        quadX[1] = (quadX[1] - dfDestOrigX) * multiDestX;
        quadX[2] = (quadX[2] - dfDestOrigX) * multiDestX;
        quadX[3] = (quadX[3] - dfDestOrigX) * multiDestX;

        quadY[0] = (quadY[0] - dfDestOrigY) * multiDestY;
        quadY[1] = (quadY[1] - dfDestOrigY) * multiDestY;
        quadY[2] = (quadY[2] - dfDestOrigY) * multiDestY;
        quadY[3] = (quadY[3] - dfDestOrigY) * multiDestY;

            // If suddenly the length of the quad is 10 times bigger, we probably have an anomaly and we should not draw it.
        // Calculate the diagonal length of the quad.
        double lengthD = (quadX[2] - quadX[0]) * (quadX[2] - quadX[0]) + (quadY[2] - quadY[0]) * (quadY[2] - quadY[0]);
        if (x == 0 && y == 0) {
          pLengthD = lengthD;
        }
        if (lengthD > pLengthD * 10) {
          doDraw = false;
        }
        pLengthD = lengthD;

        // Check the right side of the grid
        if (x == 0) avgDX = quadX[1];
        if (y == 0) avgDY = quadY[3];
        if (x == dataWidth - 1) {
          if (fabs(avgDX - quadX[0]) < fabs(quadX[0] - quadX[1]) / 2) {
            doDraw = false;
          }
          if (fabs(avgDX - quadX[1]) < fabs(quadX[0] - quadX[1]) / 2) {
            doDraw = false;
          }
        }
        // Check the bottom side of the grid
        if (y == dataHeight - 1) {
          if (fabs(avgDY - quadY[0]) < fabs(quadY[0] - quadY[3]) / 2) {
            doDraw = false;
          }
        }

        if (doDraw) {
          warperState.sourceDataPX = x + PXExtentBasedOnSource[0];
          warperState.sourceDataPY = (warperState.sourceDataHeight - 1 - (y + PXExtentBasedOnSource[1]));
          T value = ((T *)warperState.sourceData)[this->warperState.sourceDataPX + this->warperState.sourceDataPY * warperState.sourceDataWidth];
          double xCornersA[3] = {quadX[0], quadX[1], quadX[2]};
          double yCornersA[3] = {quadY[0], quadY[1], quadY[2]};
          double xCornersB[3] = {quadX[2], quadX[0], quadX[3]};
          double yCornersB[3] = {quadY[2], quadY[0], quadY[3]};
          gdwDrawTriangle(xCornersA, yCornersA, value, false, warperState, drawFunction);
          gdwDrawTriangle(xCornersB, yCornersB, value, true, warperState, drawFunction);
        }
      }
    }
  }
  delete[] px;
  delete[] py;
  delete[] skip;
  if (debug) {
    CDBDebug("render done");
  }
  return 0;
}

#define SPECIALIZE_TEMPLATE(CDFTYPE, CPPTYPE)                                                                                                                                                          \
  template int GenericDataWarper::render<CPPTYPE>(CImageWarper * warper, void *_sourceData, GeoParameters &sourceGeoParams, GeoParameters &destGeoParams,                                              \
                                                  const std::function<void(int, int, CPPTYPE, GDWState &warperState)> &drawFunction);
ENUMERATE_OVER_CDFTYPES(SPECIALIZE_TEMPLATE)
#undef SPECIALIZE_TEMPLATE
