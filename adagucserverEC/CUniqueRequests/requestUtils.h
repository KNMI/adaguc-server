#ifndef REQUESTUTILS_H
#define REQUESTUTILS_H
#include "CUniqueRequests/CURTypes.h"

bool sortDimensionKeysRecursive(CURResult &result1, CURResult &result2, int depth = 0);

struct compareFunctionCurResult {
  inline bool operator()(CURResult &result1, CURResult &result2) { return sortDimensionKeysRecursive(result1, result2, 0); }
};

#endif