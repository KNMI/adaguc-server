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

#include "CImgRenderPolylines.h"
#include <set>
#include "CConvertGeoJSON.h"
#include <string>
#include <algorithm>
#include "CRectangleText.h"

//   #define MEASURETIME

#define POLY_NODATA -32000
const char *fontLoc = getenv("ADAGUC_FONT");

struct FeatureStyle {
  CColor backgroundColor;
  CColor borderColor;
  double borderWidth; // 0 means no border
  CColor fillColor;
  bool hasFill;
  CT::string fontFile;
  double fontSize;
  CColor fontColor;
  CT::string propertyName;
  CT::string propertyFormat;
  double angle;
  int padding;
};

FeatureStyle getAttributesForFeature(CFeature *feature, CT::string id, CStyleConfiguration *styleConfig) {

  CColor backgroundColor = CColor(0, 0, 0, 0);
  for (auto featureIntervalCfg : styleConfig->featureIntervals) {
    auto &featureAttr = featureIntervalCfg->attr;
    if (styleConfig->renderMethod == RM_POLYGON && !featureAttr.bgcolor.empty()) {
      backgroundColor = featureAttr.bgcolor.c_str();
    }
    if (featureAttr.match.empty()) continue;

    CT::string matchString = id;
    if (!featureAttr.matchid.empty()) {
      // match on matchid
      std::string matchId = featureAttr.matchid.c_str();
      std::map<std::string, std::string>::iterator attributeValueItr = feature->paramMap.find(matchId);
      if (attributeValueItr != feature->paramMap.end()) {
        matchString = attributeValueItr->second.c_str();
      }
    }

    // CDBDebug("getAttributesForFeature %s", matchString.c_str());

    regex_t regex;
    int ret = regcomp(&regex, featureAttr.match.c_str(), 0);
    if (ret) continue;
    if (regexec(&regex, matchString.c_str(), 0, NULL, 0) != 0) continue;
    return {.backgroundColor = backgroundColor,
            .borderColor = featureAttr.bordercolor.empty() ? "#008000FF" : featureAttr.bordercolor.c_str(),
            .borderWidth = ((featureAttr.borderwidth.empty() == false) && ((featureAttr.borderwidth.toDouble()) > 0)) ? featureAttr.borderwidth.toDouble() : 0,
            .fillColor = featureAttr.fillcolor.empty() ? "#6060FFFF" : featureAttr.fillcolor.c_str(),
            .hasFill = styleConfig->renderMethod == RM_POLYGON && !featureAttr.fillcolor.empty(),
            .fontFile = featureAttr.labelfontfile.empty() ? fontLoc : featureAttr.labelfontfile.c_str(),
            .fontSize = ((featureAttr.labelfontsize.empty() == false) && (featureAttr.labelfontsize.toDouble() > 0)) ? featureAttr.labelfontsize.toDouble() : 0,
            .fontColor = featureAttr.labelcolor.empty() ? "#000000FF" : featureAttr.labelcolor.c_str(),
            .propertyName = featureAttr.labelpropertyname,
            .propertyFormat = featureAttr.labelpropertyformat.empty() ? "%s" : featureAttr.labelpropertyformat,
            .angle = ((featureAttr.labelangle.empty() == false) && (featureAttr.labelangle.isNumeric())) ? featureAttr.labelangle.toDouble() * M_PI / 180 : 0,
            .padding = ((featureAttr.labelpadding.empty() == false) && (featureAttr.labelpadding.isInt())) ? featureAttr.labelpadding.toInt() : 3};
  }

  return {.backgroundColor = backgroundColor,
          .borderColor = "#008000FF",
          .borderWidth = 3,
          .fillColor = "#6060FFFF",
          .hasFill = false,
          .fontFile = fontLoc == nullptr ? "" : fontLoc,
          .fontSize = 0,
          .fontColor = "#000000FF",
          .propertyName = "",
          .propertyFormat = "%s",
          .angle = 0,
          .padding = 3};
}

f8point compute2DPolygonCentroid(const f8point *vertices, int vertexCount) {
  f8point centroid = {0, 0};
  double signedArea = 0.0;
  double x0 = 0.0; // Current vertex X
  double y0 = 0.0; // Current vertex Y
  double x1 = 0.0; // Next vertex X
  double y1 = 0.0; // Next vertex Y
  double a = 0.0;  // Partial signed area

  int lastdex = vertexCount - 1;
  const f8point *prev = &(vertices[lastdex]);
  const f8point *next;

  // For all vertices in a loop
  for (int i = 0; i < vertexCount; ++i) {
    next = &(vertices[i]);
    x0 = prev->x;
    y0 = prev->y;
    x1 = next->x;
    y1 = next->y;
    a = x0 * y1 - x1 * y0;
    signedArea += a;
    centroid.x += (x0 + x1) * a;
    centroid.y += (y0 + y1) * a;
    prev = next;
  }

  signedArea *= 0.5;
  centroid.x /= (6.0 * signedArea);
  centroid.y /= (6.0 * signedArea);

  return centroid;
}

f8point getCentroid(const float *polyX, const float *polyY, const int numPoints) {
  f8point vertices[numPoints];
  for (int i = 0; i < numPoints; i++) {
    vertices[i].x = polyX[i];
    vertices[i].y = polyY[i];
  }
  return compute2DPolygonCentroid(vertices, numPoints);
}

void CImgRenderPolylines::render(CImageWarper *imageWarper, CDataSource *dataSource, CDrawImage *drawImage) {
  CColor drawPointTextColor(0, 0, 0, 255);
  CColor drawPointFillColor(0, 0, 0, 128);
  CColor drawPointLineColor(0, 0, 0, 255);

  CColor defaultColor(0, 0, 0, 255);

  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  if (styleConfiguration != NULL) {
    int numFeatures = styleConfiguration->featureIntervals.size();
    CT::string attributeValues[numFeatures];
    /* Loop through all configured FeatureInterval elements */
    for (size_t j = 0; j < styleConfiguration->featureIntervals.size(); j++) {
      CServerConfig::XMLE_FeatureInterval *featureInterval = styleConfiguration->featureIntervals[j];
      if (featureInterval->attr.match.empty() == false && featureInterval->attr.matchid.empty() == false) {
        /* Get the matchid attribute for the feature */
        CT::string attributeName = featureInterval->attr.matchid;
        for (int featureNr = 0; featureNr < numFeatures; featureNr++) {
          attributeValues[featureNr] = "";
          std::map<int, CFeature>::iterator feature = dataSource->getDataObject(0)->features.find(featureNr);
          if (feature != dataSource->getDataObject(0)->features.end()) {
            std::map<std::string, std::string>::iterator attributeValueItr = feature->second.paramMap.find(attributeName.c_str());
            if (attributeValueItr != feature->second.paramMap.end()) {
              attributeValues[featureNr] = attributeValueItr->second.c_str();
            }
          }
        }
      }
    }
  }

  CT::string name = dataSource->featureSet;

  bool projectionRequired = false;
  if (dataSource->srvParams->geoParams.crs.length() > 0) {
    projectionRequired = true;
  }

  int width = drawImage->getWidth();
  int height = drawImage->getHeight();

  double cellSizeX = dataSource->srvParams->geoParams.bbox.span().x / dataSource->dWidth;
  double cellSizeY = dataSource->srvParams->geoParams.bbox.span().y / dataSource->dHeight;
  double offsetX = dataSource->srvParams->geoParams.bbox.left + cellSizeX / 2;
  double offsetY = dataSource->srvParams->geoParams.bbox.bottom + cellSizeY / 2;

  std::map<std::string, std::vector<Feature *>> featureStore = CConvertGeoJSON::featureStore;

  bool noOverlap = true;
  bool randomStart = false;
  for (auto renderSetting : styleConfiguration->renderSettings) {
    if (!renderSetting->attr.featuresoverlap.empty()) {
      noOverlap = !renderSetting->attr.featuresoverlap.equals("true");
    }
    if (!renderSetting->attr.randomizefeatures.empty()) {
      randomStart = renderSetting->attr.randomizefeatures.equals("true"); // TODO: Ask Ernst if this is correct?
    }
  }

  bool backgroundDrawn = false;
  srand(time(NULL));
  for (std::map<std::string, std::vector<Feature *>>::iterator itf = featureStore.begin(); itf != featureStore.end(); ++itf) {
    std::string fileName = itf->first.c_str();
    if (fileName == name.c_str()) {
      std::vector<CRectangleText> rects;
      size_t featureStoreSize = featureStore[fileName].size();
      int featureRandomStart = 0; // For specifying a random polygon index to draw first
      if (randomStart) {
        featureRandomStart = rand() % featureStoreSize; // Random start for first feature to draw
      }

      for (size_t featureStepper = 0; featureStepper < featureStoreSize; featureStepper++) {
        size_t featureIndex = (featureStepper + featureRandomStart) % featureStoreSize;
        Feature *feature = featureStore[fileName][featureIndex];
        // FindAttributes for this feature
        FeatureStyle featureStyle = getAttributesForFeature(&(dataSource->getDataObject(0)->features[featureIndex]), feature->getId(), styleConfiguration);

        if (backgroundDrawn == false && featureStyle.backgroundColor.a > 0) {
          backgroundDrawn = true;
          drawImage->rectangle(0, 0, width, height, featureStyle.backgroundColor, featureStyle.backgroundColor);
        }

        std::vector<Polygon> *polygons = feature->getPolygons();
        CT::string id = feature->getId();
        for (std::vector<Polygon>::iterator itpoly = polygons->begin(); itpoly != polygons->end(); ++itpoly) {
          float *polyX = itpoly->getLons();
          float *polyY = itpoly->getLats();
          int numPoints = itpoly->getSize();
          float projectedX[numPoints];
          float projectedY[numPoints];
          bool firstPolygon = true;

          int cnt = 0;
          for (int j = 0; j < numPoints; j++) {
            double tprojectedX = polyX[j];
            double tprojectedY = polyY[j];
            int status = 0;
            if (projectionRequired) status = imageWarper->reprojfromLatLon(tprojectedX, tprojectedY);
            int dlon = 0, dlat = 0;
            if (!status && cellSizeX != 0 && cellSizeY != 0) {
              dlon = int((tprojectedX - offsetX) / cellSizeX) + 1;
              dlat = int((tprojectedY - offsetY) / cellSizeY);
              projectedX[cnt] = dlon;
              projectedY[cnt] = height - dlat;
              cnt++;
            }
          }

          drawImage->poly(projectedX, projectedY, cnt, featureStyle.borderWidth, featureStyle.borderColor, featureStyle.fillColor, true, featureStyle.hasFill);
          // Determine centroid for first polygon.
          if (firstPolygon) {
            f8point centroid = getCentroid(polyX, polyY, numPoints);
            double centroidX = centroid.x;
            double centroidY = centroid.y;
            int status = 0;
            if (projectionRequired) status = imageWarper->reprojfromLatLon(centroidX, centroidY);
            if (!status) {
              int dlon, dlat;
              dlon = int((centroidX - offsetX) / cellSizeX) + 1;
              dlat = int((centroidY - offsetY) / cellSizeY);
              std::map<std::string, FeatureProperty *>::iterator it;
              CT::string featureId;
              it = feature->getFp()->find(std::string(featureStyle.propertyName.c_str()));
              if (it != feature->getFp()->end()) {
                featureId.print(it->second->toString(featureStyle.propertyFormat).c_str());
              } else {
                featureId = feature->getId();
              }
              CColor labelColor(featureStyle.fontColor);
              drawImage->drawCenteredTextNoOverlap(dlon, height - dlat, featureStyle.fontFile.c_str(), featureStyle.fontSize, featureStyle.angle, featureStyle.padding, featureId.c_str(), labelColor,
                                                   noOverlap, rects);
            }
            firstPolygon = false;
          }
          // Draw the polygon holes
          if (true) {
            std::vector<PointArray> holes = itpoly->getHoles();
            for (std::vector<PointArray>::iterator itholes = holes.begin(); itholes != holes.end(); ++itholes) {
              float *holeX = itholes->getLons();
              float *holeY = itholes->getLats();
              int holeSize = itholes->getSize();
              float projectedHoleX[holeSize];
              float projectedHoleY[holeSize];

              for (int j = 0; j < holeSize; j++) {
                double tprojectedX = holeX[j];
                double tprojectedY = holeY[j];
                int holeStatus = 0;
                if (projectionRequired) holeStatus = imageWarper->reprojfromLatLon(tprojectedX, tprojectedY);
                int dlon, dlat;
                if (!holeStatus && cellSizeX != 0 && cellSizeY != 0) {
                  dlon = int((tprojectedX - offsetX) / cellSizeX) + 1;
                  dlat = int((tprojectedY - offsetY) / cellSizeY);
                } else {
                  dlat = POLY_NODATA;
                  dlon = POLY_NODATA;
                }
                projectedHoleX[j] = dlon;
                projectedHoleY[j] = height - dlat;
              }
              drawImage->poly(projectedHoleX, projectedHoleY, holeSize, featureStyle.borderWidth, featureStyle.borderColor, featureStyle.fillColor, true, featureStyle.hasFill);
            }
          }
        }

        std::vector<Polyline> *polylines = feature->getPolylines();
        CT::string idl = feature->getId();
        for (std::vector<Polyline>::iterator itpoly = polylines->begin(); itpoly != polylines->end(); ++itpoly) {
          float *polyX = itpoly->getLons();
          float *polyY = itpoly->getLats();
          int numPoints = itpoly->getSize();
          float projectedX[numPoints];
          float projectedY[numPoints];

          int cnt = 0;
          for (int j = 0; j < numPoints; j++) {
            double tprojectedX = polyX[j];
            double tprojectedY = polyY[j];
            int status = 0;
            if (projectionRequired) status = imageWarper->reprojfromLatLon(tprojectedX, tprojectedY);
            int dlon, dlat;
            if (!status && cellSizeX > 0 && cellSizeY > 0) {
              dlon = int((tprojectedX - offsetX) / cellSizeX) + 1;
              dlat = int((tprojectedY - offsetY) / cellSizeY);
              projectedX[cnt] = dlon;
              projectedY[cnt] = height - dlat;
              cnt++;
            }
          }

          drawImage->poly(projectedX, projectedY, cnt, featureStyle.borderWidth, featureStyle.borderColor, featureStyle.fillColor, false, featureStyle.hasFill);
        }
#ifdef MEASURETIME
        StopWatch_Stop("Feature drawn %d", featureIndex);
#endif
      }
      // Draw polygon labels here, so they end up on top
      for (CRectangleText rect : rects) {
        // drawImage->setDisc(rect.llx, rect.lly, 2, rect.color, rect.color); // dot
        drawImage->drawText(rect.llx, rect.lly, rect.fontFile.c_str(), rect.fontSize, rect.angle, rect.text.c_str(), rect.color);
      }
    }
  }
}

int CImgRenderPolylines::set(const char *values) {

  settings.copy(values);
  return 0;
}
