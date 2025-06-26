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

#ifndef CIMGRENDERPOINTS_H
#define CIMGRENDERPOINTS_H
#include "CImageWarperRenderInterface.h"
#include <set>
class CImgRenderPoints : public CImageWarperRenderInterface {
private:
  DEF_ERRORFUNCTION();
  CT::string settings;

  class SimpleSymbol {
  public:
    class Coordinate {
    public:
      Coordinate(float x, float y) {
        this->x = x;
        this->y = y;
      }
      float x, y;
    };
    std::vector<Coordinate> coordinates;
  };

  std::map<std::string, SimpleSymbol> SimpleSymbolMap;

  bool drawVector;
  bool drawPoints;
  bool drawBarb;
  bool drawDiscs;
  bool drawVolume;
  bool drawSymbol;
  bool drawZoomablePoints;
  bool doThinning;
  int thinningRadius;
  int drawPointFontSize;
  float drawPointDiscRadius;
  int drawPointTextRadius;
  bool drawPointDot;
  float drawPointAngleStart;
  float drawPointAngleStep;
  bool useDrawPointAngles;
  bool drawPointPlotStationId;
  const char *drawPointFontFile;
  CT::string drawPointTextFormat;
  CT::string drawPointPointStyle;
  CColor drawPointTextColor;
  CColor drawPointTextOutlineColor;
  CColor drawPointFillColor;
  CColor drawPointLineColor;
  CColor defaultColor;

  CColor drawVectorLineColor;
  float drawVectorLineWidth;
  bool drawVectorPlotStationId;
  bool drawVectorPlotValue;
  float drawVectorVectorScale;
  CT::string drawVectorTextFormat;
  CT::string drawVectorVectorStyle;

  std::set<std::string> usePoints;
  std::set<std::string> skipPoints;
  bool useFilter;
  bool useDrawPointFillColor;
  bool useDrawPointTextColor;
  bool isRadiusAndValue;

  std::vector<CServerConfig::XMLE_SymbolInterval *> *symbolIntervals;

  void renderSinglePoints(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration, CServerConfig::XMLE_Point *pointConfig);
  void renderVectorPoints(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration);

public:
  void render(CImageWarper *, CDataSource *, CDrawImage *);
  int set(const char *);
  int getPixelIndexForValue(CDataSource *dataSource, float val);
  CColor getPixelColorForValue(CDrawImage *drawImage, CDataSource *dataSource, float val);
};
#endif
