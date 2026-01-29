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

#ifndef CRequest_H
#define CRequest_H

#include "CDataSource.h"
#include "CImageDataWriter.h"
#include "CServerParams.h"
#include "CServerConfig_CPPXSD.h"

class CRequest {
private:
  std::vector<CDataSource *> dataSources;
  CT::string Version;
  CT::string Exceptions;
  CServerParams *srvParam;
  std::vector<CT::string *> queryDims;
  DEF_ERRORFUNCTION();
  int generateOGCGetCapabilities(CT::string *XMLdocument);
  int generateGetReferenceTimes(CDataSource *dataSource);
  int generateGetReferenceTimesDoc(CT::string *result, CDataSource *dataSource);
  int generateOGCDescribeCoverage(CT::string *XMLdocument);
  void autoDetectBBOX();
  int addDataSources(CServerConfig::XMLE_Layer *cfgLayer, int layerIndex);
  int determineTypesForDataSources();
  int handleGetMapRequest(CDataSource *firstDataSource);
  int handleGetCoverageRequest(CDataSource *firstDataSource);

public:
  static int setDimValuesForDataSource(CDataSource *dataSource, CServerParams *srvParam);
  static int queryDimValuesForDataSource(CDataSource *dataSource, CServerParams *srvParam);
  static int fillDimValuesForDataSource(CDataSource *dataSource, CServerParams *srvParam);

  CRequest() { srvParam = new CServerParams(); }
  ~CRequest() {
    for (size_t j = 0; j < dataSources.size(); j++) {
      if (dataSources[j] != NULL) {
        delete dataSources[j];
        dataSources[j] = NULL;
      }
    }
    for (size_t j = 0; j < queryDims.size(); j++) {
      delete queryDims[j];
      queryDims[j] = NULL;
    }
    delete srvParam;
  }
  static int CGI;
  int process_querystring();
  int setConfigFile(const char *pszConfigFile);
  int process_wms_getcap_request();
  int process_wms_getmap_request();
  int process_wms_getmetadata_request();
  int process_wms_getfeatureinfo_request();
  int process_wms_getlegendgraphic_request();
  int process_wcs_getcap_request();
  int process_wcs_describecov_request();
  int process_wcs_getcoverage_request();
  int process_all_layers();
  int process_wms_getreferencetimes_request();
  int process_wms_gethistogram_request();
  int updatedb(CT::string tailPath, CT::string layerPathToScan, int scanFlags, CT::string layerName);

  int runRequest();

  CServerParams *getServerParams();
};

class CImageDataWriter_addData_args {
public:
  CImageDataWriter *imageDataWriter;
  std::vector<CDataSource *> dataSources;
  int status;
  bool running;
  bool finished;
  bool used;
};

void *CImageDataWriter_addData(void *arg);

#endif
