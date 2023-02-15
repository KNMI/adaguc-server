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
#ifndef CColor_H
#define CColor_H
#include <stdlib.h>
class CColor {
public:
  unsigned char r, g, b, a;
  CColor() {
    r = 0;
    g = 0;
    b = 0;
    a = 255;
  }
  CColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = a;
  }
  CColor(const char *color) { parse(color); }
  /**
   * color can have format #RRGGBB or #RRGGBBAA
   */
  void parse(const char *color) {
    if (color[0] == '#') {
      if (strlen(color) == 7) {
        r = ((color[1] > 64) ? color[1] - 55 : color[1] - 48) * 16 + ((color[2] > 64) ? color[2] - 55 : color[2] - 48);
        g = ((color[3] > 64) ? color[3] - 55 : color[3] - 48) * 16 + ((color[4] > 64) ? color[4] - 55 : color[4] - 48);
        b = ((color[5] > 64) ? color[5] - 55 : color[5] - 48) * 16 + ((color[6] > 64) ? color[6] - 55 : color[6] - 48);
        a = 255;
      }
      if (strlen(color) == 9) {
        r = ((color[1] > 64) ? color[1] - 55 : color[1] - 48) * 16 + ((color[2] > 64) ? color[2] - 55 : color[2] - 48);
        g = ((color[3] > 64) ? color[3] - 55 : color[3] - 48) * 16 + ((color[4] > 64) ? color[4] - 55 : color[4] - 48);
        b = ((color[5] > 64) ? color[5] - 55 : color[5] - 48) * 16 + ((color[6] > 64) ? color[6] - 55 : color[6] - 48);
        a = ((color[7] > 64) ? color[7] - 55 : color[7] - 48) * 16 + ((color[8] > 64) ? color[8] - 55 : color[8] - 48);
      }
    }
  }
};
#endif
