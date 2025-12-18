/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  GeoJSON helper files.
 * Author:   Ernst de Vreede (KNMI)
 * Date:     2016-08
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

#include "CGeoJSONData.h"
#include <iostream>

#include <stdio.h>

const char *Feature::className = "Feature";

CT::string FeatureProperty::toString() {
  CT::string s;
  if (type == typeInt) {
    s.print("%ld", intVal);
  } else if (type == typeStr) {
    s.print("%s", pstr.c_str());
  } else if (type == typeDouble) {
    s.print("%f", dblVal);
  } else {
    s.print("NONE");
  }
  return s;
};

CT::string FeatureProperty::toString(const char *fmt) {
  CT::string s;
  if (type == typeInt) {
    s.print(fmt, intVal);
  } else if (type == typeStr) {
    s.print(fmt, pstr.c_str());
  } else if (type == typeDouble) {
    s.print(fmt, dblVal);
  } else {
    s.print("NONE");
  }
  return s;
};

CT::string FeatureProperty::toString(std::string fmt) { return toString(fmt.c_str()); }

GeoPoint::GeoPoint(float lon, float lat) {
  this->lat = lat;
  this->lon = lon;
}

float GeoPoint::getLon() { return this->lon; }

float GeoPoint::getLat() { return this->lat; }

CT::string GeoPoint::toString() {
  CT::string s;
  s.print("point(%f, %f)", lon, lat);
  return s;
}

void PointArray::addPoint(float lon, float lat) {
  lons.push_back(lon);
  lats.push_back(lat);
}

int PointArray::getSize() { return lons.size(); }

float *PointArray::getLats() { return &lats[0]; }

float *PointArray::getLons() { return &lons[0]; }

float *Polygon::getLats() { return points.getLats(); }

float *Polygon::getLons() { return points.getLons(); }

void Polygon::addPoint(float lon, float lat) { points.addPoint(lon, lat); }

void Polygon::newHole() {
  PointArray hole;
  holes.push_back(hole);
}

void Polygon::addHolePoint(float lon, float lat) { holes[holes.size() - 1].addPoint(lon, lat); }

CT::string Polygon::toString() {
  CT::string s;
  s.print("polygon(%d) holes[%d]\n", points.getSize(), holes.size());
  return s;
}

int Polygon::getSize() { return points.getSize(); }

std::vector<PointArray> Polygon::getHoles() { return holes; }

void Feature::newPolygon() {
  Polygon poly;
  polygons.push_back(poly);
}

CT::string Polyline::toString() {
  CT::string s;
  s.print("polyline(%d)\n", points.getSize());
  return s;
}

void Polyline::addPoint(float lon, float lat) { points.addPoint(lon, lat); }

float *Polyline::getLats() { return points.getLats(); }

float *Polyline::getLons() { return points.getLons(); }

int Polyline::getSize() { return points.getSize(); }

void Feature::newPolyline() {
  Polyline line;
  polylines.push_back(line);
}

void Feature::addPolygonPoint(float lon, float lat) { polygons[polygons.size() - 1].addPoint(lon, lat); }

void Feature::addHolePoint(float lon, float lat) { polygons[polygons.size() - 1].addHolePoint(lon, lat); }

void Feature::addPolylinePoint(float lon, float lat) { polylines[polylines.size() - 1].addPoint(lon, lat); }

void Feature::newHole() { polygons[polygons.size() - 1].newHole(); }

bool Feature::hasHoles() {
  for (unsigned int p = 0; p < polygons.size(); p++) {
    if (polygons[p].getHoles().size() > 0) {
      return true;
    }
  }
  return false;
}

std::vector<Polygon> *Feature::getPolygons() { return &polygons; }

std::vector<Polyline> *Feature::getPolylines() { return &polylines; }

std::vector<GeoPoint> *Feature::getPoints() { return &points; }

CT::string Feature::toString() {
  CT::string s;
  s.print("polygons: %d\n", polygons.size());
  for (unsigned int i = 0; i < polygons.size(); i++) {
    s += polygons[i].toString();
  }
  s.printconcat("polylines: %d\n", polylines.size());
  for (unsigned int i = 0; i < polylines.size(); i++) {
    s += polylines[i].toString();
  }
  s.printconcat("points: %d\n", points.size());
  for (unsigned int i = 0; i < points.size(); i++) {
    s += points[i].toString();
  }
  s.printconcat("\n");

  for (std::map<std::string, FeatureProperty *>::iterator it = fp.begin(); it != fp.end(); ++it) {
    s += it->first.c_str();
    s += ":";
    s += it->second->toString();
  }
  return s;
}

Feature::~Feature() {
  for (std::map<std::string, FeatureProperty *>::iterator it = fp.begin(); it != fp.end(); ++it) {
    delete it->second;
  }
  fp.clear();
}

Feature::Feature() {}

Feature::Feature(CT::string _id) { id = _id; }

Feature::Feature(const char *_id) { id = _id; }

void Feature::addPropInt64(CT::string name, int64_t v) {
  FeatureProperty *f = new FeatureProperty(v);
  fp[name.c_str()] = f;
}

void Feature::addProp(CT::string name, char *v) {
  FeatureProperty *f = new FeatureProperty(v);
  fp[name.c_str()] = f;
}

void Feature::addProp(CT::string name, double v) {
  FeatureProperty *f = new FeatureProperty(v);
  fp[name.c_str()] = f;
}

std::map<std::string, FeatureProperty *> *Feature::getFp() { return &fp; }

void Feature::addPoint(float lon, float lat) { points.push_back(GeoPoint(lon, lat)); }

// #define TESTIT
#ifdef TESTIT
int main(int argc, char *argv[]) {
  Feature poly("0001");
  poly.newPolygon();
  poly.addPolygonPoint(2, 40);
  poly.addPolygonPoint(2, 20);
  poly.addPolygonPoint(40, 20);
  poly.addPolygonPoint(40, 40);
  poly.addPolygonPoint(2, 40);
  poly.newHole();
  poly.addHolePoint(10, 10);
  poly.addHolePoint(12, 10);
  poly.addHolePoint(12, 12);
  poly.addHolePoint(10, 12);
  poly.addHolePoint(10, 10);
  poly.newPolygon();
  fprintf(stderr, poly.toString().c_str());
}
#endif

/*

*/
