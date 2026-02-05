#ifndef CDFVARIABLECACHE_H
#define CDFVARIABLECACHE_H
#include <cstddef>
#include "CCDFVariable.h"

void varCacheClear();
int varCacheReturn(CDF::Variable *var, size_t *start, size_t *count, ptrdiff_t *stride);
void varCacheAdd(CDF::Variable *var, size_t *start, size_t *count, ptrdiff_t *stride);

#endif