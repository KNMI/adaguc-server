#include <map>
#include "CURConstants.h"
#include "CURRequest.h"
#include "CURDimInfo.h"

#ifndef CURFILEINFO_H
#define CURFILEINFO_H

struct CURFileInfo {
  std::vector<CURRequest> requests;
  map_type_diminfo dimInfoMap; // AggregatedDimension name is key
};

#endif