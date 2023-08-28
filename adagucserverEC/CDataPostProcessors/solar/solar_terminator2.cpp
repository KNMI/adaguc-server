// C++ program calculating the solar position for
// the current date and a set location (latitude, longitude)
// Jarmo Lammi 1999
//
// Code refreshed to work in the newer versions of C++
// Compiled and tested with
// Apple LLVM version 6.0 (clang-600.0.56) (based on LLVM 3.5svn)
// Target: x86_64-apple-darwin14.0.0
// Jarmo Lammi 4-Jan-2015

#include <iostream>
#include <math.h>
#include <cmath>
#include <time.h>
#include <chrono>
using std::cin;
using std::cout;
using std::endl;
using std::ios;

// Helper functions
double degToRad(double deg) { return deg * M_PI / 180.0; }

double radToDeg(double rad) { return rad * 180.0 / M_PI; }

int getDayTimeCategory(double zenithAngle) {
  if (zenithAngle < 90)
    return 4; // "Daylight";
  else if (zenithAngle < 96)
    return 3; // "Civil twilight";
  else if (zenithAngle < 102)
    return 2; // "Nautical twilight";
  else if (zenithAngle < 108)
    return 1; // "Astronomical twilight";
  else
    return 0; // "Night";
}

const double PI = 3.14159265358979323846;
const double DEG_TO_RAD = PI / 180.0;
const double RAD_TO_DEG = 180.0 / PI;

double getSolarZenithAngle(double lat, double lon, double timestamp) {
  // Convert latitude and longitude to radians
  double latRad = lat * M_PI / 180.0;
  double lonRad = lon * M_PI / 180.0;
  // Calculate Julian day
  double jd = 2440587.5 + timestamp / 86400.0;

  // Calculate Julian centuries since J2000.0
  double T = (jd - 2451545.0) / 36525.0;

  double L0 = std::fmod(280.46646 + 36000.76983 * T + 0.0003032 * T * T, 360.0);

  // double J0 = 280.46061837 + 360.98564736629 * (jd - 2451545.0) + 0.000387933 * T * T - T * T * T / 38710000.0;

  // Solar mean anomaly
  double M = std::fmod(357.52910 + 35999.05030 * T - 0.0001559 * T * T - 0.00000048 * T * T * T, 360.0);

  // eccentricity of Earth's orbit (25.4)
  double e = 0.016708634 - (0.000042037 * T) - (0.0000001267 * T * T);

  // Equation of center
  double M_rad = M * DEG_TO_RAD;
  double C = (1.914602 - 0.004817 * T - 0.000014 * T * T) * sin(M_rad) + (0.019993 - 0.000101 * T) * sin(2.0 * M_rad) + 0.000290 * sin(3.0 * M_rad);

  // True solar longitude
  double L = std::fmod(L0 + C, 360.0);

  // True solar anomaly
  double n = std::fmod(M + C, 360.0);

  // U.S. Naval Observatory function for radius vector.
  // Compare to Meeus (25.5)
  double R = 1.00014 - 0.01671 * cos(M_rad) - 0.00014 * cos(2 * M_rad);

  // Correction for nutation and aberration
  double O = 125.04 - (1934.136 * T);

  // apparent longitude L (lambda) of the Sun
  double Lapp = L - 0.00569 - (0.00478 * sin(O * DEG_TO_RAD));

  // obliquity of the ecliptic (22.2)
  double epsilon = 23 + 26 / 60.0 + 21.448 / 3600 - 46.8150 * T / 3600 - 0.00059 * T * T / 3600 + 0.00183 * T * T * T / 3600 + 0.00256 * cos((125.04 - 1934.136 * T) * DEG_TO_RAD);

  // correction for parallax (25.8)
  double eCorrected = epsilon + 0.00256 * cos(O * DEG_TO_RAD);

  // Sun's right ascension a
  // double a = atan2(cos(eCorrected * DEG_TO_RAD)) * sin(Lapp * DEG_TO_RAD), cos(Lapp * DEG_TO_RAD);
  double a = fmod((atan2(cos(eCorrected * DEG_TO_RAD) * sin(eCorrected * DEG_TO_RAD), cos(eCorrected * DEG_TO_RAD))), 360.0);

  // Declination d
  double d = asin(sin(eCorrected * DEG_TO_RAD) * sin(Lapp * DEG_TO_RAD));

  // Hour angle
  double omega = std::fmod(360.0 + 180.0 - lon - (360.0 / M_PI) * std::atan2(std::sin(2 * L * M_PI / 180.0), std::cos(2 * L * M_PI / 180.0) * std::sin(23.44 * M_PI / 180.0)), 360.0);

  // Calculate the solar hour angle
  double H = (T - 12) * 15 + a - lonRad;

  // Calculate the sine of the solar elevation angle
  double sin_theta = sin(d * DEG_TO_RAD) * sin(latRad) + cos(d * DEG_TO_RAD) * cos(latRad) * cos(H);

  // Calculate the solar zenith angle in degrees based on the elevation angle
  double theta = 90 - asin(sin_theta) * RAD_TO_DEG;
  return theta;
}

// Based on Jean Meeus's Astronomical Algorithms
double getSolarZenithAngleGOOD(double lat, double lon, double timestamp) {
  // Step 1: Convert the observer's longitude to positive westward from Greenwich meridian
  lon = -lon;

  // Calculate the Julian Day (JD)
  // 86400.0 - seconds in a day
  // 2440587.5 - julian day for 1/1/1970
  double jd = (timestamp / 86400.0) + 2440587.5;

  // Calculate the Ephemeris Time (T)
  // One Julian Century = 36525 days
  // January 1, 2000 = Julian day no. 2451545
  // Accuracy here is of utmost importance (T is expressed in centuries)
  double T = (jd - 2451545.0) / 36525.0;

  // Calculate the Geometric Mean Longitude of the Sun (L) in degrees
  double L = 280.46646 + 36000.76983 * T + 0.0003032 * T * T;

  // Calculate the Mean Anomaly of the Sun (M) in degrees (page 151)
  // Or is it 0.0001537d?
  // double M = 357.52911 + 35999.05029 * T + 0.0000001537 * T * T;
  double M = 357.52910 + 35999.05030 * T - 0.0001559 * T * T - 0.00000048 * T * T * T;

  // Calculate the Eccentricity of Earth's Orbit (e) (page 151)
  // double e = 0.016708634 - 0.000042037 * T - 0.0000001267 * T * T;
  double e = 0.016708617 - 0.00004237 * T - 0.0000001236 * T * T;
  // Calculate Sun's Equation of Center (C) (page 152)
  double M_rad = M * DEG_TO_RAD;
  double C = (1.914602 - 0.004817 * T - 0.000014 * T * T) * sin(M_rad) + (0.019993 - 0.000101 * T) * sin(2.0 * M_rad) + 0.000290 * sin(3.0 * M_rad);

  // Calculate Sun's True Longitude of the Sun (λ) in degrees (page 152)
  double lambda = L + C;

  // Calculate Sun's True Anomaly (v) in degrees (page 152)
  double v = M + C;

  // Calculate the Sun's Radius Vector (R) (that is, distance between Sun's and Earth's centres)
  // double R = (1.000001018 * (1 - e * e)) / (1 + e * cos(v * DEG_TO_RAD));

  // Calculate Sun's the Apparent Longitude  (page 152)
  double omega = 125.04 - 1934.136 * T;
  double appLon = lambda - 0.00569 - 0.00478 * sin(omega * DEG_TO_RAD);

  // Calculate Mean Obliquity of the Ecliptic (ε) (page 135)
  // double epsilon = 23.0 + (26.0 + ((21.448 - (46.8150 * T + 0.00059 * T * T - 0.00183 * T * T * T)) / 60.0) / 60.0) / 60.0 + 0.00256 * cos(DEG_TO_RAD * (125.04 - 1934.136 * T));
  double epsilon = 23 + 26 / 60.0 + 21.448 / 3600 - 46.8150 * T / 3600 - 0.00059 * T * T / 3600 + 0.00183 * T * T * T / 3600 + 0.00256 * cos((125.04 - 1934.136 * T) * DEG_TO_RAD);

  // Calculate Sun's Apparent Ecliptic
  // double ecliptic = (84381.448 + -46.815 * T - 0.00059 * T * T + 0.001813 * T * T * T) / 3600.0;
  double appEcliptic = epsilon + 0.00256 * cos(omega * DEG_TO_RAD);

  // Calculate Sun's Apparent Right Ascension
  // double ra = fmod(atan2(cos(apparentEcliptic * (PI / 180.0)) * sin(apparentLong * (PI / 180.0)), cos(apparentLong * (PI / 180.0))), 360.0); // Calculate right ascension
  double rightAscension = RAD_TO_DEG * fmod((atan2(cos(appEcliptic * DEG_TO_RAD) * sin(appLon * DEG_TO_RAD), cos(appLon * DEG_TO_RAD))), 360.0);

  // Calculate Correction for the Sun's Apparent Longitude (δλ)
  // double deltaLambda = appAlpha - L;

  // Calculate Correction for the Sun's Right Ascension (δα)
  // double deltaAlpha = -deltaLambda;

  // Step 15: Calculate the Sun's Apparent Right Ascension (α')
  // double alphaPrime = appAlpha - deltaAlpha;

  // Calculate Sun's Apparent Declination (δ)
  double delta = RAD_TO_DEG * asin(sin(appEcliptic * DEG_TO_RAD) * sin(appLon * DEG_TO_RAD));

  // Calculate Greenwich Sidereal Time (GMT) in degrees
  double gst = 280.46061837 + 360.98564736629 * (jd - 2451545.0) + 0.000387933 * T * T - (T * T * T) / 38710000.0;
  gst = fmod(gst, 360.0); // Keep GST within the range of 0 to 360

  // Local Mean Sidereal Time (LMST)
  // double lmst = gst + longitude / 15.0;
  double lha = gst - lon - rightAscension * 15; // Calculate Local Hour Angle (LHA)
  // double LMST = (280.46061837 + 360.98564736629 * (jd - 2451545.0) + lon) * DEG_TO_RAD;

  // Calculate the altitude (which is the complement of zenith angle)
  double altitude = asin(sin(lat * DEG_TO_RAD) * sin(delta * DEG_TO_RAD) + cos(lat * DEG_TO_RAD) * cos(delta * DEG_TO_RAD) * cos(lha * DEG_TO_RAD));

  // Convert altitude to zenith angle and convert to degrees
  double zenith = (PI / 2 - altitude) * RAD_TO_DEG;

  // Return the Solar Zenith Angle
  return zenith;
}

/*
  // Step 17: Calculate the Observer's Local Hour Angle (LHA)
  double LHA = fmod(15 * (12 - fmod(lon + alphaPrime, 360.0) / 15), 360.0);

  // Step 18: Calculate the Solar Zenith Angle (θ)
  double theta = acos(sin(lat * DEG_TO_RAD) * sin(delta * DEG_TO_RAD) + cos(lat * DEG_TO_RAD) * cos(delta * DEG_TO_RAD) * cos(LHA * DEG_TO_RAD)) * RAD_TO_DEG;

  return theta;
} */

// Julian Century
double calcJC(double JD) { return (JD - 2451545) / 36525; }

// Geometric Mean Longitude of the Sun
double calcGeomMeanLongSun(double t) {
  double L = 280.46646 + t * (36000.76983 + 0.0003032 * t);
  while (L > 360.0) {
    L -= 360.0;
  }
  while (L < 0.0) {
    L += 360.0;
  }
  return L; // in degrees
}

// Mean Anomaly of the Sun
double calcMeanAnomalySun(double t) {
  return 357.52911 + t * (35999.05029 - 0.0001537 * t); // in degrees
}

// Eccentricity of Earth's Orbit
double calcEccentricityEarthOrbit(double t) {
  return 0.016708634 - t * (0.000042037 + 0.0000001267 * t); // unitless
}

// Sun's Equation of the Center
double calcSunEqOfCenter(double t, double m) {
  double mrad = degToRad(m);
  double sinm = sin(mrad);
  double sin2m = sin(mrad + mrad);
  double sin3m = sin(mrad + mrad + mrad);
  return sinm * (1.914602 - t * (0.004817 + 0.000014 * t)) + sin2m * (0.019993 - 0.000101 * t) + sin3m * 0.000289; // in degrees
}

// Sun's True Longitude
double calcSunTrueLong(double l, double c) {
  return l + c; // in degrees
}

// Sun's Apparent Longitude
double calcSunApparentLong(double t, double o) {
  double omega = 125.04 - 1934.136 * t;
  return o - 0.00569 - 0.00478 * sin(degToRad(omega)); // in degrees
}

// Mean Obliquity of the Ecliptic
double calcMeanObliquityOfEcliptic(double t) {
  double seconds = 21.448 - t * (46.8150 + t * (0.00059 - t * (0.001813)));
  return 23.0 + (26.0 + (seconds / 60.0)) / 60.0; // in degrees
}

// Obliquity Correction
double calcObliquityCorrection(double t, double e0) {
  double omega = 125.04 - 1934.136 * t;
  return e0 + 0.00256 * cos(degToRad(omega)); // in degrees
}

// Sun's Right Ascension
double calcSunRtAscension(double t, double theta) {
  double e0 = calcMeanObliquityOfEcliptic(t);
  double e = calcObliquityCorrection(t, e0);
  double radians = degToRad(e);
  return radToDeg(atan2(cos(radians) * sin(degToRad(theta)), cos(degToRad(theta)))); // in degrees
}

// Sun's Declination
double calcSunDeclination(double t, double theta) {
  double e0 = calcMeanObliquityOfEcliptic(t);
  double e = calcObliquityCorrection(t, e0);
  double radians = degToRad(e);
  return radToDeg(asin(sin(radians) * sin(degToRad(theta)))); // in degrees
}

// Hour Angle
double calcHourAngleSunrise(double lat, double solarDec) {
  double latRad = degToRad(lat);
  double sdRad = degToRad(solarDec);
  double HA = (acos(cos(degToRad(90.833)) / (cos(latRad) * cos(sdRad)) - tan(latRad) * tan(sdRad)));
  return HA; // in radians (for use in calculating the sunrise)
}

// Nutation effect
double nutationInLongitude(double t) {
  // Mean elongation of the Moon from the Sun
  double D = 297.85036 + 445267.111480 * t - 0.0019142 * t * t + t * t * t / 189474;

  // Mean anomaly of the Sun
  double M = 357.52772 + 35999.050340 * t - 0.0001603 * t * t - t * t * t / 300000;

  // Mean anomaly of the Moon
  double M_ = 134.96298 + 477198.867398 * t + 0.0086972 * t * t + t * t * t / 56250;

  // Moon's argument of latitude
  double F = 93.27191 + 483202.017538 * t - 0.0036825 * t * t + t * t * t / 327270;

  // Longitude of the ascending node of the Moon's mean orbit on the ecliptic, measured from the mean equinox of the date
  double Omega = 125.04452 - 1934.136261 * t + 0.0020708 * t * t + t * t * t / 450000;

  // All the values are in degrees, and should be reduced to the range 0 - 360
  D = fmod(D, 360);
  M = fmod(M, 360);
  M_ = fmod(M_, 360);
  F = fmod(F, 360);
  Omega = fmod(Omega, 360);

  // Approximate nutation in longitude, deltaPsi, in degrees
  double deltaPsi = -17.20 * sin(degToRad(Omega)) - 1.32 * sin(degToRad(2 * D)) - 0.23 * sin(degToRad(2 * M_)) + 0.21 * sin(degToRad(2 * Omega));

  // The factor 0.0001 is to convert from arcseconds to degrees
  return deltaPsi * 0.0001;
}

// Aberration
double aberrationCorrection(double t) {
  // The eccentricity of the Earth's orbit
  double e = 0.016708634 - t * (0.000042037 + 0.0000001267 * t);

  // The mean anomaly of the Sun
  double M = 357.52911 + t * (35999.05029 - 0.0001537 * t);

  // The correction for aberration, deltaTau, in degrees
  double deltaTau = -20.4898 / 3600 * (1.0 + e * cos(degToRad(M)));

  return deltaTau;
}

// Apparent Longitude of the Sun
double apparentSunLongitude(double trueLong, double nutation, double aberration) { return trueLong + nutation + aberration; }

// Zenigh Angle
double calcZenithAngle(double lat, double solarDec, double hourAngle) {
  double latRad = degToRad(lat);
  double sdRad = degToRad(solarDec);
  double HA = degToRad(hourAngle);
  return radToDeg(acos(sin(latRad) * sin(sdRad) + cos(latRad) * cos(sdRad) * cos(HA))); // in degrees
}

// Main function to get the solar zenith angle
double getSolarZenithAngle3(double lat, double lon, double timestamp) {
  // Convert timestamp to Julian Day
  double JD = timestamp / 86400.0 + 2440587.5;

  // Convert Julian Day to centuries since J2000.0
  double t = calcJC(JD);

  // Calculate geometric mean longitude of the Sun
  double L = calcGeomMeanLongSun(t);

  // Calculate mean anomaly of the Sun
  double M = calcMeanAnomalySun(t);

  // Calculate eccentricity of Earth's orbit
  double e = calcEccentricityEarthOrbit(t);

  // Calculate Sun's equation of the center
  double C = calcSunEqOfCenter(t, M);

  // Calculate Sun's true longitude
  double TrueL = calcSunTrueLong(L, C);

  // Calculate nutation in longitude and aberration correction
  double nutation = nutationInLongitude(t);
  double aberration = aberrationCorrection(t);

  // Calculate Sun's apparent longitude
  double theta = apparentSunLongitude(TrueL, nutation, aberration);

  // Calculate mean obliquity of the ecliptic
  double e0 = calcMeanObliquityOfEcliptic(t);

  // Calculate obliquity correction
  double epi = calcObliquityCorrection(t, e0);

  // Calculate Sun's right ascension
  double RA = calcSunRtAscension(t, theta);

  // Calculate Sun's declination
  double delta = calcSunDeclination(t, theta);

  // Calculate hour angle
  double H = calcHourAngleSunrise(lat, delta);

  // Finally, calculate zenith angle
  double zenith = calcZenithAngle(lat, delta, H);

  return zenith;
}

double getSolarZenithAngle4(double lat, double lon, double timestamp) {
  // Constants
  double pi = 3.14159265358979323846;
  double rad = pi / 180.0; // Radians per degree
  double d2r = pi / 180.0; // Conversion factor from degrees to radians

  // Convert UNIX timestamp to Julian Date
  double jd = timestamp / 86400.0 + 2440587.5;

  // Calculate number of centuries since J2000.0.
  // J2000.0 corresponds to the Julian day 2451545.0 (January 1, 2000, 12:00 UT)
  double T = (jd - 2451545.0) / 36525.0; // T is the time in Julian centuries from the epoch 2000.0

  // Calculate the Geometric Mean Anomaly of the Sun
  // This is the angle from the perihelion that the Earth would be on a circular orbit
  double M = 357.52911 + T * (35999.05029 - 0.0001537 * T);

  // Calculate the eccentricity of Earth's orbit
  // This is the amount the Earth's orbit deviates from a perfect circle
  double e = 0.016708634 - T * (0.000042037 + 0.0000001267 * T);

  // Calculate the Sun's equation of the center
  // This is an approximation of the true anomaly of the Sun minus the Sun's mean anomaly
  double C = sin(d2r * M) * (1.914602 - T * (0.004817 + 0.000014 * T)) + sin(d2r * 2 * M) * (0.019993 - 0.000101 * T) + sin(d2r * 3 * M) * 0.000289;

  // Calculate the Sun's true longitude
  // The Sun's true longitude is the geometric mean longitude of the Sun plus the equation of the center (which accounts for the eccentricity of the Earth's orbit)
  double sunLongitude = M + C + 180 + 102.9372; // In degrees

  // Convert the Sun's longitude to radians
  double sunLongitudeRad = d2r * sunLongitude;

  // Calculate the declination of the Sun.
  // Declination is the angle between the Sun and the celestial equator. It is equivalent to latitude in the celestial sphere
  double sinDecl = 0.39782 * sin(sunLongitudeRad);
  double cosDecl = sqrt(1 - sinDecl * sinDecl);

  // Calculate the solar hour angle
  // The hour angle is the longitude difference between the observer's meridian and the Sun.
  double Tfrac = jd + 0.5 - floor(jd + 0.5);
  double H = (Tfrac * 24.0 - 12.0) * 15.0 + lon;

  // Convert to radians
  double Hrad = d2r * H;

  // Calculate the observer's latitude in radians
  double latRad = d2r * lat;

  // Compute the solar zenith angle
  double cosZenith = sin(latRad) * sinDecl + cos(latRad) * cosDecl * cos(Hrad);
  double zenith = acos(cosZenith) / d2r;

  return zenith;
}

double getSolarZenithAngle5(double lat, double lon, double timestamp) {
  // Constants
  double pi = 3.14159265358979323846;
  double d2r = pi / 180.0; // Conversion factor from degrees to radians

  // Convert UNIX timestamp to Julian Date
  double jd = timestamp / 86400.0 + 2440587.5;

  // Calculate number of centuries since J2000.0.
  double T = (jd - 2451545.0) / 36525.0; // T is the time in Julian centuries from the epoch 2000.0

  // Calculate the Geometric Mean Anomaly of the Sun
  double M = 357.52911 + T * (35999.05029 - 0.0001537 * T);

  // Calculate the eccentricity of Earth's orbit
  double e = 0.016708634 - T * (0.000042037 + 0.0000001267 * T);

  // Calculate the Sun's equation of the center
  double C = sin(d2r * M) * (1.914602 - T * (0.004817 + 0.000014 * T)) + sin(d2r * 2 * M) * (0.019993 - 0.000101 * T) + sin(d2r * 3 * M) * 0.000289;

  // Calculate the Sun's true longitude
  double sunLongitude = M + C + 180 + 102.9372; // In degrees

  // Convert the Sun's longitude to radians
  double sunLongitudeRad = d2r * sunLongitude;

  // Calculate the Sun's apparent longitude (includes correction for aberration and nutation)
  double omega = 125.04 - 1934.136 * T;
  double lambda = sunLongitude - 0.00569 - 0.00478 * sin(d2r * omega);

  // Calculate the mean obliquity of the ecliptic (tilt of Earth's axis)
  double epsilon0 = 23.4392 - 0.0130 * T;
  // Correct for the nutation of the axis
  double epsilon = epsilon0 + 0.00256 * cos(d2r * omega);

  // Calculate the declination of the Sun
  double sinEpsilon = sin(d2r * epsilon);
  double sinLambda = sin(d2r * lambda);
  double declination = asin(sinEpsilon * sinLambda) / d2r;

  // Calculate the equation of time (difference between solar time and mean solar time)
  double y = tan(d2r * epsilon / 2.0) * tan(d2r * epsilon / 2.0);
  double equationOfTime = y * sin(2 * d2r * M) - 2 * e * sin(d2r * M) + 4 * e * y * sin(d2r * M) * cos(2 * d2r * M) - 0.5 * y * y * sin(4 * d2r * M) - 1.25 * e * e * sin(2 * d2r * M);

  // Calculate the true solar time
  double trueSolarTime = fmod((jd - floor(jd)) * 24 * 60 + equationOfTime + 4 * lon - 60 * 0, 1440.0);

  // Calculate the hour angle of the sun
  double hourAngle;
  if (trueSolarTime / 4 < 0)
    hourAngle = trueSolarTime / 4 + 180;
  else
    hourAngle = trueSolarTime / 4 - 180;

  // Calculate the solar zenith angle
  double zenithAngle = acos(sin(d2r * lat) * sin(d2r * declination) + cos(d2r * lat) * cos(d2r * declination) * cos(d2r * hourAngle)) / d2r;

  return zenithAngle;
}

#define DEG_TO_RAD (M_PI / 180.0)
#define RAD_TO_DEG (180.0 / M_PI)

double getSolarZenithAngle10(double lat, double lon, double timestamp) {
  lon = -lon;

  double jd = (timestamp / 86400.0) + 2440587.5;

  double T = (jd - 2451545.0) / 36525.0;

  double L = 280.46646 + 36000.76983 * T + 0.0003032 * T * T;

  double M = 357.52910 + 35999.05030 * T - 0.0001559 * T * T - 0.00000048 * T * T * T;

  double e = 0.016708617 - 0.00004237 * T - 0.0000001236 * T * T;

  double M_rad = M * DEG_TO_RAD;
  double C = (1.914602 - 0.004817 * T - 0.000014 * T * T) * sin(M_rad) + (0.019993 - 0.000101 * T) * sin(2.0 * M_rad) + 0.000290 * sin(3.0 * M_rad);

  double lambda = L + C;

  double v = M + C;

  // If not needed, comment this out
  // double R = (1.000001018 * (1 - e * e)) / (1 + e * cos(v * DEG_TO_RAD));

  double appLon = lambda - 0.00569 - 0.00478 * sin((125.04 - 1934.136 * T) * DEG_TO_RAD);

  double epsilon = 23 + 26 / 60.0 + 21.448 / 3600 - 46.8150 * T / 3600 - 0.00059 * T * T / 3600 + 0.00183 * T * T * T / 3600 + 0.00256 * cos((125.04 - 1934.136 * T) * DEG_TO_RAD);

  double appAlpha = RAD_TO_DEG * atan2(cos(epsilon * DEG_TO_RAD) * sin(appLon * DEG_TO_RAD), cos(appLon * DEG_TO_RAD));

  double appDelta = RAD_TO_DEG * asin(sin(epsilon * DEG_TO_RAD) * sin(appLon * DEG_TO_RAD));

  double JD0 = floor(jd + 0.5) - 0.5;
  double T0 = (JD0 - 2451545.0) / 36525.0;
  double secs = (jd - JD0) * 86400.0;
  double gmst_at_0h_ut = fmod((100.46061837 + 36000.770053608 * T0 + 0.000387933 * T0 * T0 - T0 * T0 * T0 / 38710000.0), 360.0);
  double GMST = gmst_at_0h_ut + 360.985647366287 * secs / 86400.0;

  double LHA = GMST - lon - appAlpha;
  if (LHA > 180.0) {
    LHA -= 360.0;
  } else if (LHA < -180.0) {
    LHA += 360.0;
  }

  double cosZenith = sin(lat * DEG_TO_RAD) * sin(appDelta * DEG_TO_RAD) + cos(lat * DEG_TO_RAD) * cos(appDelta * DEG_TO_RAD) * cos(LHA * DEG_TO_RAD);

  cosZenith = std::max(-1.0, std::min(1.0, cosZenith));

  double zenith = acos(cosZenith) * RAD_TO_DEG;

  return zenith;
}
