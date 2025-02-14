#ifndef CURTYPES_H
#define CURTYPES_H

#include <map>
#include "CTString.h"
#define CCUniqueRequests_MAX_DIMS 255
typedef std::map<int, std::string> map_type_dimvalindex;
typedef std::map<int, std::string>::iterator it_type_dimvalindex;

class CURUniqueRequests;

struct CURAggregatedDimensionNoName {
  int start;
  std::vector<std::string> values;
};
struct CURAggregatedDimensionAndName {
  CT::string name;
  int start;
  std::vector<std::string> values;
};

struct CURDimInfo {
  map_type_dimvalindex dimValuesMap;                          // All values, many starts with 1 count, result of set()
  std::vector<CURAggregatedDimensionNoName> aggregatedValues; // Aggregated values (start/count series etc), result of  addDimSet()
};

typedef std::map<std::string, CURDimInfo> map_type_diminfo;
typedef map_type_diminfo::iterator it_type_diminfo;

struct CURDimensionKey {
  std::string name;
  int type;
};

struct CURResult {
  CURUniqueRequests *parent;
  CURDimensionKey dimensionKeys[CCUniqueRequests_MAX_DIMS];
  CT::string value;
  int numDims;
};

struct CURFileInfo {
  std::vector<std::vector<CURAggregatedDimensionAndName>> requests;
  map_type_diminfo dimInfoMap; // AggregatedDimension name is key
};

#endif