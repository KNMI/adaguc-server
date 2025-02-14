#ifndef REQUESTUTILS_H
#define REQUESTUTILS_H
#include "CUniqueRequests/CURResult.h"

void padTo(std::string &str, const size_t num, const char paddingChar = ' ');

bool sortDimensionKeysRecursive(CURResult *result1, CURResult *result2, int depth = 0);

struct less_than_key {
  inline bool operator()(CURResult *result1, CURResult *result2) {
    return sortDimensionKeysRecursive(result1, result2, 0);
    // int *dimOrder = result1->getDimOrder();
    // std::string s1;
    // std::string s2;

    // for (int d = 0; d < result1->numDims; d++) {
    //   int dimOrderIndex = dimOrder[d];
    //   // CDBDebug("Dimension order = %d", dimOrderIndex);
    //   // CDBDebug("Dimension order A= %s", result1->dimensionKeys[dimOrderIndex].c_str());
    //   // CDBDebug("Dimension order B= %s", result2->dimensionKeys[dimOrderIndex].c_str());
    //   std::string ps1 = result1->dimensionKeys[dimOrderIndex].c_str();
    //   padTo(ps1, 8);
    //   std::string ps2 = result2->dimensionKeys[dimOrderIndex].c_str();
    //   padTo(ps2, 8);
    //   s1 += ps1;
    //   s1 += "_";
    //   s2 += ps2;
    //   s2 += "_";
    // }
    // // CDBDebug("Compare [%s] [%s]", s1.c_str(), s2.c_str());
    // if (s1.compare(s2) < 0) return true;
    // return false;
    // // return (struct1.key < struct2.key);
  }
};

#endif