/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2013-06-01
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

#include "CImgRenderStippling.h"
#include "CGenericDataWarper.h"
int CImgRenderStippling::set(const char *) { return 0; }

const char *CImgRenderStippling::className = "CImgRenderStippling";

void CImgRenderStippling::_setStippling(int screenX, int screenY, float val) {
  if (val != (float)settings->dfNodataValue && (val == val)) {
    if (styleConfiguration->legendLog != 0) {
      if (val > 0) {
        val = (float)(log10(val) / styleConfiguration->legendLog);
      } else {
        val = (float)(-styleConfiguration->legendOffset);
      }
    }
    int pcolorind = (int)(val * styleConfiguration->legendScale + styleConfiguration->legendOffset);
    int pcolorindOrg = pcolorind;
    if (pcolorind >= 239)
      pcolorind = 239;
    else if (pcolorind <= 0)
      pcolorind = 0;
    CColor c;
    if (color.empty()) {
      c = drawImage->getColorForIndex(pcolorind);
    } else {
      c.parse(color.c_str());
    }
    if (mode == CImgRenderStipplingModeDefault) {
      drawImage->setDisc(screenX, screenY, discSize, c, c);
    } else if (mode == CImgRenderStipplingModeThreshold) {
      if (pcolorindOrg >= 0 && pcolorindOrg < 239) {
        drawImage->setDisc(screenX, screenY, discSize, c, c);
      }
    }
  }
}

template <class T> void CImgRenderStippling::_stippleMiddleOfPoints() {
  double bboxWidth = (dataSource->srvParams->geoParams.bbox.right - dataSource->srvParams->geoParams.bbox.left);
  double bboxHeight = (dataSource->srvParams->geoParams.bbox.top - dataSource->srvParams->geoParams.bbox.bottom);

  for (int y = 0; y < dataSource->dHeight; y++) {
    for (int x = 0; x < dataSource->dWidth; x++) {
      double cx = dataSource->dfBBOX[0] + dataSource->dfCellSizeX * double(x) + dataSource->dfCellSizeX / 2;
      double cy = dataSource->dfBBOX[3] + dataSource->dfCellSizeY * double(y) + dataSource->dfCellSizeY / 2;
      warper->reprojpoint_inv(cx, cy);
      cx -= dataSource->srvParams->geoParams.bbox.left;
      cx /= bboxWidth;
      cx *= double(dataSource->srvParams->geoParams.width);
      cy -= dataSource->srvParams->geoParams.bbox.bottom;
      cy /= bboxHeight;
      cy = 1 - cy;
      cy *= double(dataSource->srvParams->geoParams.height);
      int dx = cx, dy = cy;
      T val = ((T *)sourceData)[x + y * dataSource->dWidth];
      CDBDebug("%f", (float)val);
      if (val == val && val != settings->dfNodataValue) {

        _setStippling(dx, dy, (float)val);
      }
    }
  }
}

// Setup projection and all other settings for the tiles to draw
void CImgRenderStippling::render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {
  CDBDebug("render");
  this->drawImage = drawImage;
  this->dataSource = dataSource;
  this->warper = warper;
  CStipplingSettings settings;
  this->settings = &settings;
  styleConfiguration = dataSource->getStyle();
  settings.dfNodataValue = dataSource->getFirstAvailableDataObject()->dfNodataValue;
  settings.legendValueRange = styleConfiguration->hasLegendValueRange;
  settings.legendLowerRange = styleConfiguration->legendLowerRange;
  settings.legendUpperRange = styleConfiguration->legendUpperRange;
  settings.hasNodataValue = dataSource->getFirstAvailableDataObject()->hasNodataValue;

  if (!settings.hasNodataValue) {
    settings.hasNodataValue = true;
    settings.dfNodataValue = -100000.f;
  }
  settings.width = drawImage->geoParams.width;
  settings.height = drawImage->geoParams.height;

  settings.dataField = new float[settings.width * settings.height];
  for (size_t y = 0; y < settings.height; y++) {
    for (size_t x = 0; x < settings.width; x++) {
      settings.dataField[x + y * settings.width] = (float)settings.dfNodataValue;
    }
  }

  CDFType dataType = dataSource->getFirstAvailableDataObject()->cdfVariable->getType();
  sourceData = dataSource->getFirstAvailableDataObject()->cdfVariable->data;
  GeoParameters sourceGeo;

  sourceGeo.width = dataSource->dWidth;
  sourceGeo.height = dataSource->dHeight;
  sourceGeo.bbox = dataSource->dfBBOX;
  sourceGeo.cellsizeX = dataSource->dfCellSizeX;
  sourceGeo.cellsizeY = dataSource->dfCellSizeY;
  sourceGeo.crs = dataSource->nativeProj4;

  xDistance = 22;
  yDistance = 22;
  discSize = 6;
  mode = CImgRenderStipplingModeDefault; // Mode 0 is standard stippling

  if (styleConfiguration != nullptr) {
    for (auto stippling : styleConfiguration->stipplingList) {
      if (!stippling->attr.distancex.empty()) {
        xDistance = stippling->attr.distancex.toInt();
      }
      if (!stippling->attr.distancey.empty()) {
        yDistance = stippling->attr.distancey.toInt();
      }
      if (!stippling->attr.discradius.empty()) {
        discSize = stippling->attr.discradius.toInt();
      }
      if (!stippling->attr.mode.empty()) {
        CT::string smode = stippling->attr.mode;
        if (smode.equals("threshold")) {
          mode = CImgRenderStipplingModeThreshold;
        }
      }
      if (!stippling->attr.color.empty()) {
        color = stippling->attr.color;
      }
    }
  }

  double bboxWidth = (dataSource->srvParams->geoParams.bbox.right - dataSource->srvParams->geoParams.bbox.left);
  double bboxHeight = (dataSource->srvParams->geoParams.bbox.top - dataSource->srvParams->geoParams.bbox.bottom);

  /*
   * Set stippling exactly in the middle of grid points
   */

  if (yDistance == 0 || xDistance == 0) {
    switch (dataType) {
    case CDF_CHAR:
      _stippleMiddleOfPoints<char>();
      break;
    case CDF_BYTE:
      _stippleMiddleOfPoints<char>();
      break;
    case CDF_UBYTE:
      _stippleMiddleOfPoints<unsigned char>();
      break;
    case CDF_SHORT:
      _stippleMiddleOfPoints<short>();
      break;
    case CDF_USHORT:
      _stippleMiddleOfPoints<unsigned short>();
      break;
    case CDF_INT:
      _stippleMiddleOfPoints<int>();
      break;
    case CDF_UINT:
      _stippleMiddleOfPoints<unsigned int>();
      break;
    case CDF_FLOAT:
      _stippleMiddleOfPoints<float>();
      break;
    case CDF_DOUBLE:
      _stippleMiddleOfPoints<double>();
      break;
    }
    return;
  }

  /*
   * Warp the data to a new grid and plot hatching in screencoordinates.
   */

  int startX = int(((-dataSource->srvParams->geoParams.bbox.left) / bboxWidth) * dataSource->srvParams->geoParams.width);
  startX = (startX % (xDistance * 2)) - (xDistance * 2);
  int startY = int(((dataSource->srvParams->geoParams.bbox.bottom) / bboxHeight) * dataSource->srvParams->geoParams.height);
  startY = (startY % (yDistance * 2)) - (yDistance * 2);

  GenericDataWarper genericDataWarper;
  GDWArgs args = {.warper = warper, .sourceData = sourceData, .sourceGeoParams = sourceGeo, .destGeoParams = drawImage->geoParams};

#define RENDER(CDFTYPE, CPPTYPE)                                                                                                                                                                       \
  if (dataType == CDFTYPE) genericDataWarper.render<CPPTYPE>(args, [&](int x, int y, CPPTYPE val, GDWState &warperState) { return drawFunction(x, y, val, warperState, settings); });
  ENUMERATE_OVER_CDFTYPES(RENDER)
#undef RENDER

  for (int y = startY; y < (int)settings.height; y = y + yDistance) {
    int oddeven = 0;
    if ((y - startY) % (yDistance * 2) == 0) {
      oddeven = 1;
    }
    for (int x1 = startX; x1 < (int)settings.width; x1 = x1 + xDistance) {
      int x = (x1) + ((oddeven) * (xDistance / 2));
      if (x >= 0 && y >= 0 && x < ((int)settings.width) && y < ((int)settings.height)) {
        float val = settings.dataField[x + y * settings.width];
        _setStippling(x, y, val);
      }
    }
  }
  settings.dataField = new float[settings.width * settings.height];

  delete[] settings.dataField;
  return;
}
