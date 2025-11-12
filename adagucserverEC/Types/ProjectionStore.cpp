/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2024-02-29
 *
 ******************************************************************************
 *
 * Copyright 2024, Royal Netherlands Meteorological Institute (KNMI)
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

#include <map>
#include "ProjectionStore.h"

bool operator<(const ProjectionMapKey &a, const ProjectionMapKey &b) { return std::make_tuple(a.sourceCRS, a.destCRS, a.extent) < std::make_tuple(b.sourceCRS, b.destCRS, b.extent); }
static std::map<ProjectionMapKey, f8box> projectionMap;

std::tuple<bool, f8box> getBBOXProjection(ProjectionMapKey key) {
  auto it = projectionMap.find(key);
  if (it == projectionMap.end()) {
    return std::make_tuple(false, f8box{});
  }
  return std::make_tuple(true, it->second);
}

void addBBOXProjection(ProjectionMapKey key, f8box bbox) { projectionMap[key] = bbox; }

void BBOXProjectionClearCache() { projectionMap.clear(); }
