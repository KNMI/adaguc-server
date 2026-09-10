#ifndef CGeoJSONData_H
#define CGeoJSONData_H
#include <vector>
#include <map>
#include "CTString.h"

class GeoPoint {
  float lon;
  float lat;

public:
  GeoPoint(float lon, float lat);
  float getLon();
  float getLat();
  std::string toString();
};

class PointArray {
  std::vector<float> lons;
  std::vector<float> lats;

public:
  void addPoint(float lon, float lat);
  float *getLons();
  float *getLats();
  std::string toString();
  int getSize();
};

class Polygon {
  PointArray points;
  std::vector<PointArray> holes;

public:
  void addPoint(float lon, float lat);
  void newHole();
  void addHolePoint(float lon, float lat);
  PointArray &getHole(int i);
  std::string toString();
  int getSize();
  float *getLats();
  float *getLons();
  std::vector<PointArray> getHoles();
};

class Polyline {
  PointArray points;

public:
  void addPoint(float lon, float lat);
  std::string toString();
  int getSize();
  float *getLats();
  float *getLons();
};

typedef enum { typeNone, typeInt, typeDouble, typeStr } FeaturePropertyType;

class FeatureProperty {
private:
  FeaturePropertyType type;
  std::string pstr;
  int64_t intVal;
  double dblVal;

public:
  FeatureProperty(int64_t i);
  FeatureProperty(std::string s);

  FeatureProperty(double d);

  FeatureProperty();

  FeaturePropertyType getType();

  double getDblVal();

  int getIntVal();

  std::string getStringVal();

  std::string toString();
  std::string toString(const char *fmt);
  std::string toString(std::string fmt);
};

class Feature {
  std::string id;
  std::vector<Polygon> polygons;
  std::map<std::string, FeatureProperty *> fp;
  std::vector<Polyline> polylines;
  std::vector<GeoPoint> points;

public:
  Feature();
  ~Feature();
  Feature(std::string _id);
  Feature(const char *_id);
  void newPolygon();
  void newPolyline();
  void addPolygonPoint(float lon, float lat);
  void addPolylinePoint(float lon, float lat);
  void newHole();
  void addHolePoint(float lon, float lat);
  std::string toString();
  std::vector<Polygon> *getPolygons();
  std::vector<Polyline> *getPolylines();
  std::vector<GeoPoint> *getPoints();
  std::string getId() { return id; }
  void setId(std::string s) { id = s; }
  void addPoint(float lon, float lat);
  void addPropInt64(std::string name, int64_t v);
  void addProp(std::string name, char *v);
  void addProp(std::string name, double v);
  std::map<std::string, FeatureProperty *> *getFp();
  bool hasHoles();
};
#endif
