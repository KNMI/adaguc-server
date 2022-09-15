#include <stdio.h>
#include <map>
#include "CURConstants.h"
#include "CTString.h"
#include "CURAggregatedDimension.h"

#ifndef CURDIMINFO_H
#define CURDIMINFO_H

class CURDimInfo {
  public:
  
  std::map<int, CT::string *> dimValuesMap;            // All values, many starts with 1 count, result of set()
  std::vector<CURAggregatedDimension *> aggregatedValues; // Aggregated values (start/count series etc), result of  addDimSet()
  ~CURDimInfo() {
    for (it_type_dimvalindex dimvalindexmapiterator = dimValuesMap.begin(); dimvalindexmapiterator != dimValuesMap.end(); dimvalindexmapiterator++) {
      delete dimvalindexmapiterator->second;
    }
    for (size_t j = 0; j < aggregatedValues.size(); j++) {
      delete aggregatedValues[j];
    }
  }
};

typedef std::map<std::string, CURDimInfo *>::iterator it_type_diminfo;  

#endif