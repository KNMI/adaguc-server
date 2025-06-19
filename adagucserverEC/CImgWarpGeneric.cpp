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
  CImgWarpGenericDrawFunctionState drawFunctionSettings;
  drawFunctionSettings.dfNodataValue = dataSource->getFirstAvailableDataObject()->dfNodataValue;
  drawFunctionSettings.legendValueRange = (bool)styleConfiguration->hasLegendValueRange;
  drawFunctionSettings.legendLowerRange = styleConfiguration->legendLowerRange;
  drawFunctionSettings.legendUpperRange = styleConfiguration->legendUpperRange;
  drawFunctionSettings.hasNodataValue = dataSource->getFirstAvailableDataObject()->hasNodataValue;

  if (!drawFunctionSettings.hasNodataValue) {
    drawFunctionSettings.hasNodataValue = true;
    drawFunctionSettings.dfNodataValue = -100000.f;
  }
  drawFunctionSettings.width = drawImage->Geo->dWidth;
  drawFunctionSettings.height = drawImage->Geo->dHeight;

  drawFunctionSettings.dataField = new float[drawFunctionSettings.width * drawFunctionSettings.height];
  for (int y = 0; y < drawFunctionSettings.height; y++) {
    for (int x = 0; x < drawFunctionSettings.width; x++) {
      drawFunctionSettings.dataField[x + y * drawFunctionSettings.width] = (float)drawFunctionSettings.dfNodataValue;
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
  drawFunctionSettings.useHalfCellOffset = true;
  switch (dataType) {
  case CDF_CHAR:
    genericDataWarper.render<char>(warper, sourceData, &sourceGeo, drawImage->Geo, &drawFunctionSettings, &drawFunction);
    break;
  case CDF_BYTE:
    genericDataWarper.render<char>(warper, sourceData, &sourceGeo, drawImage->Geo, &drawFunctionSettings, &drawFunction);
    break;
  case CDF_UBYTE:
    genericDataWarper.render<unsigned char>(warper, sourceData, &sourceGeo, drawImage->Geo, &drawFunctionSettings, &drawFunction);
    break;
  case CDF_SHORT:
    genericDataWarper.render<short>(warper, sourceData, &sourceGeo, drawImage->Geo, &drawFunctionSettings, &drawFunction);
    break;
  case CDF_USHORT:
    genericDataWarper.render<ushort>(warper, sourceData, &sourceGeo, drawImage->Geo, &drawFunctionSettings, &drawFunction);
    break;
  case CDF_INT:
    genericDataWarper.render<int>(warper, sourceData, &sourceGeo, drawImage->Geo, &drawFunctionSettings, &drawFunction);
    break;
  case CDF_UINT:
    genericDataWarper.render<uint>(warper, sourceData, &sourceGeo, drawImage->Geo, &drawFunctionSettings, &drawFunction);
    break;
  case CDF_FLOAT:
    genericDataWarper.render<float>(warper, sourceData, &sourceGeo, drawImage->Geo, &drawFunctionSettings, &drawFunction);
    break;
  case CDF_DOUBLE:
    genericDataWarper.render<double>(warper, sourceData, &sourceGeo, drawImage->Geo, &drawFunctionSettings, &drawFunction);
    break;
  }

  for (int y = 0; y < (int)drawFunctionSettings.height; y = y + 1) {
    for (int x = 0; x < (int)drawFunctionSettings.width; x = x + 1) {
      float val = drawFunctionSettings.dataField[x + y * drawFunctionSettings.width];
      if (val != (float)drawFunctionSettings.dfNodataValue && val == val) {
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
  delete[] drawFunctionSettings.dataField;
  // CDBDebug("render done");
  return;
}

int CImgWarpGeneric::set(const char *) { return 0; }