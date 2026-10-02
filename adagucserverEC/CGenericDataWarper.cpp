/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl, GST - GeoSpatialTeam KNMI
 * Date:     2026-09-10
 *
 ******************************************************************************
 *
 * Copyright 2026, Royal Netherlands Meteorological Institute (KNMI)
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
#include "Types/GeoParameters.h"
#include "CDebugger.h"
#include "GenericDataWarper/gdwFindPixelExtent.h"
#include "utils/projectionUtils.h"
#include "CCDFObject.h"
#include "CDataSource.h"
#include "CImageWarper.h"
#include "CServerConfig_CPPXSD.h"
#include "Types/CPointTypes.h"
#include <CStopWatch.h>

void ProjectionGrid::initSize(size_t dataSize) {
  StopWatch_Measure("[ProjectionGrid::initSize] dataSize %zu", dataSize);
  px = new double[dataSize];
  py = new double[dataSize];
  skip = new bool[dataSize];
  StopWatch_Measure("[/ProjectionGrid::initSize]");
}
ProjectionGrid::~ProjectionGrid() {
  StopWatch_Measure("[~ProjectionGrid]");
  delete[] px;
  delete[] py;
  delete[] skip;
  StopWatch_Measure("[/~ProjectionGrid]");
}

GenericDataWarper::~GenericDataWarper() {
  StopWatch_Measure("[~GenericDataWarper]");
  delete projectionGrid;
  projectionGrid = nullptr;
  StopWatch_Measure("[/~GenericDataWarper]");
}

// Reproj back and forth boundingbox in GeoParameters to make valid proj coordinates which always have the same range.
f8box reprojBBox(GeoParameters &input, CImageWarper *warper) {
  StopWatch_Measure("[reprojBBox]");
  f8box output = input.bbox;
  if (input.bbox.top < input.bbox.bottom) {
    if (input.bbox.bottom > -360 && input.bbox.top < 360 && input.bbox.left > -720 && input.bbox.right < 720) {
      if (isLonLatProjection(input.crs) == false) {
        double checkBBOX[4];
        input.bbox.toArray(checkBBOX);
        bool hasError = false;
        if (warper->reprojpoint_inv(checkBBOX[0], checkBBOX[1]) != 0) hasError = true;
        if (warper->reprojpoint(checkBBOX[0], checkBBOX[1]) != 0) hasError = true;
        if (warper->reprojpoint_inv(checkBBOX[2], checkBBOX[3]) != 0) hasError = true;
        if (warper->reprojpoint(checkBBOX[2], checkBBOX[3]) != 0) hasError = true;
        if (hasError == false) {
          output = checkBBOX;
        }
      }
    }
  }
  StopWatch_Measure("[/reprojBBox]");
  return output;
}

// Make a projection grid. The coordinates are all converted at once from source to destination
ProjectionGrid *makeProjection(double halfCell, CImageWarper *warper, i4box &pixelExtentBox, GeoParameters &sourceGeoParams, GeoParameters &, GDWState &warperState) {
  int dataWidth = pixelExtentBox.span().x;
  int dataHeight = pixelExtentBox.span().y;
  size_t dataSize = (dataWidth + 1) * (dataHeight + 1);
  StopWatch_Measure("[makeProjection] %dx%d", dataWidth, dataHeight);
  auto *projGrid = new ProjectionGrid();
  projGrid->initSize(dataSize);

  double dfSourcedExtW = sourceGeoParams.bbox.span().x / double(warperState.sourceGridWidth);
  double dfSourcedExtH = sourceGeoParams.bbox.span().y / double(warperState.sourceGridHeight);
  StopWatch_Measure("makeProjection: start filling source coordinates");
  for (int y = 0; y < dataHeight + 1; y++) {
    for (int x = 0; x < dataWidth + 1; x++) {
      size_t p = x + y * (dataWidth + 1);
      double valX = dfSourcedExtW * (x + halfCell + pixelExtentBox.left) + sourceGeoParams.bbox.left;
      double valY = dfSourcedExtH * (y - halfCell + pixelExtentBox.bottom) + sourceGeoParams.bbox.bottom;
      projGrid->px[p] = valX;
      projGrid->py[p] = valY;
      projGrid->skip[p] = false;
    }
  }
  StopWatch_Measure("makeProjection: done filling source coordinates");
  if (warper->isProjectionRequired()) {
    StopWatch_Measure("makeProjection: start proj_trans_generic for %zu points", dataSize);
    if (proj_trans_generic(warper->projSourceToDest, PJ_FWD, projGrid->px, sizeof(double), dataSize, projGrid->py, sizeof(double), dataSize, nullptr, 0, 0, nullptr, 0, 0) != dataSize) {
      CDBDebug("Unable to do pj_transform");
    }
    StopWatch_Measure("makeProjection: done proj_trans_generic");
  }
  StopWatch_Measure("[/makeProjection]");
  return projGrid;
}

ProjectionGrid *makeStridedProjection(double halfCell, CImageWarper *warper, i4box &pixelExtentBox, GeoParameters &sourceGeoParams, GeoParameters &, GDWState &warperState) {
  int projStrideFactor = 8;
  int dataWidth = pixelExtentBox.span().x;
  int dataHeight = pixelExtentBox.span().y;
  StopWatch_Measure("[makeStridedProjection] %dx%d", dataWidth, dataHeight);
  auto *projGrid = new ProjectionGrid();
  projGrid->initSize((dataWidth + 1) * (dataHeight + 1));
  double dfSourcedExtW = sourceGeoParams.bbox.span().x / double(warperState.sourceGridWidth);
  double dfSourcedExtH = sourceGeoParams.bbox.span().y / double(warperState.sourceGridHeight);
  size_t dataWidthStrided = ceil(double(dataWidth) / projStrideFactor);
  size_t dataHeightStrided = ceil(double(dataHeight) / projStrideFactor);
  size_t dataSizeStrided = (dataWidthStrided + 2) * (dataHeightStrided + 2);

  double *pxStrided = new double[dataSizeStrided];
  double *pyStrided = new double[dataSizeStrided];

  /* TODO faster init */
  StopWatch_Measure("makeStridedProjection: start init grid");
  for (int y = 0; y < dataHeight + 1; y++) {
    for (int x = 0; x < dataWidth + 1; x++) {
      size_t p = x + y * (dataWidth + 1);
      projGrid->px[p] = NAN;
      projGrid->py[p] = NAN;
      projGrid->skip[p] = true;
    }
  }
  StopWatch_Measure("makeStridedProjection: done init grid, start filling strided source coordinates");
  for (size_t y = 0; y < dataHeightStrided; y++) {
    for (size_t x = 0; x < dataWidthStrided; x++) {
      size_t pS = x + y * dataWidthStrided;

      double valX = dfSourcedExtW * (x * projStrideFactor + halfCell + pixelExtentBox.left) + sourceGeoParams.bbox.left;
      double valY = dfSourcedExtH * (y * projStrideFactor - halfCell + pixelExtentBox.bottom) + sourceGeoParams.bbox.bottom;
      pxStrided[pS] = valX;
      pyStrided[pS] = valY;
    }
  }
  StopWatch_Measure("makeStridedProjection: done filling strided source coordinates");

  StopWatch_Measure("makeStridedProjection: start proj_trans_generic for %zu points", dataSizeStrided);
  if (proj_trans_generic(warper->projSourceToDest, PJ_FWD, pxStrided, sizeof(double), dataSizeStrided, pyStrided, sizeof(double), dataSizeStrided, nullptr, 0, 0, nullptr, 0, 0) != dataSizeStrided) {
    CDBDebug("Unable to do pj_transform");
  }
  StopWatch_Measure("makeStridedProjection: done proj_trans_generic, start interpolating grid");

  for (int y = 0; y < dataHeight + 1; y++) {
    for (int x = 0; x < dataWidth + 1; x++) {
      size_t p = x + y * (dataWidth + 1);
      size_t pS = (x / projStrideFactor) + (y / projStrideFactor) * (dataWidthStrided);
      size_t p0 = pS;
      size_t p1 = pS + 1;
      size_t p2 = pS + dataWidthStrided;
      size_t p3 = pS + 1 + dataWidthStrided;

      double sX = double(x % projStrideFactor) / double(projStrideFactor);
      double sY = double(y % projStrideFactor) / double(projStrideFactor);
      double x1 = pxStrided[p0] * (1 - sX) + pxStrided[p1] * sX;
      double x2 = pxStrided[p2] * (1 - sX) + pxStrided[p3] * sX;
      projGrid->px[p] = x1 * (1 - sY) + x2 * sY;
      double y1 = pyStrided[p0] * (1 - sY) + pyStrided[p2] * sY;
      double y2 = pyStrided[p1] * (1 - sY) + pyStrided[p3] * sY;
      projGrid->py[p] = y1 * (1 - sX) + y2 * sX;
      projGrid->skip[p] = false;
      if (x < projStrideFactor || y < projStrideFactor || x >= dataWidth - projStrideFactor || y >= dataHeight - projStrideFactor) {
        projGrid->px[p] = dfSourcedExtW * (x + halfCell + pixelExtentBox.left) + sourceGeoParams.bbox.left;
        projGrid->py[p] = dfSourcedExtH * (y - halfCell + pixelExtentBox.bottom) + sourceGeoParams.bbox.bottom;
        projGrid->skip[p] = false;
        if (proj_trans_generic(warper->projSourceToDest, PJ_FWD, &projGrid->px[p], sizeof(double), 1, &projGrid->py[p], sizeof(double), 1, nullptr, 0, 0, nullptr, 0, 0) != 1) {
          projGrid->skip[p] = true;
        }
      }
    }
  }
  StopWatch_Measure("makeStridedProjection: done interpolating grid");
  delete[] pyStrided;
  delete[] pxStrided;
  StopWatch_Measure("[/makeStridedProjection]");
  return projGrid;
}
