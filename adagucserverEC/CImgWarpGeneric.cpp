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
#include "CImageOperators/smoothRasterField.h"

const char *CImgWarpGeneric::className = "CImgWarpGeneric";

void CImgWarpGeneric::render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {
  // CDBDebug("render");

  CT::string color;
  void *sourceData;

  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  // Settings settings;
  CDrawFunctionSettings settings = getDrawFunctionSettings(dataSource, drawImage, styleConfiguration);
  // settings.dfNodataValue = dataSource->getDataObject(0)->dfNodataValue;
  // settings.legendValueRange = (bool)styleConfiguration->hasLegendValueRange;
  // settings.legendLowerRange = styleConfiguration->legendLowerRange;
  // settings.legendUpperRange = styleConfiguration->legendUpperRange;
  // settings.hasNodataValue = dataSource->getDataObject(0)->hasNodataValue;

  // if (!settings.hasNodataValue) {
  //   settings.hasNodataValue = true;
  //   settings.dfNodataValue = -100000.f;
  // }
  // settings.width = drawImage->Geo->dWidth;
  // settings.height = drawImage->Geo->dHeight;

  // float *dataField = new float[settings.drawImage->Geo->dWidth * settings.drawImage->Geo->dHeight];
  // for (int y = 0; y < settings.drawImage->Geo->dHeight; y++) {
  //   for (int x = 0; x < settings.drawImage->Geo->dWidth; x++) {
  //     dataField[x + y * settings.drawImage->Geo->dWidth] = (float)settings.dfNodataValue;
  //   }
  // }

  CDFType dataType = dataSource->getDataObject(0)->cdfVariable->getType();
  sourceData = dataSource->getDataObject(0)->cdfVariable->data;
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
  genericDataWarper.useHalfCellOffset = true;
  switch (dataType) {
  case CDF_CHAR:
    genericDataWarper.render<char>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &drawFunction);
    break;
  case CDF_BYTE:
    genericDataWarper.render<char>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &drawFunction);
    break;
  case CDF_UBYTE:
    genericDataWarper.render<unsigned char>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &drawFunction);
    break;
  case CDF_SHORT:
    genericDataWarper.render<short>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &drawFunction);
    break;
  case CDF_USHORT:
    genericDataWarper.render<ushort>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &drawFunction);
    break;
  case CDF_INT:
    genericDataWarper.render<int>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &drawFunction);
    break;
  case CDF_UINT:
    genericDataWarper.render<uint>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &drawFunction);
    break;
  case CDF_FLOAT:
    genericDataWarper.render<float>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &drawFunction);
    break;
  case CDF_DOUBLE:
    genericDataWarper.render<double>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &drawFunction);
    break;
  }

  // for (int y = 0; y < settings.drawImage->Geo->dHeight; y = y + 1) {
  //   for (int x = 0; x < settings.drawImage->Geo->dWidth; x = x + 1) {
  //     float val = dataField[x + y * settings.drawImage->Geo->dWidth];
  //     if (val != (float)settings.dfNodataValue && val == val) {
  //       if (styleConfiguration->legendLog != 0) val = log10(val + .000001) / log10(styleConfiguration->legendLog);
  //       val *= styleConfiguration->legendScale;
  //       val += styleConfiguration->legendOffset;
  //       if (val >= 239)
  //         val = 239;
  //       else if (val < 0)
  //         val = 0;
  //       drawImage->setPixelIndexed(x, y, (unsigned char)val);
  //     }
  //   }
  // }
  // delete[] dataField;
  // CDBDebug("render done");
  return;
}

int CImgWarpGeneric::set(const char *) { return 0; }