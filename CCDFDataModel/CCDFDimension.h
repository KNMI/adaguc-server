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

#ifndef CCDFDIMENSION_H
#define CCDFDIMENSION_H
#include <string>

namespace CDF {
  class Dimension {

  public:
    Dimension();
    Dimension(const char *_name, size_t _length);
    Dimension(const std::string &_name, size_t _length);
    std::string name;
    size_t length;
    bool isIterative;
    int id;
    size_t getSize();
    void setSize(size_t _length);
    void setName(const std::string &value);
    std::string getName();
    // Returns a new copy of this dimension
    Dimension *clone();
  };
} // namespace CDF

#endif
