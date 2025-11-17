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
#include "utils/projectionUtils.h"

// Reproj back and forth boundingbox in GeoParameters to make valid proj coordinates which always have the same range.
f8box reprojBBox(GeoParameters &input, CImageWarper *warper) {
  f8box output = input.bbox;
  if (input.bbox.top < input.bbox.bottom) {
    if (input.bbox.bottom > -360 && input.bbox.top < 360 && input.bbox.left > -720 && input.bbox.right < 720) {
      if (isLonLatProjection(&input.crs) == false) {
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
  return output;
}

// Make a projection grid. The coordinates are all converted at once from source to destination
ProjectionGrid *makeProjection(double halfCell, CImageWarper *warper, i4box &pixelExtentBox, GeoParameters &sourceGeoParams, GeoParameters &, GDWState &warperState) {
  int dataWidth = pixelExtentBox.span().x;
  int dataHeight = pixelExtentBox.span().y;
  size_t dataSize = (dataWidth + 1) * (dataHeight + 1);
  ProjectionGrid *projGrid = new ProjectionGrid();
  projGrid->initSize(dataSize);

  double dfSourcedExtW = sourceGeoParams.bbox.span().x / double(warperState.sourceGridWidth);
  double dfSourcedExtH = sourceGeoParams.bbox.span().y / double(warperState.sourceGridHeight);
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
  if (warper->isProjectionRequired()) {
    if (proj_trans_generic(warper->projSourceToDest, PJ_FWD, projGrid->px, sizeof(double), dataSize, projGrid->py, sizeof(double), dataSize, nullptr, 0, 0, nullptr, 0, 0) != dataSize) {
      CDBDebug("Unable to do pj_transform");
    }
  }
  return projGrid;
}

ProjectionGrid *makeStridedProjection(double halfCell, CImageWarper *warper, i4box &pixelExtentBox, GeoParameters &sourceGeoParams, GeoParameters &, GDWState &warperState) {
  int projStrideFactor = 20;
  int dataWidth = pixelExtentBox.span().x;
  int dataHeight = pixelExtentBox.span().y;
  ProjectionGrid *projGrid = new ProjectionGrid();
  projGrid->initSize((dataWidth + 1) * (dataHeight + 1));
  double dfSourcedExtW = sourceGeoParams.bbox.span().x / double(warperState.sourceGridWidth);
  double dfSourcedExtH = sourceGeoParams.bbox.span().y / double(warperState.sourceGridHeight);
  size_t dataWidthStrided = ceil(double(dataWidth) / projStrideFactor);
  size_t dataHeightStrided = ceil(double(dataHeight) / projStrideFactor);
  size_t dataSizeStrided = (dataWidthStrided + 1) * (dataHeightStrided + 1);

  double *pxStrided = new double[dataSizeStrided];
  double *pyStrided = new double[dataSizeStrided];

  /* TODO faster init */
  for (int y = 0; y < dataHeight + 1; y++) {
    for (int x = 0; x < dataWidth + 1; x++) {
      size_t p = x + y * (dataWidth + 1);
      projGrid->px[p] = NAN;
      projGrid->py[p] = NAN;
      projGrid->skip[p] = true;
    }
  }
  for (size_t y = 0; y < dataHeightStrided; y++) {
    for (size_t x = 0; x < dataWidthStrided; x++) {
      size_t pS = x + y * dataWidthStrided;

      double valX = dfSourcedExtW * (x * projStrideFactor + halfCell + pixelExtentBox.left) + sourceGeoParams.bbox.left;
      double valY = dfSourcedExtH * (y * projStrideFactor - halfCell + pixelExtentBox.bottom) + sourceGeoParams.bbox.bottom;
      pxStrided[pS] = valX;
      pyStrided[pS] = valY;
    }
  }

  if (proj_trans_generic(warper->projSourceToDest, PJ_FWD, pxStrided, sizeof(double), dataSizeStrided, pyStrided, sizeof(double), dataSizeStrided, nullptr, 0, 0, nullptr, 0, 0) != dataSizeStrided) {
    CDBDebug("Unable to do pj_transform");
  }

  // CDBDebug("destGeoParams.bbox.bottom %f %f", destGeoParams.bbox.bottom, destGeoParams.bbox.top);
  for (int y = 0; y < dataHeight + 1; y++) {
    for (int x = 0; x < dataWidth + 1; x++) {
      size_t p = x + y * (dataWidth + 1);
      size_t pS = (x / projStrideFactor) + (y / projStrideFactor) * (dataWidthStrided);
      if (pS >= dataSizeStrided) continue;
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
  delete[] pyStrided;
  delete[] pxStrided;
  return projGrid;
}

// Transform the grid linearly. This is used in case projection are the same and is much efficienter then warping the grid.
template <typename T>
void linearTransformGrid(GDWState &warperState, bool useHalfCellOffset, CImageWarper *, void *, GeoParameters &sourceGeoParams, GeoParameters &destGeoParams,
                         const std::function<void(int, int, T, GDWState &warperState)> &drawFunction) {
  CDBDebug("linearTransformGrid");
  double halfCell = useHalfCellOffset ? 0.5 : 0;
  double dfSourceExtW = sourceGeoParams.bbox.span().x;
  double dfSourceExtH = sourceGeoParams.bbox.span().y;
  double dfSourceW = warperState.sourceGridWidth;
  double dfSourceH = warperState.sourceGridHeight;
  double dfDestW = warperState.destGridWidth;
  double dfDestH = warperState.destGridHeight;
  double dfSourceOrigX = sourceGeoParams.bbox.left;
  double dfSourceOrigY = sourceGeoParams.bbox.bottom;
  double dfDestExtW = destGeoParams.bbox.span().x;
  double dfDestExtH = -destGeoParams.bbox.span().y;
  double dfDestOrigX = destGeoParams.bbox.left;
  double dfDestOrigY = destGeoParams.bbox.top;
  int PXExtentBasedOnSource[4] = {0, 0, warperState.sourceGridWidth, warperState.sourceGridHeight};

  if (PXExtentBasedOnSource[2] - PXExtentBasedOnSource[0] <= 0) return;
  if (PXExtentBasedOnSource[3] - PXExtentBasedOnSource[1] <= 0) return;

  // Obtain pixelextent to avoid looping over all source grid cells which will never be used in the destination grid
  i4box pixelspan;
  pixelspan = PXExtentBasedOnSource;
  auto source = sourceGeoParams.bbox;
  auto dest = destGeoParams.bbox;

  f8point span = source.span();
  i4point wh = {.x = sourceGeoParams.width, .y = sourceGeoParams.height};
  f8box newbox = {
      .left = (dest.left - source.left) / span.x, .bottom = (dest.bottom - source.bottom) / span.y, .right = (dest.right - source.left) / span.x, .top = (dest.top - source.bottom) / span.y};
  i4box newpixelspan = {.left = (int)floor(newbox.left * wh.x), .bottom = (int)floor(newbox.bottom * wh.y), .right = (int)ceil(newbox.right * wh.x), .top = (int)ceil(newbox.top * wh.y)};
  newpixelspan.sort();
  newpixelspan = {
      .left = newpixelspan.left - 1,
      .bottom = newpixelspan.bottom - 1,
      .right = newpixelspan.right + 1,
      .top = newpixelspan.top + 1,

  };
  newpixelspan.clip({.left = 0, .bottom = 0, .right = wh.x, .top = wh.y});

  pixelspan = newpixelspan;

  for (int y = pixelspan.bottom; y < pixelspan.top; y++) {
    for (int x = pixelspan.left; x < pixelspan.right; x++) {
      double dfx = x + halfCell;
      double dfy = y - halfCell; // Y is inverted
      int sx1 = roundedLinearTransform(dfx, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW);
      int sx2 = roundedLinearTransform(dfx + 1, dfSourceW, dfSourceExtW, dfSourceOrigX, dfDestOrigX, dfDestExtW, dfDestW);
      int sy1 = roundedLinearTransform(dfy, dfSourceH, dfSourceExtH, dfSourceOrigY, dfDestOrigY, dfDestExtH, dfDestH);
      int sy2 = roundedLinearTransform(dfy + 1, dfSourceH, dfSourceExtH, dfSourceOrigY, dfDestOrigY, dfDestExtH, dfDestH);
      int sxw = floor(fabs(sx2 - sx1)) + 1;
      int syh = floor(fabs(sy2 - sy1)) + 1;
      if ((sx1 < -sxw && sx2 < -sxw) || (sy1 < -syh && sy2 < -syh) || (sx1 >= destGeoParams.width + sxw && sx2 >= destGeoParams.width + sxw) ||
          (sy1 >= destGeoParams.height + syh && sy2 >= destGeoParams.height + syh)) {
        continue;
      }

      warperState.sourceIndexX = x;
      warperState.sourceIndexY = sourceGeoParams.height - 1 - y;
      if (sx1 > sx2) {
        std::swap(sx1, sx2);
      }
      if (sy1 > sy2) {
        std::swap(sy1, sy2);
      }
      if (sy2 == sy1) sy2++;
      if (sx2 == sx1) sx2++;
      double h = double(sy2 - sy1);
      double w = double(sx2 - sx1);
      T value = ((T *)warperState.sourceGrid)[warperState.sourceIndexX + (warperState.sourceIndexY) * sourceGeoParams.width];

      for (int sjy = sy1; sjy < sy2; sjy++) {
        for (int sjx = sx1; sjx < sx2; sjx++) {
          warperState.sourceTileDy = (sjy - sy1) / h; // TODO: Check why sourceTileDy is upside down.
          warperState.sourceTileDx = (sjx - sx1) / w;
          warperState.destIndexX = sjx;
          warperState.destIndexY = sjy;
          drawFunction(sjx, sjy, value, warperState);
        }
      }
    }
  }
}

// Warp the grid from the source projection to the destination projection.
template <typename T>
void warpTransformGrid(GDWState &warperState, ProjectionGrid *projectionGrid, bool useHalfCellOffset, CImageWarper *warper, void *, GeoParameters &sourceGeoParams, GeoParameters &destGeoParams,
                       const std::function<void(int, int, T, GDWState &warperState)> &drawFunction) {
  CDBDebug("warpTransformGrid");
  bool debug = false;
  double halfCell = useHalfCellOffset ? 0.5 : 0;

  double dfDestW = warperState.destGridWidth;
  double dfDestH = warperState.destGridHeight;
  double dfDestExtW = destGeoParams.bbox.span().x;
  double dfDestExtH = -destGeoParams.bbox.span().y;
  double multiDestX = dfDestW / dfDestExtW;
  double multiDestY = dfDestH / dfDestExtH;
  double dfDestOrigX = destGeoParams.bbox.left;
  double dfDestOrigY = destGeoParams.bbox.top;
  i4box pixelExtentBox = {0, 0, warperState.sourceGridWidth, warperState.sourceGridHeight};

  if (pixelExtentBox.span().x <= 0 || pixelExtentBox.span().y <= 0) return;

  int dataWidth = pixelExtentBox.span().x;
  int dataHeight = pixelExtentBox.span().y;

  if (debug) {
    CDBDebug("warp is required");
  }

  size_t dataSize = (dataWidth + 1) * (dataHeight + 1);

  if (projectionGrid == nullptr) {
    // TODO: Make strided projection work in all cases
    bool useStridingProjection = false;
    if (dataWidth * dataHeight > 1000 * 1000) {
      useStridingProjection = true;
    }

    if (!useStridingProjection) {
      if (debug) {
        CDBDebug("makeProjection");
      }
      projectionGrid = makeProjection(halfCell, warper, pixelExtentBox, sourceGeoParams, destGeoParams, warperState);
    } else {
      if (debug) {
        CDBDebug("makeStridedProjection");
      }
      projectionGrid = makeStridedProjection(halfCell, warper, pixelExtentBox, sourceGeoParams, destGeoParams, warperState);
    }
  }
  auto px = projectionGrid->px;
  auto py = projectionGrid->py;
  auto skip = projectionGrid->skip;
  if (debug) {
    CDBDebug("Reprojection done");
  }

  for (size_t j = 0; j < dataSize; j++) {
    if (!(px[j] > -DBL_MAX && px[j] < DBL_MAX)) skip[j] = true;
  }

  double avgDX = 0;
  double avgDY = 0;
  double pLengthD = 0;
  bool isMercator = isMercatorProjection(&destGeoParams.crs);
  bool isLonLatOrMercatorProjection = isLonLatProjection(&destGeoParams.crs) == true || isMercator;
  double sphereWidth = isMercator ? 40000000 : 360;
  int offs1 = 0;
  int offs2 = 1;
  int offs3 = dataWidth + 1 + 1;
  int offs4 = dataWidth + 1 + 0;

  if (debug) {
    CDBDebug("start looping");
  }

  for (int y = 0; y < dataHeight; y = y + 1) {
    for (int x = 0; x < dataWidth; x = x + 1) {
      size_t p = x + y * (dataWidth + 1);
      if (skip[p + offs1] == false && skip[p + offs2] == false && skip[p + offs3] == false && skip[p + offs4] == false) {
        bool doDraw = true;
        // Order for the quad corners is:
        //  quadX[0] -- quadX[1]
        //   |      |
        //  quadX[3] -- quadX[2]

        double quadX[4] = {px[p + offs1], px[p + offs2], px[p + offs3], px[p + offs4]};
        double quadY[4] = {py[p + offs1], py[p + offs2], py[p + offs3], py[p + offs4]};

        if (isLonLatOrMercatorProjection) {
          double lonMin = std::min(quadX[0], std::min(quadX[1], std::min(quadX[2], quadX[3])));
          double lonMax = std::max(quadX[0], std::max(quadX[1], std::max(quadX[2], quadX[3])));
          if (lonMax - lonMin >= sphereWidth * 0.9) {
            double lonMiddle = (lonMin + lonMax) / 2.0;
            if (lonMiddle > 0) {
              quadX[0] += sphereWidth;
              quadX[1] += sphereWidth;
              quadX[2] += sphereWidth;
              quadX[3] += sphereWidth;
            } else {
              quadX[0] -= sphereWidth;
              quadX[1] -= sphereWidth;
              quadX[2] -= sphereWidth;
              quadX[3] -= sphereWidth;
            }
          }
        }

        quadX[0] = (quadX[0] - dfDestOrigX) * multiDestX;
        quadX[1] = (quadX[1] - dfDestOrigX) * multiDestX;
        quadX[2] = (quadX[2] - dfDestOrigX) * multiDestX;
        quadX[3] = (quadX[3] - dfDestOrigX) * multiDestX;

        quadY[0] = (quadY[0] - dfDestOrigY) * multiDestY;
        quadY[1] = (quadY[1] - dfDestOrigY) * multiDestY;
        quadY[2] = (quadY[2] - dfDestOrigY) * multiDestY;
        quadY[3] = (quadY[3] - dfDestOrigY) * multiDestY;

        // If suddenly the length of the quad is 10 times bigger, we probably have an anomaly and we should not draw it.
        // Calculate the diagonal length of the quad.
        double lengthD = (quadX[2] - quadX[0]) * (quadX[2] - quadX[0]) + (quadY[2] - quadY[0]) * (quadY[2] - quadY[0]);
        if (x == 0 && y == 0) {
          pLengthD = lengthD;
        }
        if (lengthD > pLengthD * 10) {
          doDraw = false;
        }
        pLengthD = lengthD;

        // Check the right side of the grid
        if (x == 0) avgDX = quadX[1];
        if (y == 0) avgDY = quadY[3];
        if (x == dataWidth - 1) {
          if (fabs(avgDX - quadX[0]) < fabs(quadX[0] - quadX[1]) / 2) {
            doDraw = false;
          }
          if (fabs(avgDX - quadX[1]) < fabs(quadX[0] - quadX[1]) / 2) {
            doDraw = false;
          }
        }
        // Check the bottom side of the grid
        if (y == dataHeight - 1) {
          if (fabs(avgDY - quadY[0]) < fabs(quadY[0] - quadY[3]) / 2) {
            doDraw = false;
          }
        }

        if (doDraw) {
          warperState.sourceIndexX = x + pixelExtentBox.left;
          warperState.sourceIndexY = (warperState.sourceGridHeight - 1 - (y + pixelExtentBox.bottom));
          T value = ((T *)warperState.sourceGrid)[warperState.sourceIndexX + warperState.sourceIndexY * warperState.sourceGridWidth];
          double xCornersA[3] = {quadX[0], quadX[1], quadX[2]};
          double yCornersA[3] = {quadY[0], quadY[1], quadY[2]};
          double xCornersB[3] = {quadX[2], quadX[0], quadX[3]};
          double yCornersB[3] = {quadY[2], quadY[0], quadY[3]};
          gdwDrawTriangle(xCornersA, yCornersA, value, false, warperState, drawFunction);
          gdwDrawTriangle(xCornersB, yCornersB, value, true, warperState, drawFunction);
        }
      }
    }
  }

  if (debug) {
    CDBDebug("render done");
  }
}

template <typename T>
int GenericDataWarper::render(CImageWarper *warper, void *_sourceData, GeoParameters sourceGeoParams, GeoParameters destGeoParams,
                              const std::function<void(int, int, T, GDWState &warperState)> &drawFunction) {

  // This structure is passed to drawfunctions and contains info about the current state of the warper.
  // The drawfunction will be called numerous times for each destination pixel.
  GDWState warperState = {.sourceGrid = _sourceData,                  // The source datagrid, has the same datatype as the template T
                          .hasNodataValue = 0,                        // Wether the source data grid has a nodata value
                          .dfNodataValue = 0,                         // No data value of the source grid, in double type. Can be casted to T
                          .sourceIndexX = 0,                          // Which X index is sampled from the source grid
                          .sourceIndexY = 0,                          // Which Y index is sampled for the source grid.
                          .sourceGridWidth = sourceGeoParams.width,   // The width of the sourcedata grid
                          .sourceGridHeight = sourceGeoParams.height, // The height of the source data grid
                          .destGridWidth = destGeoParams.width,       // The width of the destination grid
                          .destGridHeight = destGeoParams.height,     // The height of the destination grid
                          .sourceTileDx = 0,                          // The relative X sample position from the source grid cell from 0 to 1. Can be used for bilinear interpolation
                          .sourceTileDy = 0,                          // The relative y sample position
                          .destIndexX = 0,                            // The target X index in the target grid
                          .destIndexY = 0};                           // The target Y index in the target grid.

  sourceGeoParams.bbox = reprojBBox(sourceGeoParams, warper);

  /* When geographical map projections are equal, just do a simple linear transformation */
  if (warper->isProjectionRequired() == false) {
    linearTransformGrid(warperState, useHalfCellOffset, warper, _sourceData, sourceGeoParams, destGeoParams, drawFunction);
  } else {
    /* If geographical map projection is different, we have to transform the grid */
    warpTransformGrid(warperState, projectionGrid, useHalfCellOffset, warper, _sourceData, sourceGeoParams, destGeoParams, drawFunction);
  }

  return 0;
}

#define SPECIALIZE_TEMPLATE(CDFTYPE, CPPTYPE)                                                                                                                                                          \
  template int GenericDataWarper::render<CPPTYPE>(CImageWarper * warper, void *_sourceData, GeoParameters sourceGeoParams, GeoParameters destGeoParams,                                                \
                                                  const std::function<void(int, int, CPPTYPE, GDWState &warperState)> &drawFunction);
ENUMERATE_OVER_CDFTYPES(SPECIALIZE_TEMPLATE)
#undef SPECIALIZE_TEMPLATE
