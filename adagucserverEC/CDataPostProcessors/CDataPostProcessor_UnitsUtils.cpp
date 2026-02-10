#include "CDataPostProcessor_UnitsUtils.h"

bool isKnotsUnit(CT::string units) {
  if (units.equals("knot") || (units.equals("knots"))) return true;
  return (units.equals("kt") || (units.equals("kts")));
}

bool isDegreesUnit(CT::string units) { return (units.equals("degree") || (units.equals("degrees"))); }

bool isMpsUnit(CT::string units) { return (units.equals("m/s") || units.equals("m s-1")); }
