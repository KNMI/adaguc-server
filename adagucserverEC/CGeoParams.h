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

#ifndef CGeoParams_H
#define CGeoParams_H
#include "CTypes.h"
#include <math.h>
#include <map>

class CDataSource;
class CKeyValue {
public:
  CT::string key;
  CT::string description;
  CT::string value;

  CKeyValue(const CT::string &key, const CT::string &description, const CT::string &value) {
    this->key = key;
    this->description = description;
    this->value = value;
  }
  CKeyValue(const char *key, const char *description, const char *value) {
    this->key = key;
    this->description = description;
    this->value = value;
  }
};

bool isLonLatProjection(CT::string *projectionName);
bool isMercatorProjection(CT::string *projectionName);

struct i4point {
  int x, y;
};

struct f8point {
  double x, y;
  f8point rad() { return {.x = (x * (M_PI / 180.)), .y = (y * (M_PI / 180.))}; }
};

struct i4box {
  int left, bottom, right, top;
  void operator=(const int bbox[4]) {
    left = bbox[0];
    bottom = bbox[1];
    right = bbox[2];
    top = bbox[3];
  }
  i4point span() { return {.x = right - left, .y = top - bottom}; };
  void sort() {
    if (left > right) std::swap(left, right);
    if (bottom > top) std::swap(bottom, top);
  }
  void clip(i4box clip) {
    if (left < clip.left) left = clip.left;
    if (bottom < clip.bottom) bottom = clip.bottom;
    if (right > clip.right) right = clip.right;
    if (top > clip.top) top = clip.top;
  }
  void toArray(int box[4]) {
    box[0] = left;
    box[1] = bottom;
    box[2] = right;
    box[3] = top;
  }
};

struct f8box {
  double left, bottom, right, top;
  void operator=(const double bbox[4]) {
    left = bbox[0];
    bottom = bbox[1];
    right = bbox[2];
    top = bbox[3];
  }
  f8point span() { return {.x = right - left, .y = top - bottom}; }

  void sort() {
    if (left > right) std::swap(left, right);
    if (bottom > top) std::swap(bottom, top);
  }
  void clip(i4box clip) {
    if (left < clip.left) left = clip.left;
    if (bottom < clip.bottom) bottom = clip.bottom;
    if (right > clip.right) right = clip.right;
    if (top > clip.top) top = clip.top;
  }
  void toArray(double box[4]) {
    box[0] = left;
    box[1] = bottom;
    box[2] = right;
    box[3] = top;
  }

  f8box swapXY() { return {.left = bottom, .bottom = left, .right = top, .top = right}; }
  double get(size_t index) {
    switch (index) {
    case 0:
      return left;
    case 1:
      return bottom;
    case 2:
      return right;
    case 3:
      return top;
    default:
      return NAN;
    }
  }
};

struct f8component {
  double u, v;
  double magnitude() { return hypot(u, v); }
  double direction() { return atan2(v, u); }
  double angledeg() { return ((atan2(u, v) * (180 / M_PI) + 180)); }
};

/**
 * Class which represent discrete points as integer
 */
class PointD {
public:
  PointD(int &x, int &y) {
    this->x = x;
    this->y = y;
  }
  int x, y;
};

/**
 * Class which represent discrete points as integer with float values
 */
class PointDV {
public:
  PointDV(int &x, int &y, float &v, const char *id) {
    this->x = x;
    this->y = y;
    this->v = v;
    this->id = id;
  }
  PointDV(int &x, int &y, float &v) {
    this->x = x;
    this->y = y;
    this->v = v;
  }
  int x, y;
  float v;
  CT::string id;
};

/**
 * Class which represent points in the screenspace coordinates accompanied with lat and lon coordinates.
 */
class PointDVWithLatLon {
public:
  /**
   * @brief Construct a new Point with a value. Both screenspace coordinates and lat/lon coordinates need to be provided.
   *
   * @param x X coordinate of the point in the current screenspace, as defined in the GetMap request, available in srvParams->Geo.
   * @param y Y coordinate of the point in the current screenspace, as defined in the GetMap request, available in srvParams->Geo.
   * @param lon Longitude of the point
   * @param lat Latitude of the point
   * @param v Value of the point (float)
   */
  PointDVWithLatLon(int &x, int &y, double &lon, double &lat, float &v) {
    this->x = x;
    this->y = y;
    this->v = v;
    this->lon = lon;
    this->lat = lat;
    rotation = NAN;
  }
  /**
   * @brief Construct a new Point with a value. Both screenspace coordinates, lat/lon coordinates, rotation and radius need to be provided.
   *
   * @param x X coordinate of the point in the current screenspace, as defined in the GetMap request, available in srvParams->Geo.
   * @param y Y coordinate of the point in the current screenspace, as defined in the GetMap request, available in srvParams->Geo.
   * @param lon Longitude of the point
   * @param lat Latitude of the point
   * @param v Value of the point (float)
   * @param rotation Rotation of the point (vector)
   * @param radiusX X Radius of the point/disc
   * @param radiusY Y Radius of the point/disc
   */
  PointDVWithLatLon(int &x, int &y, double &lon, double &lat, float &v, double &rotation, float &radiusX, float &radiusY) {
    this->x = x;
    this->y = y;
    this->v = v;
    this->lon = lon;
    this->lat = lat;
    this->rotation = rotation;
    this->radiusX = radiusX;
    this->radiusY = radiusY;
  }
  int x, y;
  float v, lon, lat, rotation, radiusX, radiusY;

  /** Array containing key description and values
   * can be used to assign extra attributes to a point. By default the system will add the value of the point.
   * You are free to add additional parameters. These will show up in the GetFeatureInfo.
   */
  std::vector<CKeyValue> paramList;
};

class CFeature {
public:
  CFeature() {}
  CFeature(int _id) { this->id = _id; }
  void addProperty(const char *propertyName, const char *propertyValue) { paramMap[propertyName] = propertyValue; }
  int id = -1;
  std::map<std::string, std::string> paramMap;
};

struct CGeoParams {
  int dWidth = 1;
  int dHeight = 1;
  f8box bbox;
  double dfCellSizeX = 0;
  double dfCellSizeY = 0;
  CT::string CRS;
  CT::string BBOX_CRS;
  void copy(CGeoParams &_geo);
  void copy(CDataSource *dataSource);
  f8box swapXY();
};

void CoordinatesXYtoScreenXY(double &x, double &y, CGeoParams &geoParam);
void CoordinatesXYtoScreenXY(f8point &p, CGeoParams &geoParam);
void CoordinatesXYtoScreenXY(f8box &b, CGeoParams &geoParam);

int findClosestPoint(std::vector<PointDVWithLatLon> &points, double lon_coordinate, double lat_coordinate);

class GeoOptions {
public:
  double bbox[4];
  int indices[4];
  CT::string proj4;
  int level;
};

#endif
