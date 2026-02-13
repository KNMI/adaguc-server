/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2020-12-09
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

#include "CImgWarpGeneric.h"
#include "CImageDataWriter.h"
#include "CGenericDataWarper.h"
#include <CImageOperators/drawContour.h>
#include <CImageOperators/smoothRasterField.h>

CColor cblack = CColor(0, 0, 0, 255);
CColor cblue = CColor(0, 0, 255, 255);

MemoizitationForDeterminePixelColorFromValue memo;
template <typename T> void warpImageNearestFunction(int x, int y, T value, GDWState &warperState, GDWDrawFunctionSettings &settings) {
  if (x < 0 || y < 0 || x >= warperState.destGridWidth || y >= warperState.destGridHeight) return;
  if ((settings.hasNodataValue && ((value) == (T)settings.dfNodataValue)) || !(value == value)) return;

  if (settings.smoothingFiter > 0) {
    value = smoothingAtLocation(warperState.sourceIndexX, warperState.sourceIndexY, (T *)warperState.sourceGrid, warperState, settings);
  }
  ((float *)settings.destinationGrid)[x + y * warperState.destGridWidth] = value;
  if (settings.drawgrid) {
    auto colorIndex = memoizedDeterminePixelColorFromValue(value, &settings, memo);
    if (colorIndex.a > 0) {
      settings.drawImage->setPixel(x, y, colorIndex);
    }
  }
};

template <typename T> void warpImageBilinearFunction(int x, int y, T val, GDWState &warperState, GDWDrawFunctionSettings &settings) {
  if (x < 0 || y < 0 || x >= warperState.destGridWidth || y >= warperState.destGridHeight) return;

  if (settings.hasNodataValue) {
    if ((val) == (T)settings.dfNodataValue) return;
  }

  // Check for NaN
  if (!(val == val)) return;

  T *sourceData = (T *)warperState.sourceGrid;

  size_t sourceDataPX = warperState.sourceIndexX;
  size_t sourceDataPY = warperState.sourceIndexY;
  size_t sourceDataWidth = warperState.sourceGridWidth;
  size_t sourceDataHeight = warperState.sourceGridHeight;

  if (warperState.hasSharedBoundaryLR == false) {
    if (sourceDataPX >= sourceDataWidth - 1) return;
  }

  if (sourceDataPY >= sourceDataHeight - 1) return;

  T values[2][2] = {{0, 0}, {0, 0}};

  auto x0 = nfast_mod(sourceDataPX + 0, sourceDataWidth);
  auto x1 = nfast_mod(sourceDataPX + 1, sourceDataWidth);
  auto y0 = nfast_mod(sourceDataPY + 0, sourceDataHeight);
  auto y1 = nfast_mod(sourceDataPY + 1, sourceDataHeight);

  if (settings.smoothingFiter > 0) {
    values[0][0] = smoothingAtLocation(x0, y0, sourceData, warperState, settings);
    values[1][0] = smoothingAtLocation(x1, y0, sourceData, warperState, settings);
    values[0][1] = smoothingAtLocation(x0, y1, sourceData, warperState, settings);
    values[1][1] = smoothingAtLocation(x1, y1, sourceData, warperState, settings);
  } else {
    values[0][0] = sourceData[x0 + y0 * sourceDataWidth];
    values[1][0] = sourceData[x1 + y0 * sourceDataWidth];
    values[0][1] = sourceData[x0 + y1 * sourceDataWidth];
    values[1][1] = sourceData[x1 + y1 * sourceDataWidth];
  }

  float dx = warperState.sourceTileDx;
  float dy = warperState.sourceTileDy;
  float gx1 = (1 - dx) * values[0][0] + dx * values[1][0];
  float gx2 = (1 - dx) * values[0][1] + dx * values[1][1];
  float bilValue = (1 - dy) * gx1 + dy * gx2;
  ((float *)settings.destinationGrid)[x + y * warperState.destGridWidth] = bilValue;
  if (settings.drawgrid) {
    auto colorIndex = memoizedDeterminePixelColorFromValue(bilValue, &settings, memo);
    if (colorIndex.a > 0) {
      settings.drawImage->setPixel(x, y, colorIndex);
    }
  }
};

template <typename T> void warpImageRenderBorders(int x, int y, T val, GDWState &warperState, GDWDrawFunctionSettings &settings) {
  if (x < 0 || y < 0 || x > warperState.destGridWidth || y > warperState.destGridHeight) return;

  if (settings.hasNodataValue) {
    if ((val) == (T)settings.dfNodataValue) return;
  }

  // Check for NaN
  if (!(val == val)) return;

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

  CT::string color;
  void *sourceData;
  bool debug = false;

  CStyleConfiguration *styleConfiguration = dataSource->getStyle();

  GDWDrawFunctionSettings settings = getDrawFunctionSettings(dataSource, drawImage, styleConfiguration);

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
    if (settings.interpolationMethod == InterpolationMethodNearest) {
      if (debug) {
        CDBDebug("Render nearest");
      }
      genericDataWarper.useHalfCellOffset = false;
#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                                       \
  if (dataType == CDFTYPE) genericDataWarper.render<CPPTYPE>(args, [&](int x, int y, CPPTYPE val, GDWState &warperState) { return warpImageNearestFunction(x, y, val, warperState, settings); });
      ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER
    }
    if (settings.interpolationMethod == InterpolationMethodBilinear) {
      if (debug) {
        CDBDebug("Render bilinear");
      }
      genericDataWarper.useHalfCellOffset = true;

#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                                       \
  if (dataType == CDFTYPE) genericDataWarper.render<CPPTYPE>(args, [&](int x, int y, CPPTYPE val, GDWState &warperState) { return warpImageBilinearFunction(x, y, val, warperState, settings); });
      ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER
    }

    if (styleConfiguration->contourLines.size() > 0) {
      drawContour((float *)settings.destinationGrid, dataSource, drawImage, styleConfiguration);
    }
  }

  // Draw grid outlines
  if (settings.drawgridboxoutline) {
    genericDataWarper.useHalfCellOffset = false;

#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                                       \
  if (dataType == CDFTYPE) genericDataWarper.render<CPPTYPE>(args, [&](int x, int y, CPPTYPE val, GDWState &warperState) { return warpImageRenderBorders(x, y, val, warperState, settings); });
    ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER
  }
  if (debug) {
    CDBDebug("done");
  }
  free(settings.destinationGrid);
}

int CImgWarpGeneric::set(const char *) { return 0; }
