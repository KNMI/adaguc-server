#include <map>
#include "CURConstants.h"
#include "CURRequest.h"
#include "CURDimInfo.h"

#ifndef CURFILEINFO_H
#define CURFILEINFO_H

class CURFileInfo {
public:
  std::vector<CURRequest *> requests;
  std::map<std::string, CURDimInfo> dimInfoMap; // AggregatedDimension name is key
};

#endif