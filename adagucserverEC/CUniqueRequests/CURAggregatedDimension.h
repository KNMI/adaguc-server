#include <stdio.h>
#include <map>
#include "CTString.h"

#ifndef CURAGGREGATEDDIMENSION_H
#define CURAGGREGATEDDIMENSION_H
class CURAggregatedDimension {
public:
  CT::string name;
  int start;
  std::vector<std::string> values;
};
#endif