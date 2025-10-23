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
#include "CConvertGeoJSON.h"
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

ThinningInfo getThinningInfo(CServerConfig::XMLE_Style *s, ThinningInfo &info) {
  if (s->Thinning.size() == 1 && !s->Thinning[0]->attr.radius.empty()) {
    info.doThinning = true;
    info.thinningRadius = s->Thinning[0]->attr.radius.toInt();
  }
  return info;
}

std::vector<size_t> doThinningGetIndices(std::vector<PointDVWithLatLon> &p1, bool doThinning, double thinningRadius, std::unordered_set<std::string> useSet, std::unordered_set<std::string> skipSet) {
  size_t numberOfPoints = p1.size();
  std::vector<size_t> filteredPointIndices;

  // Filter points
  if (useSet.empty() && skipSet.empty()) {
    for (size_t pointIndex = 0; pointIndex < numberOfPoints; pointIndex++) {
      filteredPointIndices.push_back(pointIndex);
    }
  } else if (!useSet.empty()) {
    // USE ONLY these → whitelist
    for (size_t i = 0; i < numberOfPoints; ++i) {
      if (!p1[i].paramList.empty()) {
        const auto &val = p1[i].paramList[0].value;
        if (useSet.count(val.c_str())) {
          filteredPointIndices.push_back(i);
        }
      }
    }
  } else if (!skipSet.empty()) {
    for (size_t i = 0; i < numberOfPoints; ++i) {
      if (!p1[i].paramList.empty()) {
        const auto &val = p1[i].paramList[0].value;
        if (skipSet.count(val.c_str())) {
          continue;
        }
      }
      filteredPointIndices.push_back(i);
    }
  }

  std::vector<size_t> thinnedPointIndexList;

  // Check for thinning as well.
  if (doThinning) {
    thinnedPointIndexList.push_back(0); // Always put in first element
    for (size_t pointIndex = 1; pointIndex < filteredPointIndices.size(); pointIndex++) {
      size_t thinnedPointNr = 0;
      for (thinnedPointNr = 0; thinnedPointNr < thinnedPointIndexList.size(); thinnedPointNr++) {
        auto thinnedPointIndex = thinnedPointIndexList[thinnedPointNr];
        if (hypot(p1[thinnedPointIndex].x - p1[pointIndex].x, p1[thinnedPointIndex].y - p1[pointIndex].y) < thinningRadius) break;
      }
      if (thinnedPointNr == thinnedPointIndexList.size()) thinnedPointIndexList.push_back(pointIndex);
    }
  } else {
    thinnedPointIndexList = filteredPointIndices;
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

  CT::string varName1 = dataSource->getDataObject(0)->cdfVariable->name.c_str();
  CT::string varName2 = dataSource->getDataObject(1)->cdfVariable->name.c_str();

  CT::string textValue;

  float fillValueP1 = dataSource->getDataObject(0)->hasNodataValue ? dataSource->getDataObject(0)->dfNodataValue : NAN;
  float fillValueP2 = dataSource->getDataObject(1)->hasNodataValue ? dataSource->getDataObject(1)->dfNodataValue : NAN;

  CT::string units = dataSource->getDataObject(0)->getUnits();
  bool toKnots = false;
  if (!(units.equalsIgnoreCase("kt") || units.equalsIgnoreCase("kts") || units.equalsIgnoreCase("knot"))) {
    toKnots = true;
  }

  // Make a list of vector style objects based on the configuration.
  std::vector<VectorStyle> vectorStyles;
  for (auto cfgVectorStyle : s->Vector) {
    vectorStyles.push_back(getVectorStyle(cfgVectorStyle));
  }

  int vectorDiscRadius = 12;

  auto drawPointFillColor = CColor(0, 0, 0, 128);
  auto drawPointLineColor = CColor(0, 0, 0, 255);
  auto drawPointFontFile = dataSource->srvParams->cfg->WMS[0]->ContourFont[0]->attr.location.c_str();
  for (auto pointIndex : thinnedPointIndexList) {
    auto pointStrength = &(*p1)[pointIndex];
    auto pointDirection = &(*p2)[pointIndex];
    auto strength = pointStrength->v;
    auto direction = pointDirection->v;
    if (!(direction == direction) || !(strength == strength) || strength == fillValueP1 || direction == fillValueP2) continue;
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
        drawImage->setTextDisc(x, y, vectorDiscRadius, textValue.c_str(), drawPointFontFile, vectorStyle.fontSize, vectorStyle.textColor, drawPointFillColor, drawPointLineColor);
        drawImage->drawVector2(x, y, ((90 + direction) / 360.) * M_PI * 2, 10, vectorDiscRadius, drawPointFillColor, vectorStyle.lineWidth);
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
  for (size_t j = 0; j < cfg->Symbol.size(); j++) {
    auto symbolName = cfg->Symbol[j]->attr.name;
    auto coordinates = cfg->Symbol[j]->attr.coordinates;
    coordinates.replaceSelf("[", "");
    coordinates.replaceSelf("]", "");
    coordinates.replaceSelf(" ", "");
    auto coordinateStrings = coordinates.splitToStack(",");
    SimpleSymbol symbol;
    for (size_t p = 0; p < coordinateStrings.size() && p < coordinateStrings.size() + 1; p += 2) {
      symbol.push_back({.x = coordinateStrings[p].toDouble(), .y = coordinateStrings[p + 1].toDouble()});
    }
    simpleSymbolMap[symbolName.c_str()] = symbol;
  }
  return simpleSymbolMap;
}

SimpleSymbol getSymbol(CT::string symbolName, SimpleSymbolMap &simpleSymbolMap) {
  auto findIt = simpleSymbolMap.find(symbolName.c_str());
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

  int idx = 0;
  drawImage->setPixelTrueColor(x, y, 0, 0, 0, 255);
  for (int y1 = -radius; y1 <= radius; y1++) {
    for (int x1 = -radius; x1 <= radius; x1++) {
      drawImage->setPixelTrueColor(x + x1, y + y1, rvol, gvol, bvol, alphaVec[idx++]);
    }
  }
}

struct PointMinMax {
  bool isSet = false;
  float min = 0;
  float max = 0;
};

PointMinMax getPointMinMax(CServerConfig::XMLE_Point *pointConfig) {
  PointMinMax pointMinMax;
  if (!pointConfig->attr.min.empty() && !pointConfig->attr.max.empty()) {
    pointMinMax.isSet = true;
    pointMinMax.min = pointConfig->attr.min.toFloat();
    pointMinMax.max = pointConfig->attr.max.toFloat();
  }
  return pointMinMax;
}

bool shouldSkipPoint(CStyleConfiguration *styleConfiguration, float value, size_t dataObjectIndex, PointMinMax pointMinMax) {
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
  if (pointMinMax.isSet && dataObjectIndex == 0) {
    if (value < pointMinMax.min || value > pointMinMax.max) {
      skipPoint = true;
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

float getRadius(CDataSource *dataSource, size_t pointIndex, float drawPointDiscRadius) {
  if (dataSource->getNumDataObjects() == 2) {
    const auto &p2 = dataSource->getDataObject(1)->points;
    if (pointIndex < p2.size()) {
      return p2[pointIndex].v * drawPointDiscRadius;
    }
  }
  return drawPointDiscRadius;
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

void CImgRenderPoints::renderSinglePoints(std::vector<size_t> thinnedPointIndexList, CImageWarper *, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration,
                                          CServerConfig::XMLE_Point *pointConfig) {
  // Preparation for symbol drawing, only used for radiusAndValue
  SimpleSymbol currentSymbol;
  if (drawPoints && isRadiusAndValue) {
    if (simpleSymbolMapCache.empty()) {
      simpleSymbolMapCache = makeSymbolMap(dataSource->cfg);
    }
    currentSymbol = getSymbol(pointConfig->attr.symbol.c_str(), simpleSymbolMapCache);
  }

  // Preparation for volume draw
  std::vector<int> alphaVec;
  if (drawVolume) {
    alphaVec = buildAlphaVector(int(drawPointDiscRadius));
  }

  /* For thinning */
  int doneMatrixH = 2;
  int doneMatrixW = 2;
  int doneMatrixMaxPerSector = -1;

  if (!pointConfig->attr.maxpointcellsize.empty()) {
    doneMatrixH = pointConfig->attr.maxpointcellsize.toInt();
    doneMatrixW = pointConfig->attr.maxpointcellsize.toInt();
  }

  if (!pointConfig->attr.maxpointspercell.empty()) {
    doneMatrixMaxPerSector = pointConfig->attr.maxpointspercell.toInt();
  }

  unsigned char doneMatrix[doneMatrixW * doneMatrixH];
  for (size_t j = 0; j < size_t(doneMatrixW * doneMatrixH); j++) {
    doneMatrix[j] = 0;
  }

  auto pointMinMax = getPointMinMax(pointConfig);

  std::map<std::string, CDrawImage *> symbolCache;
  for (size_t dataObjectIndex = 0; dataObjectIndex < dataSource->getNumDataObjects(); dataObjectIndex++) {
    std::vector<PointDVWithLatLon> *pts = &dataSource->getDataObject(dataObjectIndex)->points;

    float usedx = 0;
    float usedy = 0;
    int kwadrant = 0;
    if (useDrawPointAngles) {
      float useangle = drawPointAngleStart + drawPointAngleStep * dataObjectIndex;
      if (useangle < 0) {
        kwadrant = 3 - int(useangle / 90);
      } else {
        kwadrant = int(useangle / 90);
      }
      usedx = drawPointTextRadius * sin(useangle * M_PI / 180);
      usedy = drawPointTextRadius * cos(useangle * M_PI / 180);
      //       CDBDebug("angles[%d] %f %d %f %f", dataObject, useangle, kwadrant, usedx, usedy);
    }

    float fillValueP1 = dataSource->getDataObject(0)->hasNodataValue ? dataSource->getDataObject(0)->dfNodataValue : NAN;
    for (auto pointIndex : thinnedPointIndexList) {

      auto pointValue = &(*pts)[pointIndex];
      float value = pointValue->v;

      if (std::isnan(value) || value == fillValueP1) continue;
      if (shouldSkipPoint(styleConfiguration, value, dataObjectIndex, pointMinMax)) continue;

      int x = pointValue->x;
      int y = dataSource->srvParams->Geo->dHeight - pointValue->y;

      if (drawPointDot) drawImage->circle(x, y, 1, drawPointLineColor, drawPointDiscRadius == 0 ? 0.65 : 1);

      if (drawVolume) {
        drawVolumeForPoint(drawImage, drawPointFillColor, x, y, drawPointDiscRadius, alphaVec);
        if (drawPointPlotStationId && pointValue->paramList.size() > 0) {
          CT::string stationId = pointValue->paramList[0].value;
          drawImage->setText(stationId.c_str(), stationId.length(), x - stationId.length() * 3, y - 20, drawPointTextColor, 0);
        }
      }

      if (drawSymbol) {
        for (auto symbolInterval : styleConfiguration->symbolIntervals) {
          if (!shouldDrawSymbol(symbolInterval, value)) continue;

          std::string symbolFile = symbolInterval->attr.file.c_str();
          drawSymbolForPoint(drawImage, symbolCache, symbolFile, symbolInterval, x, y);

          if (drawPointPlotStationId && pointValue->paramList.size() > 0) {
            CT::string stationid = pointValue->paramList[0].value;
            drawImage->drawCenteredText(x, y - drawPointTextRadius - 3, drawPointFontFile, drawPointFontSize, 0, stationid.c_str(), drawPointTextColor);
          }
        }
      }

      if (drawPoints) {
        if (!drawZoomablePoints) {
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

        CT::string text = prepareText(dataSource, dataObjectIndex, value, drawPointTextFormat);
        bool drawText = drawPointTextFormat.length() >= 2;

        if (!useDrawPointTextColor) {
          // Only calculate color for 1st dataObject, rest gets defaultColor
          drawPointTextColor = dataObjectIndex == 0 ? getDrawPointColor(dataSource, drawImage, value) : defaultColor;
        }
        if (drawPointDiscRadius == 0) {
          if (drawPointPlotStationId) {
            drawImage->drawCenteredText(x, y + drawPointTextRadius + 3, drawPointFontFile, drawPointFontSize, 0, text.c_str(), drawPointTextColor);
          } else {
            drawImage->drawCenteredText(x, y, drawPointFontFile, drawPointFontSize, 0, text.c_str(), drawPointTextColor, drawPointTextOutlineColor);
          }
        } else {                        // Text and disc
          if (!useDrawPointFillColor) { //(dataSource->getNumDataObjects()==1) {
            drawPointFillColor = getDrawPointColor(dataSource, drawImage, value);
          }
          if (isRadiusAndValue) {
            if (dataObjectIndex == 0) {
              float radius = getRadius(dataSource, pointIndex, drawPointDiscRadius);
              drawRadiusAndValueForPoint(drawImage, x, y, drawPointLineColor, drawPointFillColor, currentSymbol, radius);
            }
          } else {
            if (drawZoomablePoints) {
              drawImage->setEllipse(x, y, pointValue->radiusX, pointValue->radiusY, pointValue->rotation, drawPointFillColor, drawPointLineColor);
            } else {
              if (dataObjectIndex == 0) drawImage->setDisc(x, y, drawPointDiscRadius, drawPointFillColor, drawPointLineColor);
            }
          }

          if (drawText && dataObjectIndex == 0) {
            if (useDrawPointAngles) {
              drawImage->drawAnchoredText(x + usedx, y - usedy, drawPointFontFile, drawPointFontSize, 0, text.c_str(), drawPointTextColor, kwadrant);
            } else {
              drawImage->drawCenteredText(x, y + drawPointTextRadius, drawPointFontFile, drawPointFontSize, 0, text.c_str(), drawPointTextColor);
            }
          }
        }
        if (drawPointPlotStationId && pointValue->paramList.size() > 0) {
          CT::string stationid = pointValue->paramList[0].value;
          drawImage->drawCenteredText(x, y - drawPointTextRadius - 3, drawPointFontFile, drawPointFontSize, 0, stationid.c_str(), drawPointTextColor);
        }
      }

      if (drawDiscs) { // Filled disc with circle around it and value inside
        if (!useDrawPointTextColor) {
          drawPointTextColor = getDrawPointColor(dataSource, drawImage, value);
        }

        CT::string text = prepareText(dataSource, dataObjectIndex, value, drawPointTextFormat);
        CColor col = useDrawPointFillColor ? drawPointFillColor : getDrawPointColor(dataSource, drawImage, value);
        drawImage->setTextDisc(x, y, drawPointDiscRadius, text.c_str(), drawPointFontFile, drawPointFontSize, drawPointTextColor, col, drawPointLineColor);
      }
    }
  }

  for (const auto &entry : symbolCache) {
    // CDBDebug("Deleting entry for %s", symbolCacheIter->first.c_str());
    delete entry.second;
  }
}

void shouldUseFilterPoints(CStyleConfiguration *styleConfiguration, std::unordered_set<std::string> &useSet, std::unordered_set<std::string> &skipSet) {
  // TODO: no test uses `skip` and functionality wasn't implemented (but docs said it should work).
  CServerConfig::XMLE_Style *s = styleConfiguration->styleConfig;
  if (s->FilterPoints.size() == 0) return;
  auto attr = s->FilterPoints[0]->attr;

  if (!attr.use.empty()) {
    CT::string useStr = attr.use.c_str();
    for (const auto &token : useStr.splitToStack(",")) {
      useSet.insert(token.c_str());
    }
  }
  if (!attr.skip.empty()) {
    CT::string skipStr = attr.skip.c_str();
    for (const auto &token : skipStr.splitToStack(",")) {
      skipSet.insert(token.c_str());
    }
  }
}

void CImgRenderPoints::render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {
  drawPoints = true;
  drawDiscs = false;
  drawVolume = false;
  drawSymbol = false;
  drawZoomablePoints = false;

  drawPointPointStyle = "point";
  drawPointFontFile = dataSource->srvParams->cfg->WMS[0]->ContourFont[0]->attr.location.c_str();
  drawPointFontSize = 8;
  drawPointDiscRadius = 8;
  drawPointTextRadius = drawPointDiscRadius + 8;
  drawPointDot = false;
  drawPointAngleStart = -90;
  drawPointAngleStep = 180;
  useDrawPointAngles = false;
  drawPointPlotStationId = false;
  drawPointTextFormat = "%0.1f";
  drawPointTextColor = CColor(0, 0, 0, 255);
  drawPointTextOutlineColor = CColor(255, 255, 255, 0);
  drawPointFillColor = CColor(0, 0, 0, 128);
  drawPointLineColor = CColor(0, 0, 0, 255);
  defaultColor = CColor(0, 0, 0, 255);

  useDrawPointFillColor = false;
  useDrawPointTextColor = false;
  isRadiusAndValue = false;

  ThinningInfo thinningInfo;

  std::map<std::string, std::vector<Feature *>> featureStore = CConvertGeoJSON::featureStore;
  std::vector<Feature *> features;
  features = featureStore[dataSource->featureSet.c_str()];

  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  if (styleConfiguration == NULL || styleConfiguration->styleConfig == NULL) {
    return;
  }

  // Fill use/skip sets if FilterPoints was set
  std::unordered_set<std::string> useSet;
  std::unordered_set<std::string> skipSet;
  shouldUseFilterPoints(styleConfiguration, useSet, skipSet);

  CServerConfig::XMLE_Style *styleConfig = styleConfiguration->styleConfig;
  for (auto pointConfig : styleConfig->Point) {

    if (pointConfig->attr.fillcolor.empty() == false) {
      drawPointFillColor.parse(pointConfig->attr.fillcolor.c_str());
      useDrawPointFillColor = true;
    }
    if (pointConfig->attr.linecolor.empty() == false) {
      drawPointLineColor.parse(pointConfig->attr.linecolor.c_str());
    }
    if (pointConfig->attr.textcolor.empty() == false) {
      drawPointTextColor.parse(pointConfig->attr.textcolor.c_str());
      useDrawPointTextColor = true;
    }
    if (pointConfig->attr.textoutlinecolor.empty() == false) {
      drawPointTextOutlineColor.parse(pointConfig->attr.textoutlinecolor.c_str());
    }
    if (pointConfig->attr.fontfile.empty() == false) {
      drawPointFontFile = pointConfig->attr.fontfile.c_str();
    }
    if (pointConfig->attr.fontsize.empty() == false) {
      drawPointFontSize = pointConfig->attr.fontsize.toFloat();
    }
    if (pointConfig->attr.discradius.empty() == false) {
      drawPointDiscRadius = pointConfig->attr.discradius.toFloat();
      if (pointConfig->attr.textradius.empty() == true) {
        drawPointTextRadius = drawPointDiscRadius + 4;
      }
    }
    if (pointConfig->attr.textradius.empty() == false) {
      drawPointTextRadius = pointConfig->attr.textradius.toInt();
    }
    if (pointConfig->attr.dot.empty() == false) {
      drawPointDot = pointConfig->attr.dot.equalsIgnoreCase("true");
    }
    if (pointConfig->attr.anglestart.empty() == false) {
      drawPointAngleStart = pointConfig->attr.anglestart.toFloat();
      useDrawPointAngles = true;
    }
    if (pointConfig->attr.anglestep.empty() == false) {
      drawPointAngleStep = pointConfig->attr.anglestep.toFloat();
    }
    if (pointConfig->attr.textformat.empty() == false) {
      drawPointTextFormat = pointConfig->attr.textformat.c_str();
    }
    if (pointConfig->attr.pointstyle.empty() == false) {
      drawPointPointStyle = pointConfig->attr.pointstyle.c_str();
    }
    if (pointConfig->attr.plotstationid.empty() == false) {
      drawPointPlotStationId = pointConfig->attr.plotstationid.equalsIgnoreCase("true");
    }

    if (drawPointPointStyle.equalsIgnoreCase("disc")) {
      drawDiscs = true;
      drawPoints = false;
      drawVolume = false;

      drawSymbol = false;
    } else if (drawPointPointStyle.equalsIgnoreCase("volume")) {
      drawPoints = false;
      drawVolume = true;
      drawDiscs = false;
      drawSymbol = false;
    } else if (drawPointPointStyle.equalsIgnoreCase("symbol")) {
      drawPoints = false;
      drawVolume = false;
      drawDiscs = false;
      drawSymbol = true;
    } else if (drawPointPointStyle.equalsIgnoreCase("zoomablepoint")) {
      drawPoints = true;
      drawVolume = false;
      drawDiscs = false;
      drawSymbol = false;
      drawZoomablePoints = true;
    } else {
      drawPoints = true;
      drawDiscs = false;
      drawVolume = false;
      drawSymbol = false;
    }
    if (drawPointPointStyle.equals("radiusandvalue")) {
      isRadiusAndValue = true;
    }

    // CDBDebug("drawPoints=%d drawVolume=%d drawSymbol=%d drawDiscs=%d", drawPoints, drawVolume, drawSymbol, drawDiscs);

    // TODO: Currently thin needs to be mentioned in the rendermethod to allow thinning. We want to get rid of this. We need to adjust configs on geoservices and geoweb first.
    if (settings.indexOf("thin") != -1) {
      getThinningInfo(styleConfiguration->styleConfig, thinningInfo);
    }
    auto thinnedPointIndexList = doThinningGetIndices(dataSource->getDataObject(0)->points, thinningInfo.doThinning, thinningInfo.thinningRadius, useSet, skipSet);
    renderSinglePoints(thinnedPointIndexList, warper, dataSource, drawImage, styleConfiguration, pointConfig);
  }

  bool isVector = ((dataSource->getNumDataObjects() >= 2) && (dataSource->getDataObject(0)->cdfVariable->getAttributeNE("ADAGUC_GEOJSONPOINT") == NULL));
  if (isVector) {
    CDBDebug("VECTOR");
    std::vector<PointDVWithLatLon> *p1 = &dataSource->getDataObject(0)->points;

    getThinningInfo(styleConfiguration->styleConfig, thinningInfo);
    auto thinnedPointIndexList = doThinningGetIndices(*p1, thinningInfo.doThinning, thinningInfo.thinningRadius, useSet, skipSet);
    CDBDebug("Vector plotting %d elements %d (use=%d skip=%d)", thinnedPointIndexList.size(), useSet.size(), skipSet.size());
    renderVectorPoints(thinnedPointIndexList, warper, dataSource, drawImage, styleConfiguration);
  }
}

int CImgRenderPoints::set(const char *values) {
  settings.copy(values);
  return 0;
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
