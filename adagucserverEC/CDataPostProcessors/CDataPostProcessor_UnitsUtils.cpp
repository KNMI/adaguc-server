#include "CDataPostProcessor_UnitsUtils.h"
#include <unordered_set>

std::unordered_set<std::string> validUnitsKnots = {"knot", "knots", "kt", "kts"};
std::unordered_set<std::string> validUnitsDegrees = {"degree", "degrees"};
std::unordered_set<std::string> validUnitsMps = {"m/s", "m s-1"};

bool isKnotsUnit(const std::string &units) { return (validUnitsKnots.find(units) != validUnitsKnots.end()); }
bool isDegreesUnit(const std::string &units) { return (validUnitsDegrees.find(units) != validUnitsDegrees.end()); }
bool isMpsUnit(const std::string &units) { return (validUnitsMps.find(units) != validUnitsMps.end()); }
