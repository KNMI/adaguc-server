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
#include <stdio.h>
#include <string.h>

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

#define CXMLGEN_FATAL_ERROR_OCCURED 1
#define CXML_NON_FATAL_ERRORS_OCCURED 100

class WMSLayer {
public:
  class Dim {
  public:
    CT::string name;
    CT::string units;
    CT::string values;
    CT::string defaultValue;
    int hasMultipleValues;
    bool hidden = false;
  };
  class Projection {
  public:
    CT::string name;
    double dfBBOX[4];
  };
  class Style {
  public:
    CT::string name;
    CT::string title;
    CT::string abstract;
  };
  WMSLayer() {
    isQuerable = 0;
    hasError = 0;
    dataSource = NULL;
    dfLatLonBBOX[0] = -180;
    dfLatLonBBOX[1] = -90;
    dfLatLonBBOX[2] = 180;
    dfLatLonBBOX[3] = 90;
    abstract = "";
  }
  ~WMSLayer() {
    delete dataSource;
    dataSource = NULL;
    for (size_t j = 0; j < projectionList.size(); j++) {
      delete projectionList[j];
      projectionList[j] = NULL;
    }
    for (size_t j = 0; j < dimList.size(); j++) {
      delete dimList[j];
      dimList[j] = NULL;
    }
    for (size_t j = 0; j < styleList.size(); j++) {
      delete styleList[j];
      styleList[j] = NULL;
    }
  }
  CServerConfig::XMLE_Layer *layer;
  CT::string name, title, group, abstract, fileName;
  int isQuerable, hasError;
  CDataSource *dataSource;
  std::vector<Projection *> projectionList;
  std::vector<Dim *> dimList;
  std::vector<Style *> styleList;
  double dfLatLonBBOX[4];
};

class CXMLGen {
private:
  DEF_ERRORFUNCTION();
  int getFileNameForLayer(WMSLayer *myWMSLayer);
  int getDataSourceForLayer(WMSLayer *myWMSLayer);
  int getProjectionInformationForLayer(WMSLayer *myWMSLayer);
  int getDimsForLayer(WMSLayer *myWMSLayer);
  int getWMS_1_0_0_Capabilities(CT::string *XMLDoc, std::vector<WMSLayer *> *myWMSLayerList);
  int getWMS_1_1_1_Capabilities(CT::string *XMLDoc, std::vector<WMSLayer *> *myWMSLayerList);
  int getWMS_1_3_0_Capabilities(CT::string *XMLDoc, std::vector<WMSLayer *> *myWMSLayerList);
  int getWCS_1_0_0_Capabilities(CT::string *XMLDoc, std::vector<WMSLayer *> *myWMSLayerList);
  int getWCS_1_0_0_DescribeCoverage(CT::string *XMLDoc, std::vector<WMSLayer *> *myWMSLayerList);
  int getStylesForLayer(WMSLayer *myWMSLayer);
  void generateRangeSet(CT::string *XMLDoc, WMSLayer *layer);
  // int getStylesForLayer2(WMSLayer * myWMSLayer);
  CServerParams *srvParam;
  CT::string serviceInfo;

public:
  int OGCGetCapabilities(CServerParams *srvParam, CT::string *XMLDocument);
  int WCSDescribeCoverage(CServerParams *srvParam, CT::string *XMLDocument);
  int replace(const char *keyword, const char *replace, char *buf);
  int replace_one(const char *keyword, const char *replace, char *buf);
  // CT::string *TimePositions;

  CXMLGen() {
    // TimePositions=NULL;
  }
  ~CXMLGen() {
    // if(TimePositions!=NULL)delete[] TimePositions;
    // TimePositions=NULL;
  }
};

class CFile {
private:
  DEF_ERRORFUNCTION();
  size_t getFileSize(const char *pszFileName) {
    FILE *fp = fopen(pszFileName, "r");
    if (fp == NULL) {
      CDBError("File not found:[%s]", pszFileName);
      return 0;
    }
    fseek(fp, 0L, SEEK_END);
    long endPos = ftell(fp);
    fclose(fp);
    return endPos;
  }

  int openFileToBuf(const char *pszFileName, char *buf, size_t length) {
    FILE *fp = fopen(pszFileName, "r");
    if (fp == NULL) {
      CDBError("File not found:[%s]", pszFileName);
      return 1;
    }
    size_t result = fread(buf, 1, length, fp);
    if (result != length) {
      // CDBError("openFileToBuf: Number of bytes read != requested");
      return 1;
    }
    fclose(fp);
    return 0;
  }

public:
  char *data;
  size_t size;

  CFile() { data = NULL; }
  ~CFile() {
    if (data != NULL) delete[] data;
  }
  int open(const char *pszFileName) {
    size = getFileSize(pszFileName);

    if (size == 0) {
      data = new char[size + 1];
      data[size] = '\0';
      return 1;
    }
    if (data != NULL) delete[] data;
    data = new char[size + 1];
    int status = openFileToBuf(pszFileName, data, size);
    data[size] = '\0';
    return status;
  }
  int open(const char *path, const char *pszFileName) {
    size_t l = 0;
    l = strlen(path) + strlen(pszFileName) + 10;
    char szTemp[l + 1];
    snprintf(szTemp, l, "%s/%s", path, pszFileName);
    return open(szTemp);
  }
};

#endif
