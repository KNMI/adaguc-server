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
#include "GenericDataWarper/CGenericDataWarper.h"

const char *CImgWarpHillShaded::className = "CImgWarpHillShaded";

void CImgWarpHillShaded::render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {
  // CDBDebug("render");

  CT::string color;
  void *sourceData;

  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  HillShadeSettings settings;
  settings.dfNodataValue = dataSource->getDataObject(0)->dfNodataValue;
  settings.legendValueRange = (bool)styleConfiguration->hasLegendValueRange;
  settings.legendLowerRange = styleConfiguration->legendLowerRange;
  settings.legendUpperRange = styleConfiguration->legendUpperRange;
  settings.hasNodataValue = dataSource->getDataObject(0)->hasNodataValue;

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

  CGenericDataWarper genericDataWarper;
  switch (dataType) {
  case CDF_CHAR:
    genericDataWarper.render<char>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &hillShadedDrawFunction);
    break;
  case CDF_BYTE:
    genericDataWarper.render<char>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &hillShadedDrawFunction);
    break;
  case CDF_UBYTE:
    genericDataWarper.render<unsigned char>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &hillShadedDrawFunction);
    break;
  case CDF_SHORT:
    genericDataWarper.render<short>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &hillShadedDrawFunction);
    break;
  case CDF_USHORT:
    genericDataWarper.render<ushort>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &hillShadedDrawFunction);
    break;
  case CDF_INT:
    genericDataWarper.render<int>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &hillShadedDrawFunction);
    break;
  case CDF_UINT:
    genericDataWarper.render<uint>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &hillShadedDrawFunction);
    break;
  case CDF_FLOAT:
    genericDataWarper.render<float>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &hillShadedDrawFunction);
    break;
  case CDF_DOUBLE:
    genericDataWarper.render<double>(warper, sourceData, &sourceGeo, drawImage->Geo, &settings, &hillShadedDrawFunction);
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

int CImgWarpHillShaded::set(const char *) { return 0; }

template <class T> void hillShadedDrawFunction(int x, int y, T val, void *_settings, void *_warper) {
  return;
  HillShadeSettings *drawSettings = static_cast<HillShadeSettings *>(_settings);
  if (x < 0 || y < 0 || x > drawSettings->width || y > drawSettings->height) return;
  CGenericDataWarper *genericDataWarper = static_cast<CGenericDataWarper *>(_warper);
  bool isNodata = false;
  if (drawSettings->hasNodataValue) {
    if ((val) == (T)drawSettings->dfNodataValue) isNodata = true;
  }
  if (!(val == val)) isNodata = true;
  if (!isNodata)
    if (drawSettings->legendValueRange)
      if (val < drawSettings->legendLowerRange || val > drawSettings->legendUpperRange) isNodata = true;
  if (!isNodata) {
    T *sourceData = (T *)genericDataWarper->warperState.sourceData;
    size_t sourceDataPX = genericDataWarper->warperState.sourceDataPX;
    size_t sourceDataPY = genericDataWarper->warperState.sourceDataPY;
    size_t sourceDataWidth = genericDataWarper->warperState.sourceDataWidth;
    size_t sourceDataHeight = genericDataWarper->warperState.sourceDataHeight;

    if (sourceDataPY > sourceDataHeight - 1) return;
    if (sourceDataPX > sourceDataWidth - 1) return;

    float values[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

    /* TODO make window size configurable */
    for (int wy = -1; wy < 2; wy++) {
      for (int wx = -1; wx < 2; wx++) {
        values[0][0] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 0 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 0, sourceDataHeight) * sourceDataWidth];
        values[1][0] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 1 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 0, sourceDataHeight) * sourceDataWidth];
        values[2][0] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 2 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 0, sourceDataHeight) * sourceDataWidth];
        values[0][1] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 0 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 1, sourceDataHeight) * sourceDataWidth];
        values[1][1] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 1 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 1, sourceDataHeight) * sourceDataWidth];
        values[2][1] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 2 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 1, sourceDataHeight) * sourceDataWidth];
        values[0][2] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 0 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 2, sourceDataHeight) * sourceDataWidth];
        values[1][2] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 1 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 2, sourceDataHeight) * sourceDataWidth];
        values[2][2] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 2 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 2, sourceDataHeight) * sourceDataWidth];
      }
    }

    Vector v00 = Vector((float)(sourceDataPX + 0), (float)sourceDataPY + 0, values[0][0]);
    Vector v10 = Vector((float)(sourceDataPX + 1), (float)sourceDataPY + 0, values[1][0]);
    Vector v20 = Vector((float)(sourceDataPX + 2), (float)sourceDataPY + 0, values[2][0]);
    Vector v01 = Vector((float)(sourceDataPX + 0), (float)sourceDataPY + 1, values[0][1]);
    Vector v11 = Vector((float)(sourceDataPX + 1), (float)sourceDataPY + 1, values[1][1]);
    Vector v21 = Vector((float)(sourceDataPX + 2), (float)sourceDataPY + 1, values[2][1]);
    Vector v02 = Vector((float)(sourceDataPX + 0), (float)sourceDataPY + 2, values[0][2]);
    Vector v12 = Vector((float)(sourceDataPX + 1), (float)sourceDataPY + 2, values[1][2]);

    if (x >= 0 && y >= 0 && x < (int)drawSettings->width && y < (int)drawSettings->height) {
      Vector normal00 = CrossProduct(v10 - v00, v01 - v00).normalize();
      Vector normal10 = CrossProduct(v20 - v10, v11 - v10).normalize();
      Vector normal01 = CrossProduct(v11 - v01, v02 - v01).normalize();
      Vector normal11 = CrossProduct(v21 - v11, v12 - v11).normalize();
      Vector lightSource = (Vector(-1, -1, -1)).normalize(); /* TODO make light source configurable */
      float c00 = DotProduct(lightSource, normal00);
      float c10 = DotProduct(lightSource, normal10);
      float c01 = DotProduct(lightSource, normal01);
      float c11 = DotProduct(lightSource, normal11);
      float dx = genericDataWarper->warperState.tileDx;
      float dy = genericDataWarper->warperState.tileDy;
      float gx1 = (1 - dx) * c00 + dx * c10;
      float gx2 = (1 - dx) * c01 + dx * c11;
      float bilValue = (1 - dy) * gx1 + dy * gx2;
      drawSettings->dataField[x + y * drawSettings->width] = (bilValue + 1) / 1.816486;
    }
  }
};

template void hillShadedDrawFunction<char>(int x, int y, char val, void *_settings, void *g);
template void hillShadedDrawFunction<unsigned char>(int x, int y, unsigned char val, void *_settings, void *g);
template void hillShadedDrawFunction<short>(int x, int y, short val, void *_settings, void *g);
template void hillShadedDrawFunction<ushort>(int x, int y, ushort val, void *_settings, void *g);
template void hillShadedDrawFunction<int>(int x, int y, int val, void *_settings, void *g);
template void hillShadedDrawFunction<uint>(int x, int y, uint val, void *_settings, void *g);
template void hillShadedDrawFunction<long>(int x, int y, long val, void *_settings, void *g);
template void hillShadedDrawFunction<unsigned long>(int x, int y, unsigned long val, void *_settings, void *g);
template void hillShadedDrawFunction<float>(int x, int y, float val, void *_settings, void *g);
template void hillShadedDrawFunction<double>(int x, int y, double val, void *_settings, void *g);
