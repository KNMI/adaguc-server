#ifndef NODATAUTILS_H
#define NODATAUTILS_H

#include <cmath>

/**
 * Returns true when a value should not be drawn: it is the nodata value, it is NaN, or it lies outside the legend value range.
 *
 * The nodata value is compared at the precision of the data itself (T), because the double dfNodataValue may not be exactly representable in T.
 */
template <class T> inline bool isNodataValue(T val, bool hasNodataValue, double dfNodataValue, bool hasLegendValueRange, double legendLowerRange, double legendUpperRange) {
  if (hasNodataValue && val == (T)dfNodataValue) return true;
  if (std::isnan(val)) return true;
  return hasLegendValueRange && (val < legendLowerRange || val > legendUpperRange);
}

/**
 * Same as above, for settings structs with the members hasNodataValue, dfNodataValue, legendValueRange, legendLowerRange and legendUpperRange.
 */
template <class T, class Settings> inline bool isNodataValue(T val, const Settings &settings) {
  return isNodataValue(val, settings.hasNodataValue, settings.dfNodataValue, settings.legendValueRange != 0, settings.legendLowerRange, settings.legendUpperRange);
}

#endif
