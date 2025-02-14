#ifndef CURRESULT_H
#define CURRESULT_H
#include "CTString.h"

#include "CUniqueRequests/CURConstants.h"

class CURUniqueRequests;

class CURResult {
private:
  CURUniqueRequests *parent;

public:
  CURResult(CURUniqueRequests *parent);
  CT::string dimensionKeys[CCUniqueRequests_MAX_DIMS];
  CT::string value;
  int numDims;
  int *getDimOrder();
};

#endif