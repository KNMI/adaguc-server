/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
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

#ifndef CGeoParams_H
#define CGeoParams_H
#include "CTString.h"
#include <cmath>
#include <cstddef>

struct i4point {
  int x, y;
  bool operator==(const i4point &) const = default;
  double distance(i4point point);
};

struct f8point {
  double x, y;
  bool operator==(const f8point &) const = default;
  f8point rad();
};

struct i4box {
  int left, bottom, right, top;
  i4box &operator=(const int (&bbox)[4]);
  i4point span();
  void sort();
  void clip(i4box clip);
  void toArray(int (&box)[4]);
};

struct f8box {
  double left, bottom, right, top;
  f8box &operator=(const double (&bbox)[4]);
  f8point span();
  void sort();
  void clip(i4box clip);
  void toArray(double (&box)[4]);
  f8box swapXY();
  double get(size_t index);
  bool operator==(const f8box &) const = default;
};
/**
 * Custom comparison operator for the BBOX
 */
bool operator<(f8box a, f8box b);
/**
 * Makes a BBOX from a array of doubles
 */
f8box makef8box(const double (&dfBBOX)[4]);

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
  std::string crs;
  bool operator==(const GeoParameters &) const = default;
};

#endif
