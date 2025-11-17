#ifndef PROJECTIONUTILS_H
#define PROJECTIONUTILS_H

#include "Types/GeoParameters.h"
#include "Types/CPointTypes.h"
bool isLonLatProjection(CT::string *projectionName);
bool isMercatorProjection(CT::string *projectionName);
void CoordinatesXYtoScreenXY(double &x, double &y, GeoParameters &geoParam);
void CoordinatesXYtoScreenXY(f8point &p, GeoParameters &geoParam);
void CoordinatesXYtoScreenXY(f8box &b, GeoParameters &geoParam);
int findClosestPoint(std::vector<PointDVWithLatLon> &points, double lon_coordinate, double lat_coordinate);

#endif