#include "CDebugger.h"
#include "CUniqueRequests/requestUtils.h"
#include "CUniqueRequests/CURUniqueRequests.h"

bool sortDimensionKeysRecursive(CURResult &result1, CURResult &result2, int depth) {

  int *dimOrder = result1.parent->__getDimOrder();
  int dimOrderIndex = dimOrder[depth];

  if (depth >= result1.numDims) return false;

  if (result1.dimensionKeys[dimOrderIndex].isNumeric && result2.dimensionKeys[dimOrderIndex].isNumeric) {
    // Numeric comparison
    double n1 = std::stod(result1.dimensionKeys[dimOrderIndex].name);
    double n2 = std::stod(result2.dimensionKeys[dimOrderIndex].name);
    if (n1 != n2) {
      return n1 < n2;
    }
  } else {
    // String comparison
    auto c = result1.dimensionKeys[dimOrderIndex].name.compare(result2.dimensionKeys[dimOrderIndex].name);
    if (c != 0) {
      return c < 0;
    }
  }

  return sortDimensionKeysRecursive(result1, result2, depth + 1);
}