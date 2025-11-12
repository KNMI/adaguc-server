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

#ifndef CIMGWARPNEARESTNEIGHBOUR_H
#define CIMGWARPNEARESTNEIGHBOUR_H
// #define CIMGWARPNEARESTNEIGHBOUR_DEBUG
#include <float.h>
#include <pthread.h>
#include "CImageWarperRenderInterface.h"
#include "CGenericDataWarper.h"
#include "CAreaMapper.h"
#include "CDrawFunction.h"

/**
 *  This is the main class of this file. It renders the sourcedata on the destination image using nearest neighbour interpolation.
 *  It uses tile blocks to render the data. Only the corners of these tiles are projected. Not each source pixel is projected,
 *  because this is uneccessary time consuming for a visualization.
 */
class CImgWarpNearestNeighbour : public CImageWarperRenderInterface {
private:
  DEF_ERRORFUNCTION();

  static void drawTriangle(CDrawImage *drawImage, int *xP, int *yP, int &value);
  static void drawTriangleBil(CDrawImage *drawImage, float *destField, int *xP, int *yP, float *values);

  int set(const char *) { return 0; }
  int status;

  int internalWidth;
  int internalHeight;

  // This class represents settings/properties for each separate tile
  class DrawTileSettings {
  public:
    int id;
    double y_corners[4], x_corners[4];
    int tile_offset_x, tile_offset_y;
    CAreaMapper *drawTile;
  };
  // This class represents which bunch of tiles needs to be drawn by which thread.
  class DrawMultipleTileSettings {
  public:
    DrawTileSettings *ct;
    int numberOfTiles, startTile, endTile;
  };
  // This method draws th bunch of selected tiles with the proper renderer.
  // This method can be called by a thread.
  static void *drawTiles(void *arg) {
    DrawMultipleTileSettings *dmf = (DrawMultipleTileSettings *)arg;
    for (int j = dmf->startTile; j < dmf->endTile && j < dmf->numberOfTiles; j++) {
      DrawTileSettings *ct = &dmf->ct[j];
#ifdef CIMGWARPNEARESTNEIGHBOUR_DEBUG
// CDBDebug("Drawing tile %d",j);
#endif
      if (ct->id >= 0) {
#ifdef CIMGWARPNEARESTNEIGHBOUR_DEBUG
// CDBDebug("Drawing tile id %d",ct->id);
#endif
        // int status =
        ct->drawTile->drawTile(ct->x_corners, ct->y_corners, ct->tile_offset_x, ct->tile_offset_y);
        /*if(status!=0){
          CDBError("Unable to draw tile at line %d",status);
        }*/
      }
    }
    return arg;
  }
  // Reproject the corners of the tiles
  double y_corners[4], x_corners[4];
  double dfMaskBBOX[4];
  int reproj(CImageWarper *warper, CDataSource *, CGeoParams *GeoDest, double dfx, double dfy, double x_div, double y_div) {
    double psx[4];
    double psy[4];
    double dfTiledBBOX[4];
    double dfTileW = (GeoDest->dfBBOX[2] - GeoDest->dfBBOX[0]) / double(x_div);
    double dfTileH = (GeoDest->dfBBOX[3] - GeoDest->dfBBOX[1]) / double(y_div);

    dfTiledBBOX[0] = GeoDest->dfBBOX[0] + dfTileW * dfx;
    dfTiledBBOX[1] = GeoDest->dfBBOX[1] + dfTileH * dfy;
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

  template <class T> void _plot(CImageWarper *, CDataSource *dataSource, CDrawImage *drawImage) {
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
      T shadeDefMin[numShadeDefs];
      T shadeDefMax[numShadeDefs];
      CColor fillColors[numShadeDefs];
      CColor bgColor;
      bool hasBgColor = false;
      for (int j = 0; j < numShadeDefs; j++) {
        CServerConfig::XMLE_ShadeInterval *shadeInterval = ((styleConfiguration->shadeIntervals)[j]);
        shadeDefMin[j] = (T)shadeInterval->attr.min.toDouble();
        shadeDefMax[j] = (T)shadeInterval->attr.max.toDouble();
        fillColors[j] = CColor(shadeInterval->attr.fillcolor.c_str());
        if (j == 0) {
          if (shadeInterval->attr.bgcolor.empty() == false) {
            hasBgColor = true;
            bgColor = CColor(shadeInterval->attr.bgcolor.c_str());
          }
        }
      }

      for (int y = 0; y < drawImage->Geo->dHeight; y++) {
        for (int x = 0; x < drawImage->Geo->dWidth; x++) {
          T val = data[x + y * drawImage->Geo->dWidth];
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
                  drawImage->setPixelTrueColorOverWrite(x, (drawImage->Geo->dHeight - 1) - y, fillColors[snr].r, fillColors[snr].g, fillColors[snr].b, fillColors[snr].a);
                } else {
                  drawImage->setPixelTrueColor(x, (drawImage->Geo->dHeight - 1) - y, fillColors[snr].r, fillColors[snr].g, fillColors[snr].b, fillColors[snr].a);
                }
                drawnPixel = true;
                break;
              }
            }
          }
          if (hasBgColor && !drawnPixel) {
            drawImage->setPixelTrueColor(x, (drawImage->Geo->dHeight - 1) - y, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
          }
        }
      }
    }

    if (shade == false) {
      for (int y = 0; y < drawImage->Geo->dHeight; y++) {
        for (int x = 0; x < drawImage->Geo->dWidth; x++) {
          T val = data[x + y * drawImage->Geo->dWidth];

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

            drawImage->setPixelIndexed(x, (drawImage->Geo->dHeight - 1) - y, pcolorind);
          }
        }
      }
    }
  }

  template <class T> static void drawFunction(int x, int y, T val, GDWState &, GDWDrawFunctionSettings &settings) {
    /*
     Please note that this is part of the precise renderer. Changes to this routine should also be implemented in:

     adagucserverEC/CAreaMapper.cpp, myDrawRawTile
   */

    if (settings.drawImage->trueColorAVG_RGBA == false) {
      /* Using the precise renderer with shadeinterval */

      /* Using the precise renderer with a legend */
      setPixelInDrawImage(x, y, val, &settings);
    } else {
      if (x >= 0 && y >= 0 && x < settings.drawImage->Geo->dWidth && y < settings.drawImage->Geo->dHeight) {
        size_t p = x + y * settings.drawImage->Geo->dWidth;
        uint v = val;
        unsigned char a = ((unsigned char)(v >> 24));
        if (a == 255) {
          settings.drawImage->numField[p]++;
          unsigned char r = ((unsigned char)v);
          unsigned char g = ((unsigned char)(v >> 8));
          unsigned char b = ((unsigned char)(v >> 16));
          settings.drawImage->rField[p] += r;
          settings.drawImage->gField[p] += g;
          settings.drawImage->bField[p] += b;
          settings.drawImage->setPixelTrueColorOverWrite(x, y, r, g, b, 255);
        }
      }
    }
  }

  pthread_mutex_t CImgWarpNearestNeighbour_render_lock;

  // Setup projection and all other settings for the tiles to draw
  void render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {
#ifdef CIMGWARPNEARESTNEIGHBOUR_DEBUG
    CDBDebug("Render");
#endif

    bool fieldsAreIdentical = true;
    if ((float)dataSource->dfBBOX[0] != (float)drawImage->Geo->dfBBOX[0]) {
      fieldsAreIdentical = false;
    }
    if ((float)dataSource->dfBBOX[1] != (float)drawImage->Geo->dfBBOX[3]) {
      fieldsAreIdentical = false;
    }
    if ((float)dataSource->dfBBOX[2] != (float)drawImage->Geo->dfBBOX[2]) {
      fieldsAreIdentical = false;
    }
    if ((float)dataSource->dfBBOX[3] != (float)drawImage->Geo->dfBBOX[1]) {
      fieldsAreIdentical = false;
    }
    if ((int)dataSource->dWidth != (int)drawImage->Geo->dWidth) {
      fieldsAreIdentical = false;
    }
    if ((int)dataSource->dHeight != (int)drawImage->Geo->dHeight) {
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
    if (styleConfiguration->styleConfig != NULL && styleConfiguration->styleConfig->RenderSettings.size() == 1) {
      if (!styleConfiguration->styleConfig->RenderSettings[0]->attr.settings.empty()) {
        CT::string renderSettingsAttr = styleConfiguration->styleConfig->RenderSettings[0]->attr.settings;
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
      CGeoParams sourceGeo;

      sourceGeo.dWidth = dataSource->dWidth;
      sourceGeo.dHeight = dataSource->dHeight;
      sourceGeo.dfBBOX[0] = dataSource->dfBBOX[0];
      sourceGeo.dfBBOX[1] = dataSource->dfBBOX[1];
      sourceGeo.dfBBOX[2] = dataSource->dfBBOX[2];
      sourceGeo.dfBBOX[3] = dataSource->dfBBOX[3];
      sourceGeo.dfCellSizeX = dataSource->dfCellSizeX;
      sourceGeo.dfCellSizeY = dataSource->dfCellSizeY;
      sourceGeo.CRS = dataSource->nativeProj4;

      GenericDataWarper genericDataWarper;
      GDWArgs args = {.warper = warper, .sourceData = sourceData, .sourceGeoParams = &sourceGeo, .destGeoParams = drawImage->Geo};

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
      tile_height = drawImage->Geo->dHeight;
      tile_width = drawImage->Geo->dWidth;
      // When we are drawing just one tile, threading is not needed
      useThreading = false;
    } else {
      x_div = int((float(drawImage->Geo->dWidth) / tile_width)) + 1;
      y_div = int((float(drawImage->Geo->dHeight) / tile_height)) + 1;
    }
    internalWidth = tile_width * x_div;
    internalHeight = tile_height * y_div;

    // New geo location needs to be extended based on new width and height
    CGeoParams internalGeo;
    internalGeo.copy(drawImage->Geo);

    internalGeo.dfBBOX[2] = ((drawImage->Geo->dfBBOX[2] - drawImage->Geo->dfBBOX[0]) / double(drawImage->Geo->dWidth)) * double(internalWidth) + drawImage->Geo->dfBBOX[0];
    internalGeo.dfBBOX[1] = ((drawImage->Geo->dfBBOX[1] - drawImage->Geo->dfBBOX[3]) / double(drawImage->Geo->dHeight)) * double(internalHeight) + drawImage->Geo->dfBBOX[3];

    // Setup the renderer to draw the tiles with.We do not keep the calculated results for CDF_CHAR (faster)
    CAreaMapper *drawTileClass = new CAreaMapper();

    // Reproj back and forth datasource boundingbox
    double y1 = dataSource->dfBBOX[1];
    double y2 = dataSource->dfBBOX[3];
    double x1 = dataSource->dfBBOX[0];
    double x2 = dataSource->dfBBOX[2];

    if (y2 < y1) {
      if (y1 > -360 && y2 < 360 && x1 > -720 && x2 < 720) {
        if (CGeoParams::isLonLatProjection(&dataSource->nativeProj4) == false) {
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
        status = reproj(warper, dataSource, &internalGeo, x, (y_div - 1) - y, x_div, y_div);
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
          curTileSettings->x_corners[0] = internalGeo.dfBBOX[2];
          curTileSettings->x_corners[1] = internalGeo.dfBBOX[2];
          curTileSettings->x_corners[2] = internalGeo.dfBBOX[0];
          curTileSettings->x_corners[3] = internalGeo.dfBBOX[0];
          curTileSettings->y_corners[0] = internalGeo.dfBBOX[3];
          curTileSettings->y_corners[1] = internalGeo.dfBBOX[1];
          curTileSettings->y_corners[2] = internalGeo.dfBBOX[1];
          curTileSettings->y_corners[3] = internalGeo.dfBBOX[3];
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
      pthread_t threads[numThreads];
      DrawMultipleTileSettings dmf[numThreads];
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
};
#endif
