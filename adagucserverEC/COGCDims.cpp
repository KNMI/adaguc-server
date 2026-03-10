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
#include <algorithm>

int findCDFDimIdx(CCDFDims &dimensions, const std::string &name) {
  auto it = std::find_if(dimensions.begin(), dimensions.end(), [&name](NetCDFDim &a) { return a.name == name; });
  return it == dimensions.end() ? -1 : std::distance(dimensions.begin(), it);
}

bool isOGCTimeDim(NetCDFDim &dimension) { return (CT::indexOf(dimension.name, "time") != -1 && CT::indexOf(dimension.name, "reference_time") == -1); }

std::string _getDimensionValue(std::vector<NetCDFDim> &dimensions, int j) {
  if (j < 0 || size_t(j) >= dimensions.size()) return "NULL";
  if (isOGCTimeDim(dimensions[j])) {
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

std::string getCDFDimensionValue(CCDFDims &dimensions, const char *name) { return _getDimensionValue(dimensions, findCDFDimIdx(dimensions, name)); }

COGCDims makeTimeBasedOGCDim(std::string name, std::string value) {
  return {.name = name, .queryValue = value, .value = value, .netCDFDimName = name, .uniqueValues = {value}, .isATimeDimension = true, .hasFixedValue = false, .hidden = false};
}
COGCDims makeEmptyOGCDim() { return {.name = "none", .queryValue = "0", .value = "0", .netCDFDimName = "none", .uniqueValues = {}, .isATimeDimension = false, .hasFixedValue = false, .hidden = true}; }
