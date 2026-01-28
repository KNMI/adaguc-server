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

#include "CImgWarpHillShaded.h"
#include "CImageDataWriter.h"
#include "f8vector.h"
#include <CCDFTypes.h>
#include "CImgWarpGeneric/CImgWarpGeneric.h"

const char *CImgWarpHillShaded::className = "CImgWarpHillShaded";
/**
 * Lightsource
 */
const f8vector lightSource = (f8vector({.x = -1, .y = -1, .z = -1})).norm();

template <typename T> double getGridValueFromFloat(int x, int y, GDWState &drawSettings) { return ((T *)drawSettings.sourceGrid)[x + y * drawSettings.sourceGridWidth]; }

static inline int mfast_mod(const int input, const int ceil) {
  if (0 <= input && input < ceil)
    return input;
  int mod = input % ceil;
  if (mod < 0) {
    mod += ceil;
  }
  return mod;
}

template <class T> void hillShadedDrawFunction(int x, int y, T val, GDWState &warperState, GDWDrawFunctionSettings &drawFunctionState) {
  if (x < 0 || y < 0 || x > warperState.destGridWidth || y > warperState.destGridHeight) return;
  bool isNodata = false;
  if (drawFunctionState.hasNodataValue) {
    if ((val) == (T)drawFunctionState.dfNodataValue) isNodata = true;
  }
  if (!(val == val)) isNodata = true;
  if (!isNodata)
    if (drawFunctionState.legendValueRange)
      if (val < drawFunctionState.legendLowerRange || val > drawFunctionState.legendUpperRange) isNodata = true;
  if (!isNodata) {
    T *sourceData = (T *)warperState.sourceGrid;
    size_t sourceDataPX = warperState.sourceIndexX;
    size_t sourceDataPY = warperState.sourceIndexY;
    size_t sourceDataWidth = warperState.sourceGridWidth;
    size_t sourceDataHeight = warperState.sourceGridHeight;

    if (sourceDataPY > sourceDataHeight - 1) return;
    if (sourceDataPX > sourceDataWidth - 1) return;

    float values[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

    /* TODO make window size configurable */
    for (int wy = -1; wy < 2; wy++) {
      for (int wx = -1; wx < 2; wx++) {
        values[0][0] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 0 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 0, sourceDataHeight) * sourceDataWidth];
        values[1][0] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 1 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 0, sourceDataHeight) * sourceDataWidth];
        values[2][0] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 2 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 0, sourceDataHeight) * sourceDataWidth];
        values[0][1] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 0 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 1, sourceDataHeight) * sourceDataWidth];
        values[1][1] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 1 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 1, sourceDataHeight) * sourceDataWidth];
        values[2][1] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 2 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 1, sourceDataHeight) * sourceDataWidth];
        values[0][2] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 0 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 2, sourceDataHeight) * sourceDataWidth];
        values[1][2] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 1 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 2, sourceDataHeight) * sourceDataWidth];
        values[2][2] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 2 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 2, sourceDataHeight) * sourceDataWidth];
      }
    }

    f8vector v00 = f8vector({.x = sourceDataPX + 0., .y = sourceDataPY + 0., .z = values[0][0]});
    f8vector v10 = f8vector({.x = sourceDataPX + 1., .y = sourceDataPY + 0., .z = values[1][0]});
    f8vector v20 = f8vector({.x = sourceDataPX + 2., .y = sourceDataPY + 0., .z = values[2][0]});
    f8vector v01 = f8vector({.x = sourceDataPX + 0., .y = sourceDataPY + 1., .z = values[0][1]});
    f8vector v11 = f8vector({.x = sourceDataPX + 1., .y = sourceDataPY + 1., .z = values[1][1]});
    f8vector v21 = f8vector({.x = sourceDataPX + 2., .y = sourceDataPY + 1., .z = values[2][1]});
    f8vector v02 = f8vector({.x = sourceDataPX + 0., .y = sourceDataPY + 2., .z = values[0][2]});
    f8vector v12 = f8vector({.x = sourceDataPX + 1., .y = sourceDataPY + 2., .z = values[1][2]});

    if (x >= 0 && y >= 0 && x < (int)warperState.destGridWidth && y < (int)warperState.destGridHeight) {
      f8vector normal00 = cross(v10 - v00, v01 - v00).norm();
      f8vector normal10 = cross(v20 - v10, v11 - v10).norm();
      f8vector normal01 = cross(v11 - v01, v02 - v01).norm();
      f8vector normal11 = cross(v21 - v11, v12 - v11).norm();

      float c00 = dot(lightSource, normal00);
      float c10 = dot(lightSource, normal10);
      float c01 = dot(lightSource, normal01);
      float c11 = dot(lightSource, normal11);
      float dx = warperState.sourceTileDx;
      float dy = warperState.sourceTileDy;
      float gx1 = (1 - dx) * c00 + dx * c10;
      float gx2 = (1 - dx) * c01 + dx * c11;
      float bilValue = (1 - dy) * gx1 + dy * gx2;
      ((float *)drawFunctionState.destinationGrid)[x + y * warperState.destGridWidth] = (bilValue + 1) / 1.816486;
    }
  }
}

void CImgWarpHillShaded::render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {
  CT::string color;
  void *sourceData;

  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  GDWDrawFunctionSettings settings;
  settings.dfNodataValue = dataSource->getFirstAvailableDataObject()->dfNodataValue;
  settings.legendValueRange = (bool)styleConfiguration->hasLegendValueRange;
  settings.legendLowerRange = styleConfiguration->legendLowerRange;
  settings.legendUpperRange = styleConfiguration->legendUpperRange;
  settings.hasNodataValue = dataSource->getFirstAvailableDataObject()->hasNodataValue;

  if (!settings.hasNodataValue) {
    settings.hasNodataValue = true;
    settings.dfNodataValue = -100000.f;
  }
  int destDataWidth = drawImage->geoParams.width;
  int destDataHeight = drawImage->geoParams.height;
  size_t numGridElements = destDataWidth * destDataHeight;
  CDF::allocateData(CDF_FLOAT, &settings.destinationGrid, numGridElements);
  CDF::fill(settings.destinationGrid, CDF_FLOAT, settings.dfNodataValue, numGridElements);

  CDFType dataType = dataSource->getDataObject(0)->cdfVariable->getType();
  sourceData = dataSource->getDataObject(0)->cdfVariable->data;
  GeoParameters sourceGeo = dataSource->makeGeoParams();

  GenericDataWarper genericDataWarper;
  GDWArgs args = {.warper = warper, .sourceData = sourceData, .sourceGeoParams = sourceGeo, .destGeoParams = dataSource->srvParams->geoParams};

#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                                       \
  if (dataType == CDFTYPE) genericDataWarper.render<CPPTYPE>(args, [&](int x, int y, CPPTYPE val, GDWState &warperState) { hillShadedDrawFunction(x, y, val, warperState, settings); });
  ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER

  for (int y = 0; y < destDataHeight; y = y + 1) {
    for (int x = 0; x < destDataWidth; x = x + 1) {
      float val = ((float *)settings.destinationGrid)[x + y * destDataWidth];
      if (val != (double)settings.dfNodataValue && val == val) {
        if (styleConfiguration->legendLog != 0) val = log10(val + .000001) / log10(styleConfiguration->legendLog);
        val *= styleConfiguration->legendScale;
        val += styleConfiguration->legendOffset;
        if (val >= 239)
          val = 239;
        else if (val < 0)
          val = 0;
        drawImage->setPixelIndexed(x, y, (unsigned char)val);
      }
    }
  }
  delete[] ((float *)settings.destinationGrid);
  // CDBDebug("render done");
  return;
}

int CImgWarpHillShaded::set(const char *) { return 0; }
