#include <map>
#include "CURConstants.h"
#include "CURRequest.h"
#include "CURDimInfo.h"

#ifndef CURFILEINFO_H
#define CURFILEINFO_H

class CURFileInfo {
public:
  std::vector<CURRequest *> requests;
  std::map<std::string, CURDimInfo *> dimInfoMap; // AggregatedDimension name is key
  

  ~CURFileInfo() {
    for (it_type_diminfo diminfomapiterator = dimInfoMap.begin(); diminfomapiterator != dimInfoMap.end(); diminfomapiterator++) {
      delete diminfomapiterator->second;
    }
    for (size_t j = 0; j < requests.size(); j++) {
      delete requests[j];
    }
  }
};

#endif