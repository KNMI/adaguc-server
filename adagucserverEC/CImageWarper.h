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

#ifndef CImageWarper_H
#define CImageWarper_H
#include "CServerParams.h"
#include "CDataReader.h"
#include "CDrawImage.h"
#include <proj.h>
#include <math.h>
#include "CDebugger.h"
#include "CStopWatch.h"

#define LATLONPROJECTION "+proj=longlat +ellps=WGS84 +datum=WGS84 +no_defs"
void floatToString(char *string, size_t maxlen, float number);
void floatToString(char *string, size_t maxlen, int numdigits, float number);
void floatToString(char *string, size_t maxlen, float min, float max, float number);

class CImageWarper {
  //  CNetCDFReader reader;
private:
  double dfMaxExtent[4];
  int dMaxExtentDefined;

  DEF_ERRORFUNCTION();
  CGeoParams *_geoDest;
  CT::string sourceCRSString;
  CT::string destinationCRS;
  //     int _decodeCRS(CT::string *CRS);
  std::vector<CServerConfig::XMLE_Projection *> *prj;
  bool initialized;

  int _initreprojSynchronized(const char *projString, CGeoParams *GeoDest, std::vector<CServerConfig::XMLE_Projection *> *_prj);
  int findExtentUnSynchronized(CDataSource *dataSource, double *dfBBOX);

public:
  bool requireReprojection;
  CImageWarper() {
    prj = NULL;
    projSourceToDest = nullptr;
    projSourceToLatlon = nullptr;
    projLatlonToDest = nullptr;
    initialized = false;
  }
  ~CImageWarper() {
    if (initialized == true) {
      closereproj();
      prj = NULL;
      projSourceToDest = nullptr;
      projSourceToLatlon = nullptr;
      projLatlonToDest = nullptr;
      initialized = false;
    }
  }
  PJ *projSourceToDest, *projSourceToLatlon, *projLatlonToDest;
  CT::string getDestProjString() { return destinationCRS; }
  int initreproj(CDataSource *dataSource, CGeoParams *GeoDest, std::vector<CServerConfig::XMLE_Projection *> *prj);
  int initreproj(const char *projString, CGeoParams *GeoDest, std::vector<CServerConfig::XMLE_Projection *> *_prj);

  int closereproj();
  int reprojpoint(double &dfx, double &dfy);
  int reprojpoint(f8point &p);
  int reprojpoint_inv(double &dfx, double &dfy);

  /**
   * Reprojects a point to screen pixel coordinates
   * Same as reprojpoint_inv, but reprojects a point to screen pixel coordinates (WMS Map coordinates).
   * It uses the BBOX and WIDTH/HEIGHT for this.
   */
  int reprojpoint_inv_topx(double &dfx, double &dfy);
  int reprojpoint_inv(f8point &p);
  int reprojModelToLatLon(double &dfx, double &dfy);
  int reprojModelFromLatLon(double &dfx, double &dfy);
  int reprojModelToLatLon(f8point &point);
  void reprojBBOX(double *df4PixelExtent);
  int reprojfromLatLon(double &dfx, double &dfy);
  int reprojToLatLon(double &dfx, double &dfy);
  // int decodeCRS(CT::string *outputCRS, CT::string *inputCRS);
  int decodeCRS(CT::string *outputCRS, CT::string *inputCRS, std::vector<CServerConfig::XMLE_Projection *> *prj);
  int findExtent(CDataSource *dataSource, double *dfBBOX);
  bool isProjectionRequired() { return requireReprojection; }
  /**
   * @brief Get the Proj4 From EPSG code
   *
   * @param dataSource DataSource
   * @param projectionId EPSG code, or projection string, or string "native"
   * @return CT::string Will return proj4 parameters for given projection id
   */
  static CT::string getProj4FromId(CDataSource *dataSource, CT::string projectionId);

  /**
   * Returns the corrected projection string and a factor with witch the x and y axis of the data need to be scaled.
   * Needed for a conversion for KM to Meter for example
   */
  static std::tuple<CT::string, double> fixProjection(CT::string projectionString);
};

#endif
