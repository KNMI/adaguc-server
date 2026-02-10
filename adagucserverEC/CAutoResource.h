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

#ifndef CAUTORESOURCE_H
#define CAUTORESOURCE_H
#include "CServerParams.h"
#include "CDebugger.h"
#include "CCDFDataModel.h"
/**
 * Configures new layers in the configuration automatically based on srvParam->autoResourceLocation and srvParam->datasetLocation.
 */
class CAutoResource {
private:
  /**
   * Adds new layers to the configuration
   * @param srvParam
   * @param cdfObject
   * @param variableNames
   * @param group
   * @param location
   */
  static void addXMLLayerToConfig(CServerParams *const srvParam, CDFObject *cdfObject, std::vector<CT::string> *variableNames, const char *group, const char *location);

  /**
   * configures Source, based on  srvParam->autoResourceLocation parameter.
   * Adds new layers to the configuration object in serverparams.
   * @param srvParam The serverparams
   * returns 0 on success.
   */
  static int configureAutoResource(CServerParams *srvParam, bool plain);

  /**
   * Sets the server title, will be advertised in de WMS getcapabilities
   * @param srvParam
   * @param setServerTitle
   */
  static int setServerTitle(CServerParams *srvParams, CT::string serverTitle);

public:
  /**
   * configures Dataset, based on  srvParam->datasetLocation parameter. Used for KDC inspire
   * Adds new layers to the configuration object in serverparams.
   * @param srvParam The serverparams
   * returns 0 on success.
   */
  static int configureDataset(CServerParams *srvParam, bool plain);

  /**
   * Configures new layers in the configuration automatically based on srvParam->autoResourceLocation and srvParam->datasetLocation.
   * Usually based on &SOURCE= and &DATASET= key value pairs in WMS urls.
   * Modifies the configuration object in serverparams.
   * @param srvParam The serverparams
   * @param plain Serve data as plain as possible, when set to true. no point/swath/curvilinear transformation is applied
   * returns 0 on success.
   */
  static int configure(CServerParams *srvParam, bool plain);
};
#endif
