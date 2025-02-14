#include <stdio.h>
#include <map>
#include "CURConstants.h"
#include "CTString.h"
#include "CURAggregatedDimension.h"

#ifndef CURDIMINFO_H
#define CURDIMINFO_H

struct CURDimInfo {
  map_type_dimvalindex dimValuesMap;                          // All values, many starts with 1 count, result of set()
  std::vector<CURAggregatedDimensionNoName> aggregatedValues; // Aggregated values (start/count series etc), result of  addDimSet()
};

typedef std::map<std::string, CURDimInfo> map_type_diminfo;
typedef map_type_diminfo::iterator it_type_diminfo;

#endif