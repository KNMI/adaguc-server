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

int CImgWarpNearestNeighbour::reproj(CImageWarper *warper, CDataSource *, GeoParameters &GeoDest, double dfx, double dfy, double x_div, double y_div) {
  double psx[4];
  double psy[4];
  double dfTiledBBOX[4];
  double dfTileW = GeoDest.bbox.span().x / double(x_div);
  double dfTileH = GeoDest.bbox.span().y / double(y_div);

  dfTiledBBOX[0] = GeoDest.bbox.left + dfTileW * dfx;
  dfTiledBBOX[1] = GeoDest.bbox.bottom + dfTileH * dfy;
  dfTiledBBOX[2] = dfTiledBBOX[0] + (dfTileW);
  dfTiledBBOX[3] = dfTiledBBOX[1] + (dfTileH);
  double dfSourceBBOX[4];
  for (int k = 0; k < 4; k++) dfSourceBBOX[k] = dfMaskBBOX[k];
  if (dfMaskBBOX[3] < dfMaskBBOX[1]) {
    dfSourceBBOX[1] = dfMaskBBOX[3];
    dfSourceBBOX[3] = dfMaskBBOX[1];
  }
  if ((dfTiledBBOX[0] > dfSourceBBOX[0] - dfTileW && dfTiledBBOX[0] < dfSourceBBOX[2] + dfTileW) && (dfTiledBBOX[2] > dfSourceBBOX[0] - dfTileW && dfTiledBBOX[2] < dfSourceBBOX[2] + dfTileW) &&
      (dfTiledBBOX[1] > dfSourceBBOX[1] - dfTileH && dfTiledBBOX[1] < dfSourceBBOX[3] + dfTileH) && (dfTiledBBOX[3] > dfSourceBBOX[1] - dfTileH && dfTiledBBOX[3] < dfSourceBBOX[3] + dfTileH)) {
  } else {
    return 1;
  }
  psx[0] = dfTiledBBOX[2];
  psx[1] = dfTiledBBOX[2];
  psx[2] = dfTiledBBOX[0];
  psx[3] = dfTiledBBOX[0];
  psy[0] = dfTiledBBOX[1];
  psy[1] = dfTiledBBOX[3];
  psy[2] = dfTiledBBOX[3];
  psy[3] = dfTiledBBOX[1];
  if (warper->isProjectionRequired()) {
    if (proj_trans_generic(warper->projSourceToDest, PJ_INV, psx, sizeof(double), 4, psy, sizeof(double), 4, nullptr, 0, 0, nullptr, 0, 0) != 4) {
      CDBDebug("Unable to do proj_trans_generic");
    }
  }
  x_corners[0] = psx[1];
  y_corners[0] = psy[1];

  x_corners[1] = psx[0];
  y_corners[1] = psy[0];

  x_corners[2] = psx[3];
  y_corners[2] = psy[3];

  x_corners[3] = psx[2];
  y_corners[3] = psy[2];

  return 0;
}

void CImgWarpNearestNeighbour::render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {
#ifdef CIMGWARPNEARESTNEIGHBOUR_DEBUG
  CDBDebug("Render");
#endif

  bool fieldsAreIdentical = true;
  if ((float)dataSource->dfBBOX[0] != (float)drawImage->geoParams.bbox.left) {
    fieldsAreIdentical = false;
  }
  if ((float)dataSource->dfBBOX[1] != (float)drawImage->geoParams.bbox.top) {
    fieldsAreIdentical = false;
  }
  if ((float)dataSource->dfBBOX[2] != (float)drawImage->geoParams.bbox.right) {
    fieldsAreIdentical = false;
  }
  if ((float)dataSource->dfBBOX[3] != (float)drawImage->geoParams.bbox.bottom) {
    fieldsAreIdentical = false;
  }
  if ((int)dataSource->dWidth != (int)drawImage->geoParams.width) {
    fieldsAreIdentical = false;
  }
  if ((int)dataSource->dHeight != (int)drawImage->geoParams.height) {
    fieldsAreIdentical = false;
  }

  if (fieldsAreIdentical) {
#ifdef CIMGWARPNEARESTNEIGHBOUR_DEBUG
    CDBDebug("fieldsAreIdentical: using _plot");
#endif
    CDFType dataType = dataSource->getFirstAvailableDataObject()->cdfVariable->getType();
    switch (dataType) {
    case CDF_CHAR:
      return _plot<char>(warper, dataSource, drawImage);
      break;
    case CDF_BYTE:
      return _plot<char>(warper, dataSource, drawImage);
      break;
    case CDF_UBYTE:
      return _plot<unsigned char>(warper, dataSource, drawImage);
      break;
    case CDF_SHORT:
      return _plot<short>(warper, dataSource, drawImage);
      break;
    case CDF_USHORT:
      return _plot<ushort>(warper, dataSource, drawImage);
      break;
    case CDF_INT:
      return _plot<int>(warper, dataSource, drawImage);
      break;
    case CDF_UINT:
      return _plot<uint>(warper, dataSource, drawImage);
      break;
    case CDF_FLOAT:
      return _plot<float>(warper, dataSource, drawImage);
      break;
    case CDF_DOUBLE:
      return _plot<double>(warper, dataSource, drawImage);
      break;
    }
    return;
  }

  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  int renderSettings = 0; // auto
  for (auto renderSetting : styleConfiguration->renderSettings) {
    if (!renderSetting->attr.settings.empty()) {
      CT::string renderSettingsAttr = renderSetting->attr.settings;
      if (renderSettingsAttr.equals("fast")) {
        renderSettings = 1; // fast
      }
      if (renderSettingsAttr.equals("precise")) {
        renderSettings = 2; // precise
      }
    }
  }

  bool usePrecise = false;
  if (renderSettings == 0 && dataSource->dWidth * dataSource->dHeight < 700 * 700) {
    usePrecise = true;
  }
  if (dataSource->cfgLayer->TileSettings.size() == 1) {
    usePrecise = false;
  }
  if (renderSettings == 1) {
    usePrecise = false;
  }
  if (renderSettings == 2) {
    usePrecise = true;
  }
  if (usePrecise) {

    GDWDrawFunctionSettings settings = getDrawFunctionSettings(dataSource, drawImage, styleConfiguration);

    CDFType dataType = dataSource->getFirstAvailableDataObject()->cdfVariable->getType();
    void *sourceData = dataSource->getFirstAvailableDataObject()->cdfVariable->data;
    GeoParameters sourceGeo;
    sourceGeo.width = dataSource->dWidth;
    sourceGeo.height = dataSource->dHeight;
    sourceGeo.bbox = dataSource->dfBBOX;
    sourceGeo.cellsizeX = dataSource->dfCellSizeX;
    sourceGeo.cellsizeY = dataSource->dfCellSizeY;
    sourceGeo.crs = dataSource->nativeProj4;

    GenericDataWarper genericDataWarper;
    GDWArgs args = {.warper = warper, .sourceData = sourceData, .sourceGeoParams = sourceGeo, .destGeoParams = drawImage->geoParams};

#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                                       \
  if (dataType == CDFTYPE) genericDataWarper.render<CPPTYPE>(args, [&](int x, int y, CPPTYPE val, GDWState &warperState) { return drawFunction(x, y, val, warperState, settings); });
    ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER

    return;
  }

  // This enables if tiles are divided allong threads.
  int numThreads = 4;
  // Threading is not needed when only one thread is specified.
  bool useThreading = true;
  if (numThreads == 1) useThreading = false;

  warper->findExtent(dataSource, dfMaskBBOX);

  int tile_width = 16;
  int tile_height = 16;
  int x_div = 1;
  int y_div = 1;
  if (warper->isProjectionRequired() == false) {
    // CDBDebug("No reprojection required");
    tile_height = drawImage->geoParams.height;
    tile_width = drawImage->geoParams.width;
    // When we are drawing just one tile, threading is not needed
    useThreading = false;
  } else {
    x_div = int((float(drawImage->geoParams.width) / tile_width)) + 1;
    y_div = int((float(drawImage->geoParams.height) / tile_height)) + 1;
  }
  internalWidth = tile_width * x_div;
  internalHeight = tile_height * y_div;

  // New geo location needs to be extended based on new width and height
  GeoParameters internalGeo = drawImage->geoParams;

  internalGeo.bbox.right = ((drawImage->geoParams.bbox.right - drawImage->geoParams.bbox.left) / double(drawImage->geoParams.width)) * double(internalWidth) + drawImage->geoParams.bbox.left;
  internalGeo.bbox.bottom = ((drawImage->geoParams.bbox.bottom - drawImage->geoParams.bbox.top) / double(drawImage->geoParams.height)) * double(internalHeight) + drawImage->geoParams.bbox.top;

  // Setup the renderer to draw the tiles with.We do not keep the calculated results for CDF_CHAR (faster)
  CAreaMapper *drawTileClass = new CAreaMapper();

  // Reproj back and forth datasource boundingbox
  double y1 = dataSource->dfBBOX[1];
  double y2 = dataSource->dfBBOX[3];
  double x1 = dataSource->dfBBOX[0];
  double x2 = dataSource->dfBBOX[2];

  if (y2 < y1) {
    if (y1 > -360 && y2 < 360 && x1 > -720 && x2 < 720) {
      if (isLonLatProjection(&dataSource->nativeProj4) == false) {
        double checkBBOX[4];
        for (int j = 0; j < 4; j++) checkBBOX[j] = dataSource->dfBBOX[j];

        // CDBDebug("Current BBOX:  %f %f %f %f",dataSource->dfBBOX[0],dataSource->dfBBOX[1],dataSource->dfBBOX[2],dataSource->dfBBOX[3]);
        bool hasError = false;
        if (warper->reprojpoint_inv(checkBBOX[0], checkBBOX[1]) != 0) hasError = true;
        if (warper->reprojpoint(checkBBOX[0], checkBBOX[1]) != 0) hasError = true;

        if (warper->reprojpoint_inv(checkBBOX[2], checkBBOX[3]) != 0) hasError = true;
        if (warper->reprojpoint(checkBBOX[2], checkBBOX[3]) != 0) hasError = true;

        if (hasError == false) {
          for (int j = 0; j < 4; j++) dataSource->dfBBOX[j] = checkBBOX[j];
        }

        // checkBBOX
        // CDBDebug("New BBOX:  %f %f %f %f",dataSource->dfBBOX[0],dataSource->dfBBOX[1],dataSource->dfBBOX[2],dataSource->dfBBOX[3]);
      }
    }
  }

  drawTileClass->init(dataSource, drawImage, tile_width, tile_height);

#ifdef CIMGWARPNEARESTNEIGHBOUR_DEBUG
  CDBDebug("x_div, y_div:  %d %d", x_div, y_div);
  CDBDebug("tile_width, tile_height:  %d %d", tile_width, tile_height);
  CDBDebug("internalWidth, internalHeight:  %d %d", internalWidth, internalHeight);
  CDBDebug("datasource:  %f %f %f %f", dataSource->dfBBOX[0], dataSource->dfBBOX[1], dataSource->dfBBOX[2], dataSource->dfBBOX[3]);
  CDBDebug("destination: %f %f %f %f", internalGeo.dfBBOX[0], internalGeo.dfBBOX[1], internalGeo.dfBBOX[2], internalGeo.dfBBOX[3]);
#endif

  int numberOfTiles = x_div * y_div;
  DrawTileSettings *drawTileSettings = new DrawTileSettings[numberOfTiles];
  DrawTileSettings *curTileSettings;
  for (int x = 0; x < x_div; x++) {
    for (int y = 0; y < y_div; y++) {
      status = reproj(warper, dataSource, internalGeo, x, (y_div - 1) - y, x_div, y_div);
      int tileId = x + y * x_div;
      curTileSettings = &drawTileSettings[tileId];
      curTileSettings->id = tileId;
      if (status != 0) curTileSettings->id = (-tileId) - 1; // This one does not need to be done.
      for (int j = 0; j < 4; j++) {
        curTileSettings->x_corners[j] = x_corners[j];
        curTileSettings->y_corners[j] = y_corners[j];
      }
      // Some safety checks when odd files come out of the projection algorithm
      if ((x_corners[0] >= DBL_MAX || x_corners[0] <= -DBL_MAX) && x_div == 1 && x_div == 1) {
        curTileSettings->x_corners[0] = internalGeo.bbox.right;
        curTileSettings->x_corners[1] = internalGeo.bbox.right;
        curTileSettings->x_corners[2] = internalGeo.bbox.left;
        curTileSettings->x_corners[3] = internalGeo.bbox.left;
        curTileSettings->y_corners[0] = internalGeo.bbox.top;
        curTileSettings->y_corners[1] = internalGeo.bbox.bottom;
        curTileSettings->y_corners[2] = internalGeo.bbox.bottom;
        curTileSettings->y_corners[3] = internalGeo.bbox.top;
      }
      curTileSettings->tile_offset_x = int(x * tile_width);
      curTileSettings->tile_offset_y = int(y * tile_height);
      curTileSettings->drawTile = drawTileClass;
    }
  }

  if (useThreading == false) {
    DrawMultipleTileSettings dmf;
    dmf.ct = drawTileSettings;
    dmf.numberOfTiles = numberOfTiles;
    dmf.startTile = 0;
    dmf.endTile = numberOfTiles;
    drawTiles(&dmf);
  }

  if (useThreading == true) {
    int errcode;
    std::vector<pthread_t> threads(numThreads);
    std::vector<DrawMultipleTileSettings> dmf(numThreads);

    int tileBlockSize = numberOfTiles / numThreads;
    // Divide the tiles over the threads, and start the thread.
    for (int j = 0; j < numThreads; j++) {
      dmf[j].ct = drawTileSettings;
      dmf[j].numberOfTiles = numberOfTiles;
      dmf[j].startTile = tileBlockSize * j;
      ;
      dmf[j].endTile = tileBlockSize * (j + 1);

      // Make sure that all blocks are processed
      if (j == numThreads - 1) dmf[j].endTile = numberOfTiles;

      // CDBDebug("%d - start %d stop %d",j,dmf[j].startTile,dmf[j].endTile);
      DrawMultipleTileSettings *t_dmf = &dmf[j];
      errcode = pthread_create(&threads[j], NULL, drawTiles, t_dmf);
      if (errcode) {
        CDBError("pthread_create");
        return;
      }
    }
    // reap the threads as they exit
    for (int worker = 0; worker < numThreads; worker++) {
      int *status; // TODO fix somehow the return status of the threads.
      // wait for thread to terminate
      errcode = pthread_join(threads[worker], (void **)&status);
      if (errcode) {
        CDBError("pthread_join");
        return;
      }
    }
  }
  delete[] drawTileSettings;
  delete drawTileClass;
}

template <class T> void CImgWarpNearestNeighbour::_plot(CImageWarper *, CDataSource *dataSource, CDrawImage *drawImage) {
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  double dfNodataValue = dataSource->getFirstAvailableDataObject()->dfNodataValue;

  double legendValueRange = styleConfiguration->hasLegendValueRange;
  double legendLowerRange = styleConfiguration->legendLowerRange;
  double legendUpperRange = styleConfiguration->legendUpperRange;
  bool hasNodataValue = dataSource->getFirstAvailableDataObject()->hasNodataValue;
  T nodataValue = (T)dfNodataValue;
  float legendLog = styleConfiguration->legendLog;
  float legendLogAsLog;
  if (legendLog > 0) {
    legendLogAsLog = log10(legendLog);
  } else {
    legendLogAsLog = 0;
  }
  float legendScale = styleConfiguration->legendScale;
  float legendOffset = styleConfiguration->legendOffset;

  T *data = (T *)dataSource->getFirstAvailableDataObject()->cdfVariable->data;

  bool shade = false;
  if (styleConfiguration != NULL) {
    if (styleConfiguration->shadeIntervals.size() > 0) {
      shade = true;
    }
  }

  if (shade) {
    /* TODO: make use of drawfunction as well, less code duplication */
    int numShadeDefs = (int)styleConfiguration->shadeIntervals.size();
    T *shadeDefMin = new T[numShadeDefs];
    T *shadeDefMax = new T[numShadeDefs];
    CColor *fillColors = new CColor[numShadeDefs];
    CColor bgColor;
    bool hasBgColor = false;
    for (int j = 0; j < numShadeDefs; j++) {
      CServerConfig::XMLE_ShadeInterval &shadeInterval = ((styleConfiguration->shadeIntervals)[j]);
      shadeDefMin[j] = (T)shadeInterval.attr.min.toDouble();
      shadeDefMax[j] = (T)shadeInterval.attr.max.toDouble();
      fillColors[j] = CColor(shadeInterval.attr.fillcolor.c_str());
      if (j == 0) {
        if (shadeInterval.attr.bgcolor.empty() == false) {
          hasBgColor = true;
          bgColor = CColor(shadeInterval.attr.bgcolor.c_str());
        }
      }
    }

    for (int y = 0; y < drawImage->geoParams.height; y++) {
      for (int x = 0; x < drawImage->geoParams.width; x++) {
        T val = data[x + y * drawImage->geoParams.width];
        bool drawnPixel = false;
        bool isNodata = false;
        if (hasNodataValue) {
          if (val == nodataValue) isNodata = true;
        }
        if (!(val == val)) isNodata = true;
        if (!isNodata)
          if (legendValueRange)
            if (val < legendLowerRange || val > legendUpperRange) isNodata = true;
        if (!isNodata) {
          for (int snr = numShadeDefs - 1; snr >= 0; snr--) {
            if (val >= shadeDefMin[snr] && val < shadeDefMax[snr]) {
              if (fillColors[snr].a == 0) { // When a fully transparent color is deliberately set, force this color in the image
                drawImage->setPixelTrueColorOverWrite(x, (drawImage->geoParams.height - 1) - y, fillColors[snr].r, fillColors[snr].g, fillColors[snr].b, fillColors[snr].a);
              } else {
                drawImage->setPixelTrueColor(x, (drawImage->geoParams.height - 1) - y, fillColors[snr].r, fillColors[snr].g, fillColors[snr].b, fillColors[snr].a);
              }
              drawnPixel = true;
              break;
            }
          }
        }
        if (hasBgColor && !drawnPixel) {
          drawImage->setPixelTrueColor(x, (drawImage->geoParams.height - 1) - y, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        }
      }
    }
    delete[] shadeDefMin;
    delete[] shadeDefMax;
    delete[] fillColors;
  }

  if (shade == false) {
    for (int y = 0; y < drawImage->geoParams.height; y++) {
      for (int x = 0; x < drawImage->geoParams.width; x++) {
        T val = data[x + y * drawImage->geoParams.width];

        bool isNodata = false;
        if (hasNodataValue) {
          if (val == nodataValue) isNodata = true;
        }
        if (!(val == val)) isNodata = true;
        if (!isNodata)
          if (legendValueRange)
            if (val < legendLowerRange || val > legendUpperRange) isNodata = true;
        if (!isNodata) {
          if (legendLog != 0) {
            if (val > 0) {
              val = (T)(log10(val) / legendLogAsLog);
            } else
              val = (T)(-legendOffset);
          }
          int pcolorind = (int)(val * legendScale + legendOffset);
          // val+=legendOffset;
          if (pcolorind >= 239)
            pcolorind = 239;
          else if (pcolorind <= 0)
            pcolorind = 0;

          drawImage->setPixelIndexed(x, (drawImage->geoParams.height - 1) - y, pcolorind);
        }
      }
    }
  }
}

template <class T> void CImgWarpNearestNeighbour::drawFunction(int x, int y, T val, GDWState &, GDWDrawFunctionSettings &settings) {
  /*
   Please note that this is part of the precise renderer. Changes to this routine should also be implemented in:

   adagucserverEC/CAreaMapper.cpp, myDrawRawTile
 */

  /* Using the precise renderer with shadeinterval */

  /* Using the precise renderer with a legend */
  setPixelInDrawImage(x, y, val, &settings);
}
