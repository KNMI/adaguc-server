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

#ifndef CIMGWARPNEARESTRGBA_H
#define CIMGWARPNEARESTRGBA_H
#include <float.h>
#include <pthread.h>
#include "CImageWarperRenderInterface.h"

/**
 *  This tile just runs over the datasource field, and calculates the destination pixel color over and over again when it is requested twice.
 *  This class is very fast for large datasets, with low zoom levels (zoomed out completely)
 */
class CDrawTileObjBGRA {
public:
  double dfTileWidth, dfTileHeight;
  double dfSourceBBOX[4];
  double dfImageBBOX[4];
  double dfNodataValue;
  double legendLowerRange;
  double legendUpperRange;
  double legendValueRange;
  double hasNodataValue;
  int width, height;
  float legendLog, legendScale, legendOffset;
  float legendLogAsLog;
  CDataSource *dataSource;
  CDrawImage *drawImage;
  // size_t prev_imgpointer;
  void init(CDataSource *dataSource, CDrawImage *drawImage, int tileWidth, int tileHeight);
  int drawTile(double *x_corners, double *y_corners, int &dDestX, int &dDestY, bool debug);
  void pixel_blend(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
};

/**
 *  This is the main class of this file. It renders the sourcedata on the destination image using nearest neighbour interpolation.
 *  It uses tile blocks to render the data. Only the corners of these tiles are projected. Not each source pixel is projected,
 *  because this is uneccessary time consuming for a visualization.
 */
class CImgWarpNearestRGBA : public CImageWarperRenderInterface {
private:
  int set(const char *) { return 0; }
  int status;

  // This class represents settings/properties for each separate tile
  class DrawTileSettings {
  public:
    int id;
    double y_corners[4], x_corners[4];
    int tile_offset_x, tile_offset_y;
    CDrawTileObjBGRA *drawTile;
    bool debug;
  };
  // This class represents which bunch of tiles needs to be drawn by which thread.
  class DrawMultipleTileSettings {
  public:
    DrawTileSettings *ct;
    int numberOfTiles, startTile, endTile;
  };
  // This method draws th bunch of selected tiles with the proper renderer.
  // This method can be called by a thread.
  static void *drawTiles(void *arg);

  // Reproject the corners of the tiles
  double y_corners[4], x_corners[4];
  double dfMaskBBOX[4];
  int reproj(CImageWarper *warper, CDataSource *dataSource, GeoParameters &GeoDest, double dfx, double dfy, double x_div, double y_div);

  // Setup projection and all other settings for the tiles to draw
  void render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage);
};
#endif
