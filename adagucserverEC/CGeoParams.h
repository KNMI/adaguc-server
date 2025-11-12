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
#include "CTypes.h"
#include <math.h>
#include <map>
#include <CKeyValuePair.h>

struct i4point {
  int x, y;
};

struct f8point {
  double x, y;
  f8point rad() { return {.x = (x * (M_PI / 180.)), .y = (y * (M_PI / 180.))}; }
};

struct i4box {
  int left, bottom, right, top;
  void operator=(const int bbox[4]) {
    left = bbox[0];
    bottom = bbox[1];
    right = bbox[2];
    top = bbox[3];
  }
  i4point span() { return {.x = right - left, .y = top - bottom}; };
  void sort() {
    if (left > right) std::swap(left, right);
    if (bottom > top) std::swap(bottom, top);
  }
  void clip(i4box clip) {
    if (left < clip.left) left = clip.left;
    if (bottom < clip.bottom) bottom = clip.bottom;
    if (right > clip.right) right = clip.right;
    if (top > clip.top) top = clip.top;
  }
  void toArray(int box[4]) {
    box[0] = left;
    box[1] = bottom;
    box[2] = right;
    box[3] = top;
  }
};

struct f8box {
  double left, bottom, right, top;
  void operator=(const double bbox[4]) {
    left = bbox[0];
    bottom = bbox[1];
    right = bbox[2];
    top = bbox[3];
  }
  f8point span() { return {.x = right - left, .y = top - bottom}; }

  void sort() {
    if (left > right) std::swap(left, right);
    if (bottom > top) std::swap(bottom, top);
  }
  void clip(i4box clip) {
    if (left < clip.left) left = clip.left;
    if (bottom < clip.bottom) bottom = clip.bottom;
    if (right > clip.right) right = clip.right;
    if (top > clip.top) top = clip.top;
  }
  void toArray(double box[4]) {
    box[0] = left;
    box[1] = bottom;
    box[2] = right;
    box[3] = top;
  }

  f8box swapXY() { return {.left = bottom, .bottom = left, .right = top, .top = right}; }

  double get(size_t index) {
    switch (index) {
    case 0:
      return left;
    case 1:
      return bottom;
    case 2:
      return right;
    case 3:
      return top;
    default:
      return NAN;
    }
  }
};

struct f8component {
  double u, v;
  double magnitude() { return hypot(u, v); }
  double direction() { return atan2(v, u); }
  double angledeg() { return ((atan2(u, v) * (180 / M_PI) + 180)); }
};

struct CGeoParams {
  int dWidth = 1;
  int dHeight = 1;
  f8box bbox;
  double dfCellSizeX = 0;
  double dfCellSizeY = 0;
  CT::string CRS;
  CT::string BBOX_CRS;
};

#endif
