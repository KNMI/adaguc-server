#ifndef CURTYPES_H
#define CURTYPES_H

#include <map>
#include "CTString.h"
#include "CCDFObject.h"
#define CCUniqueRequests_MAX_DIMS 255

class CURUniqueRequests;

struct StartAnValues {
  int start;
  std::vector<std::string> values;
};

struct CURDimInfo {
  std::map<int, std::string> dimValuesMap;     // All values, many starts with 1 count, result of set()
  std::vector<StartAnValues> aggregatedValues; // Aggregated values (start/count series etc), result of  addDimSet()
};

struct CURAggregatedDimension {
  std::string name;
  int start;
  std::vector<std::string> values;
};

struct CURDimensionKey {
  std::string name;
  CDF::Variable *cdfDimensionVariable = nullptr;
  bool isNumeric = false;
};

struct CURResult {
  std::vector<int> *dimOrdering = nullptr;
  std::vector<CURDimensionKey> dimensionKeys;
  std::string value;
  int numDims = 0;
};

struct CURFileInfo {
  std::vector<std::vector<CURAggregatedDimension>> requests;
  std::map<std::string, CURDimInfo> dimInfoMap; // AggregatedDimension name is key
};

typedef std::map<std::string, CURFileInfo>::iterator it_type_file;

#endif