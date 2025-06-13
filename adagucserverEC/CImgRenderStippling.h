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

#ifndef CImgRenderStippling_H
#define CImgRenderStippling_H
#include <float.h>
#include <pthread.h>
#include "CImageWarperRenderInterface.h"
#include "CGenericDataWarper.h"

#define CImgRenderStipplingModeDefault 0
#define CImgRenderStipplingModeThreshold 1

/**
 *  This is the main class of this file. It renders the sourcedata on the destination image using nearest neighbour interpolation.
 *  It uses tile blocks to render the data. Only the corners of these tiles are projected. Not each source pixel is projected,
 *  because this is uneccessary time consuming for a visualization.
 */
class CImgRenderStippling : public CImageWarperRenderInterface {
private:
  DEF_ERRORFUNCTION();

  int xDistance, yDistance, discSize, mode;
  CT::string color;
  CStyleConfiguration *styleConfiguration;
  CDataSource *dataSource;
  CDrawImage *drawImage;
  void *sourceData;
  CImageWarper *warper;

  void _setStippling(int screenX, int screenY, float val);
  template <typename T> void _stippleMiddleOfPoints();
  class Settings {
  public:
    double dfNodataValue;
    double legendValueRange;
    double legendLowerRange;
    double legendUpperRange;
    bool hasNodataValue;
    float *dataField;
    size_t width, height;
  };
  Settings *settings;

  template <class T> static void drawFunction(int x, int y, T val, void *_settings) {
    Settings *settings = (Settings *)_settings;
    bool isNodata = false;
    if (settings->hasNodataValue) {
      if ((val) == (T)settings->dfNodataValue) isNodata = true;
    }
    if (!(val == val)) isNodata = true;
    if (!isNodata)
      if (settings->legendValueRange)
        if (val < settings->legendLowerRange || val > settings->legendUpperRange) isNodata = true;
    if (!isNodata) {
      if (x >= 0 && y >= 0 && x < (int)settings->width && y < (int)settings->height) {
        settings->dataField[x + y * settings->width] = (float)val;
      }
    }
  };

  int set(const char *settings);

  void render(CImageWarper *warper, CDataSource *dataSource, CDrawImage *drawImage);
};
#endif
