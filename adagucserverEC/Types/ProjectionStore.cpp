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

#include "ProjectionStore.h"
#include <bits/pthreadtypes.h>
#include <pthread.h>

const char *ProjectionStore::className = "ProjectionStore";

ProjectionStore projectionStore;
extern ProjectionStore projectionStore;

ProjectionStore *ProjectionStore::getProjectionStore() { return &projectionStore; }

ProjectionStore::ProjectionStore() {}
ProjectionStore::~ProjectionStore() { clear(); }

pthread_mutex_t ProjectionKey_clear;
void ProjectionStore::clear() { keys.clear(); }

int ProjectionStore::findExtentForKey(ProjectionKey pKey, BBOX *bbox) {
  for (size_t j = 0; j < this->keys.size(); j++) {
    if (this->keys[j].isSet == true) {
      bool match = true;
      // First find matching destination CRS
      if (!this->keys[j].destinationCRS.equals(pKey.destinationCRS)) match = false;
      // Next match on bbox
      if (match) {
        for (int i = 0; i < 4; i++) {
          if (this->keys[j].bbox[i] != pKey.bbox[i]) {
            // CDBDebug("%f != %f", this->keys[j].bbox[i], pKey.bbox[i]);
            match = false;
            break;
          }
        }
        // Destination CRS and BBOX do match
        if (match) {
          if (!this->keys[j].sourceCRS.equals(pKey.sourceCRS)) match = false;
          if (match) {
            // Source CRS, Destination CRS and BBOX do Match
            for (int i = 0; i < 4; i++) {
              bbox->bbox[i] = this->keys[j].foundExtent[i];
            }
            return 0;
          }
        }
      }
    }
  }
  return -1;
}