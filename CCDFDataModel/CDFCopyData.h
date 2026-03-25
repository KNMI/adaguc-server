#include "CCDFTypes.h"
#include <cstddef>
#ifndef CDF_DATACOPIER
#define CDF_DATACOPIER

int CDFCopyData(void *destdata, CDFType destType, void *sourcedata, CDFType sourcetype, size_t destinationOffset, size_t sourceOffset, size_t length);
int CDFFillData(void *destdata, CDFType destType, double value, size_t size);
template <class T> int CDFCopyData(T *destdata, void *sourcedata, CDFType sourcetype, size_t length);

#endif