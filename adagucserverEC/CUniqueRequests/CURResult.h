#ifndef CURRESULT_H
#define CURRESULT_H
#include "CTString.h"

#include "CUniqueRequests/CURConstants.h"

class CURUniqueRequests;

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

#endif