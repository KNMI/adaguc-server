#include <stdio.h>
#include <map>
#include "CTString.h"

#ifndef CURAGGREGATEDDIMENSION_H
#define CURAGGREGATEDDIMENSION_H
struct CURAggregatedDimensionNoName {
  int start;
  std::vector<std::string> values;
};
struct CURAggregatedDimensionAndName {
  CT::string name;
  int start;
  std::vector<std::string> values;
};
#endif