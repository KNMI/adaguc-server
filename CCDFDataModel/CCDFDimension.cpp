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
#include <cstddef>

#include "CCDFObject.h"
#include "CCDFDimension.h"

CDF::Dimension::Dimension() {}

CDF::Dimension::Dimension(const char *_name, size_t _length) {
  length = _length;
  name.copy(_name);
  id = -1;
}

CDF::Dimension::Dimension(CDFObject *cdfObject, const char *_name, size_t _length) {
  isIterative = false;
  length = _length;
  name.copy(_name);
  id = -1;
  cdfObject->addDimension(this);
}

CDF::Dimension *CDF::Dimension::clone() {
  CDF::Dimension *newDim = new CDF::Dimension();
  newDim->name = this->name.c_str();
  newDim->length = this->length;
  newDim->isIterative = this->isIterative;
  newDim->id = this->id;
  return newDim;
}

size_t CDF::Dimension::getSize() { return length; }

void CDF::Dimension::setSize(size_t _length) { length = _length; }

void CDF::Dimension::setName(const char *value) { name.copy(value); }

CT::string CDF::Dimension::getName() { return name; }