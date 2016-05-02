#ifndef CGeoJSONData_H
#define CGeoJSONData_H
#include <vector>
#include <map>
#include "CTypes.h"

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
  std::vector<PointArray>holes;
public:
  void addPoint(float lon, float lat);
  void newHole();
  void addHolePoint(float lon, float lat);
  PointArray& getHole(int i);
  CT::string toString();
  int getSize();
  float *getLats();
  float *getLons();
  std::vector<PointArray>getHoles();
};

typedef enum {
  typeNone,
  typeInt,
  typeStr
} FeaturePropertyType;

class FeatureProperty {
private:
  FeaturePropertyType type;
  CT::string pstr;
  int intVal;

public:
  FeatureProperty(int i) {
    type=typeInt;
    intVal=i;
  }
  FeatureProperty(CT::string s) {
    type=typeStr;
    pstr=s;
  }
  
  FeatureProperty() {
    type=typeNone;
  }
  
  CT::string toString() {
    CT::string s;
    if (type==typeInt) {
      s.print("%d", intVal);
    } else if (type==typeStr) {
      s.print("%s", pstr.c_str());
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
  std::map<std::string, FeatureProperty> fp;
public:
  Feature();
  Feature(CT::string _id);
  Feature(const char *_id);
  void newPolygon();
  void addPoint(float lon, float lat);
  void newHole();
  void addHolePoint(float lon, float lat);
  CT::string toString();
  std::vector<Polygon>getPolygons();
  CT::string getId() {
    return id;
  }
  void setId(CT::string s) {
    id=s;
  }
  void addProp(CT::string name, int v);
  void addProp(CT::string name, std::string v);
  std::map<std::string, FeatureProperty> getFp();
  bool hasHoles();
};

#endif