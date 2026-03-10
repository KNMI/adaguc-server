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

#ifndef COGCDIMS_H
#define COGCDIMS_H
#include <cstdio>
#include "CTString.h"
#include "CDebugger.h"
struct COGCDims {

  std::string name;                      // OGC name
  std::string queryValue;                // Value, as given in the query_string
  std::string value;                     // Value, are all values as given in the KVP request string, can contain / and , tokens
  std::string netCDFDimName;             // NetCDF name
  std::vector<std::string> uniqueValues; // Unique values, similar to values except that they are filtered and selected as available from the file (distinct/grouped),
  bool isATimeDimension = false;
  bool hasFixedValue = false;
  bool hidden = false;
};
COGCDims makeTimeBasedOGCDim(std::string name, std::string value);
COGCDims makeEmptyOGCDim();
struct NetCDFDim {
  std::string name;
  std::string value;
  size_t index;
};

typedef std::vector<NetCDFDim> CCDFDims;

bool isOGCTimeDim(NetCDFDim &dimension);
std::string getCDFDimensionValue(CCDFDims &dimensions, const char *name);
/**
 * Find the dimension index by name in the CCDFDim object
 * @param name: Name of the dimension in the CDF model to find
 * @return The index of the dimension, or -1 when not found
 */
int findCDFDimIdx(CCDFDims &dimensions, const std::string &name);

#endif
