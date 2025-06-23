/******************************************************************************
 *
 * Project:  XYZ vector support for ADAGUC Server
 * Purpose:  This simplifies vector calculations within adaguc server
 * Author:   Maarten Plieger, maarten.plieger@knmi.nl
 * Date:     2025-05-07
 *
 ******************************************************************************
 *
 * Copyright 2025, Royal Netherlands Meteorological Institute (KNMI)
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

#ifndef F8_VECTOR_H
#define F8_VECTOR_H
#include <math.h>
/*Usage:

    #include "f8vector.h"

    f8vector myVectorA = {.x = 0.123, .y = 0.456, .z = 0.567};
    f8vector myVectorB = {.x = 0.823, .y = 0.956, .z = 0.167};

    f8vector crossProduct = cross(myVectorA, myVectorB);
    f8vector normalized = crossProduct.norm();
    double dotProduct = dot(myVectorA, normalized);

 */

/**
 * Vector of x,y,z
 */
struct f8vector {
  double x, y, z;
  f8vector operator-(const f8vector &v) { return f8vector({.x = x - v.x, .y = y - v.y, .z = z - v.z}); }

  double square() { return x * x + y * y + z * z; }
  double magnitude() { return sqrt(x * x + y * y + z * z); }
  f8vector norm() {
    double f = magnitude();
    if (f == 0) return f8vector({.x = 0, .y = 0, .z = 0});
    return f8vector({.x = x / f, .y = y / f, .z = z / f});
  }
};

/**
 * Calculate dot product of vector a and b
 */
inline double dot(const f8vector &a, const f8vector &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

/**
 * Calculate cross product of a and b
 */
inline f8vector cross(const f8vector &v1, const f8vector &v2) { return f8vector({.x = v1.y * v2.z - v1.z * v2.y, .y = v1.z * v2.x - v1.x * v2.z, .z = v1.x * v2.y - v1.y * v2.x}); }

#endif