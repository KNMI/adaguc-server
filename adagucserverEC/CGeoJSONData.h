#ifndef CGeoJSONData_H
#define CGeoJSONData_H
#include <vector>
#include <map>
#include "CTypes.h"
#include "CDebugger.h"

class GeoPoint {
  float lon;
  float lat;

public:
  GeoPoint(float lon, float lat);
  float getLon();
  float getLat();
  CT::string toString();
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
  CT::string toString();
  int getSize();
  float *getLats();
  float *getLons();
  std::vector<PointArray> getHoles();
};

class Polyline {
  PointArray points;

public:
  void addPoint(float lon, float lat);
  CT::string toString();
  int getSize();
  float *getLats();
  float *getLons();
};

typedef enum { typeNone, typeInt, typeDouble, typeStr } FeaturePropertyType;

class FeatureProperty {
private:
  FeaturePropertyType type;
  CT::string pstr;
  int intVal;
  double dblVal;
  DEF_ERRORFUNCTION();

public:
  FeatureProperty(int i) {
    type = typeInt;
    intVal = i;
    dblVal = -12;
    pstr = "EMPTY i";
  }
  FeatureProperty(CT::string s) {
    type = typeStr;
    pstr = CT::string(s);
    intVal = -1;
    dblVal = -2;
  }

  FeatureProperty(double d) {
    type = typeDouble;
    dblVal = d;
    intVal = -21;
    pstr = "EMPTY d";
  }

  FeatureProperty() { type = typeNone; }

  FeaturePropertyType getType() { return type; }

  double getDblVal() { return dblVal; }

  int getIntVal() { return intVal; }

  CT::string getStringVal() { return pstr; }

  CT::string toString() {
    CT::string s;
    if (type == typeInt) {
      s.print("%d", intVal);
    } else if (type == typeStr) {
      s.print("%s", pstr.c_str());
    } else if (type == typeDouble) {
      s.print("%f", dblVal);
    } else {
      s.print("NONE");
    }
    return s;
  }
};

// class FeatureProperties {
//   std::map<std::string, FeatureProperty> props;
// };

class Feature {
  CT::string id;
  std::vector<Polygon> polygons;
  std::map<std::string, FeatureProperty *> fp;
  std::vector<Polyline> polylines;
  std::vector<GeoPoint> points;
  DEF_ERRORFUNCTION();

public:
  Feature();
  ~Feature();
  Feature(CT::string _id);
  Feature(const char *_id);
  void newPolygon();
  void newPolyline();
  void addPolygonPoint(float lon, float lat);
  void addPolylinePoint(float lon, float lat);
  void newHole();
  void addHolePoint(float lon, float lat);
  CT::string toString();
  std::vector<Polygon> getPolygons();
  std::vector<Polyline> getPolylines();
  std::vector<GeoPoint> getPoints();
  CT::string getId() { return id; }
  void setId(CT::string s) { id = s; }
  void addPoint(float lon, float lat);
  void addProp(CT::string name, int v);
  void addProp(CT::string name, char *v);
  void addProp(CT::string name, double v);
  std::map<std::string, FeatureProperty *> &getFp();
  bool hasHoles();
};

#endif
