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

void CImgWarpGeneric::render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {

  CT::string color;
  void *sourceData;

  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  CImgWarpGenericDrawFunctionState settings;
  settings.dfNodataValue = dataSource->getFirstAvailableDataObject()->dfNodataValue;
  settings.legendValueRange = (bool)styleConfiguration->hasLegendValueRange;
  settings.legendLowerRange = styleConfiguration->legendLowerRange;
  settings.legendUpperRange = styleConfiguration->legendUpperRange;
  settings.hasNodataValue = dataSource->getFirstAvailableDataObject()->hasNodataValue;

  if (!settings.hasNodataValue) {
    settings.hasNodataValue = true;
    settings.dfNodataValue = -100000.f;
  }
  settings.width = drawImage->Geo->dWidth;
  settings.height = drawImage->Geo->dHeight;

  settings.dataField = new float[settings.width * settings.height];
  for (int y = 0; y < settings.height; y++) {
    for (int x = 0; x < settings.width; x++) {
      settings.dataField[x + y * settings.width] = (float)settings.dfNodataValue;
    }
  }

  CDFType dataType = dataSource->getFirstAvailableDataObject()->cdfVariable->getType();
  sourceData = dataSource->getFirstAvailableDataObject()->cdfVariable->data;
  CGeoParams sourceGeo;
  sourceGeo.dWidth = dataSource->dWidth;
  sourceGeo.dHeight = dataSource->dHeight;
  sourceGeo.dfBBOX[0] = dataSource->dfBBOX[0];
  sourceGeo.dfBBOX[1] = dataSource->dfBBOX[1];
  sourceGeo.dfBBOX[2] = dataSource->dfBBOX[2];
  sourceGeo.dfBBOX[3] = dataSource->dfBBOX[3];
  sourceGeo.dfCellSizeX = dataSource->dfCellSizeX;
  sourceGeo.dfCellSizeY = dataSource->dfCellSizeY;
  sourceGeo.CRS = dataSource->nativeProj4;
  settings.useHalfCellOffset = true;

  GenericDataWarper genericDataWarper;
  GDWArgs args = {.warper = warper, .sourceData = sourceData, .sourceGeoParams = &sourceGeo, .destGeoParams = drawImage->Geo};

#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                                       \
  if (dataType == CDFTYPE) genericDataWarper.render<CPPTYPE>(args, [&](int x, int y, CPPTYPE val, GDWState &warperState) { return imgWarpGenericDrawFunction(x, y, val, warperState, settings); });
  ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER

  for (int y = 0; y < (int)settings.height; y = y + 1) {
    for (int x = 0; x < (int)settings.width; x = x + 1) {
      float val = settings.dataField[x + y * settings.width];
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
  delete[] settings.dataField;

  return;
}

int CImgWarpGeneric::set(const char *) { return 0; }

template <typename T> void imgWarpGenericDrawFunction(int x, int y, T val, GDWState &warperState, CImgWarpGenericDrawFunctionState &settings) {
  if (x < 0 || y < 0 || x > settings.width || y > settings.height) return;

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

  if (x >= 0 && y >= 0 && x < (int)settings.width && y < (int)settings.height) {
    float dx = warperState.tileDx;
    float dy = warperState.tileDy;
    float gx1 = (1 - dx) * values[0][0] + dx * values[1][0];
    float gx2 = (1 - dx) * values[0][1] + dx * values[1][1];
    float bilValue = (1 - dy) * gx1 + dy * gx2;
    settings.dataField[x + y * settings.width] = bilValue;
  }
};

// Indicate which template specializations are needed, also allows code to be compiled faster as changes are done only in the CPP file.
template void imgWarpGenericDrawFunction<char>(int x, int y, char val, GDWState &warperState, CImgWarpGenericDrawFunctionState &settings);
template void imgWarpGenericDrawFunction<int8_t>(int x, int y, int8_t val, GDWState &warperState, CImgWarpGenericDrawFunctionState &settings);
template void imgWarpGenericDrawFunction<uchar>(int x, int y, uchar val, GDWState &warperState, CImgWarpGenericDrawFunctionState &settings);
template void imgWarpGenericDrawFunction<short>(int x, int y, short val, GDWState &warperState, CImgWarpGenericDrawFunctionState &settings);
template void imgWarpGenericDrawFunction<ushort>(int x, int y, ushort val, GDWState &warperState, CImgWarpGenericDrawFunctionState &settings);
template void imgWarpGenericDrawFunction<int>(int x, int y, int val, GDWState &warperState, CImgWarpGenericDrawFunctionState &settings);
template void imgWarpGenericDrawFunction<uint>(int x, int y, uint val, GDWState &warperState, CImgWarpGenericDrawFunctionState &settings);
template void imgWarpGenericDrawFunction<long>(int x, int y, long val, GDWState &warperState, CImgWarpGenericDrawFunctionState &settings);
template void imgWarpGenericDrawFunction<ulong>(int x, int y, ulong val, GDWState &warperState, CImgWarpGenericDrawFunctionState &settings);
template void imgWarpGenericDrawFunction<float>(int x, int y, float val, GDWState &warperState, CImgWarpGenericDrawFunctionState &settings);
template void imgWarpGenericDrawFunction<double>(int x, int y, double val, GDWState &warperState, CImgWarpGenericDrawFunctionState &settings);
