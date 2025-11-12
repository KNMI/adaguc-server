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

const char *CImgWarpGeneric::className = "CImgWarpGeneric";

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

void CImgWarpGeneric::render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {

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

  int destDataWidth = drawImage->Geo.dWidth;
  int destDataHeight = drawImage->Geo.dHeight;
  size_t numGridElements = destDataWidth * destDataHeight;
  CDF::allocateData(CDF_FLOAT, &settings.destinationGrid, numGridElements);
  CDF::fill(settings.destinationGrid, CDF_FLOAT, settings.dfNodataValue, numGridElements);

  CDFType dataType = dataSource->getFirstAvailableDataObject()->cdfVariable->getType();
  sourceData = dataSource->getFirstAvailableDataObject()->cdfVariable->data;
  CGeoParams sourceGeo;
  sourceGeo.dWidth = dataSource->dWidth;
  sourceGeo.dHeight = dataSource->dHeight;
  sourceGeo.bbox = dataSource->dfBBOX;
  sourceGeo.dfCellSizeX = dataSource->dfCellSizeX;
  sourceGeo.dfCellSizeY = dataSource->dfCellSizeY;
  sourceGeo.CRS = dataSource->nativeProj4;

  GenericDataWarper genericDataWarper;
  GDWArgs args = {.warper = warper, .sourceData = sourceData, .sourceGeoParams = sourceGeo, .destGeoParams = drawImage->Geo};

#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                                       \
  if (dataType == CDFTYPE) genericDataWarper.render<CPPTYPE>(args, [&](int x, int y, CPPTYPE val, GDWState &warperState) { return warpImageBilinearFunction(x, y, val, warperState, settings); });
  ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER

  for (int y = 0; y < (int)destDataHeight; y = y + 1) {
    for (int x = 0; x < (int)destDataWidth; x = x + 1) {
      float val = ((float *)settings.destinationGrid)[x + y * destDataWidth];
      if (val != (float)settings.dfNodataValue && val == val) {
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
  delete[] (float *)settings.destinationGrid;

  return;
}

int CImgWarpGeneric::set(const char *) { return 0; }
