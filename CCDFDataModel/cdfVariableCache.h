#ifndef CDFVARIABLECACHE_H
#define CDFVARIABLECACHE_H
#include <cstddef>
#include "CCDFVariable.h"

// This file defines the interface for a variable cache used in the CDF data model.

/**
 * Clears the variable cache, removing all cached variables and their associated data.
 */
void varCacheClear();
/**
 * Checks if a variable with the specified parameters is present in the cache.
 *
 * @param var The variable to check for in the cache.
 * @param start The starting indices for the variable data.
 * @param count The counts of elements for each dimension of the variable data.
 * @param stride The strides for each dimension of the variable data.
 * @return true if the variable is found in the cache, false otherwise.
 */
int varCacheReturn(CDF::Variable *var, size_t *start, size_t *count, ptrdiff_t *stride);

/**
 * Adds a variable with the specified parameters to the cache.
 *
 * @param var The variable to add to the cache.
 * @param start The starting indices for the variable data.
 * @param count The counts of elements for each dimension of the variable data.
 * @param stride The strides for each dimension of the variable data.
 */
void varCacheAdd(CDF::Variable *var, size_t *start, size_t *count, ptrdiff_t *stride);

#endif
