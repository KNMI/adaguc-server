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

#include "CTString.h"
#include "BBOX.h"

#ifndef PROJECTIONKEY_H
#define PROJECTIONKEY_H

class ProjectionKey {
public:
  double bbox[4]; // Original boundingbox to look for
  double foundExtent[4];
  double dfMaxExtent[4];
  bool isSet;
  CT::string destinationCRS; // Projection to convert to
  CT::string sourceCRS;      // Projection to convert from
  ProjectionKey(double *_box, double *_dfMaxExtent, CT::string source, CT::string dest);
  ProjectionKey();
  void setFoundExtent(double *_foundExtent);
};

ProjectionKey MakeProjectionKey(double *box, double *dfMaxExtent, CT::string source, CT::string dest);

#endif // ! PROJECTIONKEY_H
