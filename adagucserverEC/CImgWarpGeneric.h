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

#ifndef CImgWarpGeneric_H
#define CImgWarpGeneric_H
#include <stdlib.h>

#include "CImageWarperRenderInterface.h"
#include "CGenericDataWarper.h"

static inline int nfast_mod(const int input, const int ceil) { return input >= ceil ? input % ceil : input; }

struct CImgWarpGenericDrawFunctionState : GDWDrawFunctionState {
  double dfNodataValue;
  double legendValueRange;
  double legendLowerRange;
  double legendUpperRange;
  bool hasNodataValue;
  float *dataField;
  int width, height;
};

class CImgWarpGeneric : public CImageWarperRenderInterface {
private:
  DEF_ERRORFUNCTION();

  template <class T> static void drawFunction(int x, int y, T val, void *_drawFunctionState) {
    CImgWarpGenericDrawFunctionState *drawFunctionState = static_cast<CImgWarpGenericDrawFunctionState *>(_drawFunctionState);
    if (x < 0 || y < 0 || x > drawFunctionState->width || y > drawFunctionState->height) return;

    bool isNodata = false;
    if (drawFunctionState->hasNodataValue) {
      if ((val) == (T)drawFunctionState->dfNodataValue) isNodata = true;
    }
    if (!(val == val)) isNodata = true;
    if (!isNodata) {
      T *sourceData = (T *)drawFunctionState->sourceData;
      size_t sourceDataPX = drawFunctionState->sourceDataPX;
      size_t sourceDataPY = drawFunctionState->sourceDataPY;
      size_t sourceDataWidth = drawFunctionState->sourceDataWidth;
      size_t sourceDataHeight = drawFunctionState->sourceDataHeight;

      if (sourceDataPY > sourceDataHeight - 1) return;
      if (sourceDataPX > sourceDataWidth - 1) return;

      T values[2][2] = {{0, 0}, {0, 0}};

      values[0][0] += ((T *)sourceData)[nfast_mod(sourceDataPX + 0, sourceDataWidth) + nfast_mod(sourceDataPY + 0, sourceDataHeight) * sourceDataWidth];
      values[1][0] += ((T *)sourceData)[nfast_mod(sourceDataPX + 1, sourceDataWidth) + nfast_mod(sourceDataPY + 0, sourceDataHeight) * sourceDataWidth];
      values[0][1] += ((T *)sourceData)[nfast_mod(sourceDataPX + 0, sourceDataWidth) + nfast_mod(sourceDataPY + 1, sourceDataHeight) * sourceDataWidth];
      values[1][1] += ((T *)sourceData)[nfast_mod(sourceDataPX + 1, sourceDataWidth) + nfast_mod(sourceDataPY + 1, sourceDataHeight) * sourceDataWidth];

      if (x >= 0 && y >= 0 && x < (int)drawFunctionState->width && y < (int)drawFunctionState->height) {
        float dx = drawFunctionState->tileDx;
        float dy = drawFunctionState->tileDy;
        float gx1 = (1 - dx) * values[0][0] + dx * values[1][0];
        float gx2 = (1 - dx) * values[0][1] + dx * values[1][1];
        float bilValue = (1 - dy) * gx1 + dy * gx2;
        drawFunctionState->dataField[x + y * drawFunctionState->width] = bilValue;
      }
    }
  };

public:
  CImgWarpGeneric() {}
  ~CImgWarpGeneric() {}
  void render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) override;
  int set(const char *pszSettings) override;
};

#endif
