/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2022-06-30
 *
 ******************************************************************************
 *
 * Copyright 2022, Royal Netherlands Meteorological Institute (KNMI)
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
#ifndef CCOLOR_H
#define CCOLOR_H

#include <stdio.h>
#include <string.h>
#include "CTString.h"

#define CSERVER_HEXDIGIT_TO_DEC(DIGIT) (DIGIT > 96 ? DIGIT - 87 : DIGIT > 64 ? DIGIT - 55 : DIGIT - 48) // Converts "9" to 9, "A" to 10 and "a" to 10

class CColor {
public:
  unsigned char r, g, b, a;
  CColor();
  CColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
  CColor(const char *color);
  CColor(CT::string &color);
  CT::string c_str();
  void parse(const char *color);
};
#endif
