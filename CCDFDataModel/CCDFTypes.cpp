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
#include <CDebugger.h>
#include "CDFCopyData.h"

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
  free(*p);
  *p = NULL;
  return 0;
}

// Data must be freed with freeData()
int CDF::allocateData(CDFType type, void **p, size_t length) {

  if ((*p) != NULL) {
    freeData(p);
  };
  (*p) = NULL;

  size_t typeSize = getTypeSize(type);
  if (typeSize == 0) {
    // CDBError("In CDF::allocateData: Unknown type");
    return 1;
  }

  *p = malloc(length * typeSize);

  if (*p == NULL) {
    // CDBError("In CDF::allocateData: Unable to allocate %d elements",length);
    return 1;
  }

  if (type == CDF_STRING) {
    char **data = (char **)*p;
    for (size_t j = 0; j < length; j++) {
      data[j] = nullptr;
    }
  }

  return 0;
}

void CDF::getCDFDataTypeName(char *name, const size_t maxlen, const int type) {
  snprintf(name, maxlen, "CDF_UNDEFINED");
  if (type == CDF_NONE) snprintf(name, maxlen, "CDF_NONE");
  if (type == CDF_BYTE) snprintf(name, maxlen, "CDF_BYTE");
  if (type == CDF_CHAR) snprintf(name, maxlen, "CDF_CHAR");
  if (type == CDF_SHORT) snprintf(name, maxlen, "CDF_SHORT");
  if (type == CDF_INT) snprintf(name, maxlen, "CDF_INT");
  if (type == CDF_INT64) snprintf(name, maxlen, "CDF_INT64");
  if (type == CDF_FLOAT) snprintf(name, maxlen, "CDF_FLOAT");
  if (type == CDF_DOUBLE) snprintf(name, maxlen, "CDF_DOUBLE");
  if (type == CDF_UNKNOWN) snprintf(name, maxlen, "CDF_UNKNOWN");
  if (type == CDF_UBYTE) snprintf(name, maxlen, "CDF_UBYTE");
  if (type == CDF_USHORT) snprintf(name, maxlen, "CDF_USHORT");
  if (type == CDF_UINT) snprintf(name, maxlen, "CDF_UINT");
  if (type == CDF_UINT64) snprintf(name, maxlen, "CDF_UINT64");
  if (type == CDF_STRING) snprintf(name, maxlen, "CDF_STRING");
}

void CDF::getCDataTypeName(char *name, const size_t maxlen, const int type) {
  snprintf(name, maxlen, "CDF_UNDEFINED");
  if (type == CDF_NONE) snprintf(name, maxlen, "none");
  if (type == CDF_BYTE) snprintf(name, maxlen, "uchar");
  if (type == CDF_CHAR) snprintf(name, maxlen, "char");
  if (type == CDF_SHORT) snprintf(name, maxlen, "short");
  if (type == CDF_INT) snprintf(name, maxlen, "int");
  if (type == CDF_INT64) snprintf(name, maxlen, "long");
  if (type == CDF_FLOAT) snprintf(name, maxlen, "float");
  if (type == CDF_DOUBLE) snprintf(name, maxlen, "double");
  if (type == CDF_UBYTE) snprintf(name, maxlen, "ubyte");
  if (type == CDF_USHORT) snprintf(name, maxlen, "ushort");
  if (type == CDF_UINT) snprintf(name, maxlen, "uint");
  if (type == CDF_UINT64) snprintf(name, maxlen, "ulong");
  if (type == CDF_STRING) snprintf(name, maxlen, "char*");
}

std::string CDF::getErrorMessage(const int errorCode) {
  std::string errorMessage = "CDF_E_UNDEFINED";
  if (errorCode == CDF_E_NONE) errorMessage = "CDF_E_NONE";
  if (errorCode == CDF_E_DIMNOTFOUND) errorMessage = "CDF_E_DIMNOTFOUND";
  if (errorCode == CDF_E_ATTNOTFOUND) errorMessage = "CDF_E_ATTNOTFOUND";
  if (errorCode == CDF_E_VARNOTFOUND) errorMessage = "CDF_E_VARNOTFOUND";
  if (errorCode == CDF_E_NRDIMSNOTEQUAL) errorMessage = "CDF_E_NRDIMSNOTEQUAL";
  if (errorCode == CDF_E_VARHASNOPARENT) errorMessage = "CDF_E_VARHASNOPARENT";
  if (errorCode == CDF_E_VARHASNODATA) errorMessage = "CDF_E_VARHASNODATA";
  return errorMessage;
}

CT::string CDF::getCDFDataTypeName(const int type) {
  char data[100];
  getCDFDataTypeName(data, 99, type);
  CT::string d = data;
  return d;
}

bool CDF::isCDFNumeric(CDFType type) {
  switch (type) {
  case CDF_CHAR:
  case CDF_STRING:
    return false;
  default:
    return true;
  }
}

int CDF::fill(void *destdata, CDFType destType, double value, size_t size) { return CDFFillData(destdata, destType, value, size); }
