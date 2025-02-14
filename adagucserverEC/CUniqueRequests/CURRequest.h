#include "CURConstants.h"
#include "CURAggregatedDimension.h"

#ifndef CURREQUEST_H
#define CURREQUEST_H

struct CURRequest {
  std::vector<CURAggregatedDimensionAndName> dimensions;
};

#endif