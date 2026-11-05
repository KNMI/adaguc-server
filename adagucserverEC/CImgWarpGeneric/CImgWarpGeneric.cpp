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

#include "CImgWarpGeneric.h"
#include "GenericDataWarper/gdwBilinear.h"
#include "GenericDataWarper/gdwBulkColor.h"
#include <cmath>
#include "./GenericDataWarper/GDWDrawFunctionSettings.h"
#include "CImageDataWriter.h"
#include "CGenericDataWarper.h"
#include <CImageOperators/drawContour.h>
#include <CImageOperators/smoothRasterField.h>
#include <string>
#include <vector>
#include "CCDFObject.h"
#include "CCDFTypes.h"
#include "CDataSource.h"
#include "CDrawImage.h"
#include "CImageWarper.h"
#include "CServerParams.h"
#include "CXMLParser.h"
#include "GenericDataWarper/GDWDrawFunctionSettings.h"
#include "Types/CPointTypes.h"
#include "Types/GeoParameters.h"
#include "CDrawFunction.h"
#include "CStopWatch.h"

template <typename T> void warpImageNearestFunction(int x, int y, T value, GDWState &warperState, GDWDrawFunctionSettings &settings, GDWPixelBatch<T> &batch) {
  if (x < 0 || y < 0 || x >= warperState.destGridWidth || y >= warperState.destGridHeight) return;
  if ((settings.hasNodataValue && ((value) == (T)settings.dfNodataValue)) || std::isnan(value)) return;

  if (settings.smoothingFiter > 0) {
    value = smoothingAtLocation(warperState.sourceIndexX, warperState.sourceIndexY, (T *)warperState.sourceGrid, warperState, settings);
  }
  batch.append(x, y, value);
};

template <typename T> bool loadBilinearValues(T val, GDWState &warperState, GDWDrawFunctionSettings &settings, T values[4]) {
  if (settings.hasNodataValue) {
    if ((val) == (T)settings.dfNodataValue) return false;
  }

  if (std::isnan(val)) return false;

  T *sourceData = (T *)warperState.sourceGrid;

  size_t sourceDataPX = warperState.sourceIndexX;
  size_t sourceDataPY = warperState.sourceIndexY;
  size_t sourceDataWidth = warperState.sourceGridWidth;
  size_t sourceDataHeight = warperState.sourceGridHeight;

  if (warperState.hasSharedBoundaryLR == false) {
    if (sourceDataPX >= sourceDataWidth - 1) return false;
  }

  if (sourceDataPY >= sourceDataHeight - 1) return false;

  T corners[2][2] = {{0, 0}, {0, 0}};

  const auto x0 = sourceDataPX;
  const auto x1 = sourceDataPX + 1 == sourceDataWidth ? size_t{0} : sourceDataPX + 1;
  const auto y0 = sourceDataPY;
  const auto y1 = sourceDataPY + 1;

  if (settings.smoothingFiter > 0) {
    corners[0][0] = smoothingAtLocation(x0, y0, sourceData, warperState, settings);
    corners[1][0] = smoothingAtLocation(x1, y0, sourceData, warperState, settings);
    corners[0][1] = smoothingAtLocation(x0, y1, sourceData, warperState, settings);
    corners[1][1] = smoothingAtLocation(x1, y1, sourceData, warperState, settings);
  } else {
    corners[0][0] = sourceData[x0 + y0 * sourceDataWidth];
    corners[1][0] = sourceData[x1 + y0 * sourceDataWidth];
    corners[0][1] = sourceData[x0 + y1 * sourceDataWidth];
    corners[1][1] = sourceData[x1 + y1 * sourceDataWidth];
  }

  // Reject the whole quad if any corner is nodata/NaN, like CImgWarpBilinear used to.
  for (const auto &row : corners) {
    for (const T &cornerValue : row) {
      if ((settings.hasNodataValue && cornerValue == (T)settings.dfNodataValue) || std::isnan(cornerValue)) return false;
    }
  }

  values[0] = corners[0][0];
  values[1] = corners[1][0];
  values[2] = corners[0][1];
  values[3] = corners[1][1];
  return true;
}

// Collect pixels across quads so even one-pixel triangle spans contribute SIMD lanes.
// Only sampling/classification is reordered; grid writes and blending keep traversal order.
template <typename T> class BilinearDrawFunction {
  GDWDrawFunctionSettings &settings;
  GDWBatchPainter &painter;
  mutable int cachedX = -1, cachedY = -1;
  mutable bool valid = false;
  mutable T values[4]{};
  mutable int count = 0;
  mutable int px[GDW_BATCH_SIZE], py[GDW_BATCH_SIZE];
  alignas(64) mutable float dx[GDW_BATCH_SIZE], dy[GDW_BATCH_SIZE];
  alignas(64) mutable T v00[GDW_BATCH_SIZE], v10[GDW_BATCH_SIZE], v01[GDW_BATCH_SIZE], v11[GDW_BATCH_SIZE];

  bool prepare(T val, GDWState &state) const {
    if (cachedX == state.sourceIndexX && cachedY == state.sourceIndexY) return valid;
    cachedX = state.sourceIndexX;
    cachedY = state.sourceIndexY;
    valid = loadBilinearValues(val, state, settings, values);
    return valid;
  }
  void append(int x, int y, GDWState &state) const {
    px[count] = x;
    py[count] = y;
    dx[count] = float(state.sourceTileDx);
    dy[count] = float(state.sourceTileDy);
    v00[count] = values[0];
    v10[count] = values[1];
    v01[count] = values[2];
    v11[count] = values[3];
    if (++count == GDW_BATCH_SIZE) flush();
  }

public:
  BilinearDrawFunction(GDWDrawFunctionSettings &settings, GDWBatchPainter &painter) : settings(settings), painter(painter) {}
  bool prepareQuad(T val, GDWState &state) const { return prepare(val, state); }
  void operator()(int x, int y, T val, GDWState &state) const {
    if (prepare(val, state)) append(x, y, state);
  }
  void drawSpan(int start, int end, int y, double tileDx, double tileDy, double stepDx, double stepDy, T, GDWState &state) const {
    for (int x = start; x < end; ++x, tileDx += stepDx, tileDy += stepDy) {
      state.sourceTileDx = tileDx < 0 ? 0 : (tileDx > 1 ? 1 : tileDx);
      state.sourceTileDy = tileDy < 0 ? 0 : (tileDy > 1 ? 1 : tileDy);
      append(x, y, state);
    }
  }
  void flush() const {
    if (!count) return;
    alignas(64) float output[GDW_BATCH_SIZE];
    gdwBilinearBatch(output, dx, dy, v00, v10, v01, v11, count);
    painter.write(count, px, py, output);
    count = 0;
  }
};

template <typename T> void warpImageRenderBorders(int x, int y, T val, GDWState &warperState, GDWDrawFunctionSettings &settings, MemoizationForDeterminePixelColorFromValue &memo) {
  if (x < 0 || y < 0 || x >= warperState.destGridWidth || y >= warperState.destGridHeight) return;

  if (settings.hasNodataValue) {
    if ((val) == (T)settings.dfNodataValue) return;
  }

  if (std::isnan(val)) return;

  size_t sourceDataPX = warperState.sourceIndexX;
  size_t sourceDataPY = warperState.sourceIndexY;
  size_t sourceDataWidth = warperState.sourceGridWidth;
  size_t sourceDataHeight = warperState.sourceGridHeight;

  if (sourceDataPY > sourceDataHeight - 1) return;
  if (sourceDataPX > sourceDataWidth - 1) return;
  float value = 0;

  if (warperState.sourceIndexX == 0) {
    value = 2;
  }
  if (warperState.sourceIndexY == 0) {
    value = 3;
  }
  if (warperState.sourceIndexX == (int)warperState.sourceGridWidth - 1) {
    value = 4;
  }
  if (warperState.sourceIndexY == (int)warperState.sourceGridHeight - 1) {
    value = 5;
  }

  if (x >= 0 && y >= 0 && x < (int)warperState.destGridWidth && y < (int)warperState.destGridHeight) {
    float dx = warperState.sourceTileDx;
    float dy = warperState.sourceTileDy;
    if (dx <= .02 || dy <= .02 || dx >= 0.98 || dy >= 0.98) {
      value = 1;
    }
  }

  ((float *)settings.destinationGrid)[x + y * warperState.destGridWidth] = value;
  auto colorIndex = memoizedDeterminePixelColorFromValue(value, &settings, memo);
  if (colorIndex.a > 0) {
    settings.drawImage->setPixel(x, y, colorIndex);
  }
};

void CImgWarpGeneric::render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {

  std::string color;
  void *sourceData;
  bool debug = false;

  CStyleConfiguration *styleConfiguration = dataSource->getStyle();

  GDWDrawFunctionSettings settings = getDrawFunctionSettings(dataSource, drawImage, styleConfiguration);
  MemoizationForDeterminePixelColorFromValue memo; // scoped to this render() call only

  int destDataWidth = drawImage->geoParams.width;
  int destDataHeight = drawImage->geoParams.height;
  size_t numGridElements = destDataWidth * destDataHeight;
  CDF::allocateData(CDF_FLOAT, &settings.destinationGrid, numGridElements);
  CDF::fill(settings.destinationGrid, CDF_FLOAT, settings.dfNodataValue, numGridElements);

  CDFType dataType = dataSource->getFirstAvailableDataObject()->cdfVariable->getType();
  sourceData = dataSource->getFirstAvailableDataObject()->cdfVariable->data;
  GeoParameters sourceGeo;
  sourceGeo.width = dataSource->dWidth;
  sourceGeo.height = dataSource->dHeight;
  sourceGeo.bbox = dataSource->dfBBOX;
  sourceGeo.cellsizeX = dataSource->dfCellSizeX;
  sourceGeo.cellsizeY = dataSource->dfCellSizeY;
  sourceGeo.crs = dataSource->nativeProj4;

  GenericDataWarper genericDataWarper;
  GDWArgs args = {.warper = warper, .sourceData = sourceData, .sourceGeoParams = sourceGeo, .destGeoParams = drawImage->geoParams};
  if (!settings.drawgridboxoutline) {
    GDWBatchPainter painter(settings);
    if (settings.interpolationMethod == InterpolationMethodNearest) {
      if (debug) {
        CDBDebug("Render nearest");
      }
      genericDataWarper.useHalfCellOffset = false;
#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                                       \
  if (dataType == CDFTYPE) {                                                                                                                                                                           \
    GDWPixelBatch<CPPTYPE> batch(painter);                                                                                                                                                             \
    genericDataWarper.render<CPPTYPE>(args, [&](int x, int y, CPPTYPE val, GDWState &state) { warpImageNearestFunction(x, y, val, state, settings, batch); });                                         \
    StopWatch_Measure("CImgWarpGeneric: start pixel batch");                                                                                                                                           \
    batch.flush();                                                                                                                                                                                     \
    StopWatch_Measure("CImgWarpGeneric: done pixel batch");                                                                                                                                            \
  }
      ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER
    }
    if (settings.interpolationMethod == InterpolationMethodBilinear) {
      if (debug) {
        CDBDebug("Render bilinear");
      }
      genericDataWarper.useHalfCellOffset = true;

#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                                       \
  if (dataType == CDFTYPE) {                                                                                                                                                                           \
    BilinearDrawFunction<CPPTYPE> draw(settings, painter);                                                                                                                                             \
    genericDataWarper.render<CPPTYPE>(args, draw);                                                                                                                                                     \
    StopWatch_Measure("CImgWarpGeneric: start pixel batch");                                                                                                                                           \
    draw.flush();                                                                                                                                                                                      \
    StopWatch_Measure("CImgWarpGeneric: done pixel batch");                                                                                                                                            \
  }
      ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER
    }

    if (settings.shouldDrawContours && styleConfiguration->contourLines.size() > 0) {
      drawContour((float *)settings.destinationGrid, dataSource, drawImage, styleConfiguration);
      StopWatch_Measure("CImgWarpGeneric: done drawContour");
    }
  }

  // Draw grid outlines
  if (settings.drawgridboxoutline) {
    genericDataWarper.useHalfCellOffset = false;

#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                                       \
  if (dataType == CDFTYPE) genericDataWarper.render<CPPTYPE>(args, [&](int x, int y, CPPTYPE val, GDWState &warperState) { return warpImageRenderBorders(x, y, val, warperState, settings, memo); });
    ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER
  }
  if (debug) {
    CDBDebug("done");
  }
  free(settings.destinationGrid);
}

int CImgWarpGeneric::set(const char *) { return 0; }
