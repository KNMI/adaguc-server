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

std::string CDF::getErrorMessage(const int errorCode) {
  std::string errorMessage = "CDF_E_UNDEFINED";
  if (errorCode == CDF_E_NONE) errorMessage = "CDF_E_NONE";
  if (errorCode == CDF_E_DIMNOTFOUND) errorMessage = "CDF_E_DIMNOTFOUND";
  if (errorCode == CDF_E_ATTNOTFOUND) errorMessage = "CDF_E_ATTNOTFOUND";
  if (errorCode == CDF_E_VARNOTFOUND) errorMessage = "CDF_E_VARNOTFOUND";
  if (errorCode == CDF_E_NRDIMSNOTEQUAL) errorMessage = "CDF_E_NRDIMSNOTEQUAL";
  if (errorCode == CDF_E_VARHASNOPARENT) errorMessage = "CDF_E_VARHASNOPARENT";
  if (errorCode == CDF_E_VARHASNODATA) errorMessage = "CDF_E_VARHASNODATA";
  if (errorCode == CDF_E_CANNOTCREATECOORDVARIABLE) errorMessage = "CDF_E_CANNOTCREATECOORDVARIABLE";

  return errorMessage;
}

std::string CDF::getCDFDataTypeName(const int type) {
  if (type == CDF_NONE) return "CDF_NONE";
  if (type == CDF_BYTE) return "CDF_BYTE";
  if (type == CDF_CHAR) return "CDF_CHAR";
  if (type == CDF_SHORT) return "CDF_SHORT";
  if (type == CDF_INT) return "CDF_INT";
  if (type == CDF_INT64) return "CDF_INT64";
  if (type == CDF_FLOAT) return "CDF_FLOAT";
  if (type == CDF_DOUBLE) return "CDF_DOUBLE";
  if (type == CDF_UNKNOWN) return "CDF_UNKNOWN";
  if (type == CDF_UBYTE) return "CDF_UBYTE";
  if (type == CDF_USHORT) return "CDF_USHORT";
  if (type == CDF_UINT) return "CDF_UINT";
  if (type == CDF_UINT64) return "CDF_UINT64";
  if (type == CDF_STRING) return "CDF_STRING";
  return "CDF_UNDEFINED";
}

std::string CDF::getCDataTypeName(const int type) {
  if (type == CDF_NONE) return "none";
  if (type == CDF_BYTE) return "uchar";
  if (type == CDF_CHAR) return "char";
  if (type == CDF_SHORT) return "short";
  if (type == CDF_INT) return "int";
  if (type == CDF_INT64) return "long";
  if (type == CDF_FLOAT) return "float";
  if (type == CDF_DOUBLE) return "double";
  if (type == CDF_UBYTE) return "ubyte";
  if (type == CDF_USHORT) return "ushort";
  if (type == CDF_UINT) return "uint";
  if (type == CDF_UINT64) return "ulong";
  if (type == CDF_STRING) return "char*";
  return "CDF_UNDEFINED";
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
