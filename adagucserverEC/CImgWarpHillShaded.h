/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2020-12-09
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

#ifndef CImgWarpHillShaded_H
#define CImgWarpHillShaded_H
#include <stdlib.h>
#include "CImageWarperRenderInterface.h"
#include "GenericDataWarper/CGenericDataWarper.h"
#include "utils.h"

class Vector {
public:
  Vector(float x, float y, float z) {
    this->x = x;
    this->y = y;
    this->z = z;
  }
  Vector operator-(const Vector &v) { return Vector(x - v.x, y - v.y, z - v.z); }
  float square() { return x * x + y * y + z * z; }
  float magnitude() { return sqrt(square()); }
  Vector normalize() {
    float f = magnitude();
    if (f == 0) return Vector(0, 0, 0);
    return Vector(x / f, y / f, z / f);
  }
  float x, y, z;
};

static inline Vector CrossProduct(const Vector &v1, const Vector &v2) { return Vector(v1.y * v2.z - v1.z * v2.y, v1.z * v2.x - v1.x * v2.z, v1.x * v2.y - v1.y * v2.x); }

static inline float DotProduct(const Vector &a, const Vector &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

static inline int mfast_mod(const int input, const int ceil) { return input >= ceil ? input % ceil : input; }

struct HillShadeSettings {
  double dfNodataValue;
  double legendValueRange;
  double legendLowerRange;
  double legendUpperRange;
  bool hasNodataValue;
  float *dataField;
  int width, height;
};

template <class T> void hillShadedDrawFunction(int x, int y, T val, void *_settings, void *_warper);
class CImgWarpHillShaded : public CImageWarperRenderInterface {
private:
  DEF_ERRORFUNCTION();

public:
  CImgWarpHillShaded() {}
  ~CImgWarpHillShaded() {}
  void render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) override;
  int set(const char *pszSettings) override;
};

#endif
