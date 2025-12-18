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

#ifndef CGeoParams_H
#define CGeoParams_H
#include "CTString.h"
#include <math.h>
#include <map>
#include <cstddef>

struct i4point {
  int x, y;
  double distance(i4point point);
};

struct f8point {
  double x, y;
  f8point rad();
};

struct i4box {
  int left, bottom, right, top;
  void operator=(const int bbox[4]);
  i4point span();
  void sort();
  void clip(i4box clip);
  void toArray(int box[4]);
};

struct f8box {
  double left, bottom, right, top;
  void operator=(const double bbox[4]);
  f8point span();
  void sort();
  void clip(i4box clip);
  void toArray(double box[4]);
  f8box swapXY();
  double get(size_t index);
};
/**
 * Custom comparison operator for the BBOX
 */
bool operator<(f8box a, f8box b);
/**
 * Makes a BBOX from a array of doubles
 */
f8box makef8box(double *dfBBOX);

struct f8component {
  double u, v;
  double magnitude();
  double direction();
  double angledeg();
};

struct GeoParameters {
  int width = 1;
  int height = 1;
  f8box bbox;
  double cellsizeX = 0;
  double cellsizeY = 0;
  CT::string crs;
};

#endif
