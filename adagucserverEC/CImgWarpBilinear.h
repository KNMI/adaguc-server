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

#ifndef CImgWarpBilinear_H
#define CImgWarpBilinear_H
#include <stdlib.h>
#include "CFillTriangle.h"
#include "CImageWarperRenderInterface.h"
#include "Types/DrawContourDefinition.h"
#include "Types/DrawShadeDefinition.h"
#include "Types/DrawPoint.h"

class CalculatedWindVector {
public:
  CalculatedWindVector() {}
  CalculatedWindVector(int x, int y, double dir, double strength, bool convertToKnots, bool flip) {
    this->x = x;
    this->y = y;
    this->dir = dir;
    this->strength = strength;
    this->convertToKnots = convertToKnots;
    this->flip = flip;
  }
  int x, y;
  double dir, strength;
  bool convertToKnots, flip;
};

#define CONTOURDEFINITIONLOOKUPLENGTH 32
#define DISTANCEFIELDTYPE unsigned int
class CImgWarpBilinear : public CImageWarperRenderInterface {
private:
  bool drawMap, enableContour, enableVector, enableBarb, enableShade, drawGridVectors;
  float shadeInterval;
  int smoothingFilter;

  unsigned short checkIfContourRequired(float *val);

  std::vector<ContourDefinition> contourDefinitions;
  std::vector<ShadeDefinition> shadeDefinitions;

  std::vector<PointD *> minimaPoints;
  std::vector<PointD *> maximaPoints;
  DEF_ERRORFUNCTION();
  void drawTextForContourLines(CDrawImage *drawImage, ContourDefinition *contourDefinition, int lineX, int lineY, int endX, int endY, std::vector<Point> *textLocations, float value, CColor textColor,
                               CColor textStrokeColor, const char *fontLocation, float fontSize, float textStrokeWidth);
  void traverseLine(CDrawImage *drawImage, DISTANCEFIELDTYPE *distance, float *valueField, int lineX, int lineY, int dImageWidth, int dImageHeight, float lineWidth, CColor lineColor, CColor textColor,
                    CColor textStrokeColor, ContourDefinition *contourDefinition, DISTANCEFIELDTYPE lineMask, bool drawText, std::vector<Point> *textLocations, double scaling,
                    const char *fontLocation, float fontSize, float textStrokeWidth, double *dashes, int numDashes);

public:
  CImgWarpBilinear() {
    drawMap = false;
    enableContour = false;
    enableVector = false;
    enableBarb = false;
    enableShade = false;
    smoothingFilter = 1;
    drawGridVectors = false;
  }
  ~CImgWarpBilinear() {
    for (size_t j = 0; j < minimaPoints.size(); j++) delete minimaPoints[j];
    for (size_t j = 0; j < maximaPoints.size(); j++) delete maximaPoints[j];
  }
  void render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage);
  int set(const char *pszSettings);

  int getPixelIndexForValue(CDataSource *dataSource, float val) {
    bool isNodata = false;

    if (dataSource->getDataObject(0)->hasNodataValue) {
      if (val == float(dataSource->getDataObject(0)->dfNodataValue)) isNodata = true;
      if (!(val == val)) isNodata = true;
    }
    if (!isNodata) {
      CStyleConfiguration *styleConfiguration = dataSource->getStyle();
      if (styleConfiguration->hasLegendValueRange == 1) {
        if (val < styleConfiguration->legendLowerRange || val > styleConfiguration->legendUpperRange) {
          isNodata = true;
        }
      }
      if (!isNodata) {
        if (styleConfiguration->legendLog != 0) val = log10(val + .000001) / log10(styleConfiguration->legendLog);
        val *= styleConfiguration->legendScale;
        val += styleConfiguration->legendOffset;
        if (val >= 239)
          val = 239;
        else if (val < 0)
          val = 0;
        return int(val);
      }
    }
    return 0;
  }

  static void setValuePixel(CDataSource *dataSource, CDrawImage *drawImage, int destX, int destY, float val) {
    bool isNodata = false;

    if (dataSource->getDataObject(0)->hasNodataValue) {
      if (val == float(dataSource->getDataObject(0)->dfNodataValue)) {
        isNodata = true;
        return;
      }
      if (!(val == val)) {
        isNodata = true;
        return;
      }
    }
    CStyleConfiguration *styleConfiguration = dataSource->getStyle();
    if (!isNodata) {
      if (styleConfiguration->hasLegendValueRange == 1)
        if (val < styleConfiguration->legendLowerRange || val > styleConfiguration->legendUpperRange) isNodata = true;
    }
    if (!isNodata) {
      if (styleConfiguration->legendLog != 0) val = log10(val + .000001) / log10(styleConfiguration->legendLog);
      val *= styleConfiguration->legendScale;
      val += styleConfiguration->legendOffset;
      if (val >= 239)
        val = 239;
      else if (val < 0)
        val = 0;
      drawImage->setPixelIndexed(destX, destY, (unsigned char)val);
    }
  }

  void drawContour(float *valueData, float fNodataValue, float interval, CDataSource *dataSource, CDrawImage *drawImage, bool drawLine, bool drawShade, bool drawText);
};

#endif
