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

#include "CGeoParams.h"
#include "CDataSource.h"

void CoordinatesXYtoScreenXY(double &x, double &y, CGeoParams *geoParam) {
  x -= geoParam->bbox.left;
  y -= geoParam->bbox.top;
  double bboxW = geoParam->bbox.right - geoParam->bbox.left;
  double bboxH = geoParam->bbox.bottom - geoParam->bbox.top;
  x /= bboxW;
  y /= bboxH;
  x *= double(geoParam->dWidth);
  y *= double(geoParam->dHeight);
}

void CoordinatesXYtoScreenXY(f8point &p, CGeoParams *geoParam) {
  p.x -= geoParam->bbox.left;
  p.y -= geoParam->bbox.top;
  double bboxW = geoParam->bbox.right - geoParam->bbox.left;
  double bboxH = geoParam->bbox.bottom - geoParam->bbox.top;
  p.x /= bboxW;
  p.y /= bboxH;
  p.x *= double(geoParam->dWidth);
  p.y *= double(geoParam->dHeight);
}

void CoordinatesXYtoScreenXY(f8box &b, CGeoParams *geoParam) {
  CoordinatesXYtoScreenXY(b.left, b.top, geoParam);
  CoordinatesXYtoScreenXY(b.right, b.bottom, geoParam);
}

int findClosestPoint(std::vector<PointDVWithLatLon> &points, double lon_coordinate, double lat_coordinate) {
  float closestDistance = 0;
  int closestIndex = -1;
  for (size_t j = 0; j < points.size(); j++) {
    PointDVWithLatLon &point = points[j];
    float distance = hypot(point.lon - lon_coordinate, point.lat - lat_coordinate);
    if (distance < closestDistance || j == 0) {
      closestIndex = j;
      closestDistance = distance;
    }
  }
  return closestIndex;
}

bool isLonLatProjection(CT::string *projectionName) {
  if (projectionName->indexOf("+proj=longlat") == 0) {
    return true;
  }
  if (projectionName->equals("EPSG:4326")) {
    return true;
  }
  return false;
}
bool isMercatorProjection(CT::string *projectionName) {
  if (projectionName->indexOf("+proj=merc") == 0) {
    return true;
  }
  if (projectionName->equals("EPSG:3857") || projectionName->equals("EPSG:900913")) {
    return true;
  }
  return false;
}

void CGeoParams::copy(CGeoParams &sourceGeo) {
  dWidth = sourceGeo.dWidth;
  dHeight = sourceGeo.dHeight;
  dfCellSizeX = sourceGeo.dfCellSizeX;
  dfCellSizeY = sourceGeo.dfCellSizeY;
  CRS = sourceGeo.CRS;
  BBOX_CRS = sourceGeo.BBOX_CRS;
  bbox = sourceGeo.bbox;
}

void CGeoParams::copy(CDataSource *dataSource) {
  this->dWidth = dataSource->dWidth;
  this->dHeight = dataSource->dHeight;
  this->bbox = dataSource->dfBBOX;
  this->dfCellSizeX = dataSource->dfCellSizeX;
  this->dfCellSizeY = dataSource->dfCellSizeY;
  this->CRS = dataSource->nativeProj4;
}
