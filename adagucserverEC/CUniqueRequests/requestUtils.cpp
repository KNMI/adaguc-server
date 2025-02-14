#include "CDebugger.h"
#include "CUniqueRequests/requestUtils.h"
#include "CUniqueRequests/CURUniqueRequests.h"

bool sortDimensionKeysRecursive(CURResult &result1, CURResult &result2, int depth) {
  bool d = false;
  if (depth < result1.numDims) {
    d = sortDimensionKeysRecursive(result1, result2, depth + 1);
  }

  int *dimOrder = result1.parent->__getDimOrder();
  int dimOrderIndex = dimOrder[depth];
  if (result1.dimensionKeys[dimOrderIndex].isNumeric) {
    double n1 = std::stod(result1.dimensionKeys[dimOrderIndex].name);
    double n2 = std::stod(result2.dimensionKeys[dimOrderIndex].name);
    return n1 < n2 || d;
  } else {
    return result1.dimensionKeys[dimOrderIndex].name.compare(result2.dimensionKeys[dimOrderIndex].name) < 0 || d;
  }
}