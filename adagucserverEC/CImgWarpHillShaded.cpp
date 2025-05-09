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

/**
 * Vector of x,y,z
 */
struct f8vector {
  double x, y, z;
  f8vector operator-(const f8vector &v) { return f8vector({.x = x - v.x, .y = y - v.y, .z = z - v.z}); }
  double square() { return x * x + y * y + z * z; }
  double magnitude() { return sqrt(x * x + y * y + z * z); }
  f8vector norm() {
    double f = magnitude();
    if (f == 0) return f8vector({.x = 0, .y = 0, .z = 0});
    return f8vector({.x = x / f, .y = y / f, .z = z / f});
  }
};

/**
 * Calculate dot product of vector a and b
 */
inline double dot(const f8vector &a, const f8vector &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

/**
 * Calculate cross product of a and b
 */
f8vector cross(const f8vector &v1, const f8vector &v2) { return f8vector({.x = v1.y * v2.z - v1.z * v2.y, .y = v1.z * v2.x - v1.x * v2.z, .z = v1.x * v2.y - v1.y * v2.x}); }

/**
 * Lightsource
 */
const f8vector lightSource = (f8vector({.x = -1, .y = -1, .z = -1})).norm();

struct HillShadeSettings : GDWWarperState {
  double legendValueRange;
  double legendLowerRange;
  double legendUpperRange;
};

void hillShadedDrawFunction(GDWWarperState *_drawSettings) {
  HillShadeSettings *drawSettings = (HillShadeSettings *)_drawSettings;
  if (drawSettings->destX < 0 || drawSettings->destY < 0 || drawSettings->destX > drawSettings->destDataWidth || drawSettings->destY > drawSettings->destDataHeight) return;

  double val = drawSettings->getValueFromSourceFunction(drawSettings->sourceDataPX, drawSettings->sourceDataPY, drawSettings);

  ;
  bool isNodata = false;
  if (drawSettings->hasNodataValue) {
    if (val == drawSettings->dfNodataValue) isNodata = true;
  }
  if (!(val == val)) isNodata = true;
  if (!isNodata)
    if (drawSettings->legendValueRange)
      if (val < drawSettings->legendLowerRange || val > drawSettings->legendUpperRange) isNodata = true;
  if (!isNodata) {
    int sourceDataPX = drawSettings->sourceDataPX;
    int sourceDataPY = drawSettings->sourceDataPY;
    int sourceDataWidth = drawSettings->sourceDataWidth;
    int sourceDataHeight = drawSettings->sourceDataHeight;

    if (sourceDataPY > sourceDataHeight - 1) return;
    if (sourceDataPX > sourceDataWidth - 1) return;

    double values[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

    /* TODO make window size configurable */
    for (int wy = -1; wy < 2; wy++) {
      int y0 = (sourceDataPY + 0 + wy) % sourceDataHeight;
      int y1 = (sourceDataPY + 1 + wy) % sourceDataHeight;
      int y2 = (sourceDataPY + 2 + wy) % sourceDataHeight;
      for (int wx = -1; wx < 2; wx++) {
        int x0 = (sourceDataPX + 0 + wx) % sourceDataWidth;
        int x1 = (sourceDataPX + 1 + wx) % sourceDataWidth;
        int x2 = (sourceDataPX + 2 + wx) % sourceDataWidth;

        values[0][0] += drawSettings->getValueFromSourceFunction(x0, y0, drawSettings);
        values[1][0] += drawSettings->getValueFromSourceFunction(x1, y0, drawSettings);
        values[2][0] += drawSettings->getValueFromSourceFunction(x2, y0, drawSettings);

        values[0][1] += drawSettings->getValueFromSourceFunction(x0, y1, drawSettings);
        values[1][1] += drawSettings->getValueFromSourceFunction(x1, y1, drawSettings);
        values[2][1] += drawSettings->getValueFromSourceFunction(x2, y1, drawSettings);

        values[0][2] += drawSettings->getValueFromSourceFunction(x0, y2, drawSettings);
        values[1][2] += drawSettings->getValueFromSourceFunction(x1, y2, drawSettings);
        values[2][2] += drawSettings->getValueFromSourceFunction(x2, y2, drawSettings);
      }
    }

    double sdpx = sourceDataPX;
    double sdpy = sourceDataPY;
    f8vector v00 = {.x = sdpx + 0, .y = sdpy + 0, .z = values[0][0]};
    f8vector v10 = {.x = sdpx + 1, .y = sdpy + 0, .z = values[1][0]};
    f8vector v20 = {.x = sdpx + 2, .y = sdpy + 0, .z = values[2][0]};
    f8vector v01 = {.x = sdpx + 0, .y = sdpy + 1, .z = values[0][1]};
    f8vector v11 = {.x = sdpx + 1, .y = sdpy + 1, .z = values[1][1]};
    f8vector v21 = {.x = sdpx + 2, .y = sdpy + 1, .z = values[2][1]};
    f8vector v02 = {.x = sdpx + 0, .y = sdpy + 2, .z = values[0][2]};
    f8vector v12 = {.x = sdpx + 1, .y = sdpy + 2, .z = values[1][2]};

    f8vector normal00 = cross(v10 - v00, v01 - v00).norm();
    f8vector normal10 = cross(v20 - v10, v11 - v10).norm();
    f8vector normal01 = cross(v11 - v01, v02 - v01).norm();
    f8vector normal11 = cross(v21 - v11, v12 - v11).norm();

    double c00 = dot(lightSource, normal00);
    double c10 = dot(lightSource, normal10);
    double c01 = dot(lightSource, normal01);
    double c11 = dot(lightSource, normal11);
    double dx = drawSettings->tileDx;
    double dy = drawSettings->tileDy;
    double gx1 = (1 - dx) * c00 + dx * c10;
    double gx2 = (1 - dx) * c01 + dx * c11;
    double bilValue = (1 - dy) * gx1 + dy * gx2;
    ((double *)drawSettings->destinationGrid)[drawSettings->destX + drawSettings->destY * drawSettings->destDataWidth] = (bilValue + 1) / 1.816486;
  }
};

void CImgWarpHillShaded::render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {
  CT::string color;
  void *sourceData;

  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  HillShadeSettings settings;
  settings.dfNodataValue = dataSource->getDataObject(0)->dfNodataValue;
  settings.legendValueRange = (bool)styleConfiguration->hasLegendValueRange;
  settings.legendLowerRange = styleConfiguration->legendLowerRange;
  settings.legendUpperRange = styleConfiguration->legendUpperRange;
  settings.hasNodataValue = dataSource->getDataObject(0)->hasNodataValue;
  settings.destDataWidth = drawImage->Geo->dWidth;
  settings.destDataHeight = drawImage->Geo->dHeight;
  settings.setValueInDestinationFunction = hillShadedDrawFunction;

  if (!settings.hasNodataValue) {
    settings.hasNodataValue = true;
    settings.dfNodataValue = -100000.f;
  }

  settings.destinationGrid = new double[settings.destDataWidth * settings.destDataHeight];
  for (int y = 0; y < settings.destDataHeight; y++) {
    for (int x = 0; x < settings.destDataWidth; x++) {
      ((double *)settings.destinationGrid)[x + y * settings.destDataWidth] = (double)settings.dfNodataValue;
    }
  }

  CDFType dataType = dataSource->getDataObject(0)->cdfVariable->getType();
  sourceData = dataSource->getDataObject(0)->cdfVariable->data;
  CGeoParams sourceGeo(dataSource);

  warp(warper, sourceData, dataType, &sourceGeo, drawImage->Geo, &settings);

  for (int y = 0; y < settings.destDataHeight; y = y + 1) {
    for (int x = 0; x < settings.destDataWidth; x = x + 1) {
      double val = ((double *)settings.destinationGrid)[x + y * settings.destDataWidth];
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
  delete[] ((double *)settings.destinationGrid);
  // CDBDebug("render done");
  return;
}

int CImgWarpHillShaded::set(const char *) { return 0; }
