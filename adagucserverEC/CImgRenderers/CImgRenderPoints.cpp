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

#include "CImgRenderPoints.h"
#include "CConvertGeoJSON.h"

const char *CImgRenderPoints::className = "CImgRenderPoints";

void CImgRenderPoints::renderSinglePoints(CImageWarper *, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration, CServerConfig::XMLE_Point *pointConfig) {
  SimpleSymbol *currentSymbol = NULL;
  if (pointConfig->attr.symbol.empty() == false) {
    CT::string symbolName = pointConfig->attr.symbol.c_str();
    /* Lets see if the symbol is already in the map */
    if (SimpleSymbolMap.find(symbolName.c_str()) == SimpleSymbolMap.end()) {
      /* No Lets add it */
      for (size_t j = 0; j < dataSource->cfg->Symbol.size(); j++) {
        if (dataSource->cfg->Symbol[j]->attr.name.equals(symbolName)) {
          CT::string coordinates = dataSource->cfg->Symbol[j]->attr.coordinates;
          coordinates.replaceSelf("[", "");
          coordinates.replaceSelf("]", "");
          coordinates.replaceSelf(" ", "");
          CT::StackList<CT::string> e = coordinates.splitToStack(",");
          SimpleSymbol s;
          for (size_t p = 0; p < e.size() && p < e.size() + 1; p += 2) {
            s.coordinates.push_back(SimpleSymbol::Coordinate(e[p].toFloat(), e[p + 1].toFloat()));
          }
          SimpleSymbolMap[symbolName.c_str()] = s;
        }
      }
    }
    if (SimpleSymbolMap.find(symbolName.c_str()) != SimpleSymbolMap.end()) {
      currentSymbol = &SimpleSymbolMap.find(symbolName.c_str())->second;
    }
  }

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

  int drawPointDiscRadiusInt = int(drawPointDiscRadius);
  int alphaPoint[(2 * drawPointDiscRadiusInt + 1) * (2 * drawPointDiscRadiusInt + 1)];

  /* Preparation for volume draw */
  if (drawVolume) {
    int p = 0;
    for (int y1 = -drawPointDiscRadius; y1 <= drawPointDiscRadius; y1++) {
      for (int x1 = -drawPointDiscRadius; x1 <= drawPointDiscRadius; x1++) {
        float d = sqrt(x1 * x1 + y1 * y1); // between 0 and 1.4*drawPointDiscRadius
        d = drawPointDiscRadius - d;
        //        d=10-d;
        if (d < 0) d = 0;
        d = d * 2.4 * 4;
        alphaPoint[p++] = d;
      }
    }
    CDBDebug("alphaPoint inited");
  }

  /* For thinning */

  unsigned char doneMatrix[doneMatrixW * doneMatrixH];
  for (size_t j = 0; j < size_t(doneMatrixW * doneMatrixH); j++) {
    doneMatrix[j] = 0;
  }

  std::map<std::string, CDrawImage *> symbolCache;
  //     CDBDebug("symbolCache created, size=%d", symbolCache.size());
  std::map<std::string, CDrawImage *>::iterator symbolCacheIter;
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

    // THINNING
    std::vector<PointDVWithLatLon> *p1 = &dataSource->getDataObject(dataObjectIndex)->points;
    size_t l = p1->size();
    size_t nrThinnedPoints = l;
    std::vector<size_t> thinnedPointsIndex;

    //      CDBDebug("Before thinning: %d (%d)", l, doThinning);
    if (doThinning) {
      for (size_t j = 0; j < l; j++) {
        size_t nrThinnedPoints = thinnedPointsIndex.size();
        size_t i;
        if ((useFilter && (*p1)[j].paramList.size() > 0 && usePoints.find((*p1)[j].paramList[0].value.c_str()) != usePoints.end()) || !useFilter) {
          for (i = 0; i < nrThinnedPoints; i++) {
            if (j == 0) break; // Always put in first element
            if (hypot((*p1)[thinnedPointsIndex[i]].x - (*p1)[j].x, (*p1)[thinnedPointsIndex[i]].y - (*p1)[j].y) < thinningRadius) break;
          }
          if (i == nrThinnedPoints) thinnedPointsIndex.push_back(j);
        }
      }
      nrThinnedPoints = thinnedPointsIndex.size();
    } else if (useFilter) {
      for (size_t j = 0; j < l; j++) {
        if ((*p1)[j].paramList.size() > 0 && usePoints.find((*p1)[j].paramList[0].value.c_str()) != usePoints.end()) {
          // if ((*p1)[j].paramList.size()>0 && usePoints.find((*p1)[j].paramList[0].value.c_str())!=usePoints.end()){
          thinnedPointsIndex.push_back(j);
          CDBDebug("pushed el %d: %s", j, (*p1)[j].paramList[0].value.c_str());
        }
      }
      nrThinnedPoints = thinnedPointsIndex.size();
    } else {
      // if no thinning: get all indexes
      for (size_t pointIndex = 0; pointIndex < l; pointIndex++) {
        thinnedPointsIndex.push_back(pointIndex);
      }
      nrThinnedPoints = thinnedPointsIndex.size();
    }

    bool pointMinMaxSet = false;
    float pointMin = 0;
    float pointMax = 0;
    if (!pointConfig->attr.min.empty() && !pointConfig->attr.max.empty()) {
      pointMinMaxSet = 1;
      pointMin = pointConfig->attr.min.toFloat();
      pointMax = pointConfig->attr.max.toFloat();
    }

    CT::string t;
    for (size_t pointNo = 0; pointNo < nrThinnedPoints; pointNo++) {
      size_t j = pointNo;
      j = thinnedPointsIndex[pointNo];
      float v = (*pts)[j].v;

      float perPointDrawPointDiscRadius = drawPointDiscRadius;
      bool skipPoint = false;
      if (styleConfiguration != NULL) {
        if (styleConfiguration->hasLegendValueRange) {
          double legendLowerRange = styleConfiguration->legendLowerRange;
          double legendUpperRange = styleConfiguration->legendUpperRange;
          skipPoint = true;
          if (v >= legendLowerRange && v < legendUpperRange) {
            skipPoint = false;
          }
        }
      }
      if (pointMinMaxSet && dataObjectIndex == 0) {
        if (v < pointMin || v > pointMax) {
          skipPoint = true;
        }
      }

      if (!skipPoint) {
        if (drawVolume) {
          int x = (*pts)[j].x;
          int y = dataSource->dHeight - (*pts)[j].y;
          int rvol = drawPointFillColor.r;
          int gvol = drawPointFillColor.g;
          int bvol = drawPointFillColor.b;

          drawImage->setPixelTrueColor(x, y, 0, 0, 0, 255);
          //           CDBDebug("drawVolume for [%d,%d]", x, y);
          int *p = alphaPoint;
          for (int y1 = -drawPointDiscRadius; y1 <= drawPointDiscRadius; y1++) {
            for (int x1 = -drawPointDiscRadius; x1 <= drawPointDiscRadius; x1++) {
              drawImage->setPixelTrueColor(x + x1, y + y1, rvol, gvol, bvol, *p++);
            }
          }
          if (drawPointPlotStationId) {
            if ((*pts)[j].paramList.size() > 0) {
              CT::string value = (*pts)[j].paramList[0].value;
              drawImage->setText(value.c_str(), value.length(), x - value.length() * 3, y - 20, drawPointTextColor, 0);
            }
          }
        }

        if (drawSymbol) {
          int x = (*pts)[j].x;
          int y = dataSource->dHeight - (*pts)[j].y;

          bool minMaxSet = (symbolIntervals != NULL && (symbolIntervals->size() == 1) && !(*symbolIntervals)[0]->attr.min.empty() && !(*symbolIntervals)[0]->attr.max.empty());
          // Plot symbol if either valid v or Symbolinterval.min and max not set (to plot symbol for string data type)
          if ((v == v) || (((*pts)[j].paramList.size() > 0) && !minMaxSet)) { //
            float symbol_v = v;                                               // Local copy of value
            if (!(v == v)) {
              if ((*pts)[j].paramList.size() > 0) symbol_v = 0;
            }
            if (symbolIntervals != NULL) {

              for (size_t intv = 0; intv < symbolIntervals->size(); intv++) {
                CServerConfig::XMLE_SymbolInterval *symbolInterval = ((*symbolIntervals)[intv]);
                bool drawThisOne = false;

                if (symbolInterval->attr.binary_and.empty() == false) {
                  int b = parseInt(symbolInterval->attr.binary_and.c_str());
                  if ((b & int(symbol_v)) == b) {
                    drawThisOne = true;
                    if (symbolInterval->attr.min.empty() == false && symbolInterval->attr.max.empty() == false) {
                      if ((symbol_v >= parseFloat(symbolInterval->attr.min.c_str())) && (symbol_v < parseFloat(symbolInterval->attr.max.c_str())))
                        ;
                      else
                        drawThisOne = false;
                    }
                  }

                } else {
                  if (symbolInterval->attr.min.empty() == false && symbolInterval->attr.max.empty() == false) {
                    if ((symbol_v >= parseFloat(symbolInterval->attr.min.c_str())) && (symbol_v < parseFloat(symbolInterval->attr.max.c_str()))) {
                      drawThisOne = true;
                    }
                  } else if (symbolInterval->attr.min.empty() && symbolInterval->attr.max.empty()) {
                    drawThisOne = true;
                  }
                }

                if (drawThisOne) {
                  std::string symbolFile = symbolInterval->attr.file.c_str();

                  if (symbolFile.length() > 0) {
                    CDrawImage *symbol = NULL;

                    symbolCacheIter = symbolCache.find(symbolFile);
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
                  if (drawPointPlotStationId) {
                    if ((*pts)[j].paramList.size() > 0) {
                      CT::string stationid = (*pts)[j].paramList[0].value;
                      drawImage->drawCenteredText(x, y - drawPointTextRadius - 3, drawPointFontFile, drawPointFontSize, 0, stationid.c_str(), drawPointTextColor);
                    }
                  }
                }
              }
            }
            if (drawPointDot) drawImage->circle(x, y, 1, drawPointLineColor, 0.65);
          }
        }

        if (drawPoints) {
          int x = (*pts)[j].x;
          int y = dataSource->dHeight - (*pts)[j].y;

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

          if (v == v) {
            // Determine text to plot for value
            bool drawText = true;
            if (dataSource->getDataObject(dataObjectIndex)->hasStatusFlag) {
              CT::string flagMeaning;
              CDataSource::getFlagMeaningHumanReadable(&flagMeaning, &dataSource->getDataObject(dataObjectIndex)->statusFlagList, v);
              t.print("%s", flagMeaning.c_str());
            } else if (drawPointTextFormat.length() < 2) {
              drawText = false;
            } else {
              t.print(drawPointTextFormat.c_str(), v);
            }

            if (isRadiusAndValue) {
              if (dataSource->getNumDataObjects() == 2) {
                std::vector<PointDVWithLatLon> *p2 = &dataSource->getDataObject(1)->points;
                perPointDrawPointDiscRadius = std::vector<PointDVWithLatLon>(*p2)[j].v * drawPointDiscRadius;
              }
            }

            if (!useDrawPointTextColor) {
              if (dataObjectIndex == 0) { // Only calculate color for 1st dataObject, rest gets defaultColor
                if ((dataSource->getStyle() != NULL) && (dataSource->getStyle()->shadeIntervals != NULL) && (dataSource->getStyle()->shadeIntervals->size() > 0)) {
                  drawPointTextColor = getPixelColorForValue(drawImage, dataSource, v);
                } else {
                  int pointColorIndex = getPixelIndexForValue(dataSource, v); // Use value of dataObject[0] for colour
                  drawPointTextColor = drawImage->getColorForIndex(pointColorIndex);
                }
              } else {
                drawPointTextColor = defaultColor;
              }
            }
            if (drawPointDiscRadius == 0) {
              if (drawPointPlotStationId) {
                drawImage->drawCenteredText(x, y + drawPointTextRadius + 3, drawPointFontFile, drawPointFontSize, 0, t.c_str(), drawPointTextColor);
              } else {
                drawImage->drawCenteredText(x, y, drawPointFontFile, drawPointFontSize, 0, t.c_str(), drawPointTextColor, drawPointTextOutlineColor);
              }
            } else {                        // Text and disc
              if (!useDrawPointFillColor) { //(dataSource->getNumDataObjects()==1) {
                if ((dataSource->getStyle() != NULL) && (dataSource->getStyle()->shadeIntervals != NULL) && (dataSource->getStyle()->shadeIntervals->size() > 0)) {
                  drawPointFillColor = getPixelColorForValue(drawImage, dataSource, v);
                } else {
                  int pointColorIndex = getPixelIndexForValue(dataSource, v); // Use value of dataObject[0] for colour
                  drawPointFillColor = drawImage->getColorForIndex(pointColorIndex);
                }
              }
              if (isRadiusAndValue) {
                if (dataObjectIndex == 0) {
                  if (currentSymbol != NULL) {
                    float xPoly[currentSymbol->coordinates.size()];
                    float yPoly[currentSymbol->coordinates.size()];

                    xPoly[0] = x + currentSymbol->coordinates[0].x * perPointDrawPointDiscRadius;
                    yPoly[0] = y - currentSymbol->coordinates[0].y * perPointDrawPointDiscRadius;
                    for (size_t l = 1; l < currentSymbol->coordinates.size(); l++) {
                      xPoly[l] = x + currentSymbol->coordinates[l].x * perPointDrawPointDiscRadius;
                      yPoly[l] = y - currentSymbol->coordinates[l].y * perPointDrawPointDiscRadius;
                    }
                    drawImage->poly(xPoly, yPoly, currentSymbol->coordinates.size(), 1, drawPointLineColor, drawPointFillColor, true, true);
                  } else {
                    drawImage->setDisc(x, y, perPointDrawPointDiscRadius, drawPointFillColor, drawPointLineColor);
                  }
                }
              } else {
                if (drawZoomablePoints) {
                  drawImage->setEllipse(x, y, (*pts)[j].radiusX, (*pts)[j].radiusY, (*pts)[j].rotation, drawPointFillColor, drawPointLineColor);
                } else {
                  if (dataObjectIndex == 0) drawImage->setDisc(x, y, drawPointDiscRadius, drawPointFillColor, drawPointLineColor);
                }
              }

              if (drawText) {
                if (useDrawPointAngles) {
                  if (dataObjectIndex == 0) {
                    drawImage->drawAnchoredText(x + usedx, y - usedy, drawPointFontFile, drawPointFontSize, 0, t.c_str(), drawPointTextColor, kwadrant);
                  }
                } else {
                  if (dataObjectIndex == 0) {
                    drawImage->drawCenteredText(x, y + drawPointTextRadius, drawPointFontFile, drawPointFontSize, 0, t.c_str(), drawPointTextColor);
                  }
                }
              }
              if (drawPointDot) drawImage->circle(x, y, 1, drawPointLineColor, 1);
            }
            if (drawPointPlotStationId) {
              if ((*pts)[j].paramList.size() > 0) {
                CT::string stationid = (*pts)[j].paramList[0].value;
                drawImage->drawCenteredText(x, y - drawPointTextRadius - 3, drawPointFontFile, drawPointFontSize, 0, stationid.c_str(), drawPointTextColor);
              }
            }
          } else {
            // CDBDebug("Value not available");
            if ((*pts)[j].paramList.size() > 0) {
              CT::string value = (*pts)[j].paramList[0].value;
              // CDBDebug("Extra value: %s fixed color with radius %d", value.c_str(), drawPointDiscRadius);
              if (drawPointDiscRadius == 0) {
                drawImage->drawCenteredText(x, y, drawPointFontFile, drawPointFontSize, 0, value.c_str(), drawPointTextColor);
              } else {
                drawImage->circle(x, y, drawPointDiscRadius + 1, drawPointLineColor, 0.65);
                //                 if (drawPointDot) drawImage->circle(x,y, 1, drawPointLineColor,1);
                drawImage->drawAnchoredText(x - int(float(value.length()) * 3.0f) - 2, y - drawPointTextRadius, drawPointFontFile, drawPointFontSize, 0, value.c_str(), drawPointTextColor, kwadrant);
              }
            }
          }
          if (drawPointDot) drawImage->circle(x, y, 1, drawPointLineColor, 0.65);
        }

        if (drawDiscs) { // Filled disc with circle around it and value inside
          int x = (*pts)[j].x;
          int y = dataSource->dHeight - (*pts)[j].y;
          // drawImage->circle(x,y, drawPointDiscRadius, 240,0.65);

          if (!useDrawPointTextColor) {
            if ((dataSource->getStyle() != NULL) && (dataSource->getStyle()->shadeIntervals != NULL)) {
              drawPointTextColor = getPixelColorForValue(drawImage, dataSource, v);
            } else {
              int pointColorIndex = getPixelIndexForValue(dataSource, v); // Use value of dataObject[0] for colour
              drawPointTextColor = drawImage->getColorForIndex(pointColorIndex);
            }
          }
          if (v == v) {
            if (dataSource->getDataObject(dataObjectIndex)->hasStatusFlag) {
              CT::string flagMeaning;
              CDataSource::getFlagMeaningHumanReadable(&flagMeaning, &dataSource->getDataObject(dataObjectIndex)->statusFlagList, v);
              t.print("%s", flagMeaning.c_str());
            } else {
              t.print(drawPointTextFormat.c_str(), v);
            }
            if (!useDrawPointFillColor) { //(dataSource->getNumDataObjects()==1) {
              if ((dataSource->getStyle() != NULL) && (dataSource->getStyle()->shadeIntervals != NULL)) {
                CColor col = getPixelColorForValue(drawImage, dataSource, v);
                drawImage->setTextDisc(x, y, drawPointDiscRadius, t.c_str(), drawPointFontFile, drawPointFontSize, drawPointTextColor, col, drawPointLineColor);
              } else {
                int pointColorIndex = getPixelIndexForValue(dataSource, v); // Use value of dataObject[0] for colour
                CColor col = drawImage->getColorForIndex(pointColorIndex);
                drawImage->setTextDisc(x, y, drawPointDiscRadius, t.c_str(), drawPointFontFile, drawPointFontSize, drawPointTextColor, col, drawPointLineColor);
              }
            } else {
              drawImage->setTextDisc(x, y, drawPointDiscRadius, t.c_str(), drawPointFontFile, drawPointFontSize, drawPointTextColor, drawPointFillColor, drawPointLineColor);
            }
            if (drawPointDot) drawImage->circle(x, y, 1, drawPointLineColor, 0.65);
          }
        }
      }
    }
  }

  for (symbolCacheIter = symbolCache.begin(); symbolCacheIter != symbolCache.end(); symbolCacheIter++) {
    //      CDBDebug("Deleting entry for %s", symbolCacheIter->first.c_str());
    delete (symbolCacheIter->second);
  }
}

void CImgRenderPoints::renderVectorPoints(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage, CStyleConfiguration *styleConfiguration) {

  if (styleConfiguration != NULL) {
    if (styleConfiguration->styleConfig != NULL) {
      CServerConfig::XMLE_Style *s = styleConfiguration->styleConfig;
      if (s->Vector.size() == 1) {
        if (s->Vector[0]->attr.linecolor.empty() == false) {
          drawVectorLineColor.parse(s->Vector[0]->attr.linecolor.c_str());
        }
        if (s->Vector[0]->attr.vectorstyle.empty() == false) {
          drawVectorVectorStyle = s->Vector[0]->attr.vectorstyle.c_str();
          if (drawVectorVectorStyle.equalsIgnoreCase("barb")) {
            drawBarb = true;
          } else if (drawVectorVectorStyle.equalsIgnoreCase("disc")) {
            drawDiscs = true;
          } else if (drawVectorVectorStyle.equalsIgnoreCase("vector")) {
            drawVector = true;
          } else {
            drawBarb = true;
          }
        }

        if (s->Vector[0]->attr.linewidth.empty() == false) {
          drawVectorLineWidth = s->Vector[0]->attr.linewidth.toFloat();
        }
        if (s->Vector[0]->attr.plotstationid.empty() == false) {
          drawVectorPlotStationId = s->Vector[0]->attr.plotstationid.equalsIgnoreCase("true");
        }
        if (s->Vector[0]->attr.plotvalue.empty() == false) {
          drawVectorPlotValue = s->Vector[0]->attr.plotvalue.equalsIgnoreCase("true");
        }
        if (s->Vector[0]->attr.textformat.empty() == false) {
          drawVectorTextFormat = s->Vector[0]->attr.textformat;
        }
        drawVectorVectorScale = s->Vector[0]->attr.scale;
      }
    }
  }
  //    CDBDebug("Vectors drawBarb=%d drawDiscs=%d drawVector=%d", drawBarb, drawDiscs, drawVector);

  CT::string varName1 = dataSource->getDataObject(0)->cdfVariable->name.c_str();
  CT::string varName2 = dataSource->getDataObject(1)->cdfVariable->name.c_str();
  std::vector<PointDVWithLatLon> *p1 = &dataSource->getDataObject(0)->points;
  std::vector<PointDVWithLatLon> *p2 = &dataSource->getDataObject(1)->points;
  size_t l = p1->size();
  size_t nrThinnedPoints = l;
  std::vector<size_t> thinnedPointsIndex;

  CT::string t;
  if (doThinning) {
    for (size_t j = 0; j < l; j++) {
      size_t nrThinnedPoints = thinnedPointsIndex.size();
      size_t i;
      if ((useFilter && (*p1)[j].paramList.size() > 0 && usePoints.find((*p1)[j].paramList[0].value.c_str()) != usePoints.end()) || !useFilter) {
        for (i = 0; i < nrThinnedPoints; i++) {
          if (j == 0) break; // Always put in first element
          if (hypot((*p1)[thinnedPointsIndex[i]].x - (*p1)[j].x, (*p1)[thinnedPointsIndex[i]].y - (*p1)[j].y) < thinningRadius) break;
        }
        if (i == nrThinnedPoints) thinnedPointsIndex.push_back(j);
      }
    }
    nrThinnedPoints = thinnedPointsIndex.size();
  } else if (useFilter) {
    for (size_t j = 0; j < l; j++) {
      if ((*p1)[j].paramList.size() > 0 && usePoints.find((*p1)[j].paramList[0].value.c_str()) != usePoints.end()) {
        // if ((*p1)[j].paramList.size()>0 && usePoints.find((*p1)[j].paramList[0].value.c_str())!=usePoints.end()){
        thinnedPointsIndex.push_back(j);
        CDBDebug("pushed el %d: %s", j, (*p1)[j].paramList[0].value.c_str());
      }
    }
    nrThinnedPoints = thinnedPointsIndex.size();
  } else {
    // if no thinning: get all indexes
    for (size_t pointIndex = 0; pointIndex < l; pointIndex++) {
      thinnedPointsIndex.push_back(pointIndex);
    }
    nrThinnedPoints = thinnedPointsIndex.size();
  }
  CDBDebug("Vector plotting %d elements %d %d", nrThinnedPoints, useFilter, usePoints.size());

  for (size_t pointNo = 0; pointNo < nrThinnedPoints; pointNo++) {

    size_t j = pointNo;
    j = thinnedPointsIndex[pointNo];
    int x = (*p1)[j].x;
    double lat = (*p1)[j].lat;
    double lon = (*p1)[j].lon;
    double rotation = (*p1)[j].rotation;

    if (rotation == 0) {
      double latForRot = lat;
      double lonForRot = lon;
      double latOffSetForRot = lat - 0.01;
      double lonOffSetForRot = lon;
      bool projectionRequired = true;

      if (projectionRequired) {
        warper->reprojfromLatLon(lonForRot, latForRot);
        warper->reprojfromLatLon(lonOffSetForRot, latOffSetForRot);
      }

      if (projectionRequired) {
        double dy = latForRot - latOffSetForRot;
        double dx = lonForRot - lonOffSetForRot;
        rotation = -(atan2(dy, dx) / (M_PI)) * 180 + 90;
      }
    }

    int y = dataSource->dHeight - (*p1)[j].y;

    float strength = (*p1)[j].v;
    float direction = (*p2)[j].v;
    if (direction == direction) direction += rotation; // Nan stays Nan

    if ((direction == direction) && (strength == strength)) { // Check for Nan
      //        CDBDebug("Drawing wind %f,%f for [%d,%d]", strength, direction, x, y);
      if (drawBarb) {
        CT::string units = dataSource->getDataObject(0)->getUnits();
        bool toKnots = false;
        if (!(units.equalsIgnoreCase("kt") || units.equalsIgnoreCase("kts") || units.equalsIgnoreCase("knot"))) {
          toKnots = true;
        }
        if (lat > 0) {
          drawImage->drawBarb(x, y, ((270 - direction) / 360) * M_PI * 2, 0, strength, drawVectorLineColor, drawVectorLineWidth, toKnots, false, drawVectorPlotValue);
        } else {
          drawImage->drawBarb(x, y, ((270 - direction) / 360) * M_PI * 2, 0, strength, drawVectorLineColor, drawVectorLineWidth, toKnots, true, drawVectorPlotValue);
        }
      }
      if (drawVector) {
        drawImage->drawVector(x, y, ((270 - direction) / 360) * M_PI * 2, strength * drawVectorVectorScale, drawVectorLineColor, drawVectorLineWidth);
      }
      // void drawBarb(int x,int y,double direction, double strength,int color,bool toKnots,bool flip);
      if (drawVectorPlotStationId) {
        if ((*p1)[j].paramList.size() > 0) {
          CT::string value = (*p1)[j].paramList[0].value;
          if (drawBarb) {
            if ((direction >= 90) && (direction <= 270)) {
              drawImage->setText(value.c_str(), value.length(), x - value.length() * 3, y - 20, drawPointTextColor, 0);
            } else {
              drawImage->setText(value.c_str(), value.length(), x - value.length() * 3, y + 6, drawPointTextColor, 0);
            }
          } else {
            if ((direction >= 90) && (direction <= 270)) {
              drawImage->setText(value.c_str(), value.length(), x - value.length() * 3, y + 6, drawPointTextColor, 0);
            } else {
              drawImage->setText(value.c_str(), value.length(), x - value.length() * 3, y - 20, drawPointTextColor, 0);
            }
          }
        }
      }
      if (drawVectorPlotValue && !drawBarb) {
        if (!drawDiscs) {
          t.print(drawVectorTextFormat.c_str(), strength);
          if ((direction >= 90) && (direction <= 270)) {
            drawImage->setText(t.c_str(), t.length(), x - t.length() * 3, y - 20, drawPointTextColor, 0);
          } else {
            drawImage->setText(t.c_str(), t.length(), x - t.length() * 3, y + 6, drawPointTextColor, 0);
          }
        }
      }

      if (drawDiscs) {
        // Draw a disc with the speed value in text and the dir. value as an arrow
        int x = (*p1)[j].x;
        int y = dataSource->dHeight - (*p1)[j].y;
        t.print(drawPointTextFormat.c_str(), strength);
        drawImage->setTextDisc(x, y, drawPointDiscRadius, t.c_str(), drawPointFontFile, drawPointFontSize, drawPointTextColor, drawPointFillColor, drawPointLineColor);
        drawImage->drawVector2(x, y, ((90 + direction) / 360.) * M_PI * 2, 10, drawPointDiscRadius, drawPointFillColor, drawVectorLineWidth);
      }
    }
  }
}

void CImgRenderPoints::render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) {
  drawVector = false;
  drawPoints = true;
  drawBarb = false;
  drawDiscs = false;
  drawVolume = false;
  drawSymbol = false;
  drawZoomablePoints = false;
  doThinning = false;
  thinningRadius = 25;

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

  drawVectorLineColor = CColor(0, 0, 128, 255);
  drawVectorLineWidth = 1.0;
  drawVectorPlotStationId = false;
  drawVectorPlotValue = false;
  drawVectorVectorScale = 1.0;
  drawVectorTextFormat = "%0.1f";
  drawVectorVectorStyle = "disc";

  useFilter = false;
  useDrawPointFillColor = false;
  useDrawPointTextColor = false;
  isRadiusAndValue = false;

  symbolIntervals = NULL;

  std::map<std::string, std::vector<Feature *>> featureStore = CConvertGeoJSON::featureStore;
  std::vector<Feature *> features;
  features = featureStore[dataSource->featureSet.c_str()];

  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  if (styleConfiguration == NULL || styleConfiguration->styleConfig == NULL) {
    return;
  }
  CServerConfig::XMLE_Style *styleConfig = styleConfiguration->styleConfig;
  for (size_t pointDefinitionIndex = 0; pointDefinitionIndex < styleConfig->Point.size(); pointDefinitionIndex += 1) {
    CServerConfig::XMLE_Point *pointConfig = styleConfig->Point[pointDefinitionIndex];

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
      //     drawText=true;
      drawSymbol = false;
    } else if (drawPointPointStyle.equalsIgnoreCase("volume")) {
      drawPoints = false;
      drawVolume = true;
      drawDiscs = false;
      //     drawText=false;
      drawSymbol = false;
    } else if (drawPointPointStyle.equalsIgnoreCase("symbol")) {
      drawPoints = false;
      drawVolume = false;
      drawDiscs = false;
      //     drawText=false;
      drawSymbol = true;
    } else if (drawPointPointStyle.equalsIgnoreCase("zoomablepoint")) {
      drawPoints = true;
      drawVolume = false;
      drawDiscs = false;
      //     drawText=false;
      drawSymbol = false;
      drawZoomablePoints = true;
    } else {
      drawPoints = true;
      drawDiscs = false;
      drawVolume = false;
      //     drawText=true;
      drawSymbol = false;
    }
    //   CDBDebug("drawPoints=%d drawText=%d drawBarb=%d drawVector=%d drawVolume=%d drawSymbol=%d", drawPoints, drawText, drawBarb, drawVector, drawVolume, drawSymbol);

    // CStyleConfiguration *styleConfiguration = dataSource->getStyle();
    if (styleConfiguration != NULL) {
      if (drawSymbol) {
        if (styleConfiguration->symbolIntervals != NULL) {
          symbolIntervals = styleConfiguration->symbolIntervals;
        }
      }
      if (styleConfiguration->styleConfig != NULL) {
        CServerConfig::XMLE_Style *s = styleConfiguration->styleConfig;
        if (s->FilterPoints.size() == 1) {
          if (s->FilterPoints[0]->attr.use.empty() == false) {
            CT::string filterPointsUse = s->FilterPoints[0]->attr.use.c_str();
            std::vector<CT::string> use = filterPointsUse.splitToStack(",");
            for (std::vector<CT::string>::iterator it = use.begin(); it != use.end(); ++it) {
              //            CDBDebug("adding %s to usePoints", it->c_str());
              usePoints.insert(it->c_str());
            }
            useFilter = true;
          }
          if (s->FilterPoints[0]->attr.skip.empty() == false) {
            CT::string filterPointsSkip = s->FilterPoints[0]->attr.skip.c_str();
            useFilter = true;
            std::vector<CT::string> skip = filterPointsSkip.splitToStack(",");
            for (std::vector<CT::string>::iterator it = skip.begin(); it != skip.end(); ++it) {
              skipPoints.insert(it->c_str());
            }
          }
        }
      }

    } else {

      CDBDebug("styleConfiguration==NULL!!!!");
    }

    if (settings.indexOf("thin") != -1) {
      doThinning = true;
      if (styleConfiguration != NULL) {
        if (styleConfiguration->styleConfig != NULL) {
          CServerConfig::XMLE_Style *s = styleConfiguration->styleConfig;
          if (s->Thinning.size() == 1) {
            if (s->Thinning[0]->attr.radius.empty() == false) {
              thinningRadius = s->Thinning[0]->attr.radius.toInt();
              // CDBDebug("Thinning radius = %d",s -> Thinning[0]->attr.radius.toInt());
            }
          }
        }
      }
    }

    bool isVector = ((dataSource->getNumDataObjects() == 2) && (dataSource->getDataObject(0)->cdfVariable->getAttributeNE("ADAGUC_GEOJSONPOINT") == NULL));
    if (drawPointPointStyle.equals("radiusandvalue")) {
      isVector = false;
      isRadiusAndValue = true;
    }
    if (!isVector) {
      renderSinglePoints(warper, dataSource, drawImage, styleConfiguration, pointConfig);
    }

    if (isVector) {
      CDBDebug("VECTOR");
      renderVectorPoints(warper, dataSource, drawImage, styleConfiguration);
    }
  }
}

int CImgRenderPoints::set(const char *values) {

  settings.copy(values);
  return 0;
}

int CImgRenderPoints::getPixelIndexForValue(CDataSource *dataSource, float val) {
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

CColor CImgRenderPoints::getPixelColorForValue(CDrawImage *drawImage, CDataSource *dataSource, float val) {
  bool isNodata = false;

  CColor color;
  if (dataSource->getDataObject(0)->hasNodataValue) {
    if (val == float(dataSource->getDataObject(0)->dfNodataValue)) isNodata = true;
    if (!(val == val)) isNodata = true;
  }
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  if (!isNodata) {
    for (size_t j = 0; j < styleConfiguration->shadeIntervals->size(); j++) {
      CServerConfig::XMLE_ShadeInterval *shadeInterval = ((*styleConfiguration->shadeIntervals)[j]);
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
