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
#include "CGenericDataWarper.h"
#include "CImgWarpGeneric.h"

const char *CImgWarpHillShaded::className = "CImgWarpHillShaded";

void CImgWarpHillShaded::render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {
  CT::string color;

  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  CImgWarpGenericDrawFunctionState drawFunctionState;
  drawFunctionState.dfNodataValue = dataSource->getDataObject(0)->dfNodataValue;
  drawFunctionState.legendValueRange = (bool)styleConfiguration->hasLegendValueRange;
  drawFunctionState.legendLowerRange = styleConfiguration->legendLowerRange;
  drawFunctionState.legendUpperRange = styleConfiguration->legendUpperRange;
  drawFunctionState.hasNodataValue = dataSource->getDataObject(0)->hasNodataValue;

  if (!drawFunctionState.hasNodataValue) {
    drawFunctionState.hasNodataValue = true;
    drawFunctionState.dfNodataValue = -100000.f;
  }
  drawFunctionState.width = drawImage->Geo->dWidth;
  drawFunctionState.height = drawImage->Geo->dHeight;

  drawFunctionState.dataField = new float[drawFunctionState.width * drawFunctionState.height];
  for (int y = 0; y < drawFunctionState.height; y++) {
    for (int x = 0; x < drawFunctionState.width; x++) {
      drawFunctionState.dataField[x + y * drawFunctionState.width] = (float)drawFunctionState.dfNodataValue;
    }
  }

  CDFType dataType = dataSource->getDataObject(0)->cdfVariable->getType();
  void *sourceData = dataSource->getDataObject(0)->cdfVariable->data;
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
  GDWArgs args = {.warper = warper, .sourceData = sourceData, .sourceGeoParams = &sourceGeo, .destGeoParams = drawImage->Geo};

#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                               \
  if (dataType == CDFTYPE) genericDataWarper.render<CPPTYPE>(args, [&](int x, int y, CPPTYPE val, GDWState &warperState) { return drawFunction(x, y, val, warperState, drawFunctionState); });
ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER

  for (int y = 0; y < (int)drawFunctionState.height; y = y + 1) {
    for (int x = 0; x < (int)drawFunctionState.width; x = x + 1) {
      float val = drawFunctionState.dataField[x + y * drawFunctionState.width];
      if (val != (float)drawFunctionState.dfNodataValue && val == val) {
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
  delete[] drawFunctionState.dataField;
  // CDBDebug("render done");
  return;
}

int CImgWarpHillShaded::set(const char *) { return 0; }