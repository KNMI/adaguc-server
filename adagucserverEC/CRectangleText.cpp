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
#include "CRectangleText.h"
#include "CDebugger.h"
#include "Definitions.h"

void CRectangleText::init(int llx, int lly, int urx, int ury, float angle, int padding, const char *text, const char *fontFile, float fontSize, CColor color) {
  this->llx = llx;
  this->lly = lly;
  this->urx = urx;
  this->ury = ury;
  this->angle = angle;
  this->padding = padding;
  this->text = std::string(text);
  this->fontFile = std::string(fontFile);
  this->fontSize = fontSize;
  this->color = color;
}

bool CRectangleText::overlaps(CRectangleText &r2) {
  if ((this->ury + padding < r2.lly) || (this->lly - padding > r2.ury)) return false;
  if ((this->urx + padding < r2.llx) || (this->llx - padding > r2.urx)) return false;
  return true;
}
