#include "CDFCopyData.h"
#include <cstddef>

template <typename T> int _copy(T *destdata, void *sourcedata, CDFType sourcetype, size_t destinationOffset, size_t sourceOffset, size_t length) {
  size_t dsto = destinationOffset;
  size_t srco = sourceOffset;
  if (sourcetype == CDF_STRING) {
    return 1;
  }
  CT::string t = typeid(T).name();
  if (t.equals(typeid(void).name())) {
    return 1;
  }

#define SPECIALIZE_TEMPLATE(CDFTYPE, CPPTYPE)                                                                                                                                                          \
  if (sourcetype == CDFTYPE) {                                                                                                                                                                         \
    for (size_t t = 0; t < length; t++) {                                                                                                                                                              \
      destdata[t + dsto] = (T)((CPPTYPE *)sourcedata)[t + srco];                                                                                                                                       \
    }                                                                                                                                                                                                  \
    return 0;                                                                                                                                                                                          \
  }
  ENUMERATE_OVER_CDFTYPES(SPECIALIZE_TEMPLATE)
#undef SPECIALIZE_TEMPLATE

  return 0;
}
template <class T> int copy(T *destdata, void *sourcedata, CDFType sourcetype, size_t length) { return _copy(destdata, sourcedata, sourcetype, 0, 0, length); }
template <class T> int _fill(T *data, double value, size_t size) {
  for (size_t j = 0; j < size; j++) {
    data[j] = value;
  }
  return 0;
}

int CDFFillData(void *destdata, CDFType destType, double value, size_t size) {
  if (destType == CDF_STRING) {
    for (size_t j = 0; j < size; j++) {
      free(((char **)destdata)[j]);
      ((char **)destdata)[j] = NULL;
    }
    return 0;
  }
#define SPECIALIZE_TEMPLATE(CDFTYPE, CPPTYPE)                                                                                                                                                          \
  if (destType == CDFTYPE) return _fill((CPPTYPE *)destdata, value, size);
  ENUMERATE_OVER_CDFTYPES(SPECIALIZE_TEMPLATE)
#undef SPECIALIZE_TEMPLATE
  return 1;
}

int CDFCopyData(void *destdata, CDFType destType, void *sourcedata, CDFType sourcetype, size_t destinationOffset, size_t sourceOffset, size_t length) {
  if (sourcetype == CDF_STRING || destType == CDF_STRING) {
    if (sourcetype == CDF_STRING && destType == CDF_STRING) {
      for (size_t t = 0; t < length; t++) {
        const char *sourceValue = ((char **)sourcedata)[t + sourceOffset];
        if (sourceValue != NULL) {
          size_t strlength = strlen(sourceValue);
          ((char **)destdata)[t + destinationOffset] = (char *)malloc(strlength + 1);
          strncpy(((char **)destdata)[t + destinationOffset], sourceValue, strlength);
          ((char **)destdata)[t + destinationOffset][strlength] = 0;
        }
      }
      return 0;
    }
    return 1;
  }
#define SPECIALIZE_TEMPLATE(CDFTYPE, CPPTYPE)                                                                                                                                                          \
  if (destType == CDFTYPE) return _copy((CPPTYPE *)destdata, sourcedata, sourcetype, destinationOffset, sourceOffset, length);
  ENUMERATE_OVER_CDFTYPES(SPECIALIZE_TEMPLATE)
#undef SPECIALIZE_TEMPLATE

  return 1;
}

template <class T> int CDFCopyData(T *destdata, void *sourcedata, CDFType sourcetype, size_t length) { return _copy(destdata, sourcedata, sourcetype, 0, 0, length); }

#define SPECIALIZE_TEMPLATE(CDFTYPE, CPPTYPE) template int CDFCopyData<CPPTYPE>(CPPTYPE * destdata, void *sourcedata, CDFType sourcetype, size_t length);
ENUMERATE_OVER_CDFTYPES(SPECIALIZE_TEMPLATE)
#undef SPECIALIZE_TEMPLATE