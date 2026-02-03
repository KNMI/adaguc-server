/******************************************************************************
 *
 * Project:  Proj4ToCF
 * Purpose:  Functions to convert proj4 strings to CF projection descriptions and vice versa
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

#ifndef CPROJ4TOCF_H
#define CPROJ4TOCF_H

#include <cstdio>
#include <iostream>
#include <vector>
#include <sys/stat.h>
#include <CTString.h>
#include "CCDFDataModel.h"
#include "CKeyValuePair.h"
/*******************************/
/*  CF functions               */
/*******************************/

#define CPROJ4TOCF_UNSUPPORTED_PROJECTION 1 /*Projection is not supported*/

class CProj4ToCF {
private:
  DEF_ERRORFUNCTION();

  static float convertToM(float fValue);

  CT::string getProj4Value(const char *proj4Key, std::vector<CKeyValuePair> projKVPList);

  float getProj4ValueF(const char *proj4Key, std::vector<CKeyValuePair> projKVPList, float defaultValue, float((*conversionfunction)(float)));

  float getProj4ValueF(const char *proj4Key, std::vector<CKeyValuePair> projKVPList, float defaultValue);

  void initMSGPerspective(CDF::Variable *projectionVariable, std::vector<CKeyValuePair> projKVPList);
  void initStereoGraphic(CDF::Variable *projectionVariable, std::vector<CKeyValuePair> projKVPList);
  void initLCCPerspective(CDF::Variable *projectionVariable, std::vector<CKeyValuePair> projKVPList);
  void initRPPerspective(CDF::Variable *projectionVariable, std::vector<CKeyValuePair> projKVPList);
  void initObliqueStereographicPerspective(CDF::Variable *projectionVariable, std::vector<CKeyValuePair> projKVPList);
  void initLatitudeLongitude(CDF::Variable *projectionVariable, std::vector<CKeyValuePair> projKVPList);
  void initMercator(CDF::Variable *projectionVariable, std::vector<CKeyValuePair> projKVPList);
  void initTransverseMercator(CDF::Variable *projectionVariable, std::vector<CKeyValuePair> projKVPList);
  void initLAEAPerspective(CDF::Variable *projectionVariable, std::vector<CKeyValuePair> projKVPList);
  void initGeosPerspective(CDF::Variable *projectionVariable, std::vector<CKeyValuePair> projKVPList);
  void initAEQDPerspective(CDF::Variable *projectionVariable, std::vector<CKeyValuePair> projKVPList);

  int convertBackAndFort(const char *projString, CDF::Variable *projectionVariable);

public:
  CProj4ToCF();
  ~CProj4ToCF();

  /**
   * Set to true and additonal debug attributes will be written to the CDF Variable.
   */
  bool debug;

  /**
   * Converts a proj4 string to CF mappings
   * @param projectionVariable The variable which will contain attributes with projection information after the conversion has finished
   * @param proj4String The proj4 string which holds the string to convert to CF mapping
   * @return Zero on succes and nonzero on failure
   */
  int convertProjToCF(CDF::Variable *projectionVariable, const char *proj4String);
  /**
   * Converts a CF projection variable to a proj4 stringtring to CF mappings
   * @param projectionVariable The variable with CF projection attributes to convert to the proj4 string
   * @return The proj4 string which will contain the new proj string after conversion is finished or empty if an error occured
   */
  CT::string convertCFToProj(CDF::Variable *projectionVariable);

  int __checkProjString(const char *name, const char *string);
  /**
   * Tests CProj4ToCF with several projections
   * @return Zero on success, nonzero on failure
   */
  int unitTest();
};

#endif
