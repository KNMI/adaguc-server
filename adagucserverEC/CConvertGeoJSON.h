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
#include "CImageWarper.h"
#include "json.h"
#include "CDebugger.h"

class CConvertGeoJSON {
public:
  typedef struct {
    double llX;
    double llY;
    double urX;
    double urY;
  } BBOX;

private:
  static void getBBOX(CDFObject *cdfObject, BBOX &bbox, json_value &json, std::vector<Feature *> &features);
  static void getDimensions(CDFObject *cdfObject, json_value &json, bool openAll);
  static void getPolygons(json_value &j);
  static void addCDFInfo(CDFObject *cdfObject, CServerParams *srvParams, BBOX &dfBBOX, std::vector<Feature *> &featureMap, bool openAll);

  static void drawDot(int px, int py, unsigned short v, int W, int H, unsigned short *grid);
  static void drawPolygons(Feature *feature, unsigned short int featureIndex, CDataSource *dataSource, bool projectionRequired, CImageWarper *imageWarper, double cellSizeX, double cellSizeY,
                           double offsetX, double offsetY);
  static void drawPoints(Feature *feature, unsigned short int featureIndex, CDataSource *dataSource, bool projectionRequired, CImageWarper *imageWarper, double cellSizeX, double cellSizeY,
                         double offsetX, double offsetY, float &min, float &max);

public:
  static std::map<std::string, std::vector<Feature *>> featureStore;
  static void clearFeatureStore();
  static void clearFeatureStore(CT::string name);

  static int convertGeoJSONHeader(CDFObject *cdfObject);
  static int convertGeoJSONData(CDataSource *dataSource, int mode);
  static int addPropertyVariables(CDFObject *cdfObject, std::vector<Feature *> features);
};
#endif
