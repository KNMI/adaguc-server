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

template <typename T> double getValueFromGrid(int x, int y, GDWDrawFunctionBaseState *drawFunctionState) { return ((T *)drawFunctionState->sourceData)[x + y * drawFunctionState->sourceDataWidth]; }

// #define GenericDataWarper_DEBUG
template <typename T>
int warpT(CImageWarper *warper, void *_sourceData, CDFType sourceDataType, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, GDWDrawFunctionBaseState *drawFunctionSettings) {
  drawFunctionSettings->sourceData = _sourceData;
  drawFunctionSettings->sourceDataType = sourceDataType; // TODO REMOVE

  drawFunctionSettings->destDataWidth = destGeoParams->dWidth;
  drawFunctionSettings->destDataHeight = destGeoParams->dHeight;

  drawFunctionSettings->sourceDataWidth = sourceGeoParams->dWidth;
  drawFunctionSettings->sourceDataHeight = sourceGeoParams->dHeight;

  drawFunctionSettings->getValueFromSourceFunction = getValueFromGrid<T>;

#ifdef GenericDataWarper_DEBUG
  CDBDebug("render");
#endif

  int imageHeight = destGeoParams->dHeight;
  int imageWidth = destGeoParams->dWidth;

  // Reproj back and forth sourceGeoParams boundingbox
  double y1 = sourceGeoParams->dfBBOX[1];
  double y2 = sourceGeoParams->dfBBOX[3];
  double x1 = sourceGeoParams->dfBBOX[0];
  double x2 = sourceGeoParams->dfBBOX[2];

#ifdef GenericDataWarper_DEBUG
  CDBDebug("sourceGeoParams->dfBBOX %f, %f, %f, %f", sourceGeoParams->dfBBOX[0], sourceGeoParams->dfBBOX[1], sourceGeoParams->dfBBOX[2], sourceGeoParams->dfBBOX[3]);
  CDBDebug("destGeoParams->dfBBOX %f, %f, %f, %f", destGeoParams->dfBBOX[0], destGeoParams->dfBBOX[1], destGeoParams->dfBBOX[2], destGeoParams->dfBBOX[3]);
#endif
  if (y2 < y1) {
    if (y1 > -360 && y2 < 360 && x1 > -720 && x2 < 720) {
      if (CGeoParams::isLonLatProjection(&sourceGeoParams->CRS) == false) {
        double checkBBOX[4];
        for (int j = 0; j < 4; j++) checkBBOX[j] = sourceGeoParams->dfBBOX[j];

        // CDBDebug("Current BBOX:  %f %f %f %f",sourceGeoParams->dfBBOX[0],sourceGeoParams->dfBBOX[1],sourceGeoParams->dfBBOX[2],sourceGeoParams->dfBBOX[3]);
        bool hasError = false;
        if (warper->reprojpoint_inv(checkBBOX[0], checkBBOX[1]) != 0) hasError = true;
        if (warper->reprojpoint(checkBBOX[0], checkBBOX[1]) != 0) hasError = true;

        if (warper->reprojpoint_inv(checkBBOX[2], checkBBOX[3]) != 0) hasError = true;
        if (warper->reprojpoint(checkBBOX[2], checkBBOX[3]) != 0) hasError = true;

        if (hasError == false) {
          for (int j = 0; j < 4; j++) sourceGeoParams->dfBBOX[j] = checkBBOX[j];
        }
      }
    }
  }

  double dfSourceExtW = (sourceGeoParams->dfBBOX[2] - sourceGeoParams->dfBBOX[0]);
  double dfSourceExtH = (sourceGeoParams->dfBBOX[3] - sourceGeoParams->dfBBOX[1]);
  double dfSourceW = double(sourceGeoParams->dWidth);
  double dfSourceH = double(sourceGeoParams->dHeight);

  double dfDestW = double(destGeoParams->dWidth);
  double dfDestH = double(destGeoParams->dHeight);

  double dfSourcedExtW = dfSourceExtW / (dfSourceW);
  double dfSourcedExtH = dfSourceExtH / (dfSourceH);
  double dfSourceOrigX = sourceGeoParams->dfBBOX[0];
  double dfSourceOrigY = sourceGeoParams->dfBBOX[1];

  double dfDestExtW = destGeoParams->dfBBOX[2] - destGeoParams->dfBBOX[0];
  double dfDestExtH = destGeoParams->dfBBOX[1] - destGeoParams->dfBBOX[3];
  double multiDestX = double(imageWidth) / dfDestExtW;

  double multiDestY = double(imageHeight) / dfDestExtH;

  double dfDestOrigX = destGeoParams->dfBBOX[0]; //-0.5/multiDestX;;
  double dfDestOrigY = destGeoParams->dfBBOX[3]; //+0.5/multiDestY;;;

  // Determine source BBOX of based on destination grid
#ifdef GenericDataWarper_DEBUG
  CDBDebug("Creating px extent");
#endif

  drawFunctionSettings->sourceDataWidth = sourceGeoParams->dWidth;
  drawFunctionSettings->sourceDataHeight = sourceGeoParams->dHeight;

  int PXExtentBasedOnSource[4];

  PXExtentBasedOnSource[0] = 0;
  PXExtentBasedOnSource[1] = 0;
  PXExtentBasedOnSource[2] = drawFunctionSettings->sourceDataWidth;
  PXExtentBasedOnSource[3] = drawFunctionSettings->sourceDataHeight;

  bool tryToFindExtend = false;

  if (tryToFindExtend) {
    gdwFindPixelExtent(PXExtentBasedOnSource, sourceGeoParams, destGeoParams, warper);
  }

  if (PXExtentBasedOnSource[2] - PXExtentBasedOnSource[0] <= 0) return 0;
  if (PXExtentBasedOnSource[3] - PXExtentBasedOnSource[1] <= 0) return 0;
  int dataWidth = PXExtentBasedOnSource[2] - PXExtentBasedOnSource[0];
  int dataHeight = PXExtentBasedOnSource[3] - PXExtentBasedOnSource[1];

  /* When geographical map projections are equal, just do a simple linear transformation */
  if (warper->isProjectionRequired() == false) {

    dataWidth = PXExtentBasedOnSource[2] - PXExtentBasedOnSource[0];
    dataHeight = PXExtentBasedOnSource[3] - PXExtentBasedOnSource[1];

    for (int y = PXExtentBasedOnSource[1]; y < PXExtentBasedOnSource[3]; y++) {
      for (int x = PXExtentBasedOnSource[0]; x < PXExtentBasedOnSource[2]; x++) {

        double dfx = x;
        double dfy = y;
        int sx1 = roundedLinearTransform(dfx, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW);
        int sx2 = roundedLinearTransform(dfx + 1, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW);
        int sy1 = roundedLinearTransform(dfy, dfSourceH, dfSourceExtH, dfSourceOrigY, dfDestOrigY, dfDestExtH, dfDestH);
        int sy2 = roundedLinearTransform(dfy + 1, dfSourceH, dfSourceExtH, dfSourceOrigY, dfDestOrigY, dfDestExtH, dfDestH);
        bool skip = false;
        int sxw = floor(fabs(sx2 - sx1)) + 1;
        int syh = floor(fabs(sy2 - sy1)) + 1;
        // CDBDebug("%d %d %d %d", sx1, sy1, sx2 ,sy2);
        if (sx1 < -sxw && sx2 < -sxw) skip = true;
        if (sy1 < -syh && sy2 < -syh) skip = true;
        if (sx1 >= destGeoParams->dWidth + sxw && sx2 >= destGeoParams->dWidth + sxw) skip = true;
        if (sy1 >= destGeoParams->dHeight + syh && sy2 >= destGeoParams->dHeight + syh) skip = true;
        //

        if (!skip) {
          drawFunctionSettings->sourceDataPX = x;
          drawFunctionSettings->sourceDataPY = sourceGeoParams->dHeight - 1 - y;
          // T value = ((T *)drawFunctionState.sourceData)[drawFunctionSettings->sourceDataPX + (drawFunctionSettings->sourceDataPY) * sourceGeoParams->dWidth];
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
              drawFunctionSettings->tileDy = (sjy - ly1) / double(ly2 - ly1);
              drawFunctionSettings->tileDx = (sjx - lx1) / double(lx2 - lx1);
              drawFunctionSettings->destX = sjx;
              drawFunctionSettings->destY = sjy;
              // drawFunction(sjx, sjy, value, drawFunctionSettings, this);
              drawFunctionSettings->setValueInDestinationFunction(drawFunctionSettings);
            }
          }
        }
      }
    }
    // CDBDebug("warper->isProjectionRequired() = %d: Finished simple linear transformation",warper->isProjectionRequired());
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

  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  bool useHalfCellOffset = false;

  double halfCell = useHalfCellOffset ? 0.5 : 0;

  size_t dataSize = (dataWidth + 1) * (dataHeight + 1);

  double *px = new double[dataSize];
  double *py = new double[dataSize];
  char *skip = new char[dataSize];

  if (!useStridingProjection) {
    for (int y = 0; y < dataHeight + 1; y++) {
      for (int x = 0; x < dataWidth + 1; x++) {
        size_t p = x + y * (dataWidth + 1);
        double valX = dfSourcedExtW * (x + halfCell + PXExtentBasedOnSource[0]) + dfSourceOrigX;
        double valY = dfSourcedExtH * (y - halfCell + PXExtentBasedOnSource[1]) + dfSourceOrigY;
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

        double valX = dfSourcedExtW * (x * projStrideFactor + halfCell + PXExtentBasedOnSource[0]) + dfSourceOrigX;
        double valY = dfSourcedExtH * (y * projStrideFactor - halfCell + PXExtentBasedOnSource[1]) + dfSourceOrigY;
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

        // CDBDebug("destGeoParams = %s",destGeoParams->CRS.c_str());
        if (CGeoParams::isLonLatProjection(&destGeoParams->CRS) == true || CGeoParams::isMercatorProjection(&destGeoParams->CRS) == true) {
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
          if (CGeoParams::isMercatorProjection(&destGeoParams->CRS)) {
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
          drawFunctionSettings->sourceDataPX = sourceGridX;
          drawFunctionSettings->sourceDataPY = (drawFunctionSettings->sourceDataHeight - 1 - sourceGridY);
          // T value = ((T *)drawFunctionState.sourceData)[this->drawFunctionState.sourceDataPX + this->drawFunctionState.sourceDataPY * drawFunctionState.sourceDataWidth];

          double xCornersA[3] = {px1, px2, px3};
          double yCornersA[3] = {py1, py2, py3};

          double xCornersB[3] = {px3, px1, px4};
          double yCornersB[3] = {py3, py1, py4};

          gdwDrawTriangle(xCornersA, yCornersA, false, drawFunctionSettings);
          gdwDrawTriangle(xCornersB, yCornersB, true, drawFunctionSettings);
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
template <typename T> struct DrawFunctionState : GDWDrawFunctionBaseState {

  void *oldDrawFunction;
  void (*oldDrawFunction2)(int, int, T, void *drawFunctionSettings);
  void *drawFunctionSettings;
};

template <typename T> void genericDrawFunction(GDWDrawFunctionBaseState *_drawSettings) {
  DrawFunctionState<T> *drawFunctionState = (DrawFunctionState<T> *)_drawSettings;
  int x = drawFunctionState->destX;
  int y = drawFunctionState->destY;
  if (x < 0 || y < 0 || x >= drawFunctionState->destDataWidth || y >= drawFunctionState->destDataHeight) return;
  T val = drawFunctionState->getValueFromSourceFunction(drawFunctionState->sourceDataPX, drawFunctionState->sourceDataPY, drawFunctionState);

  auto f = (void (*)(int, int, T, void *, void *))drawFunctionState->oldDrawFunction;
  f(drawFunctionState->destX, drawFunctionState->destY, val, drawFunctionState->drawFunctionSettings, nullptr);
}

template <typename T>
int GenericDataWarper::render(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                              void (*drawFunction)(int, int, T, void *drawFunctionSettings)) {

  DrawFunctionState<T> settings;
  settings.destDataWidth = destGeoParams->dWidth;
  settings.destDataHeight = destGeoParams->dHeight;
  settings.setValueInDestinationFunction = genericDrawFunction<T>;
  settings.drawFunctionSettings = drawFunctionSettings;
  settings.oldDrawFunction = (void *)drawFunction;

  return warpT<T>(warper, _sourceData, CDF_FLOAT, sourceGeoParams, destGeoParams, &settings);
}

template int GenericDataWarper::render<char>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                             void (*drawFunction)(int, int, char, void *drawFunctionSettings));
template int GenericDataWarper::render<unsigned char>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                                      void (*drawFunction)(int, int, unsigned char, void *drawFunctionSettings));
template int GenericDataWarper::render<short>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                              void (*drawFunction)(int, int, short, void *drawFunctionSettings));
template int GenericDataWarper::render<unsigned short>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                                       void (*drawFunction)(int, int, unsigned short, void *drawFunctionSettings));
template int GenericDataWarper::render<int>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                            void (*drawFunction)(int, int, int, void *drawFunctionSettings));
template int GenericDataWarper::render<uint>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                             void (*drawFunction)(int, int, uint, void *drawFunctionSettings));
template int GenericDataWarper::render<long>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                             void (*drawFunction)(int, int, long, void *drawFunctionSettings));
template int GenericDataWarper::render<unsigned long>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                                      void (*drawFunction)(int, int, unsigned long, void *drawFunctionSettings));
template int GenericDataWarper::render<float>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                              void (*drawFunction)(int, int, float, void *drawFunctionSettings));
template int GenericDataWarper::render<double>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                               void (*drawFunction)(int, int, double, void *drawFunctionSettings));