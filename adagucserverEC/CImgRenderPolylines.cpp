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

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define CCONVERTUGRIDMESH_NODATA -32000

const char *CImgRenderPolylines::className = "CImgRenderPolylines";

struct Point2D {
  double x;
  double y;
};

Point2D compute2DPolygonCentroid(const Point2D *vertices, int vertexCount) {
  Point2D centroid = {0, 0};
  double signedArea = 0.0;
  double x0 = 0.0; // Current vertex X
  double y0 = 0.0; // Current vertex Y
  double x1 = 0.0; // Next vertex X
  double y1 = 0.0; // Next vertex Y
  double a = 0.0;  // Partial signed area

  int lastdex = vertexCount - 1;
  const Point2D *prev = &(vertices[lastdex]);
  const Point2D *next;

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

Point2D getCentroid(const float *polyX, const float *polyY, const int numPoints) {
  Point2D vertices[numPoints];
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
    if (styleConfiguration->styleConfig != NULL) {
      CServerConfig::XMLE_Style *s = styleConfiguration->styleConfig;
      int numFeatures = s->FeatureInterval.size();
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
  }

  CT::string name = dataSource->featureSet;

  bool projectionRequired = false;
  if (dataSource->srvParams->Geo->CRS.length() > 0) {
    projectionRequired = true;
  }

  int height = drawImage->getHeight();

  double cellSizeX = (dataSource->srvParams->Geo->dfBBOX[2] - dataSource->srvParams->Geo->dfBBOX[0]) / double(dataSource->dWidth);
  double cellSizeY = (dataSource->srvParams->Geo->dfBBOX[3] - dataSource->srvParams->Geo->dfBBOX[1]) / double(dataSource->dHeight);
  double offsetX = dataSource->srvParams->Geo->dfBBOX[0] + cellSizeX / 2;
  double offsetY = dataSource->srvParams->Geo->dfBBOX[1] + cellSizeY / 2;

  std::map<std::string, std::vector<Feature *>> featureStore = CConvertGeoJSON::featureStore;

  bool noOverlap = true;
  if (styleConfiguration->styleConfig->RenderSettings.size() == 1) {
    noOverlap = !styleConfiguration->styleConfig->RenderSettings[0]->attr.featuresoverlap.equals("true");
  }
  bool randomStart = false;
  if (styleConfiguration->styleConfig->RenderSettings.size() == 1) {
    randomStart = styleConfiguration->styleConfig->RenderSettings[0]->attr.randomizefeatures.equals("false");
  }
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
        CColor drawPointLineColor2(featureStyle.color.c_str());
        float drawPointLineWidth = featureStyle.width;
        // if(featureIndex!=0)break;
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

          drawImage->poly(projectedX, projectedY, cnt, drawPointLineWidth, drawPointLineColor2, drawPointLineColor2, true, false);
          // Determine centroid for first polygon.
          if (firstPolygon) {
            Point2D centroid = getCentroid(polyX, polyY, numPoints);
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
                featureId.print(it->second->toString(featureStyle.propertyFormat));
              } else {
                featureId = feature->getId();
              }
              CColor labelColor(featureStyle.fontColor);
              drawImage->drawCenteredTextNoOverlap(dlon, height - dlat, featureStyle.fontFile.c_str(), featureStyle.fontSize, featureStyle.angle, featureStyle.padding, featureId, labelColor,
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
                  dlat = CCONVERTUGRIDMESH_NODATA;
                  dlon = CCONVERTUGRIDMESH_NODATA;
                }
                projectedHoleX[j] = dlon;
                projectedHoleY[j] = height - dlat;
              }
              drawImage->poly(projectedHoleX, projectedHoleY, holeSize, drawPointLineWidth, drawPointLineColor2, drawPointLineColor2, true, false);
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

          drawImage->poly(projectedX, projectedY, cnt, drawPointLineWidth, drawPointLineColor2, drawPointLineColor2, false, false);
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

FeatureStyle CImgRenderPolylines::getAttributesForFeature(CFeature *feature, CT::string id, CStyleConfiguration *styleConfig) {
  FeatureStyle fs;
  fs.color = "#008000FF";
  fs.width = 3;
  fs.fontSize = 0;
  fs.propertyFormat = "%s";
  fs.padding = 3;
  fs.angle = 0;
  fs.fontColor = "#000000FF";
  const char *fontLoc = getenv("ADAGUC_FONT");
  if (fontLoc != NULL) {
    fs.fontFile = fontLoc;
  }

  for (size_t j = 0; j < styleConfig->featureIntervals.size(); j++) {
    // Draw border if borderWidth>0
    if (styleConfig->featureIntervals[j]->attr.match.empty() == false) {
      CT::string match = styleConfig->featureIntervals[j]->attr.match;
      CT::string matchString;
      if (styleConfig->featureIntervals[j]->attr.matchid.empty() == false) {
        // match on matchid
        CT::string matchId;
        matchId = styleConfig->featureIntervals[j]->attr.matchid;
        std::map<std::string, std::string>::iterator attributeValueItr = feature->paramMap.find(matchId.c_str());
        if (attributeValueItr != feature->paramMap.end()) {
          matchString = attributeValueItr->second.c_str();
        }
      } else {
        // match on id
        matchString = id;
      }
      regex_t regex;
      int ret = regcomp(&regex, match.c_str(), 0);
      if (!ret) {
        if (regexec(&regex, matchString.c_str(), 0, NULL, 0) == 0) {
          CServerConfig::XMLE_FeatureInterval *fi = styleConfig->featureIntervals[j];
          // Matched
          if ((fi->attr.borderwidth.empty() == false) && ((fi->attr.borderwidth.toFloat()) > 0)) {
            fs.width = fi->attr.borderwidth.toFloat();
            // A border should be drawn
            if (fi->attr.bordercolor.empty() == false) {
              fs.color = fi->attr.bordercolor;
            } else {
              // Use default color
              fs.color = CT::string("#00AA22FF");
            }
          } else {
            // Draw no border
            fs.width = 0;
          }
          if ((fi->attr.labelfontsize.empty() == false) && (fi->attr.labelfontsize.toFloat() > 0)) {
            fs.fontSize = fi->attr.labelfontsize.toFloat();
          }
          if (fi->attr.labelfontfile.empty() == false) {
            fs.fontFile = fi->attr.labelfontfile;
          }
          if (fi->attr.labelcolor.empty() == false) {
            fs.fontColor = fi->attr.labelcolor;
          }
          if (fi->attr.labelpropertyname.empty() == false) {
            fs.propertyName = fi->attr.labelpropertyname;
          }
          if (fi->attr.labelpropertyformat.empty() == false) {
            fs.propertyFormat = fi->attr.labelpropertyformat;
          }
          if ((fi->attr.labelangle.empty() == false) && (fi->attr.labelangle.isNumeric())) {
            fs.angle = fi->attr.labelangle.toFloat() * M_PI / 180;
          }
          if ((fi->attr.labelpadding.empty() == false) && (fi->attr.labelpadding.isInt())) {
            fs.padding = fi->attr.labelpadding.toInt();
          }
          return fs;
        }
      }
    }
  }
  return fs;
}
