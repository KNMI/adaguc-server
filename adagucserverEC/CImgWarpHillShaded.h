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
#include "f8vector.h"
#include "CImageWarperRenderInterface.h"
#include "CImgWarpGeneric.h"
#include "utils.h"

static inline int mfast_mod(const int input, const int ceil) { return input >= ceil ? input % ceil : input; }

class CImgWarpHillShaded : public CImageWarperRenderInterface {
private:
  DEF_ERRORFUNCTION();

  template <class T> static void drawFunction(int x, int y, T val, GDWState &warperState, CImgWarpGenericDrawFunctionState &drawFunctionState) {
    if (x < 0 || y < 0 || x > drawFunctionState.width || y > drawFunctionState.height) return;
    bool isNodata = false;
    if (warperState.hasNodataValue) {
      if ((val) == (T)warperState.dfNodataValue) isNodata = true;
    }
    if (!(val == val)) isNodata = true;
    if (!isNodata)
      if (drawFunctionState.legendValueRange)
        if (val < drawFunctionState.legendLowerRange || val > drawFunctionState.legendUpperRange) isNodata = true;
    if (!isNodata) {
      T *sourceData = (T *)warperState.sourceData;
      size_t sourceDataPX = warperState.sourceDataPX;
      size_t sourceDataPY = warperState.sourceDataPY;
      size_t sourceDataWidth = warperState.sourceDataWidth;
      size_t sourceDataHeight = warperState.sourceDataHeight;

      if (sourceDataPY > sourceDataHeight - 1) return;
      if (sourceDataPX > sourceDataWidth - 1) return;

      float values[3][3] = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

      /* TODO make window size configurable */
      for (int wy = -1; wy < 2; wy++) {
        for (int wx = -1; wx < 2; wx++) {
          values[0][0] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 0 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 0, sourceDataHeight) * sourceDataWidth];
          values[1][0] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 1 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 0, sourceDataHeight) * sourceDataWidth];
          values[2][0] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 2 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 0, sourceDataHeight) * sourceDataWidth];
          values[0][1] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 0 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 1, sourceDataHeight) * sourceDataWidth];
          values[1][1] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 1 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 1, sourceDataHeight) * sourceDataWidth];
          values[2][1] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 2 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 1, sourceDataHeight) * sourceDataWidth];
          values[0][2] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 0 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 2, sourceDataHeight) * sourceDataWidth];
          values[1][2] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 1 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 2, sourceDataHeight) * sourceDataWidth];
          values[2][2] += (double)((T *)sourceData)[mfast_mod(sourceDataPX + 2 + wx, sourceDataWidth) + mfast_mod(sourceDataPY + wy + 2, sourceDataHeight) * sourceDataWidth];
        }
      }

      f8vector v00 = f8vector({.x = sourceDataPX + 0., .y = sourceDataPY + 0., .z = values[0][0]});
      f8vector v10 = f8vector({.x = sourceDataPX + 1., .y = sourceDataPY + 0., .z = values[1][0]});
      f8vector v20 = f8vector({.x = sourceDataPX + 2., .y = sourceDataPY + 0., .z = values[2][0]});
      f8vector v01 = f8vector({.x = sourceDataPX + 0., .y = sourceDataPY + 1., .z = values[0][1]});
      f8vector v11 = f8vector({.x = sourceDataPX + 1., .y = sourceDataPY + 1., .z = values[1][1]});
      f8vector v21 = f8vector({.x = sourceDataPX + 2., .y = sourceDataPY + 1., .z = values[2][1]});
      f8vector v02 = f8vector({.x = sourceDataPX + 0., .y = sourceDataPY + 2., .z = values[0][2]});
      f8vector v12 = f8vector({.x = sourceDataPX + 1., .y = sourceDataPY + 2., .z = values[1][2]});

      if (x >= 0 && y >= 0 && x < (int)drawFunctionState.width && y < (int)drawFunctionState.height) {
        f8vector normal00 = cross(v10 - v00, v01 - v00).norm();
        f8vector normal10 = cross(v20 - v10, v11 - v10).norm();
        f8vector normal01 = cross(v11 - v01, v02 - v01).norm();
        f8vector normal11 = cross(v21 - v11, v12 - v11).norm();
        f8vector lightSource = (f8vector({.x = -1., .y = -1., .z = -1.})).norm(); /* TODO make light source configurable */
        float c00 = dot(lightSource, normal00);
        float c10 = dot(lightSource, normal10);
        float c01 = dot(lightSource, normal01);
        float c11 = dot(lightSource, normal11);
        float dx = warperState.tileDx;
        float dy = warperState.tileDy;
        float gx1 = (1 - dx) * c00 + dx * c10;
        float gx2 = (1 - dx) * c01 + dx * c11;
        float bilValue = (1 - dy) * gx1 + dy * gx2;
        drawFunctionState.dataField[x + y * drawFunctionState.width] = (bilValue + 1) / 1.816486;
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
