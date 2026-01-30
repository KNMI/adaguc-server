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
#ifndef CRectangleText_H
#define CRectangleText_H
#include "CDebugger.h"
#include "CTString.h"

#include "Definitions.h"
#include "CColor.h"

class CRectangleText {
public:
  int llx;
  int lly;
  int urx;
  int ury;
  float angle;
  int padding;
  CT::string text;
  CT::string fontFile;
  float fontSize;
  CColor color;

  DEF_ERRORFUNCTION();
  CRectangleText() {}

  void init(int llx, int lly, int urx, int ury, float angle, int padding, const char *text, const char *fontFile, float fontSize, CColor color) {
    this->llx = llx;
    this->lly = lly;
    this->urx = urx;
    this->ury = ury;
    this->angle = angle;
    this->padding = padding;
    this->text = CT::string(text);
    this->fontFile = CT::string(fontFile);
    this->fontSize = fontSize;
    this->color = color;
  }
  bool overlaps(CRectangleText &r1);
};
#endif
