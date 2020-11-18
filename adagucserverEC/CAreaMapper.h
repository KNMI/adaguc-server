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

#ifndef CAREAMAPPER
#define CAREAMAPPER

#include "CDebugger.h"
#include "CDataSource.h"
#include "CDrawImage.h"
#include <stdio.h>

class CAreaMapper{
private:
  DEF_ERRORFUNCTION();
  double dfTileWidth,dfTileHeight;
  double dfSourceBBOX[4];
  double dfImageBBOX[4];
  double dfNodataValue;
  double legendLowerRange;
  double legendUpperRange;
  double legendValueRange;
  double hasNodataValue;
  int width,height;
  int internalWidth,internalHeight;
  float legendLog,legendScale,legendOffset;
  float legendLogAsLog;
  CDataSource * dataSource;
  CDrawImage *drawImage;
  bool debug;;
  template<typename T>
  int myDrawRawTile(const T *data,CDFType dataType, double *x_corners,double *y_corners,int &dDestX,int &dDestY);
public:
  int drawTile(double *x_corners,double *y_corners,int &dDestX,int &dDestY);
  void init(CDataSource *dataSource,CDrawImage *drawImage,int tileWidth,int tileHeight);
};

#endif
