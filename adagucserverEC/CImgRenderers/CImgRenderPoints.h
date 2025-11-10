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
#include "getPointStyle.h"
#include <set>

class CImgRenderPoints : public CImageWarperRenderInterface {
private:
  DEF_ERRORFUNCTION();
  bool drawPoints;
  bool drawZoomablePoints;
  const char *drawPointFontFile;
  CT::string drawPointPointStyle;
  CColor defaultColor;
  bool isRadiusAndValue;

  void renderSinglePoints(std::vector<size_t> thinnedPointIndexList, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration, PointStyle pointStyle);
  void renderSingleVolumes(std::vector<size_t> thinnedPointIndexList, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration, PointStyle pointStyle);
  void renderSingleSymbols(std::vector<size_t> thinnedPointIndexList, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration, PointStyle pointStyle);
  void renderSingleDiscs(std::vector<size_t> thinnedPointIndexList, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration, PointStyle pointStyle);
  void renderSingleDot(std::vector<size_t> thinnedPointIndexList, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration, PointStyle pointStyle);

public:
  void render(CImageWarper *, CDataSource *, CDrawImage *);
  int set(const char *) { return 0; }; // Deprecated.
};
#endif

int getPixelIndexForValue(CDataSource *dataSource, float val);
CColor getPixelColorForValue(CDrawImage *drawImage, CDataSource *dataSource, float val);