#ifndef CURTYPES_H
#define CURTYPES_H

#include <map>
#include "CTString.h"
#include "CCDFObject.h"
#define CCUniqueRequests_MAX_DIMS 255
typedef std::map<int, std::string> map_type_dimvalindex;
typedef std::map<int, std::string>::iterator it_type_dimvalindex;

class CURUniqueRequests;

struct StartAnValues {
  int start;
  std::vector<std::string> values;
};

struct CURDimInfo {
  map_type_dimvalindex dimValuesMap;           // All values, many starts with 1 count, result of set()
  std::vector<StartAnValues> aggregatedValues; // Aggregated values (start/count series etc), result of  addDimSet()
};

typedef std::map<std::string, CURDimInfo> map_type_diminfo;
typedef map_type_diminfo::iterator it_type_diminfo;

struct CURAggregatedDimension {
  CT::string name;
  int start;
  std::vector<std::string> values;
};

struct CURDimensionKey {
  std::string name;
  CDF::Variable *cdfDimensionVariable;
  bool isNumeric;
};

struct CURResult {
  CURUniqueRequests *parent;
  CURDimensionKey dimensionKeys[CCUniqueRequests_MAX_DIMS];
  CT::string value;
  int numDims;
};

struct CURFileInfo {
  std::vector<std::vector<CURAggregatedDimension>> requests;
  map_type_diminfo dimInfoMap; // AggregatedDimension name is key
};

typedef std::map<std::string, CURFileInfo>::iterator it_type_file;

#endif