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

#ifndef CXMLGen_H
#define CXMLGen_H
#include "CServerParams.h"
#include <cstdio>
#include <cstring>

#include "CInspire.h"

#include "CImageDataWriter.h"
#include "CServerError.h"
#include "CDataReader.h"
#include "CImageWarper.h"
#include "CDrawImage.h"
#include "CDataSource.h"
#include "CRequest.h"
#include "CDebugger.h"
#include "CStyleConfiguration.h"
#include "./Types/LayerMetadataType.h"

#define CXMLGEN_FATAL_ERROR_OCCURED 1
#define CXML_NON_FATAL_ERRORS_OCCURED 100

class CXMLGen {
private:
  DEF_ERRORFUNCTION();
  int getWMS_1_0_0_Capabilities(CT::string *XMLDoc, std::vector<MetadataLayer *> *metadataLayerList);
  int getWMS_1_1_1_Capabilities(CT::string *XMLDoc, std::vector<MetadataLayer *> *metadataLayerList);
  int getWMS_1_3_0_Capabilities(CT::string *XMLDoc, std::vector<MetadataLayer *> *metadataLayerList);
  int getWCS_1_0_0_Capabilities(CT::string *XMLDoc, std::vector<MetadataLayer *> *metadataLayerList);
  int getWCS_1_0_0_DescribeCoverage(CT::string *XMLDoc, std::vector<MetadataLayer *> *metadataLayerList);
  CServerParams *srvParam;
  CT::string serviceInfo;

public:
  int OGCGetCapabilities(CServerParams *srvParam, CT::string *XMLDocument);
  int WCSDescribeCoverage(CServerParams *srvParam, CT::string *XMLDocument);
};

#endif
