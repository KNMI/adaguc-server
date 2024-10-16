#ifndef SOLAR_POSITION_H
#define SOLAR_POSITION_H

double getSolarZenithAngle(double lat, double lon, double timestamp);
int getDayTimeCategory(double zenithAngle);

#endif // SOLAR_POSITION_H
