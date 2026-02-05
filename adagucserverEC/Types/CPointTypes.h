#include <CKeyValuePair.h>
#include <cmath>
#ifndef CPOINT_TYPES_H
#define CPOINT_TYPES_H

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
   * @param x X coordinate of the point in the current screenspace, as defined in the GetMap request, available in srvParams->geoParams.
   * @param y Y coordinate of the point in the current screenspace, as defined in the GetMap request, available in srvParams->geoParams.
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
   * @param x X coordinate of the point in the current screenspace, as defined in the GetMap request, available in srvParams->geoParams.
   * @param y Y coordinate of the point in the current screenspace, as defined in the GetMap request, available in srvParams->geoParams.
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
  CKeyValueDescriptionPairs paramList;
};

class CFeature {
public:
  CFeature() {}
  CFeature(int _id) { this->id = _id; }
  void addProperty(const char *propertyName, const char *propertyValue) { paramMap[propertyName] = propertyValue; }
  int id = -1;
  std::map<std::string, std::string> paramMap;
};
#endif