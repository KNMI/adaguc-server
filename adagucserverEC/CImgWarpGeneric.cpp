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
  // CDBDebug("render");

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

  GenericDataWarper genericDataWarper;
  settings.useHalfCellOffset = true;
  switch (dataType) {
  case CDF_CHAR:
    genericDataWarper.render<char>(warper, sourceData, &sourceGeo, drawImage->Geo, [&](int x, int y, char val, GDWState &warperState) { return drawFunction<char>(x, y, val, warperState, settings); });
    break;
  case CDF_BYTE:
    genericDataWarper.render<uchar>(warper, sourceData, &sourceGeo, drawImage->Geo,
                                    [&](int x, int y, uchar val, GDWState &warperState) { return drawFunction<uchar>(x, y, val, warperState, settings); });
    break;
  case CDF_UBYTE:
    genericDataWarper.render<ubyte>(warper, sourceData, &sourceGeo, drawImage->Geo,
                                    [&](int x, int y, ubyte val, GDWState &warperState) { return drawFunction<ubyte>(x, y, val, warperState, settings); });
    break;
  case CDF_SHORT:
    genericDataWarper.render<short>(warper, sourceData, &sourceGeo, drawImage->Geo,
                                    [&](int x, int y, short val, GDWState &warperState) { return drawFunction<short>(x, y, val, warperState, settings); });
    break;
  case CDF_USHORT:
    genericDataWarper.render<ushort>(warper, sourceData, &sourceGeo, drawImage->Geo,
                                     [&](int x, int y, ushort val, GDWState &warperState) { return drawFunction<ushort>(x, y, val, warperState, settings); });
    break;
  case CDF_INT:
    genericDataWarper.render<int>(warper, sourceData, &sourceGeo, drawImage->Geo, [&](int x, int y, int val, GDWState &warperState) { return drawFunction<int>(x, y, val, warperState, settings); });
    break;
  case CDF_UINT:
    genericDataWarper.render<uint>(warper, sourceData, &sourceGeo, drawImage->Geo, [&](int x, int y, uint val, GDWState &warperState) { return drawFunction<uint>(x, y, val, warperState, settings); });
    break;
  case CDF_INT64:
    genericDataWarper.render<long>(warper, sourceData, &sourceGeo, drawImage->Geo, [&](int x, int y, long val, GDWState &warperState) { return drawFunction<long>(x, y, val, warperState, settings); });
    break;
  case CDF_UINT64:
    genericDataWarper.render<ulong>(warper, sourceData, &sourceGeo, drawImage->Geo,
                                    [&](int x, int y, ulong val, GDWState &warperState) { return drawFunction<ulong>(x, y, val, warperState, settings); });
    break;
  case CDF_FLOAT:
    genericDataWarper.render<float>(warper, sourceData, &sourceGeo, drawImage->Geo,
                                    [&](int x, int y, float val, GDWState &warperState) { return drawFunction<float>(x, y, val, warperState, settings); });
    break;
  case CDF_DOUBLE:
    genericDataWarper.render<double>(warper, sourceData, &sourceGeo, drawImage->Geo,
                                     [&](int x, int y, double val, GDWState &warperState) { return drawFunction<double>(x, y, val, warperState, settings); });
    break;
  }

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
  // CDBDebug("render done");
  return;
}

int CImgWarpGeneric::set(const char *) { return 0; }