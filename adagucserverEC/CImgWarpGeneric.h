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

struct CImgWarpGenericSettings : GDWDrawFunctionState {
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

  template <class T> static void drawFunction(int x, int y, T val, void *_settings) {
    CImgWarpGenericSettings *drawSettings = static_cast<CImgWarpGenericSettings *>(_settings);
    if (x < 0 || y < 0 || x > drawSettings->width || y > drawSettings->height) return;

    bool isNodata = false;
    if (drawSettings->hasNodataValue) {
      if ((val) == (T)drawSettings->dfNodataValue) isNodata = true;
    }
    if (!(val == val)) isNodata = true;
    if (!isNodata) {
      T *sourceData = (T *)drawSettings->sourceData;
      size_t sourceDataPX = drawSettings->sourceDataPX;
      size_t sourceDataPY = drawSettings->sourceDataPY;
      size_t sourceDataWidth = drawSettings->sourceDataWidth;
      size_t sourceDataHeight = drawSettings->sourceDataHeight;

      if (sourceDataPY > sourceDataHeight - 1) return;
      if (sourceDataPX > sourceDataWidth - 1) return;

      T values[2][2] = {{0, 0}, {0, 0}};

      values[0][0] += ((T *)sourceData)[nfast_mod(sourceDataPX + 0, sourceDataWidth) + nfast_mod(sourceDataPY + 0, sourceDataHeight) * sourceDataWidth];
      values[1][0] += ((T *)sourceData)[nfast_mod(sourceDataPX + 1, sourceDataWidth) + nfast_mod(sourceDataPY + 0, sourceDataHeight) * sourceDataWidth];
      values[0][1] += ((T *)sourceData)[nfast_mod(sourceDataPX + 0, sourceDataWidth) + nfast_mod(sourceDataPY + 1, sourceDataHeight) * sourceDataWidth];
      values[1][1] += ((T *)sourceData)[nfast_mod(sourceDataPX + 1, sourceDataWidth) + nfast_mod(sourceDataPY + 1, sourceDataHeight) * sourceDataWidth];

      if (x >= 0 && y >= 0 && x < (int)drawSettings->width && y < (int)drawSettings->height) {
        float dx = drawSettings->tileDx;
        float dy = drawSettings->tileDy;
        float gx1 = (1 - dx) * values[0][0] + dx * values[1][0];
        float gx2 = (1 - dx) * values[0][1] + dx * values[1][1];
        float bilValue = (1 - dy) * gx1 + dy * gx2;
        drawSettings->dataField[x + y * drawSettings->width] = bilValue;
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
