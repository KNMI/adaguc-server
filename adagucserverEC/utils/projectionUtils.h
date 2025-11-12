#ifndef PROJECTIONUTILS_H
#define PROJECTIONUTILS_H

#include "CGeoParams.h"
#include "Types/CPointTypes.h"
bool isLonLatProjection(CT::string *projectionName);
bool isMercatorProjection(CT::string *projectionName);
void CoordinatesXYtoScreenXY(double &x, double &y, CGeoParams &geoParam);
void CoordinatesXYtoScreenXY(f8point &p, CGeoParams &geoParam);
void CoordinatesXYtoScreenXY(f8box &b, CGeoParams &geoParam);
int findClosestPoint(std::vector<PointDVWithLatLon> &points, double lon_coordinate, double lat_coordinate);

#endif