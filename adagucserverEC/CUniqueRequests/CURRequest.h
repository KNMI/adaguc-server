#include "CURConstants.h"
#include "CURAggregatedDimension.h"

#ifndef CURREQUEST_H
#define CURREQUEST_H

class CURRequest {
public:
  int numDims;
  CURAggregatedDimension *dimensions[CCUniqueRequests_MAX_DIMS];
};

#endif