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
#include "utils/projectionUtils.h"

/**
 *  This is the main class of this file. It renders the sourcedata on the destination image using nearest neighbour interpolation.
 *  It uses tile blocks to render the data. Only the corners of these tiles are projected. Not each source pixel is projected,
 *  because this is uneccessary time consuming for a visualization.
 */
class CImgWarpNearestNeighbour : public CImageWarperRenderInterface {
private:
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
  int reproj(CImageWarper *warper, CDataSource *, GeoParameters &GeoDest, double dfx, double dfy, double x_div, double y_div);

  template <class T> void _plot(CImageWarper *, CDataSource *dataSource, CDrawImage *drawImage);
  template <class T> static void drawFunction(int x, int y, T val, GDWState &, GDWDrawFunctionSettings &settings);
//  pthread_mutex_t CImgWarpNearestNeighbour_render_lock;  // It's unused. Does it do something by just being here?

  // Setup projection and all other settings for the tiles to draw
  void render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage);
};
#endif
