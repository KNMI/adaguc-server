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
#include "CTString.h"
#ifndef CCDFDIMENSION_H
#define CCDFDIMENSION_H

class CDFObject;
namespace CDF {

  class Dimension {
  public:
    CT::string name;
    size_t length = 0;
    bool isIterative = false;
    int id = -1;

    Dimension();
    Dimension(const char *_name, size_t _length);
    Dimension(CDFObject *cdfObject, const char *_name, size_t _length);

    size_t getSize();
    void setSize(size_t _length);
    void setName(const char *value);
    CT::string getName();
    Dimension *clone();
  };
} // namespace CDF

#endif
