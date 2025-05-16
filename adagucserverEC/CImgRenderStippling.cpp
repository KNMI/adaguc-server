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
  double bboxWidth = (dataSource->srvParams->Geo->dfBBOX[2] - dataSource->srvParams->Geo->dfBBOX[0]);
  double bboxHeight = (dataSource->srvParams->Geo->dfBBOX[3] - dataSource->srvParams->Geo->dfBBOX[1]);

  for (int y = 0; y < dataSource->dHeight; y++) {
    for (int x = 0; x < dataSource->dWidth; x++) {
      double cx = dataSource->dfBBOX[0] + dataSource->dfCellSizeX * double(x) + dataSource->dfCellSizeX / 2;
      double cy = dataSource->dfBBOX[3] + dataSource->dfCellSizeY * double(y) + dataSource->dfCellSizeY / 2;
      warper->reprojpoint_inv(cx, cy);
      cx -= dataSource->srvParams->Geo->dfBBOX[0];
      cx /= bboxWidth;
      cx *= double(dataSource->srvParams->Geo->dWidth);
      cy -= dataSource->srvParams->Geo->dfBBOX[1];
      cy /= bboxHeight;
      cy = 1 - cy;
      cy *= double(dataSource->srvParams->Geo->dHeight);
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
  Settings settings;
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
  settings.width = drawImage->Geo->dWidth;
  settings.height = drawImage->Geo->dHeight;

  settings.dataField = new float[settings.width * settings.height];
  for (size_t y = 0; y < settings.height; y++) {
    for (size_t x = 0; x < settings.width; x++) {
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

  xDistance = 22;
  yDistance = 22;
  discSize = 6;
  mode = CImgRenderStipplingModeDefault; // Mode 0 is standard stippling

  if (styleConfiguration != NULL && styleConfiguration->styleConfig != NULL) {
    if (styleConfiguration->styleConfig->Stippling.size() == 1) {
      if (!styleConfiguration->styleConfig->Stippling[0]->attr.distancex.empty()) {
        xDistance = styleConfiguration->styleConfig->Stippling[0]->attr.distancex.toInt();
      }
      if (!styleConfiguration->styleConfig->Stippling[0]->attr.distancey.empty()) {
        yDistance = styleConfiguration->styleConfig->Stippling[0]->attr.distancey.toInt();
      }
      if (!styleConfiguration->styleConfig->Stippling[0]->attr.discradius.empty()) {
        discSize = styleConfiguration->styleConfig->Stippling[0]->attr.discradius.toInt();
      }
      if (!styleConfiguration->styleConfig->Stippling[0]->attr.mode.empty()) {
        CT::string smode = styleConfiguration->styleConfig->Stippling[0]->attr.mode;
        if (smode.equals("threshold")) {
          mode = CImgRenderStipplingModeThreshold;
        }
      }
      if (!styleConfiguration->styleConfig->Stippling[0]->attr.color.empty()) {
        color = styleConfiguration->styleConfig->Stippling[0]->attr.color;
      }
    }
  }

  double bboxWidth = (dataSource->srvParams->Geo->dfBBOX[2] - dataSource->srvParams->Geo->dfBBOX[0]);
  double bboxHeight = (dataSource->srvParams->Geo->dfBBOX[3] - dataSource->srvParams->Geo->dfBBOX[1]);

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

  int startX = int(((-dataSource->srvParams->Geo->dfBBOX[0]) / bboxWidth) * dataSource->srvParams->Geo->dWidth);
  startX = (startX % (xDistance * 2)) - (xDistance * 2);
  int startY = int(((dataSource->srvParams->Geo->dfBBOX[1]) / bboxHeight) * dataSource->srvParams->Geo->dHeight);
  startY = (startY % (yDistance * 2)) - (yDistance * 2);

  GenericDataWarper genericDataWarper;
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
