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
#include "./GenericDataWarper/gdwDrawFunction.h"

static inline int nfast_mod(const int input, const int ceil) { return input >= ceil ? input % ceil : input; }

template <typename T> void imgWarpGenericDrawFunction(int x, int y, T val, GDWState &warperState, GDWDrawFunctionSettings &settings);

class CImgWarpGeneric : public CImageWarperRenderInterface {
private:
  DEF_ERRORFUNCTION();

public:
  CImgWarpGeneric() {}
  ~CImgWarpGeneric() {}
  void render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage) override;
  int set(const char *pszSettings) override;
};

#endif
