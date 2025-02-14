#include "CUniqueRequests/CURResult.h"
#include "CUniqueRequests/CURUniqueRequests.h"

CURResult::CURResult(CURUniqueRequests *parent) { this->parent = parent; }
int *CURResult::getDimOrder() { return parent->getDimOrder(); }