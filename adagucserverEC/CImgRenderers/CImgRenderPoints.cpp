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
#include "getPointStyle.h"
#include "getVectorStyle.h"

struct ThinningInfo {
  bool doThinning = false;
  int thinningRadius = 25;
};

typedef std::vector<f8point> SimpleSymbol;
typedef std::map<std::string, SimpleSymbol> SimpleSymbolMap;
SimpleSymbolMap simpleSymbolMapCache;

void drawTextsForVector(CDrawImage *drawImage, CDataSource *dataSource, VectorStyle &vectorStyle, PointDVWithLatLon *pointStrength, PointDVWithLatLon *pointDirection) {
  // Draw station id
  auto strength = pointStrength->v;
  auto direction = pointDirection->v;
  int x = pointStrength->x;
  int y = dataSource->srvParams->geoParams.height - pointStrength->y;

  if (vectorStyle.drawVectorPlotStationId && pointStrength->paramList.size() > 0) {
    int newY = y;
    if (vectorStyle.drawBarb) {
      newY = ((direction >= 90) && (direction <= 270)) ? y - 20 : y + 6;
    } else {
      newY = ((direction >= 90) && (direction <= 270)) ? y + 6 : y - 20;
    }
    CT::string stationId = pointStrength->paramList[0].value;
    drawImage->setText(stationId.c_str(), x - stationId.length() * 3, newY, vectorStyle.textColor);
  }

  // Draw value for vector or disc
  if (vectorStyle.drawVectorPlotValue && !vectorStyle.drawBarb) {
    if (!vectorStyle.drawDiscs) {
      int newY = ((direction >= 90) && (direction <= 270)) ? y - 20 : y + 6;
      CT::string textValue;
      textValue.print(vectorStyle.drawVectorTextFormat.c_str(), strength);
      drawImage->setText(textValue.c_str(), x - textValue.length() * 3, newY, vectorStyle.textColor);
    }
  }
}

ThinningInfo getThinningInfo(CStyleConfiguration *styleConfiguration) {
  ThinningInfo info;
  if (styleConfiguration != nullptr) {
    for (auto thinning : styleConfiguration->thinningList) {
      if (!thinning->attr.radius.empty()) {
        info.doThinning = true;
        info.thinningRadius = thinning->attr.radius.toInt();
      }
    }
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
  if (styleConfiguration == nullptr || styleConfiguration->vectorIntervals.size() == 0) {
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
    CDBError("Cannot draw barbs, lists for speed (%lu) and direction (%lu) don't have equal length ", numberOfPoints, p2->size());
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
  for (auto cfgVectorStyle : styleConfiguration->vectorIntervals) {
    vectorStyles.push_back(getVectorStyle(cfgVectorStyle, dataSource->srvParams->cfg));
  }

  for (auto pointIndex : thinnedPointIndexList) {
    auto pointStrength = &(*p1)[pointIndex];
    auto pointDirection = &(*p2)[pointIndex];
    auto strength = pointStrength->v;
    auto direction = pointDirection->v;
    if (std::isnan(direction) || std::isnan(strength) || strength == fillValueObjectOne || direction == fillValueObjectTwo) continue;
    int x = pointStrength->x;
    int y = dataSource->srvParams->geoParams.height - pointStrength->y;
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
        int y = dataSource->srvParams->geoParams.height - pointStrength->y;
        textValue.print(vectorStyle.drawVectorTextFormat.c_str(), strength);
        drawImage->setTextDisc(x, y, vectorStyle.discRadius, textValue.c_str(), vectorStyle.fontFile.c_str(), vectorStyle.fontSize, vectorStyle.textColor, vectorStyle.fillColor,
                               vectorStyle.lineColor);
        drawImage->drawVector2(x, y, ((90 + direction) / 360.) * M_PI * 2, 10, vectorStyle.discRadius, vectorStyle.fillColor, vectorStyle.lineWidth);
      }

      drawTextsForVector(drawImage, dataSource, vectorStyle, pointStrength, pointDirection);
    }
  }
}

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
    auto coordinateStrings = coordinates.split(",");
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

bool isPointOutsideLegendRange(CStyleConfiguration *styleConfiguration, float value) {
  bool skipPoint = false;
  if (styleConfiguration->hasLegendValueRange) {
    double legendLowerRange = styleConfiguration->legendLowerRange;
    double legendUpperRange = styleConfiguration->legendUpperRange;
    skipPoint = true;
    if (value >= legendLowerRange && value < legendUpperRange) {
      skipPoint = false;
    }
  }
  return skipPoint;
}

bool shouldSkipPoint(CStyleConfiguration *styleConfiguration, PointStyle pointStyle, float value, float fillValue) {
  if (std::isnan(value)) return true;
  if (value == fillValue) return true;
  if (pointStyle.isOutsideMinMax(value)) return true;
  if (isPointOutsideLegendRange(styleConfiguration, value)) return true;
  return false;
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

void drawSymbolForPoint(CDrawImage *drawImage, std::map<std::string, CDrawImage *> &symbolCache, std::string symbolFile, CServerConfig::XMLE_SymbolInterval *symbolInterval, int x, int y) {
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

  drawImage->draw(x - symbol->geoParams.width / 2 + offsetX, y - symbol->geoParams.height / 2 + offsetY, 0, 0, symbol);
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
  if (dataSource->getDataObject(dataObjectIndex) != nullptr && dataSource->getDataObject(dataObjectIndex)->hasStatusFlag) {
    CT::string flagMeaning;
    CDataSource::getFlagMeaningHumanReadable(&flagMeaning, &dataSource->getDataObject(dataObjectIndex)->statusFlagList, value);
    text.print("%s", flagMeaning.c_str());
  } else {
    text.print(drawPointTextFormat.c_str(), value);
  }
  return text;
}

int calculateYForDataobject(CDataSource *dataSource, int station_y, float textRadius, int dataObjectIndex) {
  int numDataObjects = dataSource->getNumDataObjects();
  int y;
  if (numDataObjects == 1) {
    y = station_y + textRadius;
  } else if ((numDataObjects % 2) == 0) {
    y = station_y + (-0.5 + dataObjectIndex) * (textRadius);
  } else {
    y = int(station_y + (-numDataObjects / 2.0 + dataObjectIndex + 0.5) * textRadius);
  }
  return y;
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

  float fillValueObjectOne = dataSource->getDataObject(0)->hasNodataValue ? dataSource->getDataObject(0)->dfNodataValue : NAN;
  for (size_t dataObjectIndex = 0; dataObjectIndex < dataSource->getNumDataObjects(); dataObjectIndex++) {
    auto dataObject = dataSource->getDataObject(dataObjectIndex);
    if (dataObject == nullptr) continue;
    auto pointTypeAttr = dataObject->cdfVariable->getAttributeNE("ADAGUC_ORGPOINT_TYPE");
    auto dataType = pointTypeAttr != nullptr ? pointTypeAttr->getDataAt<int>(0) : CDF_FLOAT;

    std::vector<PointDVWithLatLon> *pts = &dataObject->points;

    if (pts == nullptr) continue;
    size_t numPoints = pts->size();

    for (auto pointIndex : thinnedPointIndexList) {
      if (pointIndex >= numPoints) continue;
      auto pointValue = &(*pts)[pointIndex];
      float value = pointValue->v;

      int x = pointValue->x;
      int station_y = dataSource->srvParams->geoParams.height - pointValue->y;

      int y = calculateYForDataobject(dataSource, station_y, pointStyle.textRadius, dataObjectIndex);

      if (dataType == CDF_STRING) {
        if (dataObjectIndex == 0) {
          if (pointStyle.discRadius > 0) {
            drawImage->circle(x, station_y, pointStyle.discRadius + 1, pointStyle.lineColor, 0.65);
          }
        }
        if (pointValue->paramList.size() > 0) {
          CT::string textValue = pointValue->paramList[0].value;
          // Draw the string value
          drawImage->drawCenteredText(x, y, pointStyle.fontFile.c_str(), pointStyle.fontSize, 0, textValue.c_str(), pointStyle.textColor, pointStyle.textOutlineColor);
        }
        continue; // If type is string, then no other draw options have to be considered.
      }

      if (shouldSkipPoint(styleConfiguration, pointStyle, value, fillValueObjectOne)) continue;

      if (!drawZoomablePoint) {
        size_t doneMatrixPointer = 0;
        if (x >= 0 && station_y >= 0 && x < drawImage->geoParams.width && station_y < drawImage->geoParams.height) {
          doneMatrixPointer = int((float(x) / float(drawImage->geoParams.width)) * float(doneMatrixW)) + int((float(y) / float(drawImage->geoParams.height)) * float(doneMatrixH)) * doneMatrixH;
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
        drawImage->drawCenteredText(x, y, pointStyle.fontFile.c_str(), pointStyle.fontSize, 0, text.c_str(), pointStyle.textColor, pointStyle.textOutlineColor);
      } else { // Text and disc
        if (!pointStyle.useFillColor) {
          pointStyle.fillColor = getDrawPointColor(dataSource, drawImage, value);
        }
        if (drawRadiusAndValue) {
          if (dataObjectIndex == 0) {
            float radius = getRadius(dataSource, pointIndex, pointStyle.discRadius);
            drawRadiusAndValueForPoint(drawImage, x, station_y, pointStyle.lineColor, pointStyle.fillColor, currentSymbol, radius);
          }
        } else if (drawZoomablePoint) {
          drawImage->setEllipse(x, station_y, pointValue->radiusX, pointValue->radiusY, pointValue->rotation, pointStyle.fillColor, pointStyle.lineColor);
        } else {
          if (dataObjectIndex == 0) drawImage->setDisc(x, station_y, float(pointStyle.discRadius), pointStyle.fillColor, pointStyle.lineColor);
        }

        if (drawText) {
          drawImage->drawCenteredText(x, y, pointStyle.fontFile.c_str(), pointStyle.fontSize, 0, text.c_str(), pointStyle.textColor, pointStyle.textOutlineColor);
        }
      }
      if (pointStyle.plotStationId && (pointValue->paramList.size() > 0) && (dataObjectIndex == 0)) {
        CT::string stationid = pointValue->paramList[0].value;
        drawImage->drawCenteredText(x, station_y - pointStyle.textRadius, pointStyle.fontFile.c_str(), pointStyle.fontSize, 0, stationid.c_str(), pointStyle.textColor, pointStyle.textOutlineColor);
      }
    }
  }
}

std::unordered_set<std::string> shouldUseFilterPoints(CStyleConfiguration *styleConfiguration) {
  std::unordered_set<std::string> usePoints;

  if (styleConfiguration->filterPointList.size() == 0) return usePoints;
  for (auto filterPoint : styleConfiguration->filterPointList) {
    if (!filterPoint->attr.use.empty()) {
      for (const auto &token : filterPoint->attr.use.split(",")) {
        usePoints.insert(token.c_str());
      }
    }
  }
  return usePoints;
}

void renderSingleVolumes(std::vector<size_t> thinnedPointIndexList, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration, PointStyle pointStyle) {
  std::vector<int> alphaVec = buildAlphaVector(int(pointStyle.discRadius));
  float fillValueObjectOne = dataSource->getDataObject(0)->hasNodataValue ? dataSource->getDataObject(0)->dfNodataValue : NAN;

  for (size_t dataObjectIndex = 0; dataObjectIndex < dataSource->getNumDataObjects(); dataObjectIndex++) {
    auto dataObject = dataSource->getDataObject(dataObjectIndex);
    if (dataObject == nullptr) continue;
    std::vector<PointDVWithLatLon> *pts = &dataObject->points;

    for (auto pointIndex : thinnedPointIndexList) {
      auto pointValue = &(*pts)[pointIndex];
      float value = pointValue->v;
      if (shouldSkipPoint(styleConfiguration, pointStyle, value, fillValueObjectOne)) continue;

      int x = pointValue->x;
      int y = dataSource->srvParams->geoParams.height - pointValue->y;

      drawVolumeForPoint(drawImage, pointStyle.fillColor, x, y, pointStyle.discRadius, alphaVec);
      if (pointStyle.plotStationId && pointValue->paramList.size() > 0) {
        CT::string stationId = pointValue->paramList[0].value;
        drawImage->setText(stationId.c_str(), x - stationId.length() * 3, y - 20, pointStyle.textColor);
      }
    }
  }
}

void renderSingleSymbols(std::vector<size_t> thinnedPointIndexList, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration, PointStyle pointStyle) {
  std::map<std::string, CDrawImage *> symbolCache;
  float fillValueObjectOne = dataSource->getDataObject(0)->hasNodataValue ? dataSource->getDataObject(0)->dfNodataValue : NAN;

  for (size_t dataObjectIndex = 0; dataObjectIndex < dataSource->getNumDataObjects(); dataObjectIndex++) {
    auto dataObject = dataSource->getDataObject(dataObjectIndex);
    if (dataObject == nullptr) continue;
    std::vector<PointDVWithLatLon> *pts = &dataObject->points;
    for (auto pointIndex : thinnedPointIndexList) {
      auto pointValue = &(*pts)[pointIndex];
      float value = pointValue->v;
      if (shouldSkipPoint(styleConfiguration, pointStyle, value, fillValueObjectOne)) continue;

      int x = pointValue->x;
      int y = dataSource->srvParams->geoParams.height - pointValue->y;

      for (auto symbolInterval : styleConfiguration->symbolIntervals) {
        if (!shouldDrawSymbol(symbolInterval, value)) continue;

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
  float fillValueObjectOne = dataSource->getDataObject(0)->hasNodataValue ? dataSource->getDataObject(0)->dfNodataValue : NAN;

  for (size_t dataObjectIndex = 0; dataObjectIndex < dataSource->getNumDataObjects(); dataObjectIndex++) {
    auto dataObject = dataSource->getDataObject(dataObjectIndex);
    if (dataObject == nullptr) continue;
    std::vector<PointDVWithLatLon> *pts = &dataObject->points;

    for (auto pointIndex : thinnedPointIndexList) {
      auto pointValue = &(*pts)[pointIndex];
      float value = pointValue->v;
      if (shouldSkipPoint(styleConfiguration, pointStyle, value, fillValueObjectOne)) continue;

      int x = pointValue->x;
      int y = dataSource->srvParams->geoParams.height - pointValue->y;
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
  float fillValueObjectOne = dataSource->getDataObject(0)->hasNodataValue ? dataSource->getDataObject(0)->dfNodataValue : NAN;

  for (size_t dataObjectIndex = 0; dataObjectIndex < dataSource->getNumDataObjects(); dataObjectIndex++) {
    auto dataObject = dataSource->getDataObject(dataObjectIndex);
    if (dataObject == nullptr) continue;
    std::vector<PointDVWithLatLon> *pts = &dataObject->points;

    for (auto pointIndex : thinnedPointIndexList) {
      auto pointValue = &(*pts)[pointIndex];
      float value = pointValue->v;
      if (shouldSkipPoint(styleConfiguration, pointStyle, value, fillValueObjectOne)) continue;

      int x = pointValue->x;
      int y = dataSource->srvParams->geoParams.height - pointValue->y;
      drawImage->circle(x, y, 1, pointStyle.lineColor, pointStyle.discRadius == 0 ? 0.65 : 1);
    }
  }
}

void CImgRenderPoints::render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  if (styleConfiguration == NULL) {
    CDBDebug("Note: No styleConfiguration. Skipping.");
    return;
  }

  std::unordered_set<std::string> usePoints = shouldUseFilterPoints(styleConfiguration);
  ThinningInfo thinningInfo = getThinningInfo(styleConfiguration);

  for (auto pointConfig : styleConfiguration->pointIntervals) {
    PointStyle pointStyle = getPointStyle(pointConfig, dataSource->srvParams->cfg);
    auto thinnedPointIndexList = doThinningGetIndices(dataSource->getDataObject(0)->points, thinningInfo.doThinning, thinningInfo.thinningRadius, usePoints);
    if (dataSource->debug) {
      CDBDebug("Point plotting %lu elements %lu", thinnedPointIndexList.size(), usePoints.size());
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
    if (pointStyle.dot) {
      renderSingleDot(thinnedPointIndexList, dataSource, drawImage, styleConfiguration, pointStyle);
    }
  }

  bool isVector = ((dataSource->getNumDataObjects() >= 2) && (dataSource->getDataObject(0)->cdfVariable->getAttributeNE("ADAGUC_GEOJSONPOINT") == NULL));
  if (isVector) {
    std::vector<PointDVWithLatLon> *p1 = &dataSource->getDataObject(0)->points;

    auto thinnedPointIndexList = doThinningGetIndices(*p1, thinningInfo.doThinning, thinningInfo.thinningRadius, usePoints);
    CDBDebug("Vector plotting %lu elements %lu", thinnedPointIndexList.size(), usePoints.size());
    renderVectorPoints(thinnedPointIndexList, warper, dataSource, drawImage, styleConfiguration);
  }
}

int getPixelIndexForValue(CDataSource *dataSource, float val) {
  bool isNodata = false;

  if (dataSource->getDataObject(0)->hasNodataValue) {
    if (val == float(dataSource->getDataObject(0)->dfNodataValue)) isNodata = true;
    if (std::isnan(val)) isNodata = true;
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
    if (std::isnan(val)) isNodata = true;
  }
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  if (!isNodata) {
    for (const auto &shadeInterval : styleConfiguration->shadeIntervals) {
      if (shadeInterval.attr.min.empty() == false && shadeInterval.attr.max.empty() == false) {
        if ((val >= atof(shadeInterval.attr.min.c_str())) && (val < atof(shadeInterval.attr.max.c_str()))) {
          return CColor(shadeInterval.attr.fillcolor.c_str());
        }
      }
    }
  }
  // If shade interval is set, we have to round the color value down to the lower value of the legend class
  float newValue = styleConfiguration->shadeInterval > 0 ? val = floor(val / styleConfiguration->shadeInterval) * styleConfiguration->shadeInterval : val;
  int pointColorIndex = getPixelIndexForValue(dataSource, newValue); // Use value of dataObject[0] for colour
  return drawImage->getColorForIndex(pointColorIndex);
}