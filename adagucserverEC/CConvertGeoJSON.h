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

#ifndef CCONVERTGEOJSON_H
#define CCONVERTGEOJSON_H
#include "CDataSource.h"
#include "CGeoJSONData.h"
#include <map>
#include "json.h"
#include "CDebugger.h"

typedef struct {
  double llX;
  double llY;
  double urX;
  double urY;
} BBOX;

class CConvertGeoJSON{
  private:
  DEF_ERRORFUNCTION();
  static void getBBOX(CDFObject *cdfObject, BBOX &bbox, json_value& json, std::vector<Feature*>& features);
  static void getDimensions(CDFObject *cdfObject, json_value& json, bool openAll);
  static void getPolygons(json_value &j);
  static void addCDFInfo(CDFObject *cdfObject, CServerParams *srvParams, BBOX &dfBBOX, std::vector<Feature*>& featureMap, bool openAll);
  static void drawpoly(float *imagedata,int w,int h,int polyCorners,float *polyX,float *polyY,float value);
  static void drawpoly2(float *imagedata,int w,int h,int polyCorners,float *polyXY,float value);
  static void drawpoly2_index(unsigned short  *imagedata,int w,int h,int polyCorners,float *polyXY,unsigned short value);
  static void drawpolyWithHoles(float *imagedata,int w,int h,int polyCorners,float *polyXY,float value,int holes,int *holeCorners,float*holeXY[]);
  static void drawpolyWithHoles_index(int xMin,int yMin, int xMax, int yMax,unsigned short *imagedata,int w,int h,int polyCorners,float *polyXY,unsigned short int value,int holes,int *holeCorners,float *holeXY[]);
  static void drawpolyWithHoles_indexORG(unsigned short *imagedata,int w,int h,int polyCorners,float *polyXY,unsigned short int value,int holes,int *holeCorners,float *holeXY[]);
public: 
  static std::map<std::string, std::vector<Feature *> >  featureStore;
  static void clearFeatureStore();
  static void clearFeatureStore(CT::string name);
  
  static int convertGeoJSONHeader(CDFObject *cdfObject);
  static int convertGeoJSONData(CDataSource *dataSource,int mode);
};
#endif
