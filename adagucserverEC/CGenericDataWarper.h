#ifndef GenericDataWarper_H
#define GenericDataWarper_H
#include <math.h>
#include <stdlib.h>
#include <proj.h>
#include <math.h>
#include <cfloat>
#include "CGeoParams.h"
#include "CImageWarper.h"
#include "CDebugger.h"
#include "CGenericDataWarperTools.h"

// #define GenericDataWarper_DEBUG

class GenericDataWarper {
private:
  DEF_ERRORFUNCTION();
  template <class T>
  static int drawTriangle(int *xP, int *yP, T value, int destWidth, int destHeight, void *settings, void (*drawFunction)(int, int, T, void *settings, void *genericDataWarper), void *genericDataWarper,
                          bool aOrB) {
    GenericDataWarper *g = (GenericDataWarper *)genericDataWarper;
    int W = destWidth;
    int H = destHeight;
    if (xP[0] < 0 && xP[1] < 0 && xP[2] < 0) return 0;
    if (xP[0] >= W && xP[1] >= W && xP[2] >= W) return 0;
    if (yP[0] < 0 && yP[1] < 0 && yP[2] < 0) return 0;
    if (yP[0] >= H && yP[1] >= H && yP[2] >= H) return 0;

    unsigned int lower;
    unsigned int middle;
    unsigned int upper;

    /*Sort the vertices in Y direction*/
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

    double vX1 = aOrB ? 1 : 0;
    double vY1 = aOrB ? 0 : 1;
    double vX2 = aOrB ? 0 : 1;
    double vY2 = 1;
    double vX3 = aOrB ? 0 : 1;
    double vY3 = 0;

    // return 1;
    /*
    //  1
    //   \
    //    2
    //   /
    //  3
    */
    // If top point is equal to bottom point of triangle, skip
    // If middle is equal to top and middle is equal to bottom, skip
    if ((Y1 == Y3) || (Y2 == Y1 && Y3 == Y2)) {
      int minx = X1;
      if (minx > X2) minx = X2;
      if (minx > X3) minx = X3;
      int maxx = X1;
      if (maxx < X2) maxx = X2;
      if (maxx < X3) maxx = X3;
      g->tileDy = 0;
      for (int x = minx; x < maxx + 1; x++) {
        g->tileDx = 0; //(x - minx) / double(maxx-minx);
        drawFunction(x, yP[2], value, settings, genericDataWarper);
      }
      return 1;
    }

    /* https://codeplea.com/triangular-interpolation */
    double dn = ((yP[1] - yP[2]) * (xP[0] - xP[2]) + (xP[2] - xP[1]) * (yP[0] - yP[2]));

    double rcl = double(X3 - X1) / double(Y3 - Y1);
    if (Y2 != Y1 && Y1 < H && Y2 > 0) {
      double rca = double(X2 - X1) / double(Y2 - Y1);
      int sy = (Y1 < 0) ? 0 : Y1;
      int ey = (Y2 > H) ? H : Y2;
      for (int y = sy; y <= ey - 1; y++) {
        int xL = floor(rcl * double(y - Y1) + X1);
        int xA = floor(rca * double(y - Y1) + X1);
        int x1, x2;
        if (xL < xA) {
          x1 = xL;
          x2 = xA;
        } else {
          x2 = xL;
          x1 = xA;
        }
        if (x1 < W && x2 > 0) {
          int sx = (x1 < 0) ? 0 : x1;
          int ex = (x2 > W) ? W : x2;
          for (int x = sx; x <= ex - 1; x++) {
            double WV1 = ((yP[1] - yP[2]) * (x - xP[2]) + (xP[2] - xP[1]) * (y - yP[2])) / dn;
            double WV2 = ((yP[2] - yP[0]) * (x - xP[2]) + (xP[0] - xP[2]) * (y - yP[2])) / dn;
            double WV3 = 1 - WV1 - WV2;

            g->tileDx = WV1 * vX1 + WV2 * vX2 + WV3 * vX3;
            g->tileDy = WV1 * vY1 + WV2 * vY2 + WV3 * vY3;
            drawFunction(x, y, value, settings, genericDataWarper);
          }
        }
      }
    }
    // return 0;
    if (Y3 != Y2 && Y2 < H && Y3 > 0) {
      double rcb = double(X3 - X2) / double(Y3 - Y2);
      int sy = (Y2 < 0) ? 0 : Y2;
      int ey = (Y3 > H) ? H : Y3;
      for (int y = sy; y <= ey - 1; y++) {
        int xL = (rcl * double(y - Y1) + X1); // floor
        int xB = (rcb * double(y - Y2) + X2); // floor
        int x1, x2;
        if (xL <= xB) {
          x1 = xL;
          x2 = xB;
        } else {
          x2 = xL;
          x1 = xB;
        }
        if (x1 < W && x2 > 0) {
          int sx = (x1 < 0) ? 0 : x1;
          int ex = (x2 > W) ? W : x2;
          for (int x = sx; x < ex; x++) {
            double WV1 = ((yP[1] - yP[2]) * (x - xP[2]) + (xP[2] - xP[1]) * (y - yP[2])) / dn;
            double WV2 = ((yP[2] - yP[0]) * (x - xP[2]) + (xP[0] - xP[2]) * (y - yP[2])) / dn;
            double WV3 = 1 - WV1 - WV2;

            g->tileDx = WV1 * vX1 + WV2 * vX2 + WV3 * vX3;
            g->tileDy = WV1 * vY1 + WV2 * vY2 + WV3 * vY3;
            // http://localhost:8080/adaguc-server?DATASET=noaaglm&SERVICE=WMS&&SERVICE=WMS&VERSION=1.3.0&REQUEST=GetMap&LAYERS=LIGHTNING_COUNTS&WIDTH=1321&HEIGHT=959&CRS=EPSG%3A32661&BBOX=-8314748.562870221,-1301096.785244672,-8149138.113029413,-1180869.3655646304&STYLES=counts_knmi%2Fgeneric&FORMAT=image/png&TRANSPARENT=TRUE&&time=2024-06-28T09%3A00%3A00Z&0.013241703752992606
            drawFunction(x, y, value, settings, genericDataWarper);
          }
        }
      }
    }
    return 0;
  }

public:
  GenericDataWarper() { useHalfCellOffset = false; }

  /* Can be used in drawfunctions */
  void *sourceData;

  int sourceDataPX, sourceDataPY, sourceDataWidth, sourceDataHeight;
  double tileDx, tileDy;
  bool useHalfCellOffset;

  static int findPixelExtent(int *PXExtentBasedOnSource, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, CImageWarper *warper);

  template <class T>
  int render(CImageWarper *warper, void *_sourceData, CGeoParams *sourceGeoParams, CGeoParams *destGeoParams, void *drawFunctionSettings,
             void (*drawFunction)(int, int, T, void *drawFunctionSettings, void *genericDataWarper)) {
    this->sourceData = _sourceData;
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

    sourceDataWidth = sourceGeoParams->dWidth;
    sourceDataHeight = sourceGeoParams->dHeight;

    int PXExtentBasedOnSource[4];

    PXExtentBasedOnSource[0] = 0;
    PXExtentBasedOnSource[1] = 0;
    PXExtentBasedOnSource[2] = sourceDataWidth;
    PXExtentBasedOnSource[3] = sourceDataHeight;

    bool tryToFindExtend = false;

    if (tryToFindExtend) {
      findPixelExtent(PXExtentBasedOnSource, sourceGeoParams, destGeoParams, warper);
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
            this->sourceDataPX = x;
            this->sourceDataPY = sourceGeoParams->dHeight - 1 - y;
            T value = ((T *)sourceData)[this->sourceDataPX + (this->sourceDataPY) * sourceGeoParams->dWidth];
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
                this->tileDy = (sjy - ly1) / double(ly2 - ly1);
                this->tileDx = (sjx - lx1) / double(lx2 - lx1);
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
          ;
          double x2 = pxStrided[pS + dataWidthStrided] * (1 - sX) + pxStrided[pS + 1 + dataWidthStrided] * sX;
          ;
          px[p] = x1 * (1 - sY) + x2 * sY;

          double y1 = pyStrided[pS] * (1 - sY) + pyStrided[pS + dataWidthStrided] * sY;
          ;
          double y2 = pyStrided[pS + 1] * (1 - sY) + pyStrided[pS + dataWidthStrided + 1] * sY;
          ;
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
            this->sourceDataPX = sourceGridX;
            this->sourceDataPY = (sourceDataHeight - 1 - sourceGridY);
            T value = ((T *)sourceData)[this->sourceDataPX + this->sourceDataPY * sourceDataWidth];

            int xP[3];
            int yP[3];
            xP[0] = px1;
            yP[0] = py1;

            xP[1] = px2;
            yP[1] = py2;

            xP[2] = px3;
            yP[2] = py3;
            drawTriangle<T>(xP, yP, value, imageWidth, imageHeight, drawFunctionSettings, drawFunction, (void *)this, false);

            xP[0] = px3;
            yP[0] = py3;

            xP[1] = px1;
            yP[1] = py1;

            xP[2] = px4;
            yP[2] = py4;
            drawTriangle<T>(xP, yP, value, imageWidth, imageHeight, drawFunctionSettings, drawFunction, (void *)this, true);
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
};
#endif
