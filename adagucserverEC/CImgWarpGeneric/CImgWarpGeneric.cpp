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

const char *CImgWarpGeneric::className = "CImgWarpGeneric";

CColor cblack = CColor(0, 0, 0, 255);
CColor cblue = CColor(0, 0, 255, 255);

template <typename T> void warpImageNearestFunction(int x, int y, T value, GDWState &wState, GDWDrawFunctionSettings &settings) {
  if (x < 0 || y < 0 || x >= wState.destDataWidth || y >= wState.destDataHeight) return;
  if ((settings.hasNodataValue && ((value) == (T)settings.dfNodataValue)) || !(value == value)) return;
  ((float *)settings.destinationGrid)[x + y * wState.destDataWidth] = value;
};

template <typename T> void warpImageBilinearFunction(int x, int y, T val, GDWState &warperState, GDWDrawFunctionSettings &settings) {
  if (x < 0 || y < 0 || x > warperState.destDataWidth || y > warperState.destDataHeight) return;

  if (settings.hasNodataValue) {
    if ((val) == (T)settings.dfNodataValue) return;
  }

  // Check for NaN
  if (!(val == val)) return;

  T *sourceData = (T *)warperState.sourceData;
  size_t sourceDataPX = warperState.sourceDataPX;
  size_t sourceDataPY = warperState.sourceDataPY;
  size_t sourceDataWidth = warperState.sourceDataWidth;
  size_t sourceDataHeight = warperState.sourceDataHeight;

  if (sourceDataPY > sourceDataHeight - 1) return;
  if (sourceDataPX > sourceDataWidth - 1) return;

  T values[2][2] = {{0, 0}, {0, 0}};

  values[0][0] += ((T *)sourceData)[nfast_mod(sourceDataPX + 0, sourceDataWidth) + nfast_mod(sourceDataPY + 0, sourceDataHeight) * sourceDataWidth];
  values[1][0] += ((T *)sourceData)[nfast_mod(sourceDataPX + 1, sourceDataWidth) + nfast_mod(sourceDataPY + 0, sourceDataHeight) * sourceDataWidth];
  values[0][1] += ((T *)sourceData)[nfast_mod(sourceDataPX + 0, sourceDataWidth) + nfast_mod(sourceDataPY + 1, sourceDataHeight) * sourceDataWidth];
  values[1][1] += ((T *)sourceData)[nfast_mod(sourceDataPX + 1, sourceDataWidth) + nfast_mod(sourceDataPY + 1, sourceDataHeight) * sourceDataWidth];

  if (x >= 0 && y >= 0 && x < (int)warperState.destDataWidth && y < (int)warperState.destDataHeight) {
    float dx = warperState.tileDx;
    float dy = warperState.tileDy;
    float gx1 = (1 - dx) * values[0][0] + dx * values[1][0];
    float gx2 = (1 - dx) * values[0][1] + dx * values[1][1];
    float bilValue = (1 - dy) * gx1 + dy * gx2;
    ((float *)settings.destinationGrid)[x + y * warperState.destDataWidth] = bilValue;
  }
};

template <typename T> void warpImageRenderBorders(int x, int y, T val, GDWState &warperState, GDWDrawFunctionSettings &settings) {
  if (x < 0 || y < 0 || x > warperState.destDataWidth || y > warperState.destDataHeight) return;

  if (settings.hasNodataValue) {
    if ((val) == (T)settings.dfNodataValue) return;
  }

  // Check for NaN
  if (!(val == val)) return;

  size_t sourceDataPX = warperState.sourceDataPX;
  size_t sourceDataPY = warperState.sourceDataPY;
  size_t sourceDataWidth = warperState.sourceDataWidth;
  size_t sourceDataHeight = warperState.sourceDataHeight;

  if (sourceDataPY > sourceDataHeight - 1) return;
  if (sourceDataPX > sourceDataWidth - 1) return;
  if (warperState.sourceDataPX == 0 || warperState.sourceDataPX == 0 || warperState.sourceDataPX == (int)warperState.sourceDataWidth - 1 ||
      warperState.sourceDataPY == (int)warperState.sourceDataHeight - 1) {
    settings.drawImage->setPixel(x, y, cblue);
  }
  if (x >= 0 && y >= 0 && x < (int)warperState.destDataWidth && y < (int)warperState.destDataHeight) {
    float dx = warperState.tileDx;
    float dy = warperState.tileDy;

    if (dx <= .02 || dy <= .02 || dx >= 0.98 || dy >= 0.98) {
      settings.drawImage->setPixel(x, y, cblack);
    }
  }
};

void CImgWarpGeneric::render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {

  CT::string color;
  void *sourceData;

  CStyleConfiguration *styleConfiguration = dataSource->getStyle();

  GDWDrawFunctionSettings settings = getDrawFunctionSettings(dataSource, drawImage, styleConfiguration);
  CDBDebug("%f %d", settings.shadeInterval, settings.isUsingShadeIntervals);

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

  if (settings.drawInImage == DrawInImageNearest) {
#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                                       \
  if (dataType == CDFTYPE) genericDataWarper.render<CPPTYPE>(args, [&](int x, int y, CPPTYPE val, GDWState &warperState) { return warpImageNearestFunction(x, y, val, warperState, settings); });
    ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER
  }
  if (settings.drawInImage == DrawInImageBilinear || settings.drawInImage == DrawInImageNone) {
    genericDataWarper.useHalfCellOffset = true;

#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                                       \
  if (dataType == CDFTYPE) genericDataWarper.render<CPPTYPE>(args, [&](int x, int y, CPPTYPE val, GDWState &warperState) { return warpImageBilinearFunction(x, y, val, warperState, settings); });
    ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER
  }

  if (settings.drawInImage == DrawInImageBilinear || settings.drawInImage == DrawInImageNearest) {
    for (int y = 0; y < (int)destDataHeight; y = y + 1) {
      for (int x = 0; x < (int)destDataWidth; x = x + 1) {
        float val = ((float *)settings.destinationGrid)[x + y * destDataWidth];
        setPixelInDrawImage(x, y, val, &settings);
      }
    }
  }
  drawContour((float *)settings.destinationGrid, dataSource, drawImage, styleConfiguration);

  // Draw grid
  if (1 == 12) {
    genericDataWarper.useHalfCellOffset = true;

#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                                       \
  if (dataType == CDFTYPE) genericDataWarper.render<CPPTYPE>(args, [&](int x, int y, CPPTYPE val, GDWState &warperState) { return warpImageRenderBorders(x, y, val, warperState, settings); });
    ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER
  }

  CDBDebug("done");
  delete[] (float *)settings.destinationGrid;

  return;
}

int CImgWarpGeneric::set(const char *) { return 0; }
