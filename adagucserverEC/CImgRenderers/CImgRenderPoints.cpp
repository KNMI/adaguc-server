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

#include <unordered_set>
#include "CImgRenderPoints.h"
#include "getVectorStyle.h"

const char *CImgRenderPoints::className = "CImgRenderPoints";

void drawTextsForVector(CDrawImage *drawImage, CDataSource *dataSource, VectorStyle &vectorStyle, PointDVWithLatLon *pointStrength, PointDVWithLatLon *pointDirection) {
  // Draw station id
  auto strength = pointStrength->v;
  auto direction = pointDirection->v;
  int x = pointStrength->x;
  int y = dataSource->srvParams->Geo->dHeight - pointStrength->y;

  if (vectorStyle.drawVectorPlotStationId && pointStrength->paramList.size() > 0) {
    int newY = y;
    if (vectorStyle.drawBarb) {
      newY = ((direction >= 90) && (direction <= 270)) ? y - 20 : y + 6;
    } else {
      newY = ((direction >= 90) && (direction <= 270)) ? y + 6 : y - 20;
    }
    CT::string stationId = pointStrength->paramList[0].value;
    drawImage->setText(stationId.c_str(), stationId.length(), x - stationId.length() * 3, newY, vectorStyle.textColor, 0);
  }

  // Draw value for vector or disc
  if (vectorStyle.drawVectorPlotValue && !vectorStyle.drawBarb) {
    if (!vectorStyle.drawDiscs) {
      int newY = ((direction >= 90) && (direction <= 270)) ? y - 20 : y + 6;
      CT::string textValue;
      textValue.print(vectorStyle.drawVectorTextFormat.c_str(), strength);
      drawImage->setText(textValue.c_str(), textValue.length(), x - textValue.length() * 3, newY, vectorStyle.textColor, 0);
    }
  }
}

struct ThinningInfo {
  bool doThinning = false;
  int thinningRadius = 25;
};

ThinningInfo getThinningInfo(CServerConfig::XMLE_Style *s) {
  ThinningInfo info;
  if (s->Thinning.size() == 1 && !s->Thinning[0]->attr.radius.empty()) {
    info.doThinning = true;
    info.thinningRadius = s->Thinning[0]->attr.radius.toInt();
  }
  return info;
}

std::vector<size_t> doThinningGetIndices(std::vector<PointDVWithLatLon> &p1, bool doThinning, double thinningRadius, std::unordered_set<std::string> usePoints) {
  size_t numberOfPoints = p1.size();

  // Filter the points
  if (usePoints.size() > 0) {
    std::vector<size_t> filteredPointIndices;
    for (size_t pointIndex = 0; pointIndex < numberOfPoints; pointIndex++) {
      if (p1[pointIndex].paramList.size() > 0 && usePoints.find(p1[pointIndex].paramList[0].value.c_str()) != usePoints.end()) {
        filteredPointIndices.push_back(pointIndex);
      }
    }
    // Don't do extra thinning when use is set.
    return filteredPointIndices;
  }

  std::vector<size_t> filteredPointIndices;
  for (size_t pointIndex = 0; pointIndex < numberOfPoints; pointIndex++) {
    filteredPointIndices.push_back(pointIndex);
  }
  if (!doThinning) {
    // Use all indices instead
    return filteredPointIndices;
  }

  // Check for thinning
  std::vector<size_t> thinnedPointIndexList;
  thinnedPointIndexList.push_back(0); // Always put in first element
  for (size_t pointIndex = 1; pointIndex < filteredPointIndices.size(); pointIndex++) {
    size_t thinnedPointNr = 0;
    for (thinnedPointNr = 0; thinnedPointNr < thinnedPointIndexList.size(); thinnedPointNr++) {
      auto thinnedPointIndex = thinnedPointIndexList[thinnedPointNr];
      if (hypot(p1[thinnedPointIndex].x - p1[pointIndex].x, p1[thinnedPointIndex].y - p1[pointIndex].y) < thinningRadius) break;
    }
    if (thinnedPointNr == thinnedPointIndexList.size()) thinnedPointIndexList.push_back(pointIndex);
  }
  return thinnedPointIndexList;
}

void renderVectorPoints(std::vector<size_t> thinnedPointIndexList, CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration) {

  if (styleConfiguration == nullptr || styleConfiguration->styleConfig == nullptr) {
    return;
  }
  auto s = styleConfiguration->styleConfig;
  if (s->Vector.size() == 0) {
    return;
  }

  if (dataSource->getNumDataObjects() < 2) {
    CDBError("Cannot draw barbs, not enough data objects");
    throw __LINE__;
  }

  std::vector<PointDVWithLatLon> *p1 = &dataSource->getDataObject(0)->points;
  std::vector<PointDVWithLatLon> *p2 = &dataSource->getDataObject(1)->points;
  size_t numberOfPoints = p1->size();

  if (p2->size() != numberOfPoints) {
    CDBError("Cannot draw barbs, lists for speed (%d) and direction (%d) don't have equal length ", numberOfPoints, p2->size());
    throw __LINE__;
  }

  CT::string textValue;

  float fillValueObjectOne = dataSource->getDataObject(0)->hasNodataValue ? dataSource->getDataObject(0)->dfNodataValue : NAN;
  float fillValueObjectTwo = dataSource->getDataObject(1)->hasNodataValue ? dataSource->getDataObject(1)->dfNodataValue : NAN;

  CT::string units = dataSource->getDataObject(0)->getUnits();
  bool toKnots = false;
  if (!(units.equalsIgnoreCase("kt") || units.equalsIgnoreCase("kts") || units.equalsIgnoreCase("knot"))) {
    toKnots = true;
  }

  // Make a list of vector style objects based on the configuration.
  std::vector<VectorStyle> vectorStyles;
  for (auto cfgVectorStyle : s->Vector) {
    vectorStyles.push_back(getVectorStyle(cfgVectorStyle, dataSource->srvParams->cfg));
  }

  for (auto pointIndex : thinnedPointIndexList) {
    auto pointStrength = &(*p1)[pointIndex];
    auto pointDirection = &(*p2)[pointIndex];
    auto strength = pointStrength->v;
    auto direction = pointDirection->v;
    if (!(direction == direction) || !(strength == strength) || strength == fillValueObjectOne || direction == fillValueObjectTwo) continue;
    int x = pointStrength->x;
    int y = dataSource->srvParams->Geo->dHeight - pointStrength->y;
    double lat = pointStrength->lat;
    // Adjust direction based on projection settings
    direction += warper->getRotation(*pointStrength);

    for (auto vectorStyle : vectorStyles) {
      if (!(strength >= vectorStyle.min && strength < vectorStyle.max)) continue;
      // Draw symbol barb, vector or disc.
      if (vectorStyle.drawBarb) {
        drawImage->drawBarb(x, y, ((270 - direction) / 360) * M_PI * 2, 0, strength, vectorStyle.lineColor, vectorStyle.lineWidth, toKnots, lat <= 0, vectorStyle.drawVectorPlotValue,
                            vectorStyle.fontSize, vectorStyle.textColor, vectorStyle.outlinecolor, vectorStyle.outlinewidth);
      }
      if (vectorStyle.drawVector) {
        drawImage->drawVector(x, y, ((270 - direction) / 360) * M_PI * 2, strength * vectorStyle.symbolScaling, vectorStyle.lineColor, vectorStyle.lineWidth);
      }
      if (vectorStyle.drawDiscs) {
        // Draw a disc with the speed value in text and the dir. value as an arrow
        int x = pointStrength->x;
        int y = dataSource->srvParams->Geo->dHeight - pointStrength->y;
        textValue.print(vectorStyle.drawVectorTextFormat.c_str(), strength);
        drawImage->setTextDisc(x, y, vectorStyle.discRadius, textValue.c_str(), vectorStyle.fontFile, vectorStyle.fontSize, vectorStyle.textColor, vectorStyle.fillColor, vectorStyle.lineColor);
        drawImage->drawVector2(x, y, ((90 + direction) / 360.) * M_PI * 2, 10, vectorStyle.discRadius, vectorStyle.fillColor, vectorStyle.lineWidth);
      }

      drawTextsForVector(drawImage, dataSource, vectorStyle, pointStrength, pointDirection);
    }
  }
}

typedef std::vector<f8point> SimpleSymbol;
typedef std::map<std::string, SimpleSymbol> SimpleSymbolMap;
SimpleSymbolMap simpleSymbolMapCache;

SimpleSymbolMap makeSymbolMap(CServerConfig::XMLE_Configuration *cfg) {
  SimpleSymbolMap simpleSymbolMap;
  /**
   * Parse following coordinates into a symbol struct:
   *
   * <Symbol name="triangle" coordinates="[[-1, -1], [1, -1], [0.0, 1], [-1, -1]]"/>
   * <Symbol name="square" coordinates="[[-1, -1], [1, -1], [1, 1], [-1, 1], [-1, -1]]"/>
   *
   */
  for (size_t j = 0; j < cfg->Symbol.size(); j++) {
    auto symbolName = cfg->Symbol[j]->attr.name;
    auto coordinates = cfg->Symbol[j]->attr.coordinates;
    // Make a single list of numbers
    coordinates.replaceSelf("[", "");
    coordinates.replaceSelf("]", "");
    coordinates.replaceSelf(" ", "");
    // Split on ","
    auto coordinateStrings = coordinates.splitToStack(",");
    SimpleSymbol symbol;

    // Every pair is a coordinate.
    for (size_t p = 0; p < coordinateStrings.size() && p < coordinateStrings.size() + 1; p += 2) {
      symbol.push_back({.x = coordinateStrings[p].toDouble(), .y = coordinateStrings[p + 1].toDouble()});
    }
    simpleSymbolMap[symbolName.c_str()] = symbol;
    CDBDebug("Created simpleSymbol %s", symbolName.c_str());
  }
  return simpleSymbolMap;
}

SimpleSymbol getSymbol(std::string symbolName, SimpleSymbolMap &simpleSymbolMap) {
  auto findIt = simpleSymbolMap.find(symbolName);
  if (findIt != simpleSymbolMap.end()) {
    return findIt->second;
  }
  return {};
}

std::vector<int> buildAlphaVector(int radius) {
  int diameter = 2 * radius + 1;
  std::vector<int> alpha(diameter * diameter);

  for (int y = -radius; y <= radius; ++y) {
    for (int x = -radius; x <= radius; ++x) {
      int idx = (y + radius) * diameter + (x + radius); // 2D -> 1D
      float d = std::sqrt(float(x * x + y * y));
      d = radius - d;
      if (d < 0) d = 0;
      d = d * 2.4f * 4.0f;
      alpha[idx] = static_cast<int>(d);
    }
  }
  CDBDebug("alphaPoint initialized");

  return alpha;
}

void drawVolumeForPoint(CDrawImage *drawImage, CColor drawPointFillColor, int x, int y, float radius, const std::vector<int> &alphaVec) {
  int rvol = drawPointFillColor.r;
  int gvol = drawPointFillColor.g;
  int bvol = drawPointFillColor.b;

  // TODO: do not draw if volume pixel is invisible
  int idx = 0;
  drawImage->setPixelTrueColor(x, y, 0, 0, 0, 255);
  for (int y1 = -radius; y1 <= radius; y1++) {
    for (int x1 = -radius; x1 <= radius; x1++) {
      drawImage->setPixelTrueColor(x + x1, y + y1, rvol, gvol, bvol, alphaVec[idx++]);
    }
  }
}

bool shouldSkipPoint(CStyleConfiguration *styleConfiguration, float value, size_t dataObjectIndex) {
  bool skipPoint = false;
  if (styleConfiguration != NULL) {
    if (styleConfiguration->hasLegendValueRange) {
      double legendLowerRange = styleConfiguration->legendLowerRange;
      double legendUpperRange = styleConfiguration->legendUpperRange;
      skipPoint = true;
      if (value >= legendLowerRange && value < legendUpperRange) {
        skipPoint = false;
      }
    }
  }
  return skipPoint;
}

bool shouldDrawSymbol(CServerConfig::XMLE_SymbolInterval *symbolInterval, float symbol_v) {
  // Should not draw symbol if we want a binary match and it fails
  if (!symbolInterval->attr.binary_and.empty()) {
    int b = parseInt(symbolInterval->attr.binary_and.c_str());
    if ((b & int(symbol_v)) != b) {
      return false;
    }
  }

  bool minSet = !symbolInterval->attr.min.empty();
  bool maxSet = !symbolInterval->attr.max.empty();

  // Should draw symbol if no min/max is set at all
  if (!minSet && !maxSet) return true;

  // Should draw if min/max is set, and value is within range
  if (minSet && maxSet) {
    float minVal = parseFloat(symbolInterval->attr.min.c_str());
    float maxVal = parseFloat(symbolInterval->attr.max.c_str());
    if (symbol_v >= minVal && symbol_v < maxVal) return true;
  }

  return false;
}

CColor getDrawPointColor(CDataSource *dataSource, CDrawImage *drawImage, float value) {
  if ((dataSource->getStyle() != NULL) && (dataSource->getStyle()->shadeIntervals.size() > 0)) {
    return getPixelColorForValue(drawImage, dataSource, value);
  }
  int pointColorIndex = getPixelIndexForValue(dataSource, value); // Use value of dataObject[0] for colour
  return drawImage->getColorForIndex(pointColorIndex);
}

void drawSymbolForPoint(CDrawImage *drawImage, std::map<std::string, CDrawImage *> symbolCache, std::string symbolFile, CServerConfig::XMLE_SymbolInterval *symbolInterval, int x, int y) {
  if (symbolFile.length() == 0) return;

  CDrawImage *symbol = NULL;
  auto symbolCacheIter = symbolCache.find(symbolFile);
  if (symbolCacheIter == symbolCache.end()) {
    symbol = new CDrawImage();
    symbol->createImage(symbolFile.c_str());
    symbolCache[symbolFile] = symbol; // Remember in cache
  } else {
    symbol = (*symbolCacheIter).second;
  }
  int offsetX = 0;
  int offsetY = 0;
  if (!symbolInterval->attr.offsetX.empty()) offsetX = parseInt(symbolInterval->attr.offsetX.c_str());
  if (!symbolInterval->attr.offsetY.empty()) offsetY = parseInt(symbolInterval->attr.offsetY.c_str());

  drawImage->draw(x - symbol->Geo->dWidth / 2 + offsetX, y - symbol->Geo->dHeight / 2 + offsetY, 0, 0, symbol);
}

float getRadius(CDataSource *dataSource, size_t pointIndex, float discRadius) {
  if (dataSource->getNumDataObjects() == 2) {
    const auto &p2 = dataSource->getDataObject(1)->points;
    if (pointIndex < p2.size()) {
      return p2[pointIndex].v * discRadius;
    }
  }
  return discRadius;
}

void drawRadiusAndValueForPoint(CDrawImage *drawImage, int x, int y, CColor drawPointLineColor, CColor drawPointFillColor, SimpleSymbol currentSymbol, float radius) {
  if (currentSymbol.size() > 0) {
    std::vector<float> xPoly(currentSymbol.size()), yPoly(currentSymbol.size());
    for (size_t i = 0; i < currentSymbol.size(); ++i) {
      xPoly[i] = x + currentSymbol[i].x * radius;
      yPoly[i] = y - currentSymbol[i].y * radius;
    }
    drawImage->poly(xPoly.data(), yPoly.data(), currentSymbol.size(), 1, drawPointLineColor, drawPointFillColor, true, true);
  } else {
    drawImage->setDisc(x, y, radius, drawPointFillColor, drawPointLineColor);
  }
}

CT::string prepareText(CDataSource *dataSource, size_t dataObjectIndex, float value, CT::string &drawPointTextFormat) {
  // Determine text to plot for value
  CT::string text;
  if (dataSource->getDataObject(dataObjectIndex)->hasStatusFlag) {
    CT::string flagMeaning;
    CDataSource::getFlagMeaningHumanReadable(&flagMeaning, &dataSource->getDataObject(dataObjectIndex)->statusFlagList, value);
    text.print("%s", flagMeaning.c_str());
  } else {
    text.print(drawPointTextFormat.c_str(), value);
  }
  return text;
}

void renderSinglePoints(std::vector<size_t> thinnedPointIndexList, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration, PointStyle pointStyle) {
  CColor defaultColor = CColor(0, 0, 0, 255);
  bool drawRadiusAndValue = pointStyle.style == "radiusandvalue";
  bool drawZoomablePoint = pointStyle.style == "zoomablepoint";

  // Preparation for symbol drawing, only used for radiusAndValue
  SimpleSymbol currentSymbol;
  if (drawRadiusAndValue) {
    if (simpleSymbolMapCache.empty()) {
      simpleSymbolMapCache = makeSymbolMap(dataSource->cfg);
    }
    currentSymbol = getSymbol(pointStyle.symbol, simpleSymbolMapCache);
  }

  /* For thinning */
  int doneMatrixH = 2;
  int doneMatrixW = 2;
  int doneMatrixMaxPerSector = -1;

  // TODO: write docs and tests for maxpointspercell and maxpointcellsize
  if (pointStyle.maxPointCellSize != -1) {
    doneMatrixH = pointStyle.maxPointCellSize;
    doneMatrixW = pointStyle.maxPointCellSize;
  }

  if (pointStyle.maxPointsPerCell != -1) {
    doneMatrixMaxPerSector = pointStyle.maxPointsPerCell;
  }

  unsigned char doneMatrix[doneMatrixW * doneMatrixH];
  for (size_t j = 0; j < size_t(doneMatrixW * doneMatrixH); j++) {
    doneMatrix[j] = 0;
  }

  for (size_t dataObjectIndex = 0; dataObjectIndex < dataSource->getNumDataObjects(); dataObjectIndex++) {
    std::vector<PointDVWithLatLon> *pts = &dataSource->getDataObject(dataObjectIndex)->points;

    float usedx = 0;
    float usedy = 0;
    int kwadrant = 0;
    if (pointStyle.useAngles) {
      float useangle = pointStyle.angleStart + pointStyle.angleStep * dataObjectIndex;
      if (useangle < 0) {
        kwadrant = 3 - int(useangle / 90);
      } else {
        kwadrant = int(useangle / 90);
      }
      usedx = pointStyle.textRadius * sin(useangle * M_PI / 180);
      usedy = pointStyle.textRadius * cos(useangle * M_PI / 180);
      // CDBDebug("angles[%d] %f %d %f %f", dataObject, useangle, kwadrant, usedx, usedy);
    }

    float fillValueObjectOne = dataSource->getDataObject(0)->hasNodataValue ? dataSource->getDataObject(0)->dfNodataValue : NAN;
    for (auto pointIndex : thinnedPointIndexList) {
      auto pointValue = &(*pts)[pointIndex];
      float value = pointValue->v;

      if (value == fillValueObjectOne) continue;
      if (pointStyle.isOutsideMinMax(value)) continue;
      if (shouldSkipPoint(styleConfiguration, value, dataObjectIndex)) continue;

      int x = pointValue->x;
      int y = dataSource->srvParams->Geo->dHeight - pointValue->y;

      if (std::isnan(value)) {
        // Try to draw something if value is NaN
        if (pointValue->paramList.size() > 0) {
          CT::string textValue = pointValue->paramList[0].value;
          if (pointStyle.discRadius == 0) {
            drawImage->drawCenteredText(x, y, pointStyle.fontFile.c_str(), pointStyle.fontSize, 0, textValue.c_str(), pointStyle.textColor);
          } else {
            drawImage->circle(x, y, pointStyle.discRadius + 1, pointStyle.lineColor, 0.65);
            drawImage->drawAnchoredText(x - int(float(textValue.length()) * 3.0f) - 2, y - pointStyle.textRadius, pointStyle.fontFile.c_str(), pointStyle.fontSize, 0, textValue.c_str(),
                                        pointStyle.textColor, kwadrant);
          }
        }
        continue;
      }

      if (!drawZoomablePoint) {
        size_t doneMatrixPointer = 0;
        if (x >= 0 && y >= 0 && x < drawImage->Geo->dWidth && y < drawImage->Geo->dHeight) {
          doneMatrixPointer = int((float(x) / float(drawImage->Geo->dWidth)) * float(doneMatrixW)) + int((float(y) / float(drawImage->Geo->dHeight)) * float(doneMatrixH)) * doneMatrixH;
          if (int(doneMatrix[doneMatrixPointer]) < 200) {
            doneMatrix[doneMatrixPointer]++;
          }
        }

        if (int(doneMatrix[doneMatrixPointer]) > doneMatrixMaxPerSector && doneMatrixMaxPerSector != -1) {
          continue;
        }
      }

      CT::string text = prepareText(dataSource, dataObjectIndex, value, pointStyle.textFormat);
      bool drawText = pointStyle.textFormat.length() >= 2;

      if (!pointStyle.useTextColor) {
        // Only calculate color for 1st dataObject, rest gets defaultColor
        pointStyle.textColor = dataObjectIndex == 0 ? getDrawPointColor(dataSource, drawImage, value) : defaultColor;
      }
      if (pointStyle.discRadius == 0) {
        if (pointStyle.plotStationId) {
          drawImage->drawCenteredText(x, y + pointStyle.textRadius + 3, pointStyle.fontFile.c_str(), pointStyle.fontSize, 0, text.c_str(), pointStyle.textColor);
        } else {
          drawImage->drawCenteredText(x, y, pointStyle.fontFile.c_str(), pointStyle.fontSize, 0, text.c_str(), pointStyle.textColor, pointStyle.textOutlineColor);
        }
      } else {                          // Text and disc
        if (!pointStyle.useFillColor) { //(dataSource->getNumDataObjects()==1) {
          pointStyle.fillColor = getDrawPointColor(dataSource, drawImage, value);
        }
        if (drawRadiusAndValue) {
          if (dataObjectIndex == 0) {
            float radius = getRadius(dataSource, pointIndex, pointStyle.discRadius);
            drawRadiusAndValueForPoint(drawImage, x, y, pointStyle.lineColor, pointStyle.fillColor, currentSymbol, radius);
          }
        } else {
          if (drawZoomablePoint) {
            drawImage->setEllipse(x, y, pointValue->radiusX, pointValue->radiusY, pointValue->rotation, pointStyle.fillColor, pointStyle.lineColor);
          } else {
            if (dataObjectIndex == 0) drawImage->setDisc(x, y, float(pointStyle.discRadius), pointStyle.fillColor, pointStyle.lineColor);
          }
        }

        if (drawText && dataObjectIndex == 0) {
          if (pointStyle.useAngles) {
            drawImage->drawAnchoredText(x + usedx, y - usedy, pointStyle.fontFile.c_str(), pointStyle.fontSize, 0, text.c_str(), pointStyle.textColor, kwadrant);
          } else {
            drawImage->drawCenteredText(x, y + pointStyle.textRadius, pointStyle.fontFile.c_str(), pointStyle.fontSize, 0, text.c_str(), pointStyle.textColor);
          }
        }
      }
      if (pointStyle.plotStationId && pointValue->paramList.size() > 0) {
        CT::string stationid = pointValue->paramList[0].value;
        drawImage->drawCenteredText(x, y - pointStyle.textRadius - 3, pointStyle.fontFile.c_str(), pointStyle.fontSize, 0, stationid.c_str(), pointStyle.textColor);
      }
    }
  }
}

void shouldUseFilterPoints(CStyleConfiguration *styleConfiguration, std::unordered_set<std::string> &usePoints, std::unordered_set<std::string> &skipPoints) {
  // TODO: skip is never used, and no test exists for this case
  CServerConfig::XMLE_Style *s = styleConfiguration->styleConfig;
  if (s->FilterPoints.size() == 0) return;
  auto attr = s->FilterPoints[0]->attr;

  if (!attr.use.empty()) {
    for (const auto &token : attr.use.splitToStack(",")) {
      usePoints.insert(token.c_str());
    }
  }
  if (!attr.skip.empty()) {
    for (const auto &token : attr.skip.splitToStack(",")) {
      skipPoints.insert(token.c_str());
    }
  }
}

void renderSingleVolumes(std::vector<size_t> thinnedPointIndexList, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration, PointStyle pointStyle) {
  std::vector<int> alphaVec = buildAlphaVector(int(pointStyle.discRadius));

  for (size_t dataObjectIndex = 0; dataObjectIndex < dataSource->getNumDataObjects(); dataObjectIndex++) {
    std::vector<PointDVWithLatLon> *pts = &dataSource->getDataObject(dataObjectIndex)->points;

    float fillValueObjectOne = dataSource->getDataObject(0)->hasNodataValue ? dataSource->getDataObject(0)->dfNodataValue : NAN;
    for (auto pointIndex : thinnedPointIndexList) {
      auto pointValue = &(*pts)[pointIndex];
      float value = pointValue->v;

      if (value == fillValueObjectOne) continue;
      if (pointStyle.isOutsideMinMax(value)) continue;
      if (shouldSkipPoint(styleConfiguration, value, dataObjectIndex)) continue;

      int x = pointValue->x;
      int y = dataSource->srvParams->Geo->dHeight - pointValue->y;

      drawVolumeForPoint(drawImage, pointStyle.fillColor, x, y, pointStyle.discRadius, alphaVec);
      if (pointStyle.plotStationId && pointValue->paramList.size() > 0) {
        CT::string stationId = pointValue->paramList[0].value;
        drawImage->setText(stationId.c_str(), stationId.length(), x - stationId.length() * 3, y - 20, pointStyle.textColor, 0);
      }
    }
  }
}

void renderSingleSymbols(std::vector<size_t> thinnedPointIndexList, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration, PointStyle pointStyle) {
  std::map<std::string, CDrawImage *> symbolCache;
  for (size_t dataObjectIndex = 0; dataObjectIndex < dataSource->getNumDataObjects(); dataObjectIndex++) {
    std::vector<PointDVWithLatLon> *pts = &dataSource->getDataObject(dataObjectIndex)->points;

    float fillValueObjectOne = dataSource->getDataObject(0)->hasNodataValue ? dataSource->getDataObject(0)->dfNodataValue : NAN;
    for (auto pointIndex : thinnedPointIndexList) {
      auto pointValue = &(*pts)[pointIndex];
      if (pointValue->v == fillValueObjectOne) continue;
      if (pointStyle.isOutsideMinMax(pointValue->v)) continue;
      if (shouldSkipPoint(styleConfiguration, pointValue->v, dataObjectIndex)) continue;

      int x = pointValue->x;
      int y = dataSource->srvParams->Geo->dHeight - pointValue->y;

      for (auto symbolInterval : styleConfiguration->symbolIntervals) {
        if (!shouldDrawSymbol(symbolInterval, pointValue->v)) continue;

        std::string symbolFile = symbolInterval->attr.file.c_str();
        drawSymbolForPoint(drawImage, symbolCache, symbolFile, symbolInterval, x, y);

        if (pointStyle.plotStationId && pointValue->paramList.size() > 0) {
          CT::string stationid = pointValue->paramList[0].value;
          drawImage->drawCenteredText(x, y - pointStyle.textRadius - 3, pointStyle.fontFile.c_str(), pointStyle.fontSize, 0, stationid.c_str(), pointStyle.textColor);
        }
      }
    }
  }

  for (const auto &entry : symbolCache) {
    delete entry.second;
  }
}

void renderSingleDiscs(std::vector<size_t> thinnedPointIndexList, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration, PointStyle pointStyle) {
  for (size_t dataObjectIndex = 0; dataObjectIndex < dataSource->getNumDataObjects(); dataObjectIndex++) {
    std::vector<PointDVWithLatLon> *pts = &dataSource->getDataObject(dataObjectIndex)->points;

    float fillValueObjectOne = dataSource->getDataObject(0)->hasNodataValue ? dataSource->getDataObject(0)->dfNodataValue : NAN;
    for (auto pointIndex : thinnedPointIndexList) {
      auto pointValue = &(*pts)[pointIndex];
      float value = pointValue->v;

      if (value == fillValueObjectOne) continue;
      if (pointStyle.isOutsideMinMax(value)) continue;
      if (shouldSkipPoint(styleConfiguration, value, dataObjectIndex)) continue;

      int x = pointValue->x;
      int y = dataSource->srvParams->Geo->dHeight - pointValue->y;
      if (!pointStyle.useTextColor) {
        pointStyle.textColor = getDrawPointColor(dataSource, drawImage, value);
      }

      CT::string text = prepareText(dataSource, dataObjectIndex, value, pointStyle.textFormat);
      CColor col = pointStyle.useFillColor ? pointStyle.fillColor : getDrawPointColor(dataSource, drawImage, value);
      drawImage->setTextDisc(x, y, pointStyle.discRadius, text.c_str(), pointStyle.fontFile.c_str(), pointStyle.fontSize, pointStyle.textColor, col, pointStyle.lineColor);
    }
  }
}

void renderSingleDot(std::vector<size_t> thinnedPointIndexList, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration, PointStyle pointStyle) {
  for (size_t dataObjectIndex = 0; dataObjectIndex < dataSource->getNumDataObjects(); dataObjectIndex++) {
    std::vector<PointDVWithLatLon> *pts = &dataSource->getDataObject(dataObjectIndex)->points;

    float fillValueObjectOne = dataSource->getDataObject(0)->hasNodataValue ? dataSource->getDataObject(0)->dfNodataValue : NAN;
    for (auto pointIndex : thinnedPointIndexList) {
      auto pointValue = &(*pts)[pointIndex];
      float value = pointValue->v;

      if (value == fillValueObjectOne) continue;
      if (pointStyle.isOutsideMinMax(value)) continue;
      if (shouldSkipPoint(styleConfiguration, value, dataObjectIndex)) continue;

      int x = pointValue->x;
      int y = dataSource->srvParams->Geo->dHeight - pointValue->y;

      drawImage->circle(x, y, 1, pointStyle.lineColor, pointStyle.discRadius == 0 ? 0.65 : 1);
    }
  }
}

void CImgRenderPoints::render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  if (styleConfiguration == NULL || styleConfiguration->styleConfig == NULL) {
    CDBDebug("Note: No styleConfiguration. Skipping.");
    return;
  }

  CServerConfig::XMLE_Style *styleConfig = styleConfiguration->styleConfig;
  if (styleConfig == NULL) {
    CDBError("styleConfiguration==NULL!");
  }

  // Fill use/skip sets if FilterPoints was set
  std::unordered_set<std::string> usePoints;
  std::unordered_set<std::string> skipPoints;
  shouldUseFilterPoints(styleConfiguration, usePoints, skipPoints);

  ThinningInfo thinningInfo = getThinningInfo(styleConfiguration->styleConfig);

  for (auto pointConfig : styleConfig->Point) {
    PointStyle pointStyle = getPointStyle(pointConfig, dataSource->srvParams->cfg);
    auto thinnedPointIndexList = doThinningGetIndices(dataSource->getDataObject(0)->points, thinningInfo.doThinning, thinningInfo.thinningRadius, usePoints);

    if (pointConfig->attr.dot.equalsIgnoreCase("true")) {
      renderSingleDot(thinnedPointIndexList, dataSource, drawImage, styleConfiguration, pointStyle);
    }

    if (pointStyle.style == "disc") {
      renderSingleDiscs(thinnedPointIndexList, dataSource, drawImage, styleConfiguration, pointStyle);
    } else if (pointStyle.style == "volume") {
      renderSingleVolumes(thinnedPointIndexList, dataSource, drawImage, styleConfiguration, pointStyle);
    } else if (pointStyle.style == "symbol") {
      renderSingleSymbols(thinnedPointIndexList, dataSource, drawImage, styleConfiguration, pointStyle);
    } else { // regular points, zoomablepoint and radiusandvalue points
      renderSinglePoints(thinnedPointIndexList, dataSource, drawImage, styleConfiguration, pointStyle);
    }
  }

  bool isVector = ((dataSource->getNumDataObjects() >= 2) && (dataSource->getDataObject(0)->cdfVariable->getAttributeNE("ADAGUC_GEOJSONPOINT") == NULL));
  if (isVector) {
    std::vector<PointDVWithLatLon> *p1 = &dataSource->getDataObject(0)->points;

    auto thinnedPointIndexList = doThinningGetIndices(*p1, thinningInfo.doThinning, thinningInfo.thinningRadius, usePoints);
    CDBDebug("Vector plotting %d elements %d", thinnedPointIndexList.size(), usePoints.size());
    renderVectorPoints(thinnedPointIndexList, warper, dataSource, drawImage, styleConfiguration);
  }
}

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

CColor getPixelColorForValue(CDrawImage *drawImage, CDataSource *dataSource, float val) {
  bool isNodata = false;

  CColor color;
  if (dataSource->getDataObject(0)->hasNodataValue) {
    if (val == float(dataSource->getDataObject(0)->dfNodataValue)) isNodata = true;
    if (!(val == val)) isNodata = true;
  }
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  if (!isNodata) {
    for (auto shadeInterval : styleConfiguration->shadeIntervals) {
      if (shadeInterval->attr.min.empty() == false && shadeInterval->attr.max.empty() == false) {
        if ((val >= atof(shadeInterval->attr.min.c_str())) && (val < atof(shadeInterval->attr.max.c_str()))) {
          return CColor(shadeInterval->attr.fillcolor.c_str());
        }
      }
    }
  }
  // If shade interval is set, we have to round the color value down to the lower value of the legend class
  float newValue = styleConfiguration->shadeInterval > 0 ? val = floor(val / styleConfiguration->shadeInterval) * styleConfiguration->shadeInterval : val;
  int pointColorIndex = getPixelIndexForValue(dataSource, newValue); // Use value of dataObject[0] for colour
  return drawImage->getColorForIndex(pointColorIndex);
}