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
#include <CServerConfig_CPPXSD.h>
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
    size_t l = strlen(color);

    if (l == 7 && color[0] == '#') {
      r = CSERVER_HEXDIGIT_TO_DEC(color[1]) * 16 + CSERVER_HEXDIGIT_TO_DEC(color[2]);
      g = CSERVER_HEXDIGIT_TO_DEC(color[3]) * 16 + CSERVER_HEXDIGIT_TO_DEC(color[4]);
      b = CSERVER_HEXDIGIT_TO_DEC(color[5]) * 16 + CSERVER_HEXDIGIT_TO_DEC(color[6]);
      a = 255;
    } else if (l == 9 && color[0] == '#') {
      r = CSERVER_HEXDIGIT_TO_DEC(color[1]) * 16 + CSERVER_HEXDIGIT_TO_DEC(color[2]);
      g = CSERVER_HEXDIGIT_TO_DEC(color[3]) * 16 + CSERVER_HEXDIGIT_TO_DEC(color[4]);
      b = CSERVER_HEXDIGIT_TO_DEC(color[5]) * 16 + CSERVER_HEXDIGIT_TO_DEC(color[6]);
      a = CSERVER_HEXDIGIT_TO_DEC(color[7]) * 16 + CSERVER_HEXDIGIT_TO_DEC(color[8]);
    } else {
      r = 0;
      g = 0;
      b = 0;
      a = 255;
    }
  }
};
#endif
