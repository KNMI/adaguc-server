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
  setData(CDF_CHAR, attrString, strlen(attrString));
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

  freeData();

  length = dataLength;
  this->type = type;
  if (dataLength == 0) {
    return 0;
  }
  allocateData(length);
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

int CDF::Attribute::setData(const char *dataToSet) {
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

int CDF::Attribute::getDataAsString(CT::string *out) {
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
  getDataAsString(&out);
  return out;
}

CT::string CDF::Attribute::getDataAsString() { return toString(); }

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
    CDF::freeData(&data);
    data = nullptr;
  }
  length = 0;
}

void CDF::Attribute::allocateData(size_t size) {
  freeData();
  CDF::allocateData(type, &data, size);
  length = size;
}
