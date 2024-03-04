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

#include "ProjectionKey.h"

ProjectionKey::ProjectionKey() {}

ProjectionKey MakeProjectionKey(double *box, double *dfMaxExtent, CT::string source, CT::string dest) {
  ProjectionKey newKey;
  for (int j = 0; j < 4; j++) {
    newKey.bbox[j] = box[j];
    newKey.dfMaxExtent[j] = dfMaxExtent[j];
  }
  newKey.sourceCRS = source;
  newKey.destinationCRS = dest;
  newKey.isSet = false;
  return newKey;
}

void ProjectionKey::setFoundExtent(double *_foundExtent) {
  for (int j = 0; j < 4; j++) {
    foundExtent[j] = _foundExtent[j];
  }
  isSet = true;
}
