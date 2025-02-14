#ifndef REQUESTUTILS_H
#define REQUESTUTILS_H
#include "CUniqueRequests/CURTypes.h"

void padTo(std::string &str, const size_t num, const char paddingChar = ' ');

bool sortDimensionKeysRecursive(CURResult &result1, CURResult &result2, int depth = 0);

struct less_than_key {
  inline bool operator()(CURResult &result1, CURResult &result2) { return sortDimensionKeysRecursive(result1, result2, 0); }
};

#endif