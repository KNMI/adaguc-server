/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
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

#include "COGCDims.h"

void COGCDims::addValue(const char *value) {
  for (size_t j = 0; j < uniqueValues.size(); j++) {
    if (uniqueValues[j].equals(value)) return;
  }
  uniqueValues.push_back(value);
}

CCDFDims::~CCDFDims() { dimensions.clear(); }

void CCDFDims::addDimension(const char *name, const char *value, size_t index) {
  // CDBDebug("Adddimension %s %s %d, numdims = %d",name,value,index,dimensions.size());
  for (size_t j = 0; j < dimensions.size(); j++) {
    if (dimensions[j].name == name) {
      dimensions[j].index = index;
      dimensions[j].value = value;
      return;
    }
  }

  dimensions.push_back({.name = name, .value = value, .index = index});
}
size_t CCDFDims::getDimensionIndex(const char *name) {
  for (size_t j = 0; j < dimensions.size(); j++) {
    if (dimensions[j].name == name) {
      return dimensions[j].index;
    }
  }
  return 0;
}

int CCDFDims::getArrayIndexForName(const char *name) {
  for (size_t j = 0; j < dimensions.size(); j++) {
    if (dimensions[j].name == name) {
      return j;
    }
  }
  return -1;
}

size_t CCDFDims::getDimensionIndex(int j) {
  if (j < 0) return 0;
  if (size_t(j) >= dimensions.size()) return 0;
  return dimensions[j].index;
}
std::string CCDFDims::getDimensionValue(int j) {
  if (j < 0) return "NULL";
  if (size_t(j) >= dimensions.size()) return "NULL";
  if (isTimeDimension(j)) {
    std::string value = dimensions[j].value;
    // Format to YYYY-MM-DDTHH:MM:SSZ
    //           01234567890123456789
    if (value.length() < 20) {
      value += "Z";
    }
    value[10] = 'T';
    return value;
  }
  return dimensions[j].value;
}

std::string CCDFDims::getDimensionValuePointer(int j) {
  if (j < 0) return NULL;
  if (size_t(j) >= dimensions.size()) return NULL;

  return dimensions[j].value;
}

std::string CCDFDims::getDimensionValue(const char *name) { return getDimensionValue(getArrayIndexForName(name)); }

std::string CCDFDims::getDimensionName(int j) {
  if (j < 0) return "";
  if (size_t(j) >= dimensions.size()) return "";
  return dimensions[j].name;
}

void CCDFDims::copy(CCDFDims *dim) {
  if (dim != NULL) {
    for (size_t j = 0; j < dim->dimensions.size(); j++) {
      addDimension(dim->dimensions[j].name.c_str(), dim->dimensions[j].value.c_str(), dim->dimensions[j].index);
    }
  }
}

size_t CCDFDims::getNumDimensions() { return dimensions.size(); }

bool CCDFDims::isTimeDimension(int j) {
  std::string dimName = getDimensionName(j);
  if (dimName.empty() == false) {
    std::string dimTimeName = dimName;
    if (CT::indexOf(dimTimeName, "time") != -1 && CT::indexOf(dimTimeName, "reference_time") == -1) return true;
  }
  return false;
}

bool CCDFDims::isTimeDimension(const char *dimName) {
  if (dimName != NULL) {
    std::string dimTimeName = dimName;
    if (CT::indexOf(dimTimeName, "time") != -1 && CT::indexOf(dimTimeName, "reference_time") == -1) return true;
  }
  return false;
}
