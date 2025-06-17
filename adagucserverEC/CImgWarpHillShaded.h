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
#include "CImgWarpGeneric.h"
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

class CImgWarpHillShaded : public CImageWarperRenderInterface {
private:
  DEF_ERRORFUNCTION();

  template <class T> static void drawFunction(int x, int y, T val, void *_settings) {
    CImgWarpGenericDrawFunctionState *drawSettings = static_cast<CImgWarpGenericDrawFunctionState *>(_settings);
    if (x < 0 || y < 0 || x > drawSettings->width || y > drawSettings->height) return;
    bool isNodata = false;
    if (drawSettings->hasNodataValue) {
      if ((val) == (T)drawSettings->dfNodataValue) isNodata = true;
    }
    if (!(val == val)) isNodata = true;
    if (!isNodata)
      if (drawSettings->legendValueRange)
        if (val < drawSettings->legendLowerRange || val > drawSettings->legendUpperRange) isNodata = true;
    if (!isNodata) {
      T *sourceData = (T *)drawSettings->sourceData;
      size_t sourceDataPX = drawSettings->sourceDataPX;
      size_t sourceDataPY = drawSettings->sourceDataPY;
      size_t sourceDataWidth = drawSettings->sourceDataWidth;
      size_t sourceDataHeight = drawSettings->sourceDataHeight;

      if (sourceDataPY > sourceDataHeight - 1) return;
      if (sourceDataPX > sourceDataWidth - 1) return;

      float values[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

      /* TODO make window size configurable */
      for (int wy = -1; wy < 2; wy++) {
        for (int wx = -1; wx < 2; wx++) {
          values[0][0] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 0 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 0, sourceDataHeight) * sourceDataWidth];
          values[1][0] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 1 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 0, sourceDataHeight) * sourceDataWidth];
          values[2][0] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 2 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 0, sourceDataHeight) * sourceDataWidth];
          values[0][1] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 0 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 1, sourceDataHeight) * sourceDataWidth];
          values[1][1] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 1 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 1, sourceDataHeight) * sourceDataWidth];
          values[2][1] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 2 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 1, sourceDataHeight) * sourceDataWidth];
          values[0][2] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 0 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 2, sourceDataHeight) * sourceDataWidth];
          values[1][2] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 1 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 2, sourceDataHeight) * sourceDataWidth];
          values[2][2] += (float)((T *)sourceData)[mfast_mod(sourceDataPX + 2 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 2, sourceDataHeight) * sourceDataWidth];
        }
      }

      Vector v00 = Vector((float)(sourceDataPX + 0), (float)sourceDataPY + 0, values[0][0]);
      Vector v10 = Vector((float)(sourceDataPX + 1), (float)sourceDataPY + 0, values[1][0]);
      Vector v20 = Vector((float)(sourceDataPX + 2), (float)sourceDataPY + 0, values[2][0]);
      Vector v01 = Vector((float)(sourceDataPX + 0), (float)sourceDataPY + 1, values[0][1]);
      Vector v11 = Vector((float)(sourceDataPX + 1), (float)sourceDataPY + 1, values[1][1]);
      Vector v21 = Vector((float)(sourceDataPX + 2), (float)sourceDataPY + 1, values[2][1]);
      Vector v02 = Vector((float)(sourceDataPX + 0), (float)sourceDataPY + 2, values[0][2]);
      Vector v12 = Vector((float)(sourceDataPX + 1), (float)sourceDataPY + 2, values[1][2]);

      if (x >= 0 && y >= 0 && x < (int)drawSettings->width && y < (int)drawSettings->height) {
        Vector normal00 = CrossProduct(v10 - v00, v01 - v00).normalize();
        Vector normal10 = CrossProduct(v20 - v10, v11 - v10).normalize();
        Vector normal01 = CrossProduct(v11 - v01, v02 - v01).normalize();
        Vector normal11 = CrossProduct(v21 - v11, v12 - v11).normalize();
        Vector lightSource = (Vector(-1, -1, -1)).normalize(); /* TODO make light source configurable */
        float c00 = DotProduct(lightSource, normal00);
        float c10 = DotProduct(lightSource, normal10);
        float c01 = DotProduct(lightSource, normal01);
        float c11 = DotProduct(lightSource, normal11);
        float dx = drawSettings->tileDx;
        float dy = drawSettings->tileDy;
        float gx1 = (1 - dx) * c00 + dx * c10;
        float gx2 = (1 - dx) * c01 + dx * c11;
        float bilValue = (1 - dy) * gx1 + dy * gx2;
        drawSettings->dataField[x + y * drawSettings->width] = (bilValue + 1) / 1.816486;
      }
    }
  };

public:
  CImgWarpHillShaded() {}
  ~CImgWarpHillShaded() {}
  void render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) override;
  int set(const char *pszSettings) override;
};

#endif
