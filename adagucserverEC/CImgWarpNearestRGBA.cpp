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

// #define CIMGWARPNEARESTRGBA_DEBUG
#include "CImgWarpNearestRGBA.h"
const char *CImgWarpNearestRGBA::className = "CImgWarpNearestRGBA";
const char *CDrawTileObjBGRA::className = "CDrawTileObjBGRA";
#define CIMGWARPNEARESTRGBA_USEDRAWIMAGE

void CDrawTileObjBGRA::init(CDataSource *dataSource, CDrawImage *drawImage, int tileWidth, int tileHeight) {
  this->dataSource = dataSource;
  this->drawImage = drawImage;
  dfTileWidth = tileWidth;
  dfTileHeight = tileHeight;
  for (int k = 0; k < 4; k++) {
    dfSourceBBOX[k] = dataSource->dfBBOX[k];
    dfImageBBOX[k] = dataSource->dfBBOX[k];
  }

  // Look whether BBOX was swapped in y dir
  if (dataSource->dfBBOX[3] < dataSource->dfBBOX[1]) {
    dfSourceBBOX[1] = dataSource->dfBBOX[3];
    dfSourceBBOX[3] = dataSource->dfBBOX[1];
  }
  // Look whether BBOX was swapped in x dir
  if (dataSource->dfBBOX[2] < dataSource->dfBBOX[0]) {
    dfSourceBBOX[0] = dataSource->dfBBOX[2];
    dfSourceBBOX[2] = dataSource->dfBBOX[0];
  }

  dfNodataValue = dataSource->getDataObject(0)->dfNodataValue;
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  legendValueRange = styleConfiguration->hasLegendValueRange;
  legendLowerRange = styleConfiguration->legendLowerRange;
  legendUpperRange = styleConfiguration->legendUpperRange;
  hasNodataValue = dataSource->getDataObject(0)->hasNodataValue;
  width = dataSource->dWidth;
  height = dataSource->dHeight;
  legendLog = styleConfiguration->legendLog;
  if (legendLog > 0) {
    legendLogAsLog = log10(legendLog);
  } else {
    legendLogAsLog = 0;
  }
  legendScale = styleConfiguration->legendScale;
  legendOffset = styleConfiguration->legendOffset;
}

int CDrawTileObjBGRA::drawTile(double *x_corners, double *y_corners, int &dDestX, int &dDestY, bool debug) {
  uint *data = (uint *)dataSource->getDataObject(0)->cdfVariable->data;
#ifndef CIMGWARPNEARESTRGBA_USEDRAWIMAGE
  uint *imageData = (uint *)drawImage->getCanvasMemory();
#endif
  int imageWidth = drawImage->geoParams.width;
  int imageHeight = drawImage->geoParams.height;
#ifdef CIMGWARPNEARESTRGBA_DEBUG
  CDBDebug("myDrawRawTile %f, %f, %f, %f, %f, %f %f %f", dfSourceBBOX[0], dfSourceBBOX[1], dfSourceBBOX[2], dfSourceBBOX[3], width, height, dfTileWidth, dfTileHeight);
#endif
  double sample_sy, sample_sx;
  double line_dx1, line_dy1, line_dx2, line_dy2;
  double rcx_1, rcy_1, rcx_2, rcy_2, rcx_3, rcy_3;
  int x, y;
  int srcpixel_x, srcpixel_y;
  int dstpixel_x, dstpixel_y;
  int k;
  rcx_1 = (x_corners[0] - x_corners[3]) / dfTileWidth;
  rcy_1 = (y_corners[0] - y_corners[3]) / dfTileWidth;
  rcx_2 = (x_corners[1] - x_corners[2]) / dfTileWidth;
  rcy_2 = (y_corners[1] - y_corners[2]) / dfTileWidth;
  for (k = 0; k < 4; k++) {
    if (fabs(x_corners[k] - x_corners[0]) >= fabs(dfSourceBBOX[2] - dfSourceBBOX[0])) break;
  }
  if (k == 4) {
    for (k = 0; k < 4; k++) {
      if (x_corners[k] > dfSourceBBOX[0] && x_corners[k] < dfSourceBBOX[2]) break;
    }
    if (k == 4) return __LINE__;
  }
  for (k = 0; k < 4; k++) {
    if (fabs(y_corners[k] - y_corners[0]) >= fabs(dfSourceBBOX[3] - dfSourceBBOX[1])) break;
  }
  if (k == 4) {
    for (k = 0; k < 4; k++) {
      if (y_corners[k] > dfSourceBBOX[1] && y_corners[k] < dfSourceBBOX[3]) break;
    }
    if (k == 4) return __LINE__;
  }

  /*
   * [D: CImgWarpNearestNeighbour.h, 490 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z dfTileWidth, dfTileHeight:  1 1
   * [D: CImgWarpNearestNeighbour.h, 491 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z datasource:  6.668803 54.172408 100.846018 -15.367730
   * [D: CImgWarpNearestNeighbour.h, 492 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z destination: 6.668803 -15.367730 100.846018 54.172408
   * [D: CImgWarpNearestNeighbour.h, 371 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z Drawing tile 0
   * [D: CImgWarpNearestNeighbour.h, 375 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z Drawing tile id 0
   * [D: CImgWarpNearestNeighbour.h, 257 in CDrawTileObjBGRA]               2012-07-25T09:52:02Z myDrawRawTile 6.668803, -15.367729, 100.846016, 54.172409
   *
   *
   * [D: CImgWarpNearestNeighbour.h, 490 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z dfTileWidth, dfTileHeight:  1 1
   * [D: CImgWarpNearestNeighbour.h, 491 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z datasource:  14.914277 46.821022 109.091492 -22.719116
   * [D: CImgWarpNearestNeighbour.h, 492 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z destination: 14.914277 -22.719116 109.091492 46.821022
   * [D: CImgWarpNearestNeighbour.h, 371 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z Drawing tile 0
   * [D: CImgWarpNearestNeighbour.h, 375 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z Drawing tile id 0
   * [D: CImgWarpNearestNeighbour.h, 257 in CDrawTileObjBGRA]               2012-07-25T09:53:06Z myDrawRawTile 14.914276, -22.719116, 109.091492, 46.821022
   *
   */
#ifdef CIMGWARPNEARESTRGBA_DEBUG
  CDBDebug("myDrawRawTile %f, %f, %f, %f, %f, %f %f %f", dfSourceBBOX[0], dfSourceBBOX[1], dfSourceBBOX[2], dfSourceBBOX[3], width, height, dfTileWidth, dfTileHeight);
#endif
  line_dx1 = x_corners[3];
  line_dx2 = x_corners[2];
  line_dy1 = y_corners[3];
  line_dy2 = y_corners[2];

  size_t imgpointer;
  for (x = 0; x <= dfTileWidth - 1; x++) {
    line_dx1 += rcx_1;
    line_dx2 += rcx_2;
    line_dy1 += rcy_1;
    line_dy2 += rcy_2;
    rcx_3 = (line_dx2 - line_dx1) / dfTileHeight;
    rcy_3 = (line_dy2 - line_dy1) / dfTileHeight;
    dstpixel_x = int(x) + dDestX;
    for (y = 0; y <= dfTileHeight - 1; y = y + 1) {
      dstpixel_y = y + dDestY;
      sample_sx = line_dx1 + rcx_3 * double(y);
      if (sample_sx >= dfSourceBBOX[0] && sample_sx < dfSourceBBOX[2]) {
        sample_sy = line_dy1 + rcy_3 * y;
        if (sample_sy >= dfSourceBBOX[1] && sample_sy < dfSourceBBOX[3]) {
          srcpixel_x = int(((sample_sx - dfImageBBOX[0]) / (dfImageBBOX[2] - dfImageBBOX[0])) * (width));
          if (srcpixel_x >= 0 && srcpixel_x < width) {
            srcpixel_y = int(((sample_sy - dfImageBBOX[1]) / (dfImageBBOX[3] - dfImageBBOX[1])) * height);
            if (srcpixel_y >= 0 && srcpixel_y < height) {
              if (dstpixel_x >= 0 && dstpixel_y >= 0 && dstpixel_x < imageWidth && dstpixel_y < imageHeight) {
                imgpointer = srcpixel_x + (height - 1 - srcpixel_y) * width;
                uint v = data[imgpointer];
                // if(v!=2147483649){;//uint(-2147483647)){
                //                   if(v!=4294967295)
                {
                  // v=v*10;
                  unsigned char r = ((unsigned char)v);
                  unsigned char g = ((unsigned char)(v >> 8));
                  unsigned char b = ((unsigned char)(v >> 16));
                  unsigned char a = ((unsigned char)(v >> 24));
                  ;

                  if (debug) {
                    if (srcpixel_x == 0 || srcpixel_x == width - 1 || srcpixel_y == 0 || srcpixel_y == height - 1) {
                      r = 0;
                      g = 0;
                      b = 255;
                      a = 255;
                    }
                    if ((srcpixel_x == 10 || srcpixel_x == width - 10) && srcpixel_y > 10 && srcpixel_y < height - 10) {
                      r = 255;
                      g = 255;
                      b = 0;
                      a = 255;
                    }
                    if ((srcpixel_y == 10 || srcpixel_y == width - 10) && srcpixel_x > 10 && srcpixel_x < width - 10) {
                      r = 255;
                      g = 255;
                      b = 0;
                      a = 255;
                    }
                  }

#ifdef CIMGWARPNEARESTRGBA_USEDRAWIMAGE
                  drawImage->setPixelTrueColor(dstpixel_x, dstpixel_y, r, g, b, a);
#else
                  if (a != 255) {
                    // For cairo, Alpha is precomputed into components. We need to do this here as well.
                    unsigned char r1 = float(r) * (float(a) / 256.);
                    unsigned char g1 = float(g) * (float(a) / 256.);
                    unsigned char b1 = float(b) * (float(a) / 256.);

                    imageData[dstpixel_x + dstpixel_y * imageWidth] = b1 + g1 * 256 + r1 * 256 * 256 + a * 256 * 256 * 256;
                  } else {
                    imageData[dstpixel_x + dstpixel_y * imageWidth] = b + g * 256 + r * 256 * 256 + 4278190080;
                  }
#endif
                }
              }
            }
          }
        }
      }
    }
  }
  return 0;
}

void *CImgWarpNearestRGBA::drawTiles(void *arg) {
  DrawMultipleTileSettings *dmf = (DrawMultipleTileSettings *)arg;
  for (int j = dmf->startTile; j < dmf->endTile && j < dmf->numberOfTiles; j++) {
    DrawTileSettings *ct = &dmf->ct[j];
#ifdef CIMGWARPNEARESTRGBA_DEBUG
    CDBDebug("Drawing tile %d", j);
#endif
    if (ct->id >= 0) {
#ifdef CIMGWARPNEARESTRGBA_DEBUG
      CDBDebug("Drawing tile id %d", ct->id);
#endif
      // int status =
      ct->drawTile->drawTile(ct->x_corners, ct->y_corners, ct->tile_offset_x, ct->tile_offset_y, ct->debug);
      /*if(status!=0){
        CDBError("Unable to draw tile at line %d",status);
      }*/
    }
  }
  return arg;
}

int CImgWarpNearestRGBA::reproj(CImageWarper *warper, CDataSource *, GeoParameters &GeoDest, double dfx, double dfy, double x_div, double y_div) {
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
      (dfTiledBBOX[1] > dfSourceBBOX[1] - dfTileH && dfTiledBBOX[1] < dfSourceBBOX[3] + dfTileH) && (dfTiledBBOX[3] > dfSourceBBOX[1] - dfTileH && dfTiledBBOX[3] < dfSourceBBOX[3] + dfTileH))
    ;
  else
    return 1;
  psx[0] = dfTiledBBOX[2];
  psx[1] = dfTiledBBOX[2];
  psx[2] = dfTiledBBOX[0];
  psx[3] = dfTiledBBOX[0];
  psy[0] = dfTiledBBOX[1];
  psy[1] = dfTiledBBOX[3];
  psy[2] = dfTiledBBOX[3];
  psy[3] = dfTiledBBOX[1];
  if (warper->isProjectionRequired()) {
    //     CT::string destinationCRS;
    //     warper->decodeCRS(&destinationCRS,&GeoDest.CRS);
    if (proj_trans_generic(warper->projSourceToDest, PJ_INV, psx, sizeof(double), 4, psy, sizeof(double), 4, nullptr, 0, 0, nullptr, 0, 0) != 4) {
      // TODO: No error handling in original code
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

void CImgWarpNearestRGBA::render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {
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
  int internalWidth = tile_width * x_div;
  int internalHeight = tile_height * y_div;

  // New geo location needs to be extended based on new width and height
  GeoParameters internalGeo = drawImage->geoParams;

  internalGeo.bbox.right = ((drawImage->geoParams.bbox.right - drawImage->geoParams.bbox.left) / double(drawImage->geoParams.width)) * double(internalWidth) + drawImage->geoParams.bbox.left;
  internalGeo.bbox.bottom = ((drawImage->geoParams.bbox.bottom - drawImage->geoParams.bbox.top) / double(drawImage->geoParams.height)) * double(internalHeight) + drawImage->geoParams.bbox.top;

  // Setup the renderer to draw the tiles with.We do not keep the calculated results for CDF_CHAR (faster)
  CDrawTileObjBGRA *drawTileClass = NULL;
  if (dataSource->getDataObject(0)->cdfVariable->getType() == CDF_CHAR || dataSource->getDataObject(0)->cdfVariable->getType() == CDF_BYTE ||
      dataSource->getDataObject(0)->cdfVariable->getType() == CDF_UBYTE) {
    drawTileClass = new CDrawTileObjBGRA(); // Do not keep the calculated results for CDF_CHAR

  } else {
    // drawTileClass = new CDrawTileObjBGRAByteCache();  //keep the calculated results
    drawTileClass = new CDrawTileObjBGRA(); // Do not keep the calculated results for CDF_CHAR
  }
  // drawTileClass = new CDrawTileObjBGRAByteCache();           //Do not keep the calculated results for CDF_CHAR
  // drawTileClass = new CDrawTileObjBGRA();  //keep the calculated results
  drawTileClass->init(dataSource, drawImage, (int)tile_width, (int)tile_height);

#ifdef CIMGWARPNEARESTRGBA_DEBUG
  CDBDebug("x_div, y_div:  %d %d", x_div, y_div);
  CDBDebug("datasource:  %f %f %f %f", dataSource->bbox.left, dataSource->bbox.bottom, dataSource->bbox.right, dataSource->bbox.top);
  CDBDebug("destination: %f %f %f %f", internalGeo.bbox.left, internalGeo.bbox.bottom, internalGeo.bbox.right, internalGeo.bbox.top);
#endif
  int numberOfTiles = x_div * y_div;
  DrawTileSettings *drawTileSettings = new DrawTileSettings[numberOfTiles];
  bool debug = false;

  if (dataSource->cfgLayer->TileSettings.size() == 1) {
    if (dataSource->cfgLayer->TileSettings[0]->attr.debug.equals("true")) {
      debug = true;
    }
  }
  DrawTileSettings *curTileSettings;
  for (int x = 0; x < x_div; x++) {
    for (int y = 0; y < y_div; y++) {
      status = reproj(warper, dataSource, internalGeo, x, (y_div - 1) - y, x_div, y_div);
      int tileId = x + y * x_div;
      curTileSettings = &drawTileSettings[tileId];
      curTileSettings->debug = debug;
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
