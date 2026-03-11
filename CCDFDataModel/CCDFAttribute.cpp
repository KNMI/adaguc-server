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
#include "CCDFAttribute.h"
#include "CDFCopyData.h"

void CDF::Attribute::setName(const char *value) { name.copy(value); }

CDF::Attribute::Attribute() {
  data = NULL;
  length = 0;
}

CDF::Attribute::Attribute(Attribute *att) {
  data = NULL;
  length = 0;

  name.copy(&att->name);
  setData(att);
}

CDF::Attribute::Attribute(const char *attrName, const char *attrString) {
  data = NULL;
  length = 0;
  name.copy(attrName);
  setString(attrString);
}

CDF::Attribute::Attribute(const char *attrName, CDFType type, const void *dataToSet, size_t dataLength) {
  data = NULL;
  length = 0;
  name.copy(attrName);
  setData(type, dataToSet, dataLength);
}

CDFType CDF::Attribute::getType() { return type; }

int CDF::Attribute::setData(Attribute *attribute) {
  this->setData(attribute->type, attribute->data, attribute->size());
  return 0;
}
int CDF::Attribute::setData(CDFType type, const void *dataToSet, size_t dataLength) {

  this->freeData();
  length = dataLength;
  this->type = type;
  if (dataLength == 0) {
    return 0;
  }
  this->allocateData(length);
  if (type == CDF_CHAR || type == CDF_UBYTE || type == CDF_BYTE) memcpy(data, dataToSet, length);
  if (type == CDF_SHORT || type == CDF_USHORT) memcpy(data, dataToSet, length * sizeof(short));
  if (type == CDF_INT || type == CDF_UINT) memcpy(data, dataToSet, length * sizeof(int));
  if (type == CDF_INT64 || type == CDF_UINT64) memcpy(data, dataToSet, length * sizeof(long));
  if (type == CDF_FLOAT) memcpy(data, dataToSet, length * sizeof(float));
  if (type == CDF_DOUBLE) {
    memcpy(data, dataToSet, length * sizeof(double));
  }
  if (type == CDF_STRING) {
    for (size_t j = 0; j < dataLength; j += 1) {
      const char *sourceStr = ((char **)dataToSet)[j];
      size_t length = strlen(sourceStr) + 1;
      ((char **)data)[j] = (char *)malloc(length * sizeof(char));
      strncpy(((char **)data)[j], ((char **)dataToSet)[j], length);
    }
  }
  return 0;
}

int CDF::Attribute::setString(const char *dataToSet) {
  freeData();
  auto data_length = strlen(dataToSet);
  this->type = CDF_CHAR;
  allocateData(data_length + 1);
  if (type == CDF_CHAR) {
    memcpy(data, dataToSet, data_length); // TODO support other data types as well
    ((char *)data)[data_length] = '\0';
  }
  return 0;
}

int CDF::Attribute::_getDataAsString(CT::string *out) {
  out->copy("");
  if (type == CDF_CHAR) {
    out->copy((const char *)data, length);
    int a = strlen(out->c_str());
    out->setSize(a);
    return 0;
  }
  if (type == CDF_BYTE)
    for (size_t n = 0; n < length; n++) {
      if (out->length() > 0) out->concat(" ");
      out->printconcat("%d", ((char *)data)[n]);
    }
  if (type == CDF_UBYTE)
    for (size_t n = 0; n < length; n++) {
      if (out->length() > 0) out->concat(" ");
      out->printconcat("%u", ((unsigned char *)data)[n]);
    }

  if (type == CDF_INT)
    for (size_t n = 0; n < length; n++) {
      if (out->length() > 0) out->concat(" ");
      out->printconcat("%d", ((int *)data)[n]);
    }
  if (type == CDF_UINT)
    for (size_t n = 0; n < length; n++) {
      if (out->length() > 0) out->concat(" ");
      out->printconcat("%u", ((unsigned int *)data)[n]);
    }

  if (type == CDF_INT64)
    for (size_t n = 0; n < length; n++) {
      if (out->length() > 0) out->concat(" ");
      out->printconcat("%ld", ((long *)data)[n]);
    }
  if (type == CDF_UINT64)
    for (size_t n = 0; n < length; n++) {
      if (out->length() > 0) out->concat(" ");
      out->printconcat("%lu", ((unsigned long *)data)[n]);
    }

  if (type == CDF_SHORT)
    for (size_t n = 0; n < length; n++) {
      if (out->length() > 0) out->concat(" ");
      out->printconcat("%d", ((short *)data)[n]);
    }
  if (type == CDF_USHORT)
    for (size_t n = 0; n < length; n++) {
      if (out->length() > 0) out->concat(" ");
      out->printconcat("%u", ((unsigned short *)data)[n]);
    }

  if (type == CDF_FLOAT)
    for (size_t n = 0; n < length; n++) {
      if (out->length() > 0) out->concat(" ");
      out->printconcat("%f", ((float *)data)[n]);
    }
  if (type == CDF_DOUBLE)
    for (size_t n = 0; n < length; n++) {
      if (out->length() > 0) out->concat(" ");
      out->printconcat("%f", ((double *)data)[n]);
    }
  if (type == CDF_STRING)
    for (size_t n = 0; n < length; n++) {
      if (out->length() > 0) out->concat(" ");
      out->printconcat("%s", ((char **)data)[n]);
    }
  return 0;
}

CT::string CDF::Attribute::toString() {
  CT::string out = "";
  _getDataAsString(&out);
  return out;
}

size_t CDF::Attribute::size() { return length; }

CDF::Attribute::~Attribute() { freeData(); }

void CDF::Attribute::freeData() {
  if (data != nullptr) {
    if (type == CDF_STRING) {
      char **vardata = (char **)this->data;
      for (size_t j = 0; j < length; j++) {
        free(vardata[j]);
        vardata[j] = nullptr;
      }
    }
    free(data);
    data = nullptr;
  }
  length = 0;
}

void CDF::Attribute::allocateData(size_t size) {
  this->freeData();
  CDF::allocateData(type, &data, size);
  length = size;
}

template <class T> int CDF::Attribute::setData(CDFType type, T data) {

#define SPECIALIZE_TEMPLATE(CDFTYPE, CPPTYPE)                                                                                                                                                          \
  if (type == CDFTYPE) {                                                                                                                                                                               \
    CPPTYPE d = data;                                                                                                                                                                                  \
    setData(type, &d, 1);                                                                                                                                                                              \
  };
  ENUMERATE_OVER_CDFTYPES(SPECIALIZE_TEMPLATE)
#undef SPECIALIZE_TEMPLATE

  return 0;
}

template <class T> int CDF::Attribute::getData(T *dataToGet, size_t getlength) {
  if (data == NULL) return 0;
  if (getlength > length) getlength = length;
  CDFCopyData(dataToGet, data, type, getlength);
  return getlength;
}

template <class T> T CDF::Attribute::getDataAt(int index) {
  if (data == NULL) {
    throw(CDF_E_VARHASNODATA);
  }
  T dataElement = 0;

#define SPECIALIZE_TEMPLATE(CDFTYPE, CPPTYPE)                                                                                                                                                          \
  if (type == CDFTYPE) dataElement = (T)((CPPTYPE *)data)[index];
  ENUMERATE_OVER_CDFTYPES(SPECIALIZE_TEMPLATE)
#undef SPECIALIZE_TEMPLATE

  return dataElement;
}

#define SPECIALIZE_TEMPLATE(CDFTYPE, CPPTYPE) template int CDF::Attribute::setData<CPPTYPE>(CDFType type, CPPTYPE data);
ENUMERATE_OVER_CDFTYPES(SPECIALIZE_TEMPLATE)
#undef SPECIALIZE_TEMPLATE

#define SPECIALIZE_TEMPLATE(CDFTYPE, CPPTYPE) template int CDF::Attribute::getData<CPPTYPE>(CPPTYPE * dataToGet, size_t getlength);
ENUMERATE_OVER_CDFTYPES(SPECIALIZE_TEMPLATE)
#undef SPECIALIZE_TEMPLATE

#define SPECIALIZE_TEMPLATE(CDFTYPE, CPPTYPE) template CPPTYPE CDF::Attribute::getDataAt<CPPTYPE>(int index);
ENUMERATE_OVER_CDFTYPES(SPECIALIZE_TEMPLATE)
#undef SPECIALIZE_TEMPLATE