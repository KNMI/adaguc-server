#include "CDebugger.h"
#include "CUniqueRequests/requestUtils.h"

void padTo(std::string &str, const size_t num, const char paddingChar) {
  if (num > str.size()) str.insert(0, num - str.size(), paddingChar);
}

bool sortDimensionKeysRecursive(CURResult *result1, CURResult *result2, int depth) {
  bool d = false;
  if (depth < result1->numDims) {
    d = sortDimensionKeysRecursive(result1, result2, depth + 1);
  }

  int *dimOrder = result1->getDimOrder();
  int dimOrderIndex = dimOrder[depth];
  std::string s1 = result1->dimensionKeys[dimOrderIndex].c_str();
  std::string s2 = result2->dimensionKeys[dimOrderIndex].c_str();
  padTo(s1, 8);
  std::string ps2 = result2->dimensionKeys[dimOrderIndex].c_str();
  padTo(s2, 8);
  return s1.compare(s2) < 0 || d;
}