#include <iostream>
#include <cmath>
#include <cmath>
#include <ctime>

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

const double DEG_TO_RAD = M_PI / 180.0;
const double RAD_TO_DEG = 180.0 / M_PI;

double get_julian_day(double timestamp) {
  // Transform from UNIX timestamp to Julian Day
  // January 1, 1970 - Julian day no. 2440587.5
  double jd = 2440587.5 + timestamp / 86400.0;
  return jd;
}

double get_julian_century(double jd) {
  // One Julian Century = 36525 days
  // January 1, 2000 = Julian day no. 2451545
  // Accuracy here is of utmost importance (T is expressed in centuries)
  double T = (jd - 2451545.0) / 36525.0;
  return T;
}

double get_solar_mean_longitude(double T) {
  // Measured in degrees
  double L0 = std::fmod(280.46646 + T * (36000.76983 + 0.0003032 * T), 360);
  while (L0 > 360.0) {
    L0 -= 360.0;
  }
  while (L0 < 0.0) {
    L0 += 360.0;
  }
  return L0;
}

double get_solar_mean_anomaly(double T) {
  // Measured in degrees
  double M = 357.52910 + 35999.05030 * T - 0.0001559 * T * T - 0.00000048 * T * T * T;
  return M;
}

double get_solar_equation_of_center(double T, double M) {
  // Measured in degrees
  double M_rad = M * DEG_TO_RAD;
  double C = (1.914602 - 0.004817 * T - 0.000014 * T * T) * sin(M_rad) + (0.019993 - 0.000101 * T) * sin(2.0 * M_rad) + 0.000290 * sin(3.0 * M_rad);
  return C;
}

double get_long_asc_node(double T) {
  // Measured in degrees
  double omega = 125.04 - 1934.136 * T;
  return omega;
}

double get_solar_app_longitude(double L, double omega) {
  // Measured in degrees
  double Lapp = L - 0.00569 - 0.00478 * sin(omega * DEG_TO_RAD);
  return Lapp;
}

double get_solar_mean_obliquity_ecliptic(double T) {
  // Measured in degrees
  double e0 = 23 + 26 / 60.0 + 21.448 / 3600 - 46.8150 * T / 3600 - 0.00059 * T * T / 3600 + 0.00183 * T * T * T / 3600 + 0.00256 * cos((125.04 - 1934.136 * T) * DEG_TO_RAD);
  return e0;
}

double get_solar_obliquity_correction(double e0, double omega) {
  // Measured in degrees
  double e = e0 + 0.00256 * cos(omega * DEG_TO_RAD);
  return e;
}

double get_solar_declination(double e, double Lapp) {
  // Measured in degrees
  double delta = (asin(sin(e * DEG_TO_RAD) * sin(Lapp * DEG_TO_RAD))) * RAD_TO_DEG;
  return delta;
}

double get_right_ascension(double L, double e) {
  double ra = fmod(atan2(sin(L * DEG_TO_RAD) * cos(e * DEG_TO_RAD), cos(L * DEG_TO_RAD)) * RAD_TO_DEG, 360.0);
  return ra;
}

double get_greenwich_hour_angle(double jd, double T) {
  double gha = 280.46061837 + 360.98564736629 * (jd - 2451545) + 0.000387933 * T * T - T * T * T / 38710000;
  return gha;
}

double get_local_hour_angle(double gha, double longitude, double ra) {
  double lha = gha + longitude - ra;
  lha = fmod(lha, 360.0);
  if (lha < 0.0) {
    lha += 360.0; // Ensure positive LHA
  }
  return lha;
}

double getSolarZenithAngle(double lat, double lon, double timestamp) {
  // Using formula cos(θ) = sin(δ) × sin(φ) + cos(δ) × cos(φ) × cos(H)
  // θ is the solar zenith angle,
  // δ is the solar declination
  // φ is the observer's latitude,
  // H is the hour angle of the Sun.
  // Get current Julian Day
  double jd = get_julian_day(timestamp);

  // Get current Julian Century
  double T = get_julian_century(jd);

  // Get Mean Longitude of the Sun
  double L0 = get_solar_mean_longitude(T);

  // Get Mean Anomaly of the Sun (page 151)
  double M = get_solar_mean_anomaly(T);

  // Get Equation of Center of the Sun
  double C = get_solar_equation_of_center(T, M);

  // Get True Longitude of the Sun (λ) (page 152)
  double L = L0 + C;

  // Get Longitude of the Ascending Node
  double omega = get_long_asc_node(T);

  // Get Apparent Longitude of the Sun
  double Lapp = get_solar_app_longitude(L, omega);

  // Get Mean Obliquity of the Ecliptic of the Sun
  double e0 = get_solar_mean_obliquity_ecliptic(T);

  // Get Obliquity correction
  double e = get_solar_obliquity_correction(e0, omega);

  // Get Declination of the Sun
  double delta = get_solar_declination(e, Lapp);

  // Get Greenwich Hour Angle
  double gha = get_greenwich_hour_angle(jd, T);

  // Get Right Ascension Angle
  double ra = get_right_ascension(L, e);

  // Get Local Hour Angle
  double lha = get_local_hour_angle(gha, lon, ra);

  // Final calculation of solar zenith angle
  // cos(θ) = sin(δ) × sin(φ) + cos(δ) × cos(φ) × cos(H)
  // θ is the solar zenith angle - called theta here
  // δ is the solar declination - called delta here
  // φ is the observer's latitude - called lat here
  // H is the hour angle of the Sun - called lha here
  double theta = RAD_TO_DEG * acos(sin(delta * DEG_TO_RAD) * sin(lat * DEG_TO_RAD) + cos(delta * DEG_TO_RAD) * cos(lat * DEG_TO_RAD) * cos(lha * DEG_TO_RAD));
  return theta;
}