/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Generic Data model to read netcdf and hdf5
 * Author:   Maarten Plieger, plieger "at" knmi.nl, GST - GeoSpatialTeam KNMI
 * Date:     2026-09-10
 *
 ******************************************************************************
 *
 * Copyright 2026, Royal Netherlands Meteorological Institute (KNMI)
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

#include "CCDFDimension.h"

CDF::Dimension::Dimension() {
  isIterative = false;
  length = 0;
  id = -1;
}

CDF::Dimension::Dimension(const char *_name, size_t _length) {
  isIterative = false;
  length = _length;
  name = (_name);
  id = -1;
}

CDF::Dimension::Dimension(const std::string &_name, size_t _length) {
  isIterative = false;
  length = _length;
  name = (_name);
  id = -1;
}

size_t CDF::Dimension::getSize() { return length; }
void CDF::Dimension::setSize(size_t _length) { length = _length; }
void CDF::Dimension::setName(const std::string &value) { name = value; }
std::string CDF::Dimension::getName() { return name; }

// Returns a new copy of this dimension
CDF::Dimension *CDF::Dimension::clone() {
  Dimension *newDim = new Dimension();
  newDim->name = name;
  newDim->length = length;
  newDim->isIterative = isIterative;
  newDim->id = id;
  return newDim;
}
