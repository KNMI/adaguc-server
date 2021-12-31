/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Generic Data model to read netcdf and hdf5
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2013-06-01
 *
 ******************************************************************************
 *
 * Copyright 2013, Royal Netherlands Meteorological Institute (KNMI)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************************/

#include "CCDFTypes.h"

int CDF::getTypeSize(CDFType type) {
  if (type == CDF_CHAR || type == CDF_UBYTE || type == CDF_BYTE) return 1;
  if (type == CDF_SHORT || type == CDF_USHORT) return 2;
  if (type == CDF_INT || type == CDF_UINT) return 4;
  if (type == CDF_INT64 || type == CDF_UINT64) return 8;
  if (type == CDF_FLOAT) return 4;
  if (type == CDF_DOUBLE) return 8;
  if (type == CDF_UNKNOWN) return 8;
  if (type == CDF_STRING) return sizeof(char *);
  return 0;
}

int CDF::freeData(void **p) {
#ifdef CCDFTYPES_MEMLEAKCHECK
  if (Tracer::Ready) NewTrace.Remove(*p);
#endif
  free(*p);
  *p = NULL;
  return 0;
}

/* NOTE! Data must be freed with freeData() */
int CDF::allocateData(CDFType type, void **p, size_t length) {

  if ((*p) != NULL) {
    freeData(p);
  };
  (*p) = NULL;

  size_t typeSize = getTypeSize(type);
  if (typeSize == 0) {
    return 1;
  }
  *p = malloc(length * typeSize);

  if (*p == NULL) {
    return 1;
  }

#ifdef CCDFTYPES_MEMLEAKCHECK
  if (Tracer::Ready) NewTrace.Add(*p, __FILENAME__, __LINE__);
#endif

  if (type == CDF_STRING) {
    for (size_t j = 0; j < length; j++) {
      ((char **)*p)[j] = NULL;
    }
  }
  return 0;
}

CT::string CDF::getCDataTypeName(CDFType type) {
  if (type == CDF_NONE) return ("none");
  if (type == CDF_BYTE) return ("uchar");
  if (type == CDF_CHAR) return ("char");
  if (type == CDF_SHORT) return ("short");
  if (type == CDF_INT) return ("int");
  if (type == CDF_INT64) return ("long");
  if (type == CDF_FLOAT) return ("float");
  if (type == CDF_DOUBLE) return ("double");
  if (type == CDF_UBYTE) return ("ubyte");
  if (type == CDF_USHORT) return ("ushort");
  if (type == CDF_UINT) return ("uint");
  if (type == CDF_UINT64) return ("ulong");
  if (type == CDF_STRING) return ("char*");
  return "CDF_UNDEFINED";
}

CT::string CDF::getCDFDataTypeName(CDFType type) {
  if (type == CDF_NONE) return ("CDF_NONE");
  if (type == CDF_BYTE) return ("CDF_BYTE");
  if (type == CDF_CHAR) return ("CDF_CHAR");
  if (type == CDF_SHORT) return ("CDF_SHORT");
  if (type == CDF_INT) return ("CDF_INT");
  if (type == CDF_INT64) return ("CDF_INT64");
  if (type == CDF_FLOAT) return ("CDF_FLOAT");
  if (type == CDF_DOUBLE) return ("CDF_DOUBLE");
  if (type == CDF_UNKNOWN) return ("CDF_UNKNOWN");
  if (type == CDF_UBYTE) return ("CDF_UBYTE");
  if (type == CDF_USHORT) return ("CDF_USHORT");
  if (type == CDF_UINT) return ("CDF_UINT");
  if (type == CDF_UINT64) return ("CDF_UINT64");
  if (type == CDF_STRING) return ("CDF_STRING");
  return "CDF_UNDEFINED";
}

CT::string CDF::getErrorMessage(int errorCode) {
  if (errorCode == CDF_E_NONE) return ("CDF_E_NONE");
  if (errorCode == CDF_E_DIMNOTFOUND) return ("CDF_E_DIMNOTFOUND");
  if (errorCode == CDF_E_ATTNOTFOUND) return ("CDF_E_ATTNOTFOUND");
  if (errorCode == CDF_E_VARNOTFOUND) return ("CDF_E_VARNOTFOUND");
  if (errorCode == CDF_E_NRDIMSNOTEQUAL) return ("CDF_E_NRDIMSNOTEQUAL");
  if (errorCode == CDF_E_VARHASNOPARENT) return ("CDF_E_VARHASNOPARENT");
  if (errorCode == CDF_E_VARHASNODATA) return ("CDF_E_VARHASNODATA");
  return "CDF_E_UNDEFINED";
}

template <class T> int CDF::DataCopierDestDataTemplated<T>::copy(T *destdata, void *sourcedata, CDFType sourcetype, size_t destinationOffset, size_t sourceOffset, size_t length) {

  size_t dsto = destinationOffset;
  size_t srco = sourceOffset;
  if (sourcetype == CDF_STRING) {
    // CDBError("Unable to copy CDF_STRING");
    return 1;
  }
  CT::string t = typeid(T).name();
  if (t.equals(typeid(void).name())) {
    return 1;
  }

  if (sourcetype == CDF_CHAR || sourcetype == CDF_BYTE)
    for (size_t t = 0; t < length; t++) {
      destdata[t + dsto] = (T)((char *)sourcedata)[t + srco];
    }
  if (sourcetype == CDF_CHAR || sourcetype == CDF_UBYTE)
    for (size_t t = 0; t < length; t++) {
      destdata[t + dsto] = (T)((unsigned char *)sourcedata)[t + srco];
    }
  if (sourcetype == CDF_SHORT)
    for (size_t t = 0; t < length; t++) {
      destdata[t + dsto] = (T)((short *)sourcedata)[t + srco];
    }
  if (sourcetype == CDF_USHORT)
    for (size_t t = 0; t < length; t++) {
      destdata[t + dsto] = (T)((unsigned short *)sourcedata)[t + srco];
    }
  if (sourcetype == CDF_INT)
    for (size_t t = 0; t < length; t++) {
      destdata[t + dsto] = (T)((int *)sourcedata)[t + srco];
    }
  if (sourcetype == CDF_UINT)
    for (size_t t = 0; t < length; t++) {
      destdata[t + dsto] = (T)((unsigned int *)sourcedata)[t + srco];
    }
  if (sourcetype == CDF_INT64)
    for (size_t t = 0; t < length; t++) {
      destdata[t + dsto] = (T)((long *)sourcedata)[t + srco];
    }
  if (sourcetype == CDF_UINT64)
    for (size_t t = 0; t < length; t++) {
      destdata[t + dsto] = (T)((unsigned long *)sourcedata)[t + srco];
    }
  if (sourcetype == CDF_FLOAT)
    for (size_t t = 0; t < length; t++) {
      destdata[t + dsto] = (T)((float *)sourcedata)[t + srco];
    }
  if (sourcetype == CDF_DOUBLE)
    for (size_t t = 0; t < length; t++) {
      destdata[t + dsto] = (T)((double *)sourcedata)[t + srco];
    }
  return 0;
}

template <class T> int CDF::DataCopierDestDataTemplated<T>::copy(T *destdata, void *sourcedata, CDFType sourcetype, size_t length) { return copy(destdata, sourcedata, sourcetype, 0, 0, length); }

template <class T> void CDF::DataCopierDestDataTemplated<T>::fill(T *data, double value, size_t size) {
  for (size_t j = 0; j < size; j++) {
    data[j] = value;
  }
}

// Explicit template instantiation for DataCopierDestDataTemplated
template class CDF::DataCopierDestDataTemplated<unsigned char>;
template class CDF::DataCopierDestDataTemplated<char>;
template class CDF::DataCopierDestDataTemplated<unsigned short>;
template class CDF::DataCopierDestDataTemplated<short>;
template class CDF::DataCopierDestDataTemplated<unsigned int>;
template class CDF::DataCopierDestDataTemplated<int>;
template class CDF::DataCopierDestDataTemplated<unsigned long>;
template class CDF::DataCopierDestDataTemplated<long>;
template class CDF::DataCopierDestDataTemplated<float>;
template class CDF::DataCopierDestDataTemplated<double>;

int CDF::copy(void *destdata, CDFType destType, void *sourcedata, CDFType sourcetype, size_t destinationOffset, size_t sourceOffset, size_t length) {
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
  switch (destType) {

  case CDF_CHAR:
  case CDF_BYTE:
    CDF::DataCopierDestDataTemplated<char>::copy((char *)destdata, sourcedata, sourcetype, destinationOffset, sourceOffset, length);
    break;
  case CDF_UBYTE:
    CDF::DataCopierDestDataTemplated<unsigned char>::copy((unsigned char *)destdata, sourcedata, sourcetype, destinationOffset, sourceOffset, length);
    break;
  case CDF_SHORT:
    CDF::DataCopierDestDataTemplated<short>::copy((short *)destdata, sourcedata, sourcetype, destinationOffset, sourceOffset, length);
    break;
  case CDF_USHORT:
    CDF::DataCopierDestDataTemplated<unsigned short>::copy((unsigned short *)destdata, sourcedata, sourcetype, destinationOffset, sourceOffset, length);
    break;
  case CDF_INT:
    CDF::DataCopierDestDataTemplated<int>::copy((int *)destdata, sourcedata, sourcetype, destinationOffset, sourceOffset, length);
    break;
  case CDF_UINT:
    CDF::DataCopierDestDataTemplated<unsigned int>::copy((unsigned int *)destdata, sourcedata, sourcetype, destinationOffset, sourceOffset, length);
    break;
  case CDF_INT64:
    CDF::DataCopierDestDataTemplated<long>::copy((long *)destdata, sourcedata, sourcetype, destinationOffset, sourceOffset, length);
    break;
  case CDF_UINT64:
    CDF::DataCopierDestDataTemplated<unsigned long>::copy((unsigned long *)destdata, sourcedata, sourcetype, destinationOffset, sourceOffset, length);
    break;
  case CDF_FLOAT:
    CDF::DataCopierDestDataTemplated<float>::copy((float *)destdata, sourcedata, sourcetype, destinationOffset, sourceOffset, length);
    break;
  case CDF_DOUBLE:
    CDF::DataCopierDestDataTemplated<double>::copy((double *)destdata, sourcedata, sourcetype, destinationOffset, sourceOffset, length);
    break;
  default:
    return 1;
  }
  return 0;
}

int CDF::fill(void *destdata, CDFType destType, double value, size_t size) {
  if (destType == CDF_STRING) {
    for (size_t j = 0; j < size; j++) {
      free(((char **)destdata)[j]);
      ((char **)destdata)[j] = NULL;
    }
    return 0;
  }
  switch (destType) {
  case CDF_CHAR:
    DataCopierDestDataTemplated<char>::fill((char *)destdata, value, size);
    break;
  case CDF_BYTE:
    DataCopierDestDataTemplated<char>::fill((char *)destdata, value, size);
    break;
  case CDF_UBYTE:
    DataCopierDestDataTemplated<unsigned char>::fill((unsigned char *)destdata, value, size);
    break;
  case CDF_SHORT:
    DataCopierDestDataTemplated<short>::fill((short *)destdata, value, size);
    break;
  case CDF_USHORT:
    DataCopierDestDataTemplated<unsigned short>::fill((unsigned short *)destdata, value, size);
    break;
  case CDF_INT:
    DataCopierDestDataTemplated<int>::fill((int *)destdata, value, size);
    break;
  case CDF_UINT:
    DataCopierDestDataTemplated<unsigned int>::fill((unsigned int *)destdata, value, size);
    break;
  case CDF_INT64:
    DataCopierDestDataTemplated<long>::fill((long *)destdata, value, size);
    break;
  case CDF_UINT64:
    DataCopierDestDataTemplated<unsigned long>::fill((unsigned long *)destdata, value, size);
    break;
  case CDF_FLOAT:
    DataCopierDestDataTemplated<float>::fill((float *)destdata, value, size);
    break;
  case CDF_DOUBLE:
    DataCopierDestDataTemplated<double>::fill((double *)destdata, value, size);
    break;
  default:
    return 1;
  }
  return 0;
}
