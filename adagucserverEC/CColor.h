/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl, GST - GeoSpatialTeam KNMI
 * Date:     2026-09-10
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
#include <string>

#ifndef CCOLOR_H
#define CCOLOR_H
struct CColor {

  unsigned char r = 0, g = 0, b = 0, a = 255;
  CColor();
  CColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
  CColor(const char *color);

  CColor &operator=(const char *color);

  CColor(const std::string &color);

  CColor &operator=(const std::string &color);

  std::string c_str();
  void parse(const std::string &color);
};
#endif
