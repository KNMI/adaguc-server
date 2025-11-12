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

// #define GenericDataWarper_DEBUG

template <typename T>
int GenericDataWarper::render(CImageWarper *warper, void *_sourceData, CGeoParams &sourceGeoParams, CGeoParams &destGeoParams,
                              const std::function<void(int, int, T, GDWState &warperState)> &drawFunction) {

  warperState.sourceData = _sourceData;
  warperState.destDataWidth = destGeoParams.dWidth;
  warperState.destDataHeight = destGeoParams.dHeight;
  warperState.sourceDataWidth = sourceGeoParams.dWidth;
  warperState.sourceDataHeight = sourceGeoParams.dHeight;

#ifdef GenericDataWarper_DEBUG
  CDBDebug("render");
#endif

  int imageHeight = destGeoParams.dHeight;
  int imageWidth = destGeoParams.dWidth;

  // Reproj back and forth sourceGeoParams boundingbox
  double y1 = sourceGeoParams.bbox.bottom;
  double y2 = sourceGeoParams.bbox.top;
  double x1 = sourceGeoParams.bbox.left;
  double x2 = sourceGeoParams.bbox.right;

#ifdef GenericDataWarper_DEBUG
  CDBDebug("sourceGeoParams.dfBBOX %f, %f, %f, %f", sourceGeoParams.bbox.left, sourceGeoParams.bbox.bottom, sourceGeoParams.bbox.right, sourceGeoParams.bbox.top);
  CDBDebug("destGeoParams.dfBBOX %f, %f, %f, %f", destGeoParams.bbox.left, destGeoParams.bbox.bottom, destGeoParams.bbox.right, destGeoParams.bbox.top);
#endif
  if (y2 < y1) {
    if (y1 > -360 && y2 < 360 && x1 > -720 && x2 < 720) {
      if (isLonLatProjection(&sourceGeoParams.CRS) == false) {
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
  double dfSourceW = double(sourceGeoParams.dWidth);
  double dfSourceH = double(sourceGeoParams.dHeight);

  double dfDestW = double(destGeoParams.dWidth);
  double dfDestH = double(destGeoParams.dHeight);

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
#ifdef GenericDataWarper_DEBUG
  CDBDebug("Creating px extent");
#endif

  warperState.sourceDataWidth = sourceGeoParams.dWidth;
  warperState.sourceDataHeight = sourceGeoParams.dHeight;

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
    i4point wh = {.x = sourceGeoParams.dWidth, .y = sourceGeoParams.dHeight};
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
        if (sx1 >= destGeoParams.dWidth + sxw && sx2 >= destGeoParams.dWidth + sxw) skip = true;
        if (sy1 >= destGeoParams.dHeight + syh && sy2 >= destGeoParams.dHeight + syh) skip = true;
        //

        if (!skip) {
          warperState.sourceDataPX = x;
          warperState.sourceDataPY = sourceGeoParams.dHeight - 1 - y;
          T value = ((T *)warperState.sourceData)[warperState.sourceDataPX + (warperState.sourceDataPY) * sourceGeoParams.dWidth];
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
#ifdef GenericDataWarper_DEBUG
  CDBDebug("warp is required");
#endif

  bool useStridingProjection = false;
  int projStrideFactor = 16;
  if (dataWidth * dataHeight > 1000 * 1000) {
    useStridingProjection = true;
  }

  size_t dataSize = (dataWidth + 1) * (dataHeight + 1);

  double *px = new double[dataSize];
  double *py = new double[dataSize];
  char *skip = new char[dataSize];

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
#ifdef GenericDataWarper_DEBUG
  CDBDebug("Reprojection done");
#endif

  for (size_t j = 0; j < dataSize; j++) {
    if (!(px[j] > -DBL_MAX && px[j] < DBL_MAX)) skip[j] = true;
  }

  double avgDX = 0;
  double avgDY = 0;
  // double prevpx1,prevpx2;

  double pLengthD = 0;

  for (int y = 0; y < dataHeight; y = y + 1) {
    for (int x = 0; x < dataWidth; x = x + 1) {
      size_t p = x + y * (dataWidth + 1);
      if (skip[p] == false && skip[p + 1] == false && skip[p + dataWidth + 1] == false && skip[p + dataWidth + 2] == false) {
        bool doDraw = true;
        // Order for the quad corners is:
        //  px1 -- px2
        //   |      |
        //  px4 -- px3

        double px1 = px[p];
        double px2 = px[p + 1];
        double px3 = px[p + dataWidth + 2];
        double px4 = px[p + dataWidth + 1];

        double py1 = py[p];
        double py2 = py[p + 1];
        double py3 = py[p + dataWidth + 2];
        double py4 = py[p + dataWidth + 1];

        // CDBDebug("destGeoParams = %s",destGeoParams.CRS.c_str());
        if (isLonLatProjection(&destGeoParams.CRS) == true || isMercatorProjection(&destGeoParams.CRS) == true) {
          double lons[4];
          lons[0] = px1;
          lons[1] = px2;
          lons[2] = px3;
          lons[3] = px4;

          double lonMin, lonMax, lonMiddle = 0;
          for (int j = 0; j < 4; j++) {
            double lon = lons[j];
            if (j == 0) {
              lonMin = lon;
              lonMax = lon;
            } else {
              if (lon < lonMin) lonMin = lon;
              if (lon > lonMax) lonMax = lon;
            }
            lonMiddle += lon;
            if (lon == INFINITY || lon == -INFINITY || !(lon == lon)) {
              doDraw = false;
              break;
            }
          }
          lonMiddle /= 4;
          double sphereWidth = 360;
          if (isMercatorProjection(&destGeoParams.CRS)) {
            sphereWidth = 40000000;
          }

          if (lonMax - lonMin >= sphereWidth * 0.9) {
            if (lonMiddle > 0) {
              for (int j = 0; j < 4; j++)
                if (lons[j] < lonMiddle) lons[j] += sphereWidth;
            } else {
              for (int j = 0; j < 4; j++)
                if (lons[j] > lonMiddle) lons[j] -= sphereWidth;
            }
          }
          px1 = lons[0];
          px2 = lons[1];
          px3 = lons[2];
          px4 = lons[3];
        }

        px1 = (px1 - dfDestOrigX) * multiDestX;
        px2 = (px2 - dfDestOrigX) * multiDestX;
        px3 = (px3 - dfDestOrigX) * multiDestX;
        px4 = (px4 - dfDestOrigX) * multiDestX;

        py1 = (py1 - dfDestOrigY) * multiDestY;
        py2 = (py2 - dfDestOrigY) * multiDestY;
        py3 = (py3 - dfDestOrigY) * multiDestY;
        py4 = (py4 - dfDestOrigY) * multiDestY;

        if (x == 0) avgDX = px2;
        if (y == 0) avgDY = py4;

        // Calculate the diagonal length of the quad.
        double lengthD = (px3 - px1) * (px3 - px1) + (py3 - py1) * (py3 - py1);

        if (x == 0 && y == 0) {
          pLengthD = lengthD;
        }

        // If suddenly the length of the quad is 10 times bigger, we probably have an anomaly and we should not draw it.
        if (lengthD > pLengthD * 10) {
          doDraw = false;
        }

        if (x == dataWidth - 1) {
          if (fabs(avgDX - px1) < fabs(px1 - px2) / 2) {
            doDraw = false;
          }
          if (fabs(avgDX - px2) < fabs(px1 - px2) / 2) {
            doDraw = false;
          }
        }
        if (y == dataHeight - 1) {
          if (fabs(avgDY - py1) < fabs(py1 - py4) / 2) {
            doDraw = false;
          }
        }

        if (doDraw) {
          int sourceGridX = x + PXExtentBasedOnSource[0];
          int sourceGridY = y + PXExtentBasedOnSource[1];
          warperState.sourceDataPX = sourceGridX;
          warperState.sourceDataPY = (warperState.sourceDataHeight - 1 - sourceGridY);
          T value = ((T *)warperState.sourceData)[this->warperState.sourceDataPX + this->warperState.sourceDataPY * warperState.sourceDataWidth];

          double xCornersA[3] = {px1, px2, px3};
          double yCornersA[3] = {py1, py2, py3};

          double xCornersB[3] = {px3, px1, px4};
          double yCornersB[3] = {py3, py1, py4};

          gdwDrawTriangle(xCornersA, yCornersA, value, false, warperState, drawFunction);
          gdwDrawTriangle(xCornersB, yCornersB, value, true, warperState, drawFunction);
        }
        pLengthD = lengthD;
      }
    }
  }
  delete[] px;
  delete[] py;
  delete[] skip;
#ifdef GenericDataWarper_DEBUG
  CDBDebug("render done");
#endif
  return 0;
}

#define SPECIALIZE_TEMPLATE(CDFTYPE, CPPTYPE)                                                                                                                                                          \
  template int GenericDataWarper::render<CPPTYPE>(CImageWarper * warper, void *_sourceData, CGeoParams &sourceGeoParams, CGeoParams &destGeoParams,                                                    \
                                                  const std::function<void(int, int, CPPTYPE, GDWState &warperState)> &drawFunction);
ENUMERATE_OVER_CDFTYPES(SPECIALIZE_TEMPLATE)
#undef SPECIALIZE_TEMPLATE
