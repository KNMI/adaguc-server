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

const char *Feature::className="Feature";

void PointArray::addPoint(float lon, float lat) {
  lons.push_back(lon);
  lats.push_back(lat);
}

int PointArray::getSize() {
  return lons.size();
}

float *PointArray::getLats() {
  return &lats[0];
}

float *PointArray::getLons() {
  return &lons[0];
}

float *Polygon::getLats(){
  return points.getLats();
}

float *Polygon::getLons(){
  return points.getLons();
}

void Polygon::addPoint(float lon, float lat) {
  points.addPoint(lon, lat);
}

void Polygon::newHole() {
  PointArray hole;
  holes.push_back(hole);
}

void Polygon::addHolePoint(float lon, float lat) {
  holes[holes.size()-1].addPoint(lon, lat);
}

CT::string Polygon::toString() {
  CT::string s;
  s.print("poly(%d) holes[%d]\n", points.getSize(), holes.size());
  return s;
}

int Polygon::getSize(){
  return points.getSize();
}

std::vector<PointArray> Polygon::getHoles(){
  return holes;
}

void Feature::newPolygon() {
  Polygon poly;
  polygons.push_back(poly);
}

void Feature::addPoint(float lon, float lat) {
  polygons[polygons.size()-1].addPoint(lon, lat);
}

void Feature::addHolePoint(float lon, float lat){
  polygons[polygons.size()-1].addHolePoint(lon, lat);
}

void Feature::newHole(){
  polygons[polygons.size()-1].newHole();
}

bool Feature::hasHoles(){
  for (unsigned int p=0;p<polygons.size(); p++) {
    if (polygons[p].getHoles().size()>0) {
      return true;
    }
  }
  return false;
}

std::vector<Polygon>Feature::getPolygons(){
  return polygons;
}

CT::string Feature::toString() {
  CT::string s;
  s.print("polygons: %d\n", polygons.size());
  for (unsigned int i=0; i<polygons.size(); i++) {
    s+=polygons[i].toString();
  }
  return s;
}


Feature::~Feature(){
  for (std::map<std::string, FeatureProperty*>::iterator it=fp.begin(); it!=fp.end(); ++it){
    delete it->second;
  }
  fp.clear();
}

Feature::Feature(){
}
Feature::Feature(CT::string _id){
    id=_id;
}
Feature::Feature(const char *_id) {
    id=_id;
}

void Feature::addProp(CT::string name, int v){
  FeatureProperty* f=new FeatureProperty(v);
  fp[name.c_str()]=f;
}

void Feature::addProp(CT::string name, char *v){
  FeatureProperty* f=new FeatureProperty(v);
  fp[name.c_str()]=f;
} 

void Feature::addProp(CT::string name, double v){
  FeatureProperty* f=new FeatureProperty(v);
  fp[name.c_str()]=f;
}

std::map<std::string, FeatureProperty*>& Feature::getFp(){
  return fp;
}


//#define TESTIT
#ifdef TESTIT
int main(int argc, char *argv[]) {
  Feature poly("0001");
  poly.newPolygon();
  poly.addPoint(2,40);
  poly.addPoint(2,20);
  poly.addPoint(40,20);
  poly.addPoint(40,40);
  poly.addPoint(2,40);
  poly.newHole();
  poly.addHolePoint(10,10);
  poly.addHolePoint(12,10);
  poly.addHolePoint(12,12);
  poly.addHolePoint(10,12);
  poly.addHolePoint(10,10);
  poly.newPolygon();
  fprintf(stderr, poly.toString().c_str());
}
#endif

/*

*/
