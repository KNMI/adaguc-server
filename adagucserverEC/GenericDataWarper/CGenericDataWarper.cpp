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
const char *CGenericDataWarper::className = "GenericDataWarper";
// #define GenericDataWarper_DEBUG
template <typename T>
int CGenericDataWarper::render(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                               void (*drawFunction)(int, int, T, void *drawFunctionSettings, void *genericDataWarper)) {
  this->warperState.sourceData = _sourceData;
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

  warperState.sourceDataWidth = sourceGeoParams->dWidth;
  warperState.sourceDataHeight = sourceGeoParams->dHeight;

  int PXExtentBasedOnSource[4];

  PXExtentBasedOnSource[0] = 0;
  PXExtentBasedOnSource[1] = 0;
  PXExtentBasedOnSource[2] = warperState.sourceDataWidth;
  PXExtentBasedOnSource[3] = warperState.sourceDataHeight;

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
    // CDBDebug("warper->isProjectionRequired() = %d: Applying simple linear transformation",warper->isProjectionRequired());

    // // PXExtentBasedOnSource[0] = source

    // CDBDebug("dfSourceW [%f] ", dfSourceW);
    // CDBDebug("dfSourceOrigX [%f] ", dfSourceOrigX);
    // CDBDebug("dfSourceExtW [%f] ", dfSourceExtW);

    // CDBDebug("dfDestW [%f] ", dfDestW);
    // CDBDebug("dfDestOrigX [%f] ", dfDestOrigX);
    // CDBDebug("dfDestExtW [%f] ", dfDestExtW);

    // double tX = ((dfDestOrigX-dfSourceOrigX)/dfSourceExtW)*dfSourceW;
    // double tY = (((dfDestOrigY+dfDestExtH)-dfSourceOrigY)/dfSourceExtH)*dfSourceH;

    // double eY = ((dfDestOrigY-dfSourceOrigY)/dfSourceExtH)*dfSourceH + 1;
    // double eX = (((dfDestOrigX+dfDestExtW)-dfSourceOrigX)/dfSourceExtW)*dfSourceW + 1;

    // // CDBDebug("TESTX = %f;%f, %f;%f", tX, eX, tY, eY);

    // PXExtentBasedOnSource[0] = tX < 0 ? 0 : tX;
    // PXExtentBasedOnSource[1] = tY < 0 ? 0 : tY;
    // PXExtentBasedOnSource[2] = eX > sourceDataWidth  ?  sourceDataWidth : eX;
    // PXExtentBasedOnSource[3] = eY > sourceDataHeight  ? sourceDataHeight : eY;
    // // CDBDebug("PX extent is [%d,%d,%d,%d] ", PXExtentBasedOnSource[0], PXExtentBasedOnSource[1], PXExtentBasedOnSource[2], PXExtentBasedOnSource[3]);

    dataWidth = PXExtentBasedOnSource[2] - PXExtentBasedOnSource[0];
    dataHeight = PXExtentBasedOnSource[3] - PXExtentBasedOnSource[1];

#ifdef GenericDataWarper_DEBUG

    CDBDebug("PX extent is [%d,%d,%d,%d] ", PXExtentBasedOnSource[0], PXExtentBasedOnSource[1], PXExtentBasedOnSource[2], PXExtentBasedOnSource[3]);

    CDBDebug("dfSourceW [%f] ", dfSourceW);
    CDBDebug("dfDestW [%f] ", dfDestW);
    CDBDebug("dfSourceExtW [%f] ", dfSourceExtW);
    CDBDebug("dfDestExtW [%f] ", dfDestExtW);
    CDBDebug("dfSourceOrigX [%f] ", dfSourceOrigX);
    CDBDebug("dfDestOrigX [%f] ", dfDestOrigX);

    CDBDebug("dfSourceH [%f] ", dfSourceH);
    CDBDebug("dfDestH [%f] ", dfDestH);
    CDBDebug("dfSourceExtH [%f] ", dfSourceExtH);
    CDBDebug("dfDestExtH [%f] ", dfDestExtH);
    CDBDebug("dfSourceOrigY [%f] ", dfSourceOrigX);
    CDBDebug("dfDestOrigY [%f] ", dfDestOrigX);

#endif

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
          this->warperState.sourceDataPX = x;
          this->warperState.sourceDataPY = sourceGeoParams->dHeight - 1 - y;
          T value = ((T *)warperState.sourceData)[this->warperState.sourceDataPX + (this->warperState.sourceDataPY) * sourceGeoParams->dWidth];
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
              this->warperState.tileDy = (sjy - ly1) / double(ly2 - ly1);
              this->warperState.tileDx = (sjx - lx1) / double(lx2 - lx1);
              drawFunction(sjx, sjy, value, drawFunctionSettings, this);
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

        // /*
        //   If the previous pixel width is suddenly 10 times bigger,
        //   or 10 times smaller, skip it .
        //   It is probably wrapped arround the date border.
        // */
        // if (x ==0 && y==0) {
        //   prevpx1=px1;
        //   prevpx2=px2;
        // }
        // if (fabs(prevpx2-prevpx1) < (fabs(px2-px1)/10.0f)) {
        //    doDraw = false;
        // }
        // if (fabs(prevpx2-prevpx1) > (fabs(px2-px1)*10.0f)) {
        //    doDraw = false;
        // }

        // prevpx1=px1;
        // prevpx2=px2;

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
          this->warperState.sourceDataPX = sourceGridX;
          this->warperState.sourceDataPY = (warperState.sourceDataHeight - 1 - sourceGridY);
          T value = ((T *)warperState.sourceData)[this->warperState.sourceDataPX + this->warperState.sourceDataPY * warperState.sourceDataWidth];

          int xP[3];
          int yP[3];
          xP[0] = px1 - 0.5;
          yP[0] = py1 + 0.5;

          xP[1] = px2 - 0.5;
          yP[1] = py2 + 0.5;

          xP[2] = px3 - 0.5;
          yP[2] = py3 + 0.5;
          gdwDrawTriangle<T>(xP, yP, value, imageWidth, imageHeight, drawFunctionSettings, drawFunction, (void *)this, false);

          xP[0] = px3 - 0.5;
          yP[0] = py3 + 0.5;

          xP[1] = px1 - 0.5;
          yP[1] = py1 + 0.5;

          xP[2] = px4 - 0.5;
          yP[2] = py4 + 0.5;
          gdwDrawTriangle<T>(xP, yP, value, imageWidth, imageHeight, drawFunctionSettings, drawFunction, (void *)this, true);
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

template int CGenericDataWarper::render<char>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                              void (*drawFunction)(int, int, char, void *drawFunctionSettings, void *genericDataWarper));
template int CGenericDataWarper::render<unsigned char>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                                       void (*drawFunction)(int, int, unsigned char, void *drawFunctionSettings, void *genericDataWarper));
template int CGenericDataWarper::render<short>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                               void (*drawFunction)(int, int, short, void *drawFunctionSettings, void *genericDataWarper));
template int CGenericDataWarper::render<ushort>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                                void (*drawFunction)(int, int, ushort, void *drawFunctionSettings, void *genericDataWarper));
template int CGenericDataWarper::render<int>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                             void (*drawFunction)(int, int, int, void *drawFunctionSettings, void *genericDataWarper));
template int CGenericDataWarper::render<uint>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                              void (*drawFunction)(int, int, uint, void *drawFunctionSettings, void *genericDataWarper));
template int CGenericDataWarper::render<long>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                              void (*drawFunction)(int, int, long, void *drawFunctionSettings, void *genericDataWarper));
template int CGenericDataWarper::render<unsigned long>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                                       void (*drawFunction)(int, int, unsigned long, void *drawFunctionSettings, void *genericDataWarper));
template int CGenericDataWarper::render<float>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                               void (*drawFunction)(int, int, float, void *drawFunctionSettings, void *genericDataWarper));
template int CGenericDataWarper::render<double>(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
                                                void (*drawFunction)(int, int, double, void *drawFunctionSettings, void *genericDataWarper));