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
// static float  NegativeNaN = log(-1) ;

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

class CGeoParams {
public:
  int dWidth, dHeight;
  double dfBBOX[4];
  double dfCellSizeX, dfCellSizeY;
  CT::string CRS, BBOX_CRS;
  CGeoParams() {
    dWidth = 1;
    dHeight = 1;
    dfBBOX[0] = 0;
    dfBBOX[1] = 0;
    dfBBOX[2] = 0;
    dfBBOX[3] = 0;
    dfCellSizeX = 0;
    dfCellSizeY = 0;
  }
  int copy(CGeoParams *_Geo) {
    if (_Geo == NULL) return 1;
    dWidth = _Geo->dWidth;
    dHeight = _Geo->dHeight;
    dfCellSizeX = _Geo->dfCellSizeX;
    dfCellSizeY = _Geo->dfCellSizeY;
    CRS.copy(&_Geo->CRS);
    BBOX_CRS.copy(&_Geo->BBOX_CRS);
    for (int j = 0; j < 4; j++) dfBBOX[j] = _Geo->dfBBOX[j];
    return 0;
  }
  static bool isLonLatProjection(CT::string *projectionName) {
    if (projectionName->indexOf("+proj=longlat") == 0) {
      return true;
    }
    if (projectionName->equals("EPSG:4326")) {
      return true;
    }
    return false;
  }
  static bool isMercatorProjection(CT::string *projectionName) {
    if (projectionName->indexOf("+proj=merc") == 0) {
      return true;
    }
    if (projectionName->equals("EPSG:3857") || projectionName->equals("EPSG:900913")) {
      return true;
    }
    return false;
  }
};

class CBBOX {
public:
  double left, bottom, right, top;
  CBBOX &operator=(double f[4]) {
    left = f[0];
    bottom = f[1];
    right = f[2];
    top = f[3];
    return *this;
  }
};

class CPoint {
public:
  double x, y;
  CPoint &operator=(double f[2]) {
    x = f[0];
    y = f[1];
    return *this;
  }
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
    rotation = 0;
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

void CoordinatesXYtoScreenXY(double &x, double &y, CGeoParams *geoParam);
void CoordinatesXYtoScreenXY(CPoint &p, CGeoParams *geoParam);
void CoordinatesXYtoScreenXY(CBBOX &b, CGeoParams *geoParam);
#endif
