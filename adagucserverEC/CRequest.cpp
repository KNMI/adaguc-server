
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

// #define CREQUEST_DEBUG
// #define MEASURETIME

#include "Types/ProjectionStore.h"
#include "CRequest.h"
#include "COpenDAPHandler.h"
#include "CDBFactory.h"
#include "CAutoResource.h"
#include "CNetCDFDataWriter.h"
#include "CConvertGeoJSON.h"
#include "CCreateScaleBar.h"
#include "CSLD.h"
#include "CHandleMetadata.h"
#include "CCreateTiles.h"
#include "LayerTypeLiveUpdate/LayerTypeLiveUpdate.h"
#include <CReadFile.h>
#include "Definitions.h"
#include "utils/LayerUtils.h"
#include "utils/XMLGenUtils.h"
#include "utils/LayerMetadataStore.h"
#include <json_adaguc.h>
#include "utils/LayerMetadataToJson.h"
#include "utils/CRequestUtils.h"
#include "handleTileRequest.h"
#include <traceTimings/traceTimings.h>
#include "utils/serverutils.h"
#include "CCreateHistogram.h"
#ifdef ADAGUC_USE_GDAL
#include "CGDALDataWriter.h"
#endif

int CRequest::CGI = 0;

// Entry point for all runs
int CRequest::runRequest() {
  int status = process_querystring();
  CDFObjectStore::getCDFObjectStore()->clear();
  CConvertGeoJSON::clearFeatureStore();
  CDFStore::clear();
  CDBFactory::clear();
  return status;
}

void writeLogFile3(const char *msg) {
  char *logfile = getenv("ADAGUC_LOGFILE");
  if (logfile != NULL) {
    FILE *pFile = NULL;
    pFile = fopen(logfile, "a");
    if (pFile != NULL) {
      fputs(msg, pFile);
      if (strncmp(msg, "[D:", 3) == 0 || strncmp(msg, "[W:", 3) == 0 || strncmp(msg, "[E:", 3) == 0) {
        time_t myTime = time(NULL);
        tm *myUsableTime = localtime(&myTime);
        char szTemp[128];
        snprintf(szTemp, 127, "%.4d-%.2d-%.2dT%.2d:%.2d:%.2dZ ", myUsableTime->tm_year + 1900, myUsableTime->tm_mon + 1, myUsableTime->tm_mday, myUsableTime->tm_hour, myUsableTime->tm_min,
                 myUsableTime->tm_sec);
        fputs(szTemp, pFile);
      }
      fclose(pFile);
    } // else CDBError("Unable to write logfile %s",logfile);
  }
}

int CRequest::setConfigFile(const char *pszConfigFile) {
  if (pszConfigFile == NULL) {
    CDBError("No config file set");
    return 1;
  }
#ifdef MEASURETIME
  StopWatch_Stop("Set config file %s", pszConfigFile);
#endif

  CT::string configFile = pszConfigFile;
  std::vector<CT::string> configFileList = configFile.split(",");

  // Parse the main configuration file
  int status = srvParam->parseConfigFile(configFileList[0]);

  if (status == 0 && srvParam->configObj->Configuration.size() == 1) {

    srvParam->configFileName.copy(pszConfigFile);
    srvParam->cfg = srvParam->configObj->Configuration[0];

    // Include additional config files given as argument
    if (configFileList.size() > 1) {
      for (size_t j = 1; j < configFileList.size() - 1; j++) {
        // CDBDebug("Include '%s'", configFileList[j].c_str());
        status = srvParam->parseConfigFile(configFileList[j]);
        if (status != 0) {
          CDBError("There is an error with include '%s'", configFileList[j].c_str());
          return 1;
        }
      }

      // The last configration file is considered the dataset one, strip path and extension and give it to configurer
      if (configFileList.size() > 1) {
        srvParam->datasetLocation.copy(CT::basename(configFileList[configFileList.size() - 1]).c_str());
        srvParam->datasetLocation.substringSelf(0, srvParam->datasetLocation.lastIndexOf("."));
        if (srvParam->verbose) {
          CDBDebug("Dataset name based on passed configfile is [%s]", srvParam->datasetLocation.c_str());
        }

        status = CAutoResource::configureDataset(srvParam, false);
        if (status != 0) {
          CDBError("ConfigureDataset failed for %s", configFileList[1].c_str());
          return status;
        }
      }
    }

    const char *pszQueryString = getenv("QUERY_STRING");
    if (pszQueryString != NULL) {
      CT::string queryString(pszQueryString);
      queryString.decodeURLSelf();
      auto parameters = queryString.split("&");
      for (size_t j = 0; j < parameters.size(); j++) {
        CT::string value0Cap;
        CT::string values[2];
        int equalPos = parameters[j].indexOf("="); // split("=");
        if (equalPos != -1) {
          values[0] = parameters[j].substring(0, equalPos);
          values[1] = parameters[j].c_str() + equalPos + 1;
        } else {
          values[0] = parameters[j].c_str();
          values[1] = "";
        }
        value0Cap.copy(&values[0]);
        value0Cap.toUpperCaseSelf();
        if (value0Cap.equals("DATASET")) {
          if (srvParam->datasetLocation.empty()) {

            srvParam->datasetLocation.copy(values[1].c_str());
            status = CAutoResource::configureDataset(srvParam, false);
            if (status != 0) {
              CDBError("CAutoResource::configureDataset failed");
              return status;
            }
          }
        }

        // Check if parameter name is a SLD parameter AND have file name
        CSLD csld;
        if (csld.parameterIsSld(values[0])) {
#ifdef CREQUEST_DEBUG
          CDBDebug("Found SLD parameter in query");
#endif

          // Set server params
          csld.setServerParams(srvParam);

          // Process the SLD URL
          if (values[1].empty()) {
            setStatusCode(HTTP_STATUSCODE_404_NOT_FOUND);
            return 1;
          }
          status = csld.processSLDUrl(values[1]);

          if (status != 0) {
            CDBError("Processing SLD failed");
            return status;
          }
        }
      }
    }

    // Include additional config files given in the include statement of the config file
    // Last config file is included first
    for (size_t j = 0; j < srvParam->cfg->Include.size(); j++) {
      if (srvParam->cfg->Include[j]->attr.location.empty() == false) {
        int index = (srvParam->cfg->Include.size() - 1) - j;
#ifdef CREQUEST_DEBUG
        CDBDebug("Include '%s'", srvParam->cfg->Include[index]->attr.location.c_str());
#endif
        status = srvParam->parseConfigFile(srvParam->cfg->Include[index]->attr.location);
        if (status != 0) {
          CDBError("There is an error with include '%s'", srvParam->cfg->Include[index]->attr.location.c_str());
          return 1;
        }
      }
    }

  } else {
    srvParam->cfg = NULL;
    CDBError("Invalid XML file %s", pszConfigFile);
    return 1;
  }

#ifdef MEASURETIME
  StopWatch_Stop("Config file parsed");
#endif

  // Check for mandatory attributes
  for (size_t j = 0; j < srvParam->cfg->Layer.size(); j++) {
    if (srvParam->cfg->Layer[j]->attr.type.equals("database")) {
      if (srvParam->cfg->Layer[j]->Variable.size() == 0) {
        CDBError("Configuration error at layer %lu: <Variable> not defined", j);
        return 1;
      }
      if (srvParam->cfg->Layer[j]->FilePath.size() == 0) {
        CDBError("Configuration error at layer %lu: <FilePath> not defined", j);
        return 1;
      }
    }
  }
  // Check for autoscan elements
  for (size_t j = 0; j < srvParam->cfg->Layer.size(); j++) {
    if (srvParam->cfg->Layer[j]->attr.type.equals("autoscan")) {

      if (srvParam->cfg->Layer[j]->FilePath.size() == 0) {
        CDBError("Configuration error at layer %lu: <FilePath> not defined", j);
        return 1;
      }
      try {
        /* Create the list of layers from a directory list */
        const char *baseDir = srvParam->cfg->Layer[j]->FilePath[0]->value.c_str();

        CDBDebug("autoscan");
        std::vector<std::string> fileList;
        try {
          fileList = CDBFileScanner::searchFileNames(baseDir, srvParam->cfg->Layer[j]->FilePath[0]->attr.filter.c_str(), NULL);
        } catch (int linenr) {
          CDBError("Could not find any file in directory '%s'", baseDir);
          throw(__LINE__);
        }

        if (fileList.size() == 0) {
          CDBError("Could not find any file in directory '%s'", baseDir);
          throw(__LINE__);
        }
        size_t nrOfFileErrors = 0;
        for (size_t j = 0; j < fileList.size(); j++) {
          try {
            CT::string baseDirStr = baseDir;
            CT::string groupName = fileList[j].c_str();
            groupName.substringSelf(baseDirStr.length(), -1);

            // Open file
            // CDBDebug("Opening file %s",fileList[j].c_str());
            CDFObject *cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeader(NULL, srvParam, fileList[j].c_str());
            if (cdfObject == NULL) {
              CDBError("Unable to read file %s", fileList[j].c_str());
              throw(__LINE__);
            }

            // std::vector<CT::string> variables;
            // List variables
            for (size_t v = 0; v < cdfObject->variables.size(); v++) {
              CDF::Variable *var = cdfObject->variables[v];
              if (var->isDimension == false) {
                if (var->dimensionlinks.size() >= 2) {
                  // variables.push_back(new CT::string(var->name.c_str()));
                  CServerConfig::XMLE_Layer *xmleLayer = new CServerConfig::XMLE_Layer();
                  CServerConfig::XMLE_Group *xmleGroup = new CServerConfig::XMLE_Group();
                  CServerConfig::XMLE_Variable *xmleVariable = new CServerConfig::XMLE_Variable();
                  CServerConfig::XMLE_FilePath *xmleFilePath = new CServerConfig::XMLE_FilePath();
                  // CServerConfig::XMLE_Cache* xmleCache = new CServerConfig::XMLE_Cache();
                  // xmleCache->attr.enabled.copy("false");
                  xmleLayer->attr.type.copy("database");
                  xmleVariable->value.copy(var->name.c_str());
                  xmleFilePath->value.copy(fileList[j].c_str());
                  xmleGroup->attr.value.copy(groupName.c_str());
                  xmleLayer->Variable.push_back(xmleVariable);
                  xmleLayer->FilePath.push_back(xmleFilePath);
                  // xmleLayer->Cache.push_back(xmleCache);
                  xmleLayer->Group.push_back(xmleGroup);
                  srvParam->cfg->Layer.push_back(xmleLayer);
                }
              }
            }

          } catch (int e) {
            nrOfFileErrors++;
          }
        }
        if (nrOfFileErrors != 0) {
          CDBError("%lu files are not readable", nrOfFileErrors);
        }

      } catch (int line) {
        return 1;
      }
    }
  }
#ifdef MEASURETIME
  StopWatch_Stop("Config file checked");
#endif

  return status;
}

int CRequest::process_wms_getmetadata_request() { return process_all_layers(); }

CServerParams *CRequest::getServerParams() { return srvParam; }

int CRequest::generateGetReferenceTimesDoc(CT::string *result, CDataSource *dataSource) {
  bool hasReferenceTimeDimension = false;
  CT::string dimName = "";
  for (size_t l = 0; l < dataSource->cfgLayer->Dimension.size(); l++) {
    if (dataSource->cfgLayer->Dimension[l]->value.equals("reference_time")) {
      dimName = dataSource->cfgLayer->Dimension[l]->attr.name.c_str();
      hasReferenceTimeDimension = true;
      break;
    }
  }

  if (hasReferenceTimeDimension) {
    CT::string tableName;

    try {
      tableName = CDBFactory::getDBAdapter(srvParam->cfg)
                      ->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), dimName.c_str(), dataSource);
    } catch (int e) {
      CDBError("Unable to create tableName from '%s' '%s' '%s'", dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), dimName.c_str());
      return 1;
    }

    CDBStore::Store *store = CDBFactory::getDBAdapter(srvParam->cfg)->getUniqueValuesOrderedByValue(dimName.c_str(), -1, false, tableName.c_str());
    if (store == NULL) {
      setExceptionType(InvalidDimensionValue);
      CDBError("Invalid dimension value for layer %s", dataSource->cfgLayer->Name[0]->value.c_str());
      return 1;
    }
    result->copy("[");
    bool first = true;
    for (size_t k = 0; k < store->getSize(); k++) {
      if (!first) {
        result->concat(",");
      }
      first = false;
      result->concat("\"");
      CT::string ymd;
      ymd = store->getRecord(k)->get(0);
      ymd.setChar(10, 'T');
      // 01234567890123456789
      // YYYY-MM-DDTHH:MM:SSZ
      if (ymd.length() == 19) {
        ymd.concat("Z");
      }
      result->concat(ymd);
      result->concat("\"");
    }
    result->concat("]");
    delete store;
    return 0;
  } else {
    // Set WMSLayers:
    std::set<std::string> WMSGroups;
    for (size_t j = 0; j < dataSource->srvParams->cfg->Layer.size(); j++) {
      CT::string groupName;
      dataSource->srvParams->makeLayerGroupName(&groupName, dataSource->srvParams->cfg->Layer[j]);
      if (groupName.testRegEx("[[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]]"
                              "_[[:digit:]][[:digit:]]")) {
        CT::string ymd = groupName.substring(0, 8);
        CT::string hh = groupName.substring(9, 11);
        ymd.concat(hh);
        ymd.concat("00");
        WMSGroups.insert(ymd.c_str());
      } else if (groupName.testRegEx("[[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:"
                                     "digit:]][[:digit:]][[:digit:]]")) {
        groupName.concat("00");
        WMSGroups.insert(groupName.c_str());
      } else if (groupName.testRegEx("[[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]][[:"
                                     "digit:]][[:digit:]][[:digit:]][[:digit:]][[:digit:]]")) {
        WMSGroups.insert(groupName.c_str());
      }
    }
    result->copy("[");
    bool first = true;
    for (std::set<std::string>::reverse_iterator it = WMSGroups.rbegin(); it != WMSGroups.rend(); ++it) {
      if (!first) {
        result->concat(",");
      }
      first = false;
      result->concat("\"");
      result->concat((*it).c_str());
      result->concat("\"");
    }
    result->concat("]");
  }
  return 0;
}

int CRequest::process_wms_getlegendgraphic_request() { return process_all_layers(); }
int CRequest::process_wms_getfeatureinfo_request() { return process_all_layers(); }

int CRequest::process_wcs_getcoverage_request() {
#ifndef ADAGUC_USE_GDAL
  CServerParams::showWCSNotEnabledErrorMessage();
  return 1;
#else
  return process_all_layers();
#endif
}

int CRequest::generateOGCGetCapabilities(CT::string *XMLdocument) {
  CXMLGen XMLGen;
  return XMLGen.OGCGetCapabilities(srvParam, XMLdocument);
}

int CRequest::generateGetReferenceTimes(CDataSource *dataSource) {
  CT::string XMLdocument;

  int status = generateGetReferenceTimesDoc(&XMLdocument, dataSource);

  if (status == CXMLGEN_FATAL_ERROR_OCCURED) return 1;

  if (srvParam->JSONP.length() == 0) {
    printf("%s%s%c%c\n", "Content-Type: application/json ", srvParam->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
    printf("%s", XMLdocument.c_str());
  } else {
    printf("%s%s%c%c\n", "Content-Type: application/javascript ", srvParam->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
    printf("%s(%s)", srvParam->JSONP.c_str(), XMLdocument.c_str());
  }

  return 0;
}

int CRequest::generateOGCDescribeCoverage(CT::string *XMLdocument) {
  CXMLGen XMLGen;
  for (size_t j = 0; j < srvParam->requestedLayerNames.size(); j++) {
    CDBDebug("WCS_DESCRIBECOVERAGE %s", srvParam->requestedLayerNames[j].c_str());
  }
  return XMLGen.OGCGetCapabilities(srvParam, XMLdocument);
}

int CRequest::process_wms_getcap_request() {
#ifdef CREQUEST_DEBUG
  CDBDebug("WMS GETCAPABILITIES [%s]", srvParam->datasetLocation.c_str());
#endif

  CT::string XMLdocument;

  int status = generateOGCGetCapabilities(&XMLdocument);

  if (status == CXMLGEN_FATAL_ERROR_OCCURED) return 1;

  const char *pszADAGUCWriteToFile = getenv("ADAGUC_WRITETOFILE");
  if (pszADAGUCWriteToFile != NULL) {
    CReadFile::write(pszADAGUCWriteToFile, XMLdocument.c_str(), XMLdocument.length());
  } else {
    printf("%s%s%c%c\n", "Content-Type:text/xml", srvParam->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
    printf("%s", XMLdocument.c_str());
  }

  return 0;
}

int CRequest::process_wms_getreferencetimes_request() {
  CDBDebug("WMS GETREFERENCETIMES [%s]", srvParam->datasetLocation.c_str());
  return process_all_layers();
}

int CRequest::process_wcs_getcap_request() {
  CDBDebug("WCS GETCAPABILITIES");
  return process_wms_getcap_request();
}

int CRequest::process_wcs_describecov_request() { return process_all_layers(); }

int CRequest::process_wms_getmap_request() {
#ifdef CREQUEST_DEBUG
  CT::string message = "WMS GETMAP ";
  for (size_t j = 0; j < srvParam->requestedLayerNames.size(); j++) {
    if (j > 0) message.concat(",");
    message.printconcat("(%d) %s", j, srvParam->requestedLayerNames[j].c_str());
  }
  CDBDebug(message.c_str());
#endif
  return process_all_layers();
}

int CRequest::process_wms_gethistogram_request() {

  CT::string message = "WMS GETHISTOGRAM ";
  for (size_t j = 0; j < srvParam->requestedLayerNames.size(); j++) {
    if (j > 0) message.concat(",");
    message.printconcat("(%d) %s", j, srvParam->requestedLayerNames[j].c_str());
  }
  CDBDebug("%s", message.c_str());

  return process_all_layers();
}

int CRequest::setDimValuesForDataSource(CDataSource *dataSource, CServerParams *srvParam) {
#ifdef CREQUEST_DEBUG
  CDBDebug("setDimValuesForDataSource");
#endif
  int status = fillDimValuesForDataSource(dataSource, srvParam);
  if (status != 0) return status;

  return queryDimValuesForDataSource(dataSource, srvParam);
};

int CRequest::fillDimValuesForDataSource(CDataSource *dataSource, CServerParams *srvParam) {

#ifdef CREQUEST_DEBUG
  StopWatch_Stop("### [fillDimValuesForDataSource]");
#endif
  int status = 0;
  try {
    /*
     * Check if all tables are available, if not update the db
     */
    if (srvParam->isAutoResourceEnabled()) {
      status = CDBFactory::getDBAdapter(srvParam->cfg)->autoUpdateAndScanDimensionTables(dataSource);
      if (status != 0) {
        CDBError("makeDimensionTables checkAndUpdateDimTables failed");
        return status;
      }
    }
    for (size_t j = 0; j < dataSource->timeSteps.size(); j++) {
      delete dataSource->timeSteps[j];
    }
    dataSource->timeSteps.clear();
    /*
     * Get the number of required dims from the given dims
     * Check if all dimensions are given
     */
#ifdef CREQUEST_DEBUG
    CDBDebug("Get DIMS from query string");
#endif
    for (size_t k = 0; k < srvParam->requestDims.size(); k++) srvParam->requestDims[k]->name.toLowerCaseSelf();

    bool hasReferenceTimeDimension = false;
    for (size_t l = 0; l < dataSource->cfgLayer->Dimension.size(); l++) {
      if (dataSource->cfgLayer->Dimension[l]->value.equals("reference_time")) {
        hasReferenceTimeDimension = true;
        break;
      }
    }

    for (size_t i = 0; i < dataSource->cfgLayer->Dimension.size(); i++) {
      CT::string dimName(dataSource->cfgLayer->Dimension[i]->value.c_str());
      dimName.toLowerCaseSelf();
#ifdef CREQUEST_DEBUG
      CDBDebug("dimName \"%s\"", dimName.c_str());
#endif
      // Check if this dim is not already added
      bool alreadyAdded = false;

      /* A dimension where the default value is set to filetimedate is not a required dim and should not be queried from the db */
      if (dataSource->cfgLayer->Dimension[i]->attr.defaultV.equals("filetimedate")) {
        alreadyAdded = true;
      }

      for (size_t l = 0; l < dataSource->requiredDims.size(); l++) {
        if (dataSource->requiredDims[l]->name.equals(dimName)) {
          alreadyAdded = true;
          break;
        }
      }

#ifdef CREQUEST_DEBUG
      CDBDebug("alreadyAdded = %d", alreadyAdded);
#endif
      if (alreadyAdded == false) {
        for (size_t k = 0; k < srvParam->requestDims.size(); k++) {
          if (srvParam->requestDims[k]->name.equals(dimName)) {
#ifdef CREQUEST_DEBUG
            CDBDebug("DIM COMPARE: %s==%s", srvParam->requestDims[k]->name.c_str(), dimName.c_str());
#endif

            // This dimension has been specified in the request, so the dimension has been found:
            COGCDims *ogcDim = new COGCDims();
            dataSource->requiredDims.push_back(ogcDim);
            ogcDim->name.copy(&dimName);
            ogcDim->value.copy(&srvParam->requestDims[k]->value);
            ogcDim->queryValue.copy(&srvParam->requestDims[k]->value);
            ogcDim->netCDFDimName.copy(dataSource->cfgLayer->Dimension[i]->attr.name.c_str());
            ogcDim->hidden = dataSource->cfgLayer->Dimension[i]->attr.hidden;

            if (ogcDim->name.equals("time") || ogcDim->name.equals("reference_time")) {
              // Make nice time value 1970-01-01T00:33:26 --> 1970-01-01T00:33:26Z
              if (ogcDim->value.charAt(10) == 'T') {
                if (ogcDim->value.length() == 19) {
                  ogcDim->value.concat("Z");
                }

                /* Try to make sense of other timestrings as well */
                if (ogcDim->value.indexOf("/") == -1 && ogcDim->value.indexOf(",") == -1) {
#ifdef CREQUEST_DEBUG
                  CDBDebug("Got Time value [%s]", ogcDim->value.c_str());
#endif

                  try {
                    CTime *ctime = CTime::GetCTimeEpochInstance();
                    if (ctime == nullptr) {
                      CDBError("Unable to get time instance");
                      return 1;
                    }
                    double currentTimeAsEpoch = ctime->dateToOffset(ctime->freeDateStringToDate(ogcDim->value.c_str()));
                    auto currentDateConverted = ctime->dateToISOString(ctime->getDate(currentTimeAsEpoch));
                    ogcDim->value = currentDateConverted;
                  } catch (int e) {
                    CDBDebug("Unable to convert '%s' to epoch", ogcDim->value.c_str());
                    return 1;
                  }
#ifdef CREQUEST_DEBUG
                  CDBDebug("Converted to Time value [%s]", ogcDim->value.c_str());
#endif
                }
              }
              // If we have a dimension value quantizer adjust the value accordingly
              if (!dataSource->cfgLayer->Dimension[i]->attr.quantizeperiod.empty()) {
                CDBDebug("For dataSource %s found quantizeperiod %s", dataSource->layerName.c_str(), dataSource->cfgLayer->Dimension[i]->attr.quantizeperiod.c_str());
                CT::string quantizemethod = "round";
                CT::string quantizeperiod = dataSource->cfgLayer->Dimension[i]->attr.quantizeperiod;
                if (!dataSource->cfgLayer->Dimension[i]->attr.quantizemethod.empty()) {
                  quantizemethod = dataSource->cfgLayer->Dimension[i]->attr.quantizemethod;
                }
                // Start time quantization with quantizeperiod and quantizemethod
                ogcDim->value = CTime::quantizeTimeToISO8601(ogcDim->value, quantizeperiod, quantizemethod);
              }
            }

            // If we have value 'current', give the dim a special status
            if (ogcDim->value.equals("current")) {
              CT::string tableName;

              try {
                tableName = CDBFactory::getDBAdapter(srvParam->cfg)
                                ->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(),
                                                                        ogcDim->netCDFDimName.c_str(), dataSource);
              } catch (int e) {
                CDBError("Unable to create tableName from '%s' '%s' '%s'", dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(),
                         ogcDim->netCDFDimName.c_str());
                return 1;
              }

              if (hasReferenceTimeDimension == false) {

                // For observations, take the latest:
                CDBStore::Store *maxStore = CDBFactory::getDBAdapter(srvParam->cfg)->getMax(ogcDim->netCDFDimName.c_str(), tableName.c_str());
                if (maxStore == NULL) {
                  CDBError("Unable to get max dimension value");
                  return 1;
                }
                ogcDim->value.copy(maxStore->getRecord(0)->get(0));
                delete maxStore;
              } else {
                // For models with a reference_time, select the nearest time to current system clock

                // For time:
                if (dataSource->cfgLayer->Dimension[i]->value.equals("time")) {
                  CDBStore::Store *maxStore = CDBFactory::getDBAdapter(srvParam->cfg)->getClosestDataTimeToSystemTime(ogcDim->netCDFDimName.c_str(), tableName.c_str());

                  if (maxStore == NULL) {
                    CDBDebug("Invalid dimension value for layer %s", dataSource->cfgLayer->Name[0]->value.c_str());
                    throw InvalidDimensionValue;
                    //                     setExceptionType(InvalidDimensionValue);
                    //                     CDBError("Invalid dimension value for layer
                    //                     %s",dataSource->cfgLayer->Name[0]->value.c_str()); CDBError("query failed");
                    //                     return 1;
                  }
                  ogcDim->value.copy(maxStore->getRecord(0)->get(0));

                  delete maxStore;
                  CDBDebug("%s %s", ogcDim->netCDFDimName.c_str(), ogcDim->value.c_str());
                } else {
                  // For other dimensions than time take the latest
                  CDBStore::Store *maxStore = CDBFactory::getDBAdapter(srvParam->cfg)->getMax(ogcDim->netCDFDimName.c_str(), tableName.c_str());
                  if (maxStore == NULL) {
                    CDBDebug("Invalid dimension value for layer %s", dataSource->cfgLayer->Name[0]->value.c_str());
                    throw InvalidDimensionValue;
                    //                     setExceptionType(InvalidDimensionValue);
                    //                     CDBError("Invalid dimension value for layer
                    //                     %s",dataSource->cfgLayer->Name[0]->value.c_str()); CDBError("query failed");
                    //                     return 1;
                  }
                  ogcDim->value.copy(maxStore->getRecord(0)->get(0));
                  delete maxStore;
                }
              }
            }
          }
        }
      }
    }
#ifdef CREQUEST_DEBUG
    CDBDebug("Get DIMS from query string ready");
#endif

    /* Fill in the undefined dims */

    for (size_t i = 0; i < dataSource->cfgLayer->Dimension.size(); i++) {
      CT::string dimName(dataSource->cfgLayer->Dimension[i]->value.c_str());
      dimName.toLowerCaseSelf();
      bool alreadyAdded = false;

      for (size_t k = 0; k < dataSource->requiredDims.size(); k++) {
        if (dataSource->requiredDims[k]->name.equals(dimName)) {
          alreadyAdded = true;
          break;
        }
      }
      if (alreadyAdded == false) {
        CT::string netCDFDimName(dataSource->cfgLayer->Dimension[i]->attr.name.c_str());
        if (netCDFDimName.equals("none")) {
          continue;
        }
        /* A dimension where the default value is set to filetimedate should not be queried from the db */
        if (dataSource->cfgLayer->Dimension[i]->attr.defaultV.equals("filetimedate")) {
          continue;
        }
        CT::string tableName;
        try {
          tableName =
              CDBFactory::getDBAdapter(srvParam->cfg)
                  ->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), netCDFDimName.c_str(), dataSource);
        } catch (int e) {
          CDBError("Unable to create tableName from '%s' '%s' '%s'", dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(), netCDFDimName.c_str());
          return 1;
        }

        // Add the undefined dims to the srvParams as additional dims
        COGCDims *ogcDim = new COGCDims();
        dataSource->requiredDims.push_back(ogcDim);
        ogcDim->name.copy(&dimName);
        ogcDim->netCDFDimName.copy(dataSource->cfgLayer->Dimension[i]->attr.name.c_str());
        ogcDim->hidden = dataSource->cfgLayer->Dimension[i]->attr.hidden;

        bool isReferenceTimeDimension = false;
        if (dataSource->cfgLayer->Dimension[i]->value.equals("reference_time")) {
          isReferenceTimeDimension = true;
        }

        CDBStore::Store *maxStore = NULL;
        if (!isReferenceTimeDimension) {
          // Try to find the max value for this dim name from the database
          maxStore = CDBFactory::getDBAdapter(srvParam->cfg)->getMax(dataSource->cfgLayer->Dimension[i]->attr.name.c_str(), tableName.c_str());
        } else {
          // Try to find a reference time closest to the given time value?

          // The current time is:
          CT::string timeValue;
          CT::string netcdfTimeDimName;
          for (size_t j = 0; j < dataSource->requiredDims.size(); j++) {
            // CDBDebug("DIMS: %d [%s] [%s]", j, dataSource->requiredDims[j]->name.c_str(), dataSource->requiredDims[j]->value.c_str());
            if (dataSource->requiredDims[j]->name.equals("time")) {
              timeValue = dataSource->requiredDims[j]->value;
              netcdfTimeDimName = dataSource->requiredDims[j]->netCDFDimName;
              break;
            }
          }
          if (timeValue.empty()) {
            // CDBDebug("Time value is not available, getting max reference_time");
            maxStore = CDBFactory::getDBAdapter(srvParam->cfg)->getMax(dataSource->cfgLayer->Dimension[i]->attr.name.c_str(), tableName.c_str());
          } else {
            // TIME is set! Get

            CT::string timeTableName;
            try {
              timeTableName = CDBFactory::getDBAdapter(srvParam->cfg)
                                  ->getTableNameForPathFilterAndDimension(dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(),
                                                                          netcdfTimeDimName.c_str(), dataSource);
            } catch (int e) {
              CDBError("Unable to create tableName from '%s' '%s' '%s'", dataSource->cfgLayer->FilePath[0]->value.c_str(), dataSource->cfgLayer->FilePath[0]->attr.filter.c_str(),
                       netcdfTimeDimName.c_str());
              return 1;
            }

            maxStore = CDBFactory::getDBAdapter(srvParam->cfg)->getReferenceTime(ogcDim->netCDFDimName.c_str(), netcdfTimeDimName.c_str(), timeValue.c_str(), timeTableName.c_str(), tableName.c_str());

            //
          }
        }

        if (maxStore == NULL) {
          CDBDebug("No table with values for layer %s", dataSource->cfgLayer->Name[0]->value.c_str());
          throw InvalidDimensionValue;
        }
        ogcDim->value.copy(maxStore->getRecord(0)->get(0));

        delete maxStore;
      }
    }

#ifdef CREQUEST_DEBUG
    CDBDebug("Fix found time values:");
#endif
    // Fix found time values which are retrieved from the database
    for (size_t i = 0; i < dataSource->requiredDims.size(); i++) {
      if (dataSource->requiredDims[i]->name.indexOf("time") != -1) {
        if (dataSource->requiredDims[i]->value.length() > 12) {
          dataSource->requiredDims[i]->isATimeDimension = true;
          if (dataSource->requiredDims[i]->value.charAt(10) == ' ') {
            dataSource->requiredDims[i]->value.setChar(10, 'T');
          }
          if (dataSource->requiredDims[i]->value.charAt(10) == 'T') {
            if (dataSource->requiredDims[i]->value.length() == 19) {
              dataSource->requiredDims[i]->value.concat("Z");
            }
          }
        }
      }
    }
    // Check and set value when the value is forced in the layer dimension configuration
    for (size_t i = 0; i < dataSource->cfgLayer->Dimension.size(); i++) {
      if (!dataSource->cfgLayer->Dimension[i]->attr.fixvalue.empty()) {
        CT::string dimName(dataSource->cfgLayer->Dimension[i]->value.c_str());
        CT::string fixedValue = dataSource->cfgLayer->Dimension[i]->attr.fixvalue;
        dimName.toLowerCaseSelf();
        for (auto &requiredDim : dataSource->requiredDims) {
          if (requiredDim->name.equals(dimName)) {
            CDBDebug("Forcing dimension %s from %s to %s", dimName.c_str(), requiredDim->value.c_str(), fixedValue.c_str());
            requiredDim->value = fixedValue;
            requiredDim->hasFixedValue = true;
            break;
          }
        }
      }
    }

    // Check if requested time dimensions use valid characters
    for (auto &dim : dataSource->requiredDims) {
      // FIXME: checkTimeFormat used to get called on every dim value, not just datetime. Check if this is required
      if (!dim->isATimeDimension) continue;

      auto dimValues = dim->value.split(",");
      for (auto &dimValue : dimValues) {
        if (!CServerParams::checkTimeFormat(dimValue)) {
          CDBError("Queried dimension %s=%s failed datetime regex", dim->name.c_str(), dim->value.c_str());
          throw InvalidDimensionValue;
        }
      }
    }

    // STOP NOW
  } catch (int i) {
    CDBError("%d", i);
    return 2;
  }

  if (dataSource->requiredDims.size() == 0) {
    COGCDims *ogcDim = new COGCDims();
    dataSource->requiredDims.push_back(ogcDim);
    ogcDim->name.copy("none");
    ogcDim->value.copy("0");
    ogcDim->netCDFDimName.copy("none");
    ogcDim->hidden = true;
  }

#ifdef CREQUEST_DEBUG
  for (size_t j = 0; j < dataSource->requiredDims.size(); j++) {
    auto *requiredDim = dataSource->requiredDims[j];
    CDBDebug("dataSource->requiredDims[%d][%s] = [%s] (%s)", j, requiredDim->name.c_str(), requiredDim->value.c_str(), requiredDim->netCDFDimName.c_str());
    CDBDebug("%s: %s === %s", requiredDim->name.c_str(), requiredDim->value.c_str(), requiredDim->queryValue.c_str());
  }
  CDBDebug("### [</fillDimValuesForDataSource>]");
#endif
  bool allNonFixedDimensionsAreAsRequestedInQueryString = true;
  for (auto requiredDim : dataSource->requiredDims) {
    // CDBDebug("%s: [%s] === [%s], fixed:%d", requiredDim->name.c_str(), requiredDim->value.c_str(), requiredDim->queryValue.c_str(), requiredDim->hasFixedValue);
    if (!requiredDim->hasFixedValue && !requiredDim->value.equals(requiredDim->queryValue)) {
      allNonFixedDimensionsAreAsRequestedInQueryString = false;
    }
  }

  // CDBDebug("allNonFixedDimensionsAreAsRequestedInQueryString %d", allNonFixedDimensionsAreAsRequestedInQueryString);
  if (allNonFixedDimensionsAreAsRequestedInQueryString) {
    srvParam->setCacheControlOption(CSERVERPARAMS_CACHE_CONTROL_OPTION_FULLYCACHEABLE);
  } else {
    srvParam->setCacheControlOption(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE);
  }

  return 0;
}
int CRequest::queryDimValuesForDataSource(CDataSource *dataSource, CServerParams *srvParam) {

  try {
    CDBStore::Store *store = NULL;

    bool hasTileSettings = dataSource->cfgLayer->TileSettings.size() > 0;
    if (!srvParam->geoParams.crs.empty() && hasTileSettings) {
      store = handleTileRequest(dataSource);
      if (store == nullptr || store->getSize() == 0) {
        CDBDebug("Unable to handleTileRequest");
        return 0;
      }
    } else {
      /* Do queries without boundingbox
         - If there are tilesettings configured, query all tiled and non tiled versions
         - If there are tilesettings configured, do not include tiles when no querybbox is defined.
      */
      dataSource->queryBBOX = false;
      dataSource->queryLevel = hasTileSettings ? 0 : -1;

      int maxQueryResultLimit = 512;

      /* Get maxquerylimit from database configuration */
      if (srvParam->cfg->DataBase.size() == 1 && srvParam->cfg->DataBase[0]->attr.maxquerylimit.empty() == false) {
        maxQueryResultLimit = srvParam->cfg->DataBase[0]->attr.maxquerylimit.toInt();
      }
      /* Get maxquerylimit from layer */
      if (dataSource->isConfigured && dataSource->cfgLayer != NULL && dataSource->cfgLayer->FilePath.size() > 0) {
        if (dataSource->cfgLayer->FilePath[0]->attr.maxquerylimit.empty() == false) {
          maxQueryResultLimit = dataSource->cfgLayer->FilePath[0]->attr.maxquerylimit.toInt();
        }
      }
      // CDBDebug("Using maxquerylimit %d", maxQueryResultLimit);
      store = CDBFactory::getDBAdapter(srvParam->cfg)->getFilesAndIndicesForDimensions(dataSource, maxQueryResultLimit, true);
    }

    if (store == NULL) {
      CDBDebug("Invalid dimension value for layer %s", dataSource->cfgLayer->Name[0]->value.c_str());
      throw InvalidDimensionValue;
    }
    if (store->getSize() == 0) {
      delete store;
      if (dataSource->queryBBOX) {
        // No tiles found can mean that we are outside an area. TODO check whether this has to to with wrong dims or
        // with missing area.
        CDBDebug("No tiles found can mean that we are outside an area. TODO check whether this has to to with wrong "
                 "dims or with missing area.");
        CDBDebug("dataSource->requiredDims.size() %lu", dataSource->requiredDims.size());
        for (size_t i = 0; i < dataSource->requiredDims.size(); i++) {
          CDBDebug("  [%s] = [%s]", dataSource->requiredDims[i]->netCDFDimName.c_str(), dataSource->requiredDims[i]->value.c_str());
        }
        return 0;
      }
      throw InvalidDimensionValue;
    }

    for (size_t k = 0; k < store->getSize(); k++) {
      CDBStore::Record *record = store->getRecord(k);
      // CDBDebug("Addstep");
      dataSource->addStep(record->get(0)->c_str());
#ifdef CREQUEST_DEBUG
      CDBDebug("Step %d: [%s]", k, record->get(0)->c_str());
#endif
      // For each timesteps a new set of dimensions is added with corresponding dim array indices.
      for (size_t i = 0; i < dataSource->requiredDims.size(); i++) {
        CT::string value = record->get(1 + i * 2)->c_str();
        dataSource->getCDFDims()->addDimension(dataSource->requiredDims[i]->netCDFDimName.c_str(), value.c_str(), atoi(record->get(2 + i * 2)->c_str()));
#ifdef CREQUEST_DEBUG
        CDBDebug("queryDimValuesForDataSource dataSource->queryBBOX %s for step %d/%d", dataSource->layerName.c_str(), dataSource->getCurrentTimeStep(), dataSource->getNumTimeSteps());
        CDBDebug("  [%s][%d] = [%s]", dataSource->requiredDims[i]->netCDFDimName.c_str(), atoi(record->get(2 + i * 2)->c_str()), value.c_str());
#endif
        dataSource->requiredDims[i]->addValue(value.c_str());
      }
    }

    delete store;
  } catch (int i) {
    CDBError("Exception %d in queryDimValuesForDataSource", i);
    throw i;
  }
#ifdef CREQUEST_DEBUG
  CDBDebug("Datasource has %d steps", dataSource->getNumTimeSteps());
  StopWatch_Stop("[/setDimValuesForDataSource]");
#endif
  return 0;
}

int CRequest::process_all_layers() {
  CT::string pathFileName;

  // No layers defined, so maybe the DescribeCoverage request did not define any coverages...
  if (srvParam->requestedLayerNames.size() == 0) {
    if (srvParam->requestType == REQUEST_WCS_DESCRIBECOVERAGE) {
      // Therefore we read all the coverages, we do this by defining WMSLayers, as if the user entered in the coverages
      // section all layers.
      srvParam->requestType = REQUEST_WCS_DESCRIBECOVERAGE;
      for (size_t j = 0; j < srvParam->cfg->Layer.size(); j++) {
        srvParam->requestedLayerNames.push_back(makeUniqueLayerName(srvParam->cfg->Layer[j]));
      }
    } else {
      CDBError("No layers/coverages defined");
      return 1;
    }
  } else {
    // Otherwise WMSLayers are defined by the user, so we need only the selection the user has made
    if (srvParam->requestedLayerNames.size() == 0) {
      CDBError("No layers/coverages defined");
      return 1;
    }

    // Loop through all layers as request in the request
    for (size_t layerIndex = 0; layerIndex < srvParam->requestedLayerNames.size(); layerIndex++) {
      CT::string requestedLayerName = srvParam->requestedLayerNames[layerIndex];
      auto cfgLayer = findLayerConfigForRequestedLayer(srvParam, requestedLayerName);
      if (cfgLayer == nullptr) {

        // Do not set status code for a missing layer to 404 when we request GetCapabilities
        // GetCapabilities should return statuscode 200 with layers that work.
        if (srvParam->requestType != REQUEST_WMS_GETCAPABILITIES && srvParam->requestType != REQUEST_WCS_GETCAPABILITIES && srvParam->requestType != REQUEST_WCS_DESCRIBECOVERAGE) {
          setStatusCode(HTTP_STATUSCODE_404_NOT_FOUND);
          CDBError("Layer [%s] not found", requestedLayerName.c_str());
        } else {
          CDBDebug("Layer [%s] not found", requestedLayerName.c_str());
        }
        return 1;
      }

      if (cfgLayer != nullptr) {
        CT::string layerName = makeUniqueLayerName(cfgLayer);
        if (layerName.equals(requestedLayerName.c_str())) {
          if (this->addDataSources(cfgLayer, layerIndex) != 0) {
            CDBError("Unable to create datasources for %s", layerName.c_str());
            return 1;
          }
        }
      }
    }
  }

  int status;
  if (srvParam->serviceType == SERVICE_WMS) {
    if (srvParam->geoParams.width > MAX_IMAGE_WIDTH) {
      CDBError("Parameter WIDTH must be smaller than %d", MAX_IMAGE_WIDTH);
      return 1;
    }
    if (srvParam->geoParams.height > MAX_IMAGE_HEIGHT) {
      CDBError("Parameter HEIGHT must be smaller than %d", MAX_IMAGE_HEIGHT);
      return 1;
    }
  }

  if (srvParam->serviceType == SERVICE_WCS) {
    if (srvParam->requestType == REQUEST_WCS_DESCRIBECOVERAGE) {
      CT::string XMLDocument;
      status = generateOGCDescribeCoverage(&XMLDocument);
      if (status == CXMLGEN_FATAL_ERROR_OCCURED) return 1;
      const char *pszADAGUCWriteToFile = getenv("ADAGUC_WRITETOFILE");
      if (pszADAGUCWriteToFile != NULL) {
        CReadFile::write(pszADAGUCWriteToFile, XMLDocument.c_str(), XMLDocument.length());
      } else {
        printf("%s%s%c%c\n", "Content-Type:text/xml", srvParam->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
        printf("%s", XMLDocument.c_str());
      }
      return 0;
    }
  }

  // Set layer types for all datasources
  if (this->determineTypesForDataSources() != 0) {
    return 1;
  }

  // Try to find BBOX automatically, when not provided.
  this->autoDetectBBOX();

  auto firstDataSource = dataSources[0];
  /**************************************/
  /* Handle WMS Getmap database request */
  /**************************************/
  if (firstDataSource->dLayerType == CConfigReaderLayerTypeDataBase || firstDataSource->dLayerType == CConfigReaderLayerTypeGraticule ||
      firstDataSource->dLayerType == CConfigReaderLayerTypeBaseLayer || (firstDataSource->dLayerType == CConfigReaderLayerTypeLiveUpdate && srvParam->requestType != REQUEST_WMS_GETMAP)) {
    try {
      for (size_t d = 0; d < dataSources.size(); d++) {
        dataSources[d]->setTimeStep(0);
      }
      int status = 0;
      if (srvParam->requestType == REQUEST_WMS_GETMAP) {
        status = this->handleGetMapRequest(firstDataSource);
      }

      if (srvParam->requestType == REQUEST_WCS_GETCOVERAGE) {
        status = handleGetCoverageRequest(firstDataSource);
      }

      if (srvParam->requestType == REQUEST_WMS_GETFEATUREINFO) {
        if (firstDataSource->dLayerType == CConfigReaderLayerTypeLiveUpdate) {
          layerTypeLiveUpdateRender(firstDataSource, srvParam);
        } else {
          CImageDataWriter imageDataWriter;
          status = imageDataWriter.init(srvParam, firstDataSource, firstDataSource->getNumTimeSteps());
          if (status != 0) throw(__LINE__);
          status = imageDataWriter.getFeatureInfo(dataSources, 0, int(srvParam->dX), int(srvParam->dY));
          if (status != 0) throw(__LINE__);
          status = imageDataWriter.end();
          if (status != 0) throw(__LINE__);
        }
      }

      // WMS Getlegendgraphic
      if (srvParam->requestType == REQUEST_WMS_GETLEGENDGRAPHIC) {
        CImageDataWriter imageDataWriter;
        status = imageDataWriter.init(srvParam, firstDataSource, 1);
        if (status != 0) throw(__LINE__);
        bool rotate = srvParam->geoParams.width > srvParam->geoParams.height;
        CDBDebug("creatinglegend %dx%d %d", srvParam->geoParams.width, srvParam->geoParams.height, rotate);
        status = imageDataWriter.createLegend(firstDataSource, &imageDataWriter.drawImage, rotate);
        if (status != 0) throw(__LINE__);
        status = imageDataWriter.end();
        if (status != 0) throw(__LINE__);
      }

      // WMS GETHISTOGRAM
      if (srvParam->requestType == REQUEST_WMS_GETHISTOGRAM) {
        CCreateHistogram histogramCreator;
        CDBDebug("REQUEST_WMS_GETHISTOGRAM");

        try {
          status = histogramCreator.init(srvParam, firstDataSource, firstDataSource->getNumTimeSteps());
          if (status != 0) throw(__LINE__);
        } catch (int e) {
          CDBError("Exception code %d", e);
          throw(__LINE__);
        }

        try {
          status = histogramCreator.addData(dataSources);
        } catch (int e) {
          CDBError("Exception code %d", e);
          throw(__LINE__);
        }
        if (status != 0) throw(__LINE__);

        try {
          status = histogramCreator.end();
          if (status != 0) throw(__LINE__);
        } catch (int e) {
          CDBError("Exception code %d", e);
          throw(__LINE__);
        }
      }

      // WMS GetMetaData
      if (srvParam->requestType == REQUEST_WMS_GETMETADATA) {
        CDataReader reader;
        status = reader.open(firstDataSource, CNETCDFREADER_MODE_OPEN_HEADER);
        if (status != 0) {
          CDBError("Could not open file: %s", firstDataSource->getFileName());
          throw(__LINE__);
        }
        CT::string dumpString = CDF::dump(firstDataSource->getDataObject(0)->cdfObject);
        CT::string cacheControl = srvParam->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_NOCACHE);
        printf("%s%s%c%c\n", "Content-Type: text/plain", cacheControl.c_str(), 13, 10);
        printf("%s", dumpString.c_str());
        reader.close();
        return 0;
      }

      if (srvParam->requestType == REQUEST_WMS_GETREFERENCETIMES) {
        status = generateGetReferenceTimes(firstDataSource);
        if (status != 0) {
          throw(__LINE__);
        }
      }
    } catch (int i) {
      // Exception thrown: Cleanup and return;
      CDBError("Returning from line %d", i);
      return 1;
    }
  } else if (firstDataSource->dLayerType == CConfigReaderLayerTypeLiveUpdate) {
    try {
      layerTypeLiveUpdateRender(firstDataSource, srvParam);
    } catch (int e) {
      CDBError("Exception in layerTypeLiveUpdateRender %d", e);
      return 1;
    }
  } else {
    CDBError("Unknown layer type");
  }
  //}

  return 0;
}

int CRequest::process_querystring() {

#ifdef MEASURETIME
  StopWatch_Stop("Start processing query string");
#endif

  if (srvParam == nullptr || srvParam->cfg == nullptr || srvParam->cfg->WMS.size() != 1) {
    CDBError("WMS element has not been configured");
    return 1;
  }
  if (srvParam->cfg->WCS.size() != 1) {
    CDBError("WCS element has not been configured");
    return 1;
  }

  seterrormode(EXCEPTIONS_PLAINTEXT);
  CT::string SERVICE, REQUEST;

  int dFound_Width = 0;
  int dFound_Height = 0;
  int dFound_X = 0;
  int dFound_Y = 0;
  int dFound_I = 0;
  int dFound_J = 0;
  int dFound_RESX = 0;
  int dFound_RESY = 0;

  int dFound_SRS = 0;
  int dFound_CRS = 0;
  int dFound_RESPONSE_CRS = 0;

  // int dFound_Debug=0;
  int dFound_Request = 0;
  int dFound_Service = 0;
  int dFound_Format = 0;
  int dFound_InfoFormat = 0;
  int dFound_Transparent = 0;
  int dFound_BGColor = 0;
  int dErrorOccured = 0;
  int dFound_WMSLAYERS = 0;
  int dFound_WCSCOVERAGE = 0;
  int dFound_Version = 0;
  int dFound_Exceptions = 0;
  int dFound_Styles = 0;
  int dFound_Style = 0;
  int dFound_JSONP = 0;

  int dFound_autoResourceLocation = 0;
  // int dFound_OpenDAPVariable=0;

  const char *pszQueryString = getenv("QUERY_STRING");

  /**
   * Check for OPENDAP
   */
  if (srvParam->cfg->OpenDAP.size() == 1) {
    if (srvParam->cfg->OpenDAP[0]->attr.enabled.equals("true")) {
      CT::string defaultPath = "opendap";
      if (srvParam->cfg->OpenDAP[0]->attr.path.empty() == false) {
        defaultPath = srvParam->cfg->OpenDAP[0]->attr.path.c_str();
      }
      const char *SCRIPT_NAME = getenv("SCRIPT_NAME");
      const char *REQUEST_URI = getenv("REQUEST_URI");
      // CDBDebug("SCRIPT_NAME [%s], REQUEST_URI [%s]",SCRIPT_NAME,REQUEST_URI);
      if (SCRIPT_NAME != NULL && REQUEST_URI != NULL) {
        size_t SCRIPT_NAME_length = strlen(SCRIPT_NAME);
        size_t REQUEST_URI_length = strlen(REQUEST_URI);
        if (REQUEST_URI_length > SCRIPT_NAME_length + 1) {
          CT::string dapPath = REQUEST_URI + (SCRIPT_NAME_length + 1);

          if (dapPath.indexOf(defaultPath.c_str()) == 0) {
            // THIS is OPENDAP!
            auto items = dapPath.split("?");
            if (items.size() > 0) {
              COpenDAPHandler opendapHandler;
              opendapHandler.handleOpenDAPRequest(items[0].c_str(), pszQueryString, srvParam);
              return 0;
            }
          }
        }
      }
    }
  }

  CT::string queryString(pszQueryString);
  if (queryString.empty()) {
    queryString = "SERVICE=WMS&request=getcapabilities";
    CGI = 0;
  } else {
    CGI = 1;
  }

  queryString.decodeURLSelf();
  // CDBDebug("QueryString: \"%s\"", queryString.c_str());
  auto parameters = queryString.split("&");

#ifdef CREQUEST_DEBUG
  CDBDebug("Parsing query string parameters");
#endif
  for (size_t j = 0; j < parameters.size(); j++) {
    CT::string uriKeyUpperCase;
    CT::string uriValue;

    int equalPos = parameters[j].indexOf("="); // split("=");

    if (equalPos != -1) {
      uriKeyUpperCase = parameters[j].substring(0, equalPos);
      uriValue = parameters[j].c_str() + equalPos + 1;
    } else {
      uriKeyUpperCase = parameters[j].c_str();
    }

    uriKeyUpperCase.toUpperCaseSelf();

    if (uriKeyUpperCase.equals("STYLES")) {
      if (dFound_Styles == 0) {
        if (!uriValue.empty()) {
          srvParam->Styles.copy(&uriValue);
        } else
          srvParam->Styles.copy("");
        dFound_Styles = 1;
      } else {
        CDBError("ADAGUC Server: Styles already defined");
        dErrorOccured = 1;
      }
    }
    // Style parameter
    if (uriKeyUpperCase.equals("STYLE")) {
      if (dFound_Style == 0) {
        if (!uriValue.empty()) {
          srvParam->Style.copy(&uriValue);
        } else
          srvParam->Style.copy("");
        dFound_Style = 1;
      } else {
        CDBError("ADAGUC Server: Style already defined");
        dErrorOccured = 1;
      }
    }
    if (!uriValue.empty()) {
      // BBOX Parameters
      if (uriKeyUpperCase.equals("BBOX")) {
        auto bboxvalues = uriValue.replace("%2C", ",").split(",");
        if (bboxvalues.size() == 4) {
          srvParam->geoParams.bbox.left = atof(bboxvalues[0].c_str());
          srvParam->geoParams.bbox.bottom = atof(bboxvalues[1].c_str());
          srvParam->geoParams.bbox.right = atof(bboxvalues[2].c_str());
          srvParam->geoParams.bbox.top = atof(bboxvalues[3].c_str());

        } else {
          CDBError("ADAGUC Server: Invalid BBOX values");
          dErrorOccured = 1;
        }
        srvParam->dFound_BBOX = 1;
      }
      if (uriKeyUpperCase.equals("BBOXWIDTH")) {

        srvParam->geoParams.bbox.left = 0;
        srvParam->geoParams.bbox.bottom = 0;
        srvParam->geoParams.bbox.right = uriValue.toDouble();
        srvParam->geoParams.bbox.top = uriValue.toDouble();

        srvParam->dFound_BBOX = 1;
      }

      if (uriKeyUpperCase.equals("FIGWIDTH")) {
        srvParam->figWidth = atoi(uriValue.c_str());
        if (srvParam->figWidth < 1) srvParam->figWidth = -1;
      }
      if (uriKeyUpperCase.equals("FIGHEIGHT")) {
        srvParam->figHeight = atoi(uriValue.c_str());
        if (srvParam->figHeight < 1) srvParam->figHeight = -1;
      }

      // Width Parameters
      if (uriKeyUpperCase.equals("WIDTH")) {
        srvParam->geoParams.width = atoi(uriValue.c_str());
        if (srvParam->geoParams.width < 1) {
          CDBError("ADAGUC Server: Parameter Width should be at least 1");
          dErrorOccured = 1;
        }
        dFound_Width = 1;
      }
      // Height Parameters
      if (uriKeyUpperCase.equals("HEIGHT")) {
        srvParam->geoParams.height = atoi(uriValue.c_str());
        if (srvParam->geoParams.height < 1) {
          CDBError("ADAGUC Server: Parameter Height should be at least 1");
          dErrorOccured = 1;
        }

        dFound_Height = 1;
      }
      // RESX Parameters
      if (uriKeyUpperCase.equals("RESX")) {
        srvParam->dfResX = atof(uriValue.c_str());
        if (srvParam->dfResX == 0) {
          CDBError("ADAGUC Server: Parameter RESX should not be zero");
          dErrorOccured = 1;
        }
        dFound_RESX = 1;
      }
      // RESY Parameters
      if (uriKeyUpperCase.equals("RESY")) {
        srvParam->dfResY = atof(uriValue.c_str());
        if (srvParam->dfResY == 0) {
          CDBError("ADAGUC Server: Parameter RESY should not be zero");
          dErrorOccured = 1;
        }
        dFound_RESY = 1;
      }

      // X/I Parameters
      if (strncmp(uriKeyUpperCase.c_str(), "X", 1) == 0 && uriKeyUpperCase.length() == 1) {
        srvParam->dX = atof(uriValue.c_str());
        dFound_X = 1;
      }
      if (strncmp(uriKeyUpperCase.c_str(), "I", 1) == 0 && uriKeyUpperCase.length() == 1) {
        srvParam->dX = atof(uriValue.c_str());
        dFound_I = 1;
      }
      // Y/J Parameter
      if (strncmp(uriKeyUpperCase.c_str(), "Y", 1) == 0 && uriKeyUpperCase.length() == 1) {
        srvParam->dY = atof(uriValue.c_str());
        dFound_Y = 1;
      }
      if (strncmp(uriKeyUpperCase.c_str(), "J", 1) == 0 && uriKeyUpperCase.length() == 1) {
        srvParam->dY = atof(uriValue.c_str());
        dFound_J = 1;
      }
      // SRS / CRS Parameters
      if (uriKeyUpperCase.equals("SRS")) {
        if (uriValue.length() > 2) {
          srvParam->geoParams.crs.copy(uriValue);
          // srvParam->geoParams.CRS.decodeURLSelf();
          dFound_SRS = 1;
        }
      }
      if (uriKeyUpperCase.equals("CRS")) {
        if (uriValue.length() > 2) {
          srvParam->geoParams.crs.copy(uriValue);
          dFound_CRS = 1;
        }
      }

      if (uriKeyUpperCase.equals("RESPONSE_CRS")) {
        if (uriValue.length() > 2) {
          srvParam->responceCrs.copy(uriValue);
          dFound_RESPONSE_CRS = 1;
        }
      }

      // DIM Params
      int foundDim = -1;
      if (uriKeyUpperCase.equals("TIME") || uriKeyUpperCase.equals("ELEVATION")) {
        foundDim = 0;
      } else if (uriKeyUpperCase.indexOf("DIM_") == 0) {
        // We store the OGCdim without the DIM_ prefix
        foundDim = 4;
      }
      if (foundDim != -1) {
        COGCDims *ogcDim = NULL;
        const char *ogcDimName = uriKeyUpperCase.c_str() + foundDim;
        for (size_t j = 0; j < srvParam->requestDims.size(); j++) {
          if (srvParam->requestDims[j]->name.equals(ogcDimName)) {
            ogcDim = srvParam->requestDims[j];
            break;
          }
        }
        if (ogcDim == NULL) {
          ogcDim = new COGCDims();
          srvParam->requestDims.push_back(ogcDim);
        } else {
          CDBDebug("OGC Dim %s reused", ogcDimName);
        }
        ogcDim->name.copy(ogcDimName);
        ogcDim->value.copy(&uriValue);
      }

      // FORMAT parameter
      if (uriKeyUpperCase.equals("FORMAT")) {
        if (dFound_Format == 0) {
          if (uriValue.length() > 1) {
            srvParam->Format.copy(&uriValue);
            dFound_Format = 1;
          }
        } else {
          CDBError("ADAGUC Server: FORMAT already defined");
          dErrorOccured = 1;
        }
      }

      // INFO_FORMAT parameter
      if (uriKeyUpperCase.equals("INFO_FORMAT")) {
        if (dFound_InfoFormat == 0) {
          if (uriValue.length() > 1) {
            srvParam->InfoFormat.copy(&uriValue);
            dFound_InfoFormat = 1;
          }
        } else {
          CDBError("ADAGUC Server: INFO_FORMAT already defined");
          dErrorOccured = 1;
        }
      }

      // TRANSPARENT parameter
      if (uriKeyUpperCase.equals("TRANSPARENT")) {
        if (dFound_Transparent == 0) {
          if (uriValue.length() > 1) {
            if (uriValue.toUpperCase().equals("TRUE")) {
              srvParam->Transparent = true;
            }
            dFound_Transparent = 1;
          }
        } else {
          CDBError("ADAGUC Server: TRANSPARENT already defined");
          dErrorOccured = 1;
        }
      }
      // BGCOLOR parameter
      if (uriKeyUpperCase.equals("BGCOLOR")) {
        if (dFound_BGColor == 0) {
          if (uriValue.length() > 1) {
            srvParam->BGColor.copy(&uriValue);
            dFound_BGColor = 1;
          }
        } else {
          CDBError("ADAGUC Server: FORMAT already defined");
          dErrorOccured = 1;
        }
      }

      // Version parameter
      if (uriKeyUpperCase.equals("VERSION")) {
        if (dFound_Version == 0) {
          if (uriValue.length() > 1) {
            Version.copy(&uriValue);
            dFound_Version = 1;
          }
        }
        // ARCGIS user Friendliness, version can be defined multiple times.
        /*else{
          CDBError("ADAGUC Server: Version already defined");
          dErrorOccured=1;
        }*/
      }

      // Exceptions parameter
      if (uriKeyUpperCase.equals("EXCEPTIONS")) {
        if (dFound_Exceptions == 0) {
          if (uriValue.length() > 1) {
            Exceptions.copy(&uriValue);
            dFound_Exceptions = 1;
          }
        } else {
          CDBError("ADAGUC Server: Exceptions already defined");
          dErrorOccured = 1;
        }
      }

      // Opendap source parameter
      if (dFound_autoResourceLocation == 0) {
        if (uriKeyUpperCase.equals("SOURCE")) {
          if (srvParam->autoResourceLocation.empty()) {
            auto hashList = uriValue.split("#");
            if (hashList.size() > 0) {
              srvParam->autoResourceLocation.copy(hashList[0].c_str());
            }
          }
          dFound_autoResourceLocation = 1;
        }
      }

      // WMS Layers parameter
      if (uriKeyUpperCase.equals("LAYERS") || uriKeyUpperCase.equals("LAYER")) {
        srvParam->requestedLayerNames = uriValue.split(",");
        dFound_WMSLAYERS = 1;
      }

      // WMS Layer parameter
      if (uriKeyUpperCase.equals("QUERY_LAYERS")) {
        srvParam->requestedLayerNames = uriValue.split(",");
        dFound_WMSLAYERS = 1;
      }
      // WCS Coverage parameter
      if (uriKeyUpperCase.equals("COVERAGE")) {
        if (srvParam->requestedLayerNames.size() > 0) {
          CDBError("ADAGUC Server: COVERAGE already defined");
          dErrorOccured = 1;
        } else {
          srvParam->requestedLayerNames = uriValue.split(",");
        }
        dFound_WCSCOVERAGE = 1;
      }

      // Service parameters
      if (uriKeyUpperCase.equals("SERVICE")) {
        SERVICE.copy(uriValue.toUpperCase());
        dFound_Service = 1;
      }
      // Request parameters
      if (uriKeyUpperCase.equals("REQUEST")) {
        REQUEST.copy(uriValue.toUpperCase());
        dFound_Request = 1;
      }

      // debug Parameters
      if (uriKeyUpperCase.equals("DEBUG")) {
        if (uriValue.equals("ON")) {
          printf("%s%c%c\n", "Content-Type:text/plain", 13, 10);
          printf("Debug mode:ON\nDebug messages:<br>\r\n\n");
          // dFound_Debug=1;
        }
      }

      if (uriKeyUpperCase.equals("TITLE")) {
        if (uriValue.length() > 0) {
          srvParam->mapTitle = uriValue.c_str();
        }
      }
      if (uriKeyUpperCase.equals("SUBTITLE")) {
        if (uriValue.length() > 0) {
          srvParam->mapSubTitle = uriValue.c_str();
        }
      }
      if (uriKeyUpperCase.equals("SHOWDIMS")) {
        if (!uriValue.toLowerCase().equals("false")) {
          srvParam->showDimensionsInImage = true;
        }
      }
      if (uriKeyUpperCase.equals("SHOWLEGEND")) {
        if (!uriValue.toLowerCase().equals("false")) {
          srvParam->showLegendInImage = uriValue.toLowerCase();
        }
      }
      if (uriKeyUpperCase.equals("SHOWSCALEBAR")) {
        if (uriValue.toLowerCase().equals("true")) {
          srvParam->showScaleBarInImage = true;
        }
      }
      if (uriKeyUpperCase.equals("SHOWNORTHARROW")) {
        if (uriValue.toLowerCase().equals("true")) {
          srvParam->showNorthArrow = true;
        }
      }

      // http://www.resc.rdg.ac.uk/trac/ncWMS/wiki/WmsExtensions
      if (uriKeyUpperCase.equals("OPACITY")) {
        srvParam->wmsExtensions.opacity = uriValue.toDouble();
      }
      if (uriKeyUpperCase.equals("COLORSCALERANGE")) {
        auto valuesC = uriValue.split(",");
        if (valuesC.size() == 2) {
          srvParam->wmsExtensions.colorScaleRangeMin = valuesC[0].toDouble();
          srvParam->wmsExtensions.colorScaleRangeMax = valuesC[1].toDouble();
          srvParam->wmsExtensions.colorScaleRangeSet = true;
        }
      }
      if (uriKeyUpperCase.equals("NUMCOLORBANDS")) {
        srvParam->wmsExtensions.numColorBands = uriValue.toFloat();
        srvParam->wmsExtensions.numColorBandsSet = true;
      }
      if (uriKeyUpperCase.equals("LOGSCALE")) {
        if (uriValue.toLowerCase().equals("true")) {
          srvParam->wmsExtensions.logScale = true;
        }
      }
      // JSONP parameter
      if (uriKeyUpperCase.equals("JSONP")) {
        if (dFound_JSONP == 0) {
          if (uriValue.length() > 1) {
            srvParam->JSONP.copy(&uriValue);
            dFound_JSONP = 1;
          }
        } else {
          CDBError("ADAGUC Server: JSONP already defined");
          dErrorOccured = 1;
        }
      }
    }
  }

  if (dFound_Width == 0 && dFound_Height == 0) {
    if (srvParam->dFound_BBOX && dFound_RESX && dFound_RESY) {
      srvParam->geoParams.width = int(((srvParam->geoParams.bbox.right - srvParam->geoParams.bbox.left) / srvParam->dfResX));
      srvParam->geoParams.height = int(((srvParam->geoParams.bbox.bottom - srvParam->geoParams.bbox.top) / srvParam->dfResY));
      srvParam->geoParams.height = abs(srvParam->geoParams.height);
#ifdef CREQUEST_DEBUG
      CDBDebug("Calculated width height based on resx resy %d,%d", srvParam->geoParams.dWidth, srvParam->geoParams.dHeight);
#endif
    }
  }
#ifdef CREQUEST_DEBUG
  CDBDebug("Finished parsing query string parameters");
#endif
#ifdef MEASURETIME
  StopWatch_Stop("query string processed");
#endif

  if (dFound_Service == 0) {
    CDBError("ADAGUC Server: Parameter SERVICE missing");
    dErrorOccured = 1;
    setStatusCode(HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY);
  }
  if (dFound_Styles == 0) {
    srvParam->Styles.copy("");
  }
  if (SERVICE.equals("WMS"))
    srvParam->serviceType = SERVICE_WMS;
  else if (SERVICE.equals("WCS"))
    srvParam->serviceType = SERVICE_WCS;
  else if (SERVICE.equals("METADATA"))
    srvParam->serviceType = SERVICE_METADATA;
  else { // Service not recognised
    CDBError("ADAGUC Server: Parameter SERVICE invalid");
    dErrorOccured = 1;
    setStatusCode(HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY);
  }

  if (dErrorOccured == 0 && srvParam->serviceType == SERVICE_WMS) {
#ifdef CREQUEST_DEBUG
    CDBDebug("Getting parameters for WMS service");
#endif

    // Default is 1.3.0

    srvParam->OGCVersion = WMS_VERSION_1_3_0;

    if (dFound_Request == 0) {
      CDBError("ADAGUC Server: Parameter REQUEST missing");
      dErrorOccured = 1;
      setStatusCode(HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY);
    } else {
      if (REQUEST.equals("GETCAPABILITIES"))
        srvParam->requestType = REQUEST_WMS_GETCAPABILITIES;
      else if (REQUEST.equals("GETMAP"))
        srvParam->requestType = REQUEST_WMS_GETMAP;
      else if (REQUEST.equals("GETHISTOGRAM"))
        srvParam->requestType = REQUEST_WMS_GETHISTOGRAM;
      else if (REQUEST.equals("GETSCALEBAR"))
        srvParam->requestType = REQUEST_WMS_GETSCALEBAR;
      else if (REQUEST.equals("GETFEATUREINFO"))
        srvParam->requestType = REQUEST_WMS_GETFEATUREINFO;
      else if (REQUEST.equals("GETPOINTVALUE"))
        srvParam->requestType = REQUEST_WMS_GETPOINTVALUE;
      else if (REQUEST.equals("GETLEGENDGRAPHIC"))
        srvParam->requestType = REQUEST_WMS_GETLEGENDGRAPHIC;
      else if (REQUEST.equals("GETMETADATA"))
        srvParam->requestType = REQUEST_WMS_GETMETADATA;
      else if (REQUEST.equals("GETREFERENCETIMES"))
        srvParam->requestType = REQUEST_WMS_GETREFERENCETIMES;
      else {
        dErrorOccured = 1;
        setStatusCode(HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY);
        CDBError("ADAGUC Server: Parameter REQUEST invalid");
      }
    }

    // For getlegend graphic the parameter is style, not styles
    if (dFound_Style == 0) {
      srvParam->Style.copy("");
    } else {
      // For getlegend graphic the parameter is style, not styles
      if (srvParam->requestType == REQUEST_WMS_GETLEGENDGRAPHIC) {
        srvParam->Styles.copy(&srvParam->Style);
      }
    }

    seterrormode(WMS_EXCEPTIONS_XML_1_3_0);
    // Check the version
    if (dFound_Version != 0) {
      srvParam->OGCVersion = -1; // WMS_VERSION_1_1_1;
      if (Version.equals("1.0.0")) srvParam->OGCVersion = WMS_VERSION_1_0_0;
      if (Version.equals("1.1.1")) srvParam->OGCVersion = WMS_VERSION_1_1_1;
      if (Version.equals("1.3.0")) srvParam->OGCVersion = WMS_VERSION_1_3_0;
      if (srvParam->OGCVersion == -1) {
        CDBError("Invalid version ('%s'): WMS 1.0.0, WMS 1.1.1 and WMS 1.3.0 are supported", Version.c_str());
        dErrorOccured = 1;
      }
    }
    // Set the exception response
    if (srvParam->OGCVersion == WMS_VERSION_1_0_0) {
      seterrormode(EXCEPTIONS_PLAINTEXT);
      if (srvParam->requestType == REQUEST_WMS_GETMAP) seterrormode(WMS_EXCEPTIONS_IMAGE);
      if (srvParam->requestType == REQUEST_WMS_GETLEGENDGRAPHIC) seterrormode(WMS_EXCEPTIONS_IMAGE);
    }

    if (srvParam->OGCVersion == WMS_VERSION_1_1_1) {
      seterrormode(WMS_EXCEPTIONS_XML_1_1_1);
      // Check if default has been set for EXCEPTIONS
      if ((srvParam->requestType == REQUEST_WMS_GETMAP) || (srvParam->requestType == REQUEST_WMS_GETLEGENDGRAPHIC)) {
        if ((dFound_Exceptions == 0) && (srvParam->cfg->WMS[0]->WMSExceptions.size() > 0)) {
          if (srvParam->cfg->WMS[0]->WMSExceptions[0]->attr.defaultValue.empty() == false) {
            Exceptions = srvParam->cfg->WMS[0]->WMSExceptions[0]->attr.defaultValue;
            dFound_Exceptions = 1;
          }
        }
      }
    }

    if (srvParam->OGCVersion == WMS_VERSION_1_3_0) {
      seterrormode(WMS_EXCEPTIONS_XML_1_3_0);
      // Check if default has been set for EXCEPTIONS
      if ((srvParam->requestType == REQUEST_WMS_GETMAP) || (srvParam->requestType == REQUEST_WMS_GETLEGENDGRAPHIC)) {
        if ((dFound_Exceptions == 0) && (srvParam->cfg->WMS[0]->WMSExceptions.size() > 0)) {
          if (srvParam->cfg->WMS[0]->WMSExceptions[0]->attr.defaultValue.empty() == false) {
            Exceptions = srvParam->cfg->WMS[0]->WMSExceptions[0]->attr.defaultValue;
            dFound_Exceptions = 1;
            CDBDebug("Changing default to `%s' ", Exceptions.c_str());
          }
        }
      }

      if (srvParam->checkBBOXXYOrder(NULL) == true) {
        srvParam->geoParams.bbox = srvParam->geoParams.bbox.swapXY();
      }
    }

    if (dFound_Exceptions != 0) {
      if ((srvParam->requestType == REQUEST_WMS_GETMAP) || (srvParam->requestType == REQUEST_WMS_GETLEGENDGRAPHIC)) {
        // Overrule found EXCEPTIONS with value of WMSExceptions.default if force is set and default is defined
        if (srvParam->cfg->WMS[0]->WMSExceptions.size() > 0) {
          if ((srvParam->cfg->WMS[0]->WMSExceptions[0]->attr.defaultValue.empty() == false) && (srvParam->cfg->WMS[0]->WMSExceptions[0]->attr.force.empty() == false)) {
            if (srvParam->cfg->WMS[0]->WMSExceptions[0]->attr.force.equals("true")) {
              Exceptions = srvParam->cfg->WMS[0]->WMSExceptions[0]->attr.defaultValue;
              CDBDebug("Overruling default Exceptions %s", Exceptions.c_str());
            }
          }
        }
      }

      if (Exceptions.equals("application/vnd.ogc.se_xml")) {
        if (srvParam->OGCVersion == WMS_VERSION_1_1_1) seterrormode(WMS_EXCEPTIONS_XML_1_1_1);
      }
      if (Exceptions.equals("application/vnd.ogc.se_inimage")) {
        seterrormode(WMS_EXCEPTIONS_IMAGE);
      }
      if (Exceptions.equals("application/vnd.ogc.se_blank")) {
        seterrormode(WMS_EXCEPTIONS_BLANKIMAGE);
      }
      if (Exceptions.equals("INIMAGE")) {
        seterrormode(WMS_EXCEPTIONS_IMAGE);
      }
      if (Exceptions.equals("BLANK")) {
        seterrormode(WMS_EXCEPTIONS_BLANKIMAGE);
      }
      if (Exceptions.equals("XML")) {
        if (srvParam->OGCVersion == WMS_VERSION_1_1_1) seterrormode(WMS_EXCEPTIONS_XML_1_1_1);
        if (srvParam->OGCVersion == WMS_VERSION_1_3_0) seterrormode(WMS_EXCEPTIONS_XML_1_3_0);
      }
    } else {
      // EXCEPTIONS not set in request
    }
  }

  if (dErrorOccured == 0) {
    if (CAutoResource::configure(srvParam, false) != 0) {
      CDBError("AutoResource failed");
      return 1;
    }
  }

  // WMS Service
  if (dErrorOccured == 0 && srvParam->serviceType == SERVICE_WMS) {
    // CDBDebug("Entering WMS service");
    if (srvParam->requestType == REQUEST_WMS_GETREFERENCETIMES) {
      int status = process_wms_getreferencetimes_request();
      if (status != 0) {
        CDBError("WMS GetReferenceTimes Request failed");
        return 1;
      }
      return 0;
    }
    if (srvParam->requestType == REQUEST_WMS_GETMAP || srvParam->requestType == REQUEST_WMS_GETLEGENDGRAPHIC) {
      if (dFound_Format == 0) {
        CDBError("ADAGUC Server: Parameter FORMAT missing");
        dErrorOccured = 1;
      } else {

        // Mapping
        CT::string currentFormat = srvParam->Format;
        for (size_t j = 0; j < srvParam->cfg->WMS[0]->WMSFormat.size(); j++) {
          if (currentFormat.equals(srvParam->cfg->WMS[0]->WMSFormat[j]->attr.name.c_str())) {
            if (srvParam->cfg->WMS[0]->WMSFormat[j]->attr.format.empty() == false) {
              srvParam->Format.copy(srvParam->cfg->WMS[0]->WMSFormat[j]->attr.format.c_str());
            }
            if (srvParam->cfg->WMS[0]->WMSFormat[j]->attr.quality.empty() == false) {
              srvParam->imageQuality = srvParam->cfg->WMS[0]->WMSFormat[j]->attr.quality.toInt();
            }
            break;
          }
        }

        // Set format
        // CDBDebug("FORMAT: %s",srvParam->Format.c_str());
        // srvParam->imageFormat=IMAGEFORMAT_IMAGEPNG8;
        CT::string outputFormat = srvParam->Format;
        outputFormat.toLowerCaseSelf();
        if (outputFormat.indexOf("webp") > 0) {
          srvParam->imageFormat = IMAGEFORMAT_IMAGEWEBP;
          srvParam->imageMode = SERVERIMAGEMODE_RGBA;
        } else if (outputFormat.indexOf("32") > 0) {
          srvParam->imageFormat = IMAGEFORMAT_IMAGEPNG32;
          srvParam->imageMode = SERVERIMAGEMODE_RGBA;
        } else if (outputFormat.indexOf("24") > 0) {
          srvParam->imageFormat = IMAGEFORMAT_IMAGEPNG24;
          srvParam->imageMode = SERVERIMAGEMODE_RGBA;
        } else if (outputFormat.indexOf("8bit_noalpha") > 0) {
          srvParam->imageFormat = IMAGEFORMAT_IMAGEPNG8_NOALPHA;
          srvParam->imageMode = SERVERIMAGEMODE_RGBA;
        } else if (outputFormat.indexOf("png8_noalpha") > 0) {
          srvParam->imageFormat = IMAGEFORMAT_IMAGEPNG8_NOALPHA;
          srvParam->imageMode = SERVERIMAGEMODE_RGBA;
        } else if (outputFormat.indexOf("8") > 0) {
          srvParam->imageFormat = IMAGEFORMAT_IMAGEPNG8;
          srvParam->imageMode = SERVERIMAGEMODE_RGBA;
        }
      }
    }

    if (dErrorOccured == 0 && (srvParam->requestType == REQUEST_WMS_GETMAP || srvParam->requestType == REQUEST_WMS_GETFEATUREINFO || srvParam->requestType == REQUEST_WMS_GETPOINTVALUE ||
                               srvParam->requestType == REQUEST_WMS_GETHISTOGRAM

                               )) {

      if (srvParam->requestType == REQUEST_WMS_GETFEATUREINFO || srvParam->requestType == REQUEST_WMS_GETPOINTVALUE || srvParam->requestType == REQUEST_WMS_GETHISTOGRAM) {
        int status = CServerParams::checkDataRestriction();
        if ((status & ALLOW_GFI) == false) {
          CDBError("ADAGUC Server: This layer is not queryable.");
          return 1;
        }
      }
      // Check if styles is defined for WMS 1.1.1
      if (dFound_Styles == 0 && srvParam->requestType == REQUEST_WMS_GETMAP) {
        if (srvParam->OGCVersion == WMS_VERSION_1_1_1) {
          // CDBError("ADAGUC Server: Parameter STYLES missing");TODO Google Earth does not provide this! Disabled this
          // check for the moment.
        }
      }

      if (srvParam->requestType == REQUEST_WMS_GETPOINTVALUE) {
        /*
         * Maps REQUEST_WMS_GETPOINTVALUE to REQUEST_WMS_GETFEATUREINFO
         * SERVICE=WMS&REQUEST=GetPointValue&VERSION=1.1.1&SRS=EPSG%3A4326&QUERY_LAYERS=PMSL_sfc_0&X=3.74&Y=52.34&INFO_FORMAT=text/html&time=2011-08-18T09:00:00Z/2011-08-18T18:00:00Z&DIM_sfc_snow=0
         *
         */

        srvParam->dFound_BBOX = 1;
        dFound_WMSLAYERS = 1;
        dFound_Width = 1;
        dFound_Height = 1;
        srvParam->geoParams.bbox.left = srvParam->dX;
        srvParam->geoParams.bbox.bottom = srvParam->dY;
        srvParam->geoParams.bbox.right = srvParam->geoParams.bbox.left;
        srvParam->geoParams.bbox.top = srvParam->geoParams.bbox.bottom;
        srvParam->geoParams.width = 1;
        srvParam->geoParams.height = 1;
        srvParam->dX = 0;
        srvParam->dY = 0;
        srvParam->requestType = REQUEST_WMS_GETFEATUREINFO;
      }

      if (srvParam->dFound_BBOX == 0) {
        /*
         * TODO enable strict WMS. If bbox is not given, ADAGUC calculates the best fit bbox itself, handy for preview
         * images!!!
         */
        //        CDBError("ADAGUC Server: Parameter BBOX missing");
        //        dErrorOccured=1;
      }

      if (dFound_Width == 0 && dFound_Height == 0) {
        CDBError("ADAGUC Server: Parameter WIDTH or HEIGHT missing");
        dErrorOccured = 1;
      }

      if (dFound_Width == 0) {
        if (srvParam->geoParams.bbox.right != srvParam->geoParams.bbox.left) {
          float r = fabs(srvParam->geoParams.bbox.top - srvParam->geoParams.bbox.bottom) / fabs(srvParam->geoParams.bbox.right - srvParam->geoParams.bbox.left);
          srvParam->geoParams.width = int(float(srvParam->geoParams.height) / r);
          if (srvParam->geoParams.width > MAX_IMAGE_WIDTH) {
            srvParam->geoParams.width = srvParam->geoParams.height;
          }
        } else {
          srvParam->geoParams.width = srvParam->geoParams.height;
        }
      }

      if (dFound_Height == 0) {
        if (srvParam->geoParams.bbox.right != srvParam->geoParams.bbox.left) {
          float r = fabs(srvParam->geoParams.bbox.top - srvParam->geoParams.bbox.bottom) / fabs(srvParam->geoParams.bbox.right - srvParam->geoParams.bbox.left);
          CDBDebug("NNOX = %f, %f, %f, %f", srvParam->geoParams.bbox.left, srvParam->geoParams.bbox.bottom, srvParam->geoParams.bbox.right, srvParam->geoParams.bbox.top);
          CDBDebug("R = %f", r);
          srvParam->geoParams.height = int(float(srvParam->geoParams.width) * r);
          if (srvParam->geoParams.height > MAX_IMAGE_HEIGHT) {
            srvParam->geoParams.height = srvParam->geoParams.width;
          }
        } else {
          srvParam->geoParams.height = srvParam->geoParams.width;
        }
      }

      if (srvParam->geoParams.width < 0) srvParam->geoParams.width = 1;
      if (srvParam->geoParams.height < 0) srvParam->geoParams.height = 1;

      // When error is image, utilize full image size
      setErrorImageSize(srvParam->geoParams.width, srvParam->geoParams.height, srvParam->imageFormat, srvParam->Transparent);

      if (srvParam->OGCVersion == WMS_VERSION_1_0_0 || srvParam->OGCVersion == WMS_VERSION_1_1_1) {
        if (dFound_SRS == 0) {
          CDBError("ADAGUC Server: Parameter SRS missing");
          dErrorOccured = 1;
        }
      }

      if (srvParam->OGCVersion == WMS_VERSION_1_3_0) {
        if (dFound_CRS == 0) {
          CDBError("ADAGUC Server: Parameter CRS missing");
          dErrorOccured = 1;
        }
      }

      if (dFound_WMSLAYERS == 0) {
        CDBError("ADAGUC Server: Parameter LAYERS missing");
        dErrorOccured = 1;
      }
      if (dErrorOccured == 0) {
        if (srvParam->requestType == REQUEST_WMS_GETMAP) {
          int status = process_wms_getmap_request();
          if (status != 0) {
            CDBError("WMS GetMap Request failed");
            return 1;
          }
          return 0;
        }

        if (srvParam->requestType == REQUEST_WMS_GETHISTOGRAM) {
          int status = process_wms_gethistogram_request();
          if (status != 0) {
            CDBError("WMS GetMap Request failed");
            return 1;
          }
          return 0;
        }

        if (srvParam->requestType == REQUEST_WMS_GETFEATUREINFO) {
          if (srvParam->OGCVersion == WMS_VERSION_1_0_0 || srvParam->OGCVersion == WMS_VERSION_1_1_1) {
            if (dFound_X == 0) {
              CDBError("ADAGUC Server: Parameter X missing");
              dErrorOccured = 1;
            }
            if (dFound_Y == 0) {
              CDBError("ADAGUC Server: Parameter Y missing");
              dErrorOccured = 1;
            }
          }

          if (srvParam->OGCVersion == WMS_VERSION_1_3_0) {
            if (dFound_I == 0 && dFound_X == 0) {
              CDBError("ADAGUC Server: Parameter I or X missing");
              dErrorOccured = 1;
            }
            if (dFound_J == 0 && dFound_Y == 0) {
              CDBError("ADAGUC Server: Parameter J or Y missing");
              dErrorOccured = 1;
            }
          }

          int status = process_wms_getfeatureinfo_request();
          if (status != 0) {
            if (status != 2) {
              CDBError("WMS GetFeatureInfo Request failed, status=%d", status);
            }
            return 1;
          }
          return 0;
        }
      }
    }

    // WMS GETSCALEBAR
    if (dErrorOccured == 0 && srvParam->requestType == REQUEST_WMS_GETSCALEBAR) {

      CDrawImage drawImage;

      drawImage.setCanvasColorType(CDRAWIMAGE_COLORTYPE_ARGB);
      drawImage.enableTransparency(true);

      // Set font location
      if (srvParam->cfg->WMS[0]->ContourFont.size() != 0) {
        if (srvParam->cfg->WMS[0]->ContourFont[0]->attr.location.empty() == false) {
          drawImage.setTTFFontLocation(srvParam->cfg->WMS[0]->ContourFont[0]->attr.location.c_str());
          if (srvParam->cfg->WMS[0]->ContourFont[0]->attr.size.empty() == false) {
            CT::string fontSize = "7"; // srvParam->cfg->WMS[0]->ContourFont[0]->attr.size.c_str();
            drawImage.setTTFFontSize(fontSize.toFloat());
          }
        } else {
          CDBError("In <Font>, attribute \"location\" missing");
          return 1;
        }
      }
      drawImage.createImage(300, 30);
      drawImage.create685Palette();
      try {
        CCreateScaleBar::createScaleBar(&drawImage, srvParam->geoParams, 1);
      } catch (int e) {
        CDBError("Exception %d", e);
        return 1;
      }
      drawImage.crop(1);
      const char *cacheControl = srvParam->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_FULLYCACHEABLE).c_str();
      printf("%s%s%c%c\n", "Content-Type:image/png", cacheControl, 13, 10);
      drawImage.printImagePng8(true);
      return 0;
    }

    if (dErrorOccured == 0 && srvParam->requestType == REQUEST_WMS_GETLEGENDGRAPHIC) {
      if (dFound_WMSLAYERS == 0) {
        CDBError("ADAGUC Server: Parameter LAYER missing");
        dErrorOccured = 1;
      }
      if (dErrorOccured == 0) {
        int status = process_wms_getlegendgraphic_request();
        if (status != 0) {
          CDBError("WMS GetLegendGraphic Request failed");
          return 1;
        }
        return 0;
      }
    }

    if (dErrorOccured == 0 && srvParam->requestType == REQUEST_WMS_GETCAPABILITIES) {
      int status = process_wms_getcap_request();
      if (status != 0) {
        CDBError("GetCapabilities failed");
      }
      return status;
    }
    if (dErrorOccured == 0 && srvParam->requestType == REQUEST_WMS_GETMETADATA) {
      int status = CServerParams::checkDataRestriction();
      if ((status & ALLOW_METADATA) == false) {
        CDBError("ADAGUC Server: GetMetaData is restricted");
        return 1;
      }

      if (srvParam->Format.equals("application/json")) {
        // GetMetadata for specific dataset and layer
        json result;
        traceTimingsSpanStart(TraceTimingType::GETMETADATAJSON);
        getLayerMetadataAsJson(srvParam, result);
        traceTimingsSpanEnd(TraceTimingType::GETMETADATAJSON);
        auto headers = srvParam->getResponseHeaders(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE);
        printf("%s%s%c%c\n", "Content-Type: application/json", headers.c_str(), 13, 10);
        printf("%s", result.dump().c_str());
        return 0;
      }

      if (dFound_WMSLAYERS == 0) {
        CDBError("ADAGUC Server: Parameter LAYER missing");
        dErrorOccured = 1;
      }
      if (dFound_Format == 0) {
        CDBError("ADAGUC Server: Parameter FORMAT missing");
        dErrorOccured = 1;
      }
      if (dErrorOccured == 0) {
        int status = process_wms_getmetadata_request();
        if (status != 0) {
          CDBError("WMS GetMetaData Request failed");
          return 1;
        }
      }
      return 0;
    }
  }

  if (dErrorOccured == 0 && srvParam->serviceType == SERVICE_WCS) {
    srvParam->OGCVersion = WCS_VERSION_1_0;
    int status = CServerParams::checkDataRestriction();
    if ((status & ALLOW_WCS) == false) {
      CDBError("ADAGUC Server: WCS Service is disabled.");
      return 1;
    }
    if (dFound_Request == 0) {
      dErrorOccured = 1;
      setStatusCode(HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY);
      CDBError("ADAGUC Server: Parameter REQUEST missing");
      return 1;
    } else {
      if (REQUEST.equals("GETCAPABILITIES"))
        srvParam->requestType = REQUEST_WCS_GETCAPABILITIES;
      else if (REQUEST.equals("DESCRIBECOVERAGE"))
        srvParam->requestType = REQUEST_WCS_DESCRIBECOVERAGE;
      else if (REQUEST.equals("GETCOVERAGE"))
        srvParam->requestType = REQUEST_WCS_GETCOVERAGE;
      else {
        dErrorOccured = 1;
        setStatusCode(HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY);
        CDBError("ADAGUC Server: Parameter REQUEST invalid");
        return 1;
      }
    }

    if (dErrorOccured == 0 && srvParam->requestType == REQUEST_WCS_DESCRIBECOVERAGE) {
      if (dErrorOccured == 0) {
        int status = process_wcs_describecov_request();
        if (status != 0) {
          CDBError("WCS DescribeCoverage Request failed");
        }
      }
      return 0;
    }
    if (dErrorOccured == 0 && srvParam->requestType == REQUEST_WCS_GETCOVERAGE) {

      if (dFound_Width == 0 && dFound_Height == 0 && dFound_RESX == 0 && dFound_RESY == 0 && srvParam->dFound_BBOX == 0 && dFound_CRS == 0)
        srvParam->WCS_GoNative = 1;
      else {
        if (dFound_CRS == 1 && dFound_RESPONSE_CRS == 1) {
          CT::string CRS = srvParam->responceCrs;
          CT::string RESPONSE_CRS = srvParam->geoParams.crs;
          srvParam->geoParams.crs = CRS;
          srvParam->responceCrs = RESPONSE_CRS;
        } else {
          srvParam->responceCrs = srvParam->geoParams.crs;
        }
        srvParam->WCS_GoNative = 0;
        if (srvParam->dFound_BBOX == 0) {
          CDBError("ADAGUC Server: Parameter BBOX missing");
          dErrorOccured = 1;
        }
        if (dFound_CRS == 0) {
          CDBError("ADAGUC Server: Parameter CRS missing");
          dErrorOccured = 1;
        }
        if (dFound_Format == 0) {
          CDBError("ADAGUC Server: Parameter FORMAT missing");
          dErrorOccured = 1;
        }
        if (dFound_WCSCOVERAGE == 0) {
          CDBError("ADAGUC Server: Parameter COVERAGE missing");
          dErrorOccured = 1;
        }
      }

      if (dErrorOccured == 0) {
        int status = process_wcs_getcoverage_request();
        if (status != 0) {
          CDBError("WCS GetCoverage Request failed");
          return 1;
        }
      }
      if (dErrorOccured != 0) {
        return 1;
      }
      return 0;
    }
    if (dErrorOccured == 0 && srvParam->requestType == REQUEST_WCS_GETCAPABILITIES) {
      return process_wcs_getcap_request();
    }
  }

  if (dErrorOccured == 0 && srvParam->serviceType == SERVICE_METADATA) {
    if (REQUEST.equals("GETMETADATA")) srvParam->requestType = REQUEST_METADATA_GETMETADATA;
    if (srvParam->autoResourceLocation.empty()) {
      CDBError("No source defined for metadata request");
      dErrorOccured = 1;
    }
    if (dErrorOccured == 0 && srvParam->requestType == REQUEST_METADATA_GETMETADATA) {
      return CHandleMetadata().process(srvParam);
    }
  }
  // An error occured, stopping..
  if (dErrorOccured == 1) {
    return 1;
  }
  if (srvParam->serviceType == SERVICE_WCS) {
    CDBError("ADAGUC Server: Invalid value for request. Supported requests are: getcapabilities, describecoverage and "
             "getcoverage");
    setStatusCode(HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY);
    return 1;
  } else if (srvParam->serviceType == SERVICE_WMS) {
    CDBError("ADAGUC Server: Invalid value for request. Supported requests are: getcapabilities, getmap, gethistogram, "
             "getfeatureinfo, getpointvalue, getmetadata, getReferencetimes, getstyles and getlegendgraphic");
    setStatusCode(HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY);
    return 1;
  } else {
    CDBError("ADAGUC Server: Unknown service");
    setStatusCode(HTTP_STATUSCODE_422_UNPROCESSABLE_ENTITY);
    return 1;
  }
#ifdef MEASURETIME
  StopWatch_Stop("End of query string");
#endif

  return 0;
}

int CRequest::updatedb(CT::string tailPath, CT::string layerPathToScan, int scanFlags, CT::string layerName) {
  int errorHasOccured = 0;
  int status;
  // Fill in all data sources from the configuration object
  size_t numberOfLayers = srvParam->cfg->Layer.size();

  for (size_t layerNo = 0; layerNo < numberOfLayers; layerNo++) {
    if (!layerPathToScan.empty()) {
      if (!checkIfFileMatchesLayer(layerPathToScan, srvParam->cfg->Layer[layerNo])) {
        continue;
      }
    }
    CDataSource *dataSource = new CDataSource();
    auto cfgLayer = srvParam->cfg->Layer[layerNo];
    if (dataSource->setCFGLayer(srvParam, srvParam->configObj->Configuration[0], cfgLayer, NULL, layerNo) != 0) {
      delete dataSource;
      return 1;
    }
    if (!layerName.empty()) {
      if (cfgLayer->Name.size() == 1) {
        CT::string simpleLayerName = cfgLayer->Name[0]->value;
        if (layerName.equals(simpleLayerName)) {
          dataSources.push_back(dataSource);
        }
      }
    } else {
      dataSources.push_back(dataSource);
    }
  }

  if (dataSources.size() == 0) {
    if (!layerPathToScan.empty()) {
      CDBWarning("The specified file %s did not match to any of the layers", layerPathToScan.c_str());
    } else {
      CDBWarning("No layers found");
    }
  }
  srvParam->requestType = REQUEST_UPDATEDB;

  for (size_t j = 0; j < dataSources.size(); j++) {
    if (dataSources[j]->dLayerType == CConfigReaderLayerTypeDataBase || dataSources[j]->dLayerType == CConfigReaderLayerTypeBaseLayer) {
      if (scanFlags & CDBFILESCANNER_UPDATEDB) {
        status = CDBFileScanner::updatedb(dataSources[j], tailPath, layerPathToScan, scanFlags);
      }
      if (scanFlags & CDBFILESCANNER_CREATETILES) {
        status = CCreateTiles::createTiles(dataSources[j], scanFlags);
      }
      if (status != CDBFILESCANNER_RETURN_FILEDOESNOTMATCH && status != 0) {
        CDBError("Could not update db for: %s", dataSources[j]->cfgLayer->Name[0]->value.c_str());
        errorHasOccured++;
      }
    }
  }
  if (errorHasOccured) {
    CDBDebug("***** Finished DB Update with %d errors *****\n\n", errorHasOccured);
  } else {
    CDBDebug("***** Finished DB Update *****\n\n");
  }
  // TODO: 2024-09-20: Probably not the right place to clear these
  CDFObjectStore::getCDFObjectStore()->clear();
  CConvertGeoJSON::clearFeatureStore();
  CDFStore::clear();

  return errorHasOccured > 0;
}

// pthread_mutex_t CImageDataWriter_addData_lock;
void *CImageDataWriter_addData(void *arg) {
  CImageDataWriter_addData_args *imgdwArg = (CImageDataWriter_addData_args *)arg;
  imgdwArg->status = imgdwArg->imageDataWriter->addData(imgdwArg->dataSources);
  imgdwArg->finished = true;
  imgdwArg->running = false;
  return NULL;
}

void CRequest::autoDetectBBOX() {
  if (srvParam->requestType == REQUEST_WMS_GETMAP && srvParam->dFound_BBOX == 0) {
    int found;
    f8box bbox;
    std::tie(found, bbox) = findBBoxForDataSource(dataSources);
    if (found == 0) {
      srvParam->geoParams.bbox = bbox;
      srvParam->dFound_BBOX = 1;
    }
  }
}

int CRequest::determineTypesForDataSources() {
  for (size_t j = 0; j < dataSources.size(); j++) {

    if (dataSources[j]->dLayerType == CConfigReaderLayerTypeUnknown) {
      CDBError("Unknow layer type");
      return 0;
    }

    /***************************/
    /* Type = Database layer   */
    /***************************/
    if (dataSources[j]->dLayerType == CConfigReaderLayerTypeDataBase || dataSources[j]->dLayerType == CConfigReaderLayerTypeBaseLayer) {

      if (dataSources[j]->cfgLayer->Dimension.size() == 0) {

        if (CAutoConfigure::autoConfigureDimensions(dataSources[j]) != 0) {
          CDBError("Unable to configure dimensions automatically");
          return 1;
        }
      }
      if (dataSources[j]->cfgLayer->Dimension.size() != 0) {
        try {
          if (setDimValuesForDataSource(dataSources[j], srvParam) != 0) {
            CDBError("Error in setDimValuesForDataSource: Unable to find data for layer %s", dataSources[j]->layerName.c_str());
            return 1;
          }
        } catch (ServiceExceptionCode e) {
          CDBError("Exception in setDimValuesForDataSource for layer %s with ServiceExceptionCode=%d", dataSources[j]->layerName.c_str(), e);
          setExceptionType(e);
          return 1;
        }
      } else {
        CDBDebug("Layer has no dims");
        // This layer has no dimensions, but we need to add one timestep with data in order to make the next code work.

        std::vector<std::string> fileList;
        try {
          fileList = CDBFileScanner::searchFileNames(dataSources[j]->cfgLayer->FilePath[0]->value.c_str(), dataSources[j]->cfgLayer->FilePath[0]->attr.filter, NULL);
        } catch (int linenr) {
          CDBError("Could not find any filename");
          return 1;
        }

        if (fileList.size() == 0) {
          CDBError("fileList.size()==0");
          return 1;
        }

        CDBDebug("Addstep");
        dataSources[j]->addStep(fileList[0].c_str());
        // dataSources[j]->getCDFDims()->addDimension("none","0",0);
      }
    }

    if (dataSources[j]->dLayerType == CConfigReaderLayerTypeGraticule) {
      // This layer has no dimensions, but we need to add one timestep with data in order to make the next code work.
      CDBDebug("Addstep");
      dataSources[j]->addStep("");
      // dataSources[j]->getCDFDims()->addDimension("none","0",0);
    }
    if (dataSources[j]->dLayerType == CConfigReaderLayerTypeLiveUpdate) {
      // This layer has no dimensions, but we need to add one timestep with data in order to make the next code work.
      layerTypeLiveUpdateConfigureDimensionsInDataSource(dataSources[j]);
    }
  }
  return 0;
}

int CRequest::addDataSources(CServerConfig::XMLE_Layer *cfgLayer, int layerIndex) {
  CT::string layerName = makeUniqueLayerName(cfgLayer);
  CDataSource *dataSource = new CDataSource();

  dataSources.push_back(dataSource);

  if (dataSource->setCFGLayer(srvParam, srvParam->configObj->Configuration[0], cfgLayer, layerName.c_str(), layerIndex) != 0) {
    return 1;
  }
  if (srvParam->requestType != REQUEST_WMS_GETMAP) {
    return 0;
  }

  // Check if layer has an additional layer
  for (size_t additionalLayerNr = 0; additionalLayerNr < cfgLayer->AdditionalLayer.size(); additionalLayerNr++) {
    CServerConfig::XMLE_AdditionalLayer *additionalLayer = cfgLayer->AdditionalLayer[additionalLayerNr];
    bool replacePreviousDataSource = false;
    bool replaceAllDataSource = false;

    if (additionalLayer->attr.replace.equals("true") || additionalLayer->attr.replace.equals("previous")) {
      replacePreviousDataSource = true;
    }
    if (additionalLayer->attr.replace.equals("all")) {
      replaceAllDataSource = true;
    }

    CT::string additionalLayerName = additionalLayer->value.c_str();
    size_t additionalLayerNo = 0;
    for (additionalLayerNo = 0; additionalLayerNo < srvParam->cfg->Layer.size(); additionalLayerNo++) {
      CT::string additional = makeUniqueLayerName(srvParam->cfg->Layer[additionalLayerNo]);
      // CDBDebug("comparing for additionallayer %s==%s", additionalLayerName.c_str(), additional.c_str());
      if (additionalLayerName.equals(additional)) {
        // CDBDebug("Found additionalLayer [%s]", additional.c_str());
        CDataSource *additionalDataSource = new CDataSource();

        // CDBDebug("setCFGLayer for additionallayer %s", additionalLayerName.c_str());
        if (additionalDataSource->setCFGLayer(srvParam, srvParam->configObj->Configuration[0], srvParam->cfg->Layer[additionalLayerNo], additionalLayerName.c_str(), layerIndex) != 0) {
          delete additionalDataSource;
          return 1;
        }

        /* Configure the Dimensions object if not set. */
        if (additionalDataSource->cfgLayer->Dimension.size() == 0) {
          // CDBDebug("additionalDataSource: Dimensions not configured, trying to do now");
          if (CAutoConfigure::autoConfigureDimensions(additionalDataSource) != 0) {
            CDBError("additionalDataSource: : setCFGLayer::Unable to configure dimensions automatically");
          }
          // else {
          //   for (size_t j = 0; j < additionalDataSource->cfgLayer->Dimension.size(); j++) {
          //     CDBDebug("additionalDataSource: : Configured dim %d %s", j, additionalDataSource->cfgLayer->Dimension[j]->value.c_str());
          //   }
          // }
        }

        /* Set the dims based on server parameters */
        try {
          CRequest::fillDimValuesForDataSource(additionalDataSource, additionalDataSource->srvParams);
        } catch (ServiceExceptionCode e) {
          CDBError("additionalDataSource: Exception in setDimValuesForDataSource");
          return 1;
        }
        bool add = true;

        CDataSource *checkForData = additionalDataSource->clone();

        try {
          if (setDimValuesForDataSource(checkForData, srvParam) != 0) {
            CDBDebug("setDimValuesForDataSource for additionallayer %s failed", additionalLayerName.c_str());
            add = false;
          }
        } catch (ServiceExceptionCode e) {
          CDBDebug("setDimValuesForDataSource for additionallayer %s failed", additionalLayerName.c_str());
          add = false;
        }
        delete checkForData;

        // CDBDebug("add = %d replaceAllDataSource = %d replacePreviousDataSource = %d", add, replaceAllDataSource, replacePreviousDataSource);
        if (add) {
          if (replaceAllDataSource) {
            for (size_t j = 0; j < dataSources.size(); j++) {
              delete dataSources[j];
            }
            dataSources.clear();
          } else {
            if (replacePreviousDataSource) {
              if (dataSources.size() > 0) {
                delete dataSources.back();
                dataSources.pop_back();
              }
            }
          }
          if (additionalLayer->attr.style.empty() == false) {
            additionalDataSource->setStyle(additionalLayer->attr.style.c_str());
          } else {
            additionalDataSource->setStyle("default");
          }
          dataSources.push_back(additionalDataSource);
        } else {
          delete additionalDataSource;
        }

        break;
      }
    } /* End of looping additionallayers */
  }
  return 0;
}

int CRequest::handleGetMapRequest(CDataSource *firstDataSource) {
  int status = 0;
  if (srvParam->requestType == REQUEST_WMS_GETMAP) {

    CImageDataWriter imageDataWriter;

    /**
      We want like give priority to our own internal layers, instead to external cascaded layers. This is because
      our internal layers have an exact customized legend, and we would like to use this always.
    */

    bool imageDataWriterIsInitialized = false;
    int dataSourceToUse = 0;
    for (size_t d = 0; d < dataSources.size() && imageDataWriterIsInitialized == false; d++) {
      if (dataSources[d]->dLayerType != CConfigReaderLayerTypeGraticule) {
        // CDBDebug("INIT");
        status = imageDataWriter.init(srvParam, dataSources[d], dataSources[d]->getNumTimeSteps());
        if (status != 0) throw(__LINE__);
        imageDataWriterIsInitialized = true;
        dataSourceToUse = d;
      }
    }

    // There are only cascaded layers, so we intialize the imagedatawriter with this the first layer.
    if (imageDataWriterIsInitialized == false) {
      status = imageDataWriter.init(srvParam, dataSources[0], dataSources[0]->getNumTimeSteps());
      if (status != 0) throw(__LINE__);
      dataSourceToUse = 0;
      imageDataWriterIsInitialized = true;
    }
    bool measurePerformance = false;

    bool useThreading = false;
    int numThreads = 4;
    if (dataSources[dataSourceToUse]->cfgLayer->TileSettings.size() == 1) {
      if (dataSources[dataSourceToUse]->cfgLayer->TileSettings[0]->attr.threads.empty() == false) {
        numThreads = dataSources[dataSourceToUse]->cfgLayer->TileSettings[0]->attr.threads.toInt();
        if (numThreads <= 1) {
          useThreading = false;
        } else {
          useThreading = true;
        }
        // measurePerformance = true;
      }
    }
    if (measurePerformance) {
      StopWatch_Stop("Start imagewarper");
    }
    if (useThreading) {
      size_t numTimeSteps = (size_t)dataSources[dataSourceToUse]->getNumTimeSteps();

      int errcode;
      pthread_t threads[numThreads];

      CImageDataWriter_addData_args args[numThreads];
      for (int worker = 0; worker < numThreads; worker++) {

        for (size_t d = 0; d < dataSources.size(); d++) {
          args[worker].dataSources.push_back(dataSources[d]->clone());
          ;
        }
        args[worker].imageDataWriter = &imageDataWriter;
        args[worker].finished = false;
        args[worker].running = false;
        args[worker].used = false;
      }

      for (size_t k = 0; k < numTimeSteps; k = k + 1) {

        if (firstDataSource->dLayerType == CConfigReaderLayerTypeDataBase || firstDataSource->dLayerType == CConfigReaderLayerTypeGraticule ||
            firstDataSource->dLayerType == CConfigReaderLayerTypeBaseLayer) {
          bool OK = false;
          while (OK == false) {
            for (int worker = 0; worker < numThreads && OK == false; worker++) {
              if (args[worker].running == false) {
                args[worker].running = true;
                args[worker].used = true;

                args[worker].dataSources[dataSourceToUse]->setTimeStep(k);
                for (size_t d = 0; d < args[worker].dataSources.size(); d++) {
                  args[worker].dataSources[d]->threadNr = worker;
                }

                errcode = pthread_create(&threads[worker], NULL, CImageDataWriter_addData, &args[worker]);
                if (errcode) {
                  CDBError("pthread_create");
                  return 1;
                }

                if (measurePerformance) {
                  StopWatch_Stop("Started thread %d for timestep %d", worker, k);
                }
                OK = true;
                break;
              }
            }
            if (OK == false) {
              usleep(1000);
            }
          }
        }
      }
      if (measurePerformance) {
        StopWatch_Stop("All submitted");
      }
      for (int worker = 0; worker < numThreads; worker++) {
        if (args[worker].used) {
          args[worker].used = false;
          errcode = pthread_join(threads[worker], (void **)&status);
          if (errcode) {
            CDBError("pthread_join");
            return 1;
          }
        }
      }
      if (measurePerformance) {
        StopWatch_Stop("All done");
      }
      for (int worker = 0; worker < numThreads; worker++) {
        for (size_t j = 0; j < args[worker].dataSources.size(); j++) {
          delete args[worker].dataSources[j];
        }
        args[worker].dataSources.clear();
      }
      if (measurePerformance) {
        StopWatch_Stop("All deleted");
      }
    } else {
      /*Standard non threading functionality */
      for (size_t k = 0; k < (size_t)dataSources[dataSourceToUse]->getNumTimeSteps(); k++) {
        for (size_t d = 0; d < dataSources.size(); d++) {
          dataSources[d]->setTimeStep(k);
        }
        if (firstDataSource->dLayerType == CConfigReaderLayerTypeDataBase || firstDataSource->dLayerType == CConfigReaderLayerTypeGraticule ||
            firstDataSource->dLayerType == CConfigReaderLayerTypeBaseLayer) {

          status = imageDataWriter.addData(dataSources);
          if (status != 0) {
            /**
             * Adding data failed:
             * Do not ruin an animation if one timestep fails to load.
             * If there is a single time step then throw an exception otherwise continue.
             */
            if (dataSources[dataSourceToUse]->getNumTimeSteps() == 1) {
              // Not an animation, so throw an exception
              CDBError("Unable to convert datasource %s to image", firstDataSource->layerName.c_str());
              throw(__LINE__);
            } else {
              // This is an animation, report an error and continue with adding images.
              CDBError("Unable to load datasource %s at line %d", dataSources[dataSourceToUse]->getDataObject(0)->variableName.c_str(), __LINE__);
            }
          }
        }
        if (dataSources[dataSourceToUse]->getNumTimeSteps() > 1 && dataSources[dataSourceToUse]->queryBBOX == false) {
          // Print the animation data into the image
          char szTemp[1024];
          snprintf(szTemp, 1023, "%s UTC", dataSources[dataSourceToUse]->getDimensionValueForNameAndStep("time", k).c_str());
          imageDataWriter.setDate(szTemp);
        }
      }
    }
    if (measurePerformance) {
      StopWatch_Stop("Finished imagewarper");
    }

    CColor textBGColor = CColor(255, 255, 255, 0); /* TODO: 2021-01-12, Maarten Plieger: Should make the text background configurable */

    double scaling = dataSources[dataSourceToUse]->getScaling();
    int textY = (int)(scaling * 6);
    // int prevTextY=0;
    if (srvParam->mapTitle.length() > 0) {
      if (srvParam->cfg->WMS[0]->TitleFont.size() > 0) {
        float fontSize = parseFloat(srvParam->cfg->WMS[0]->TitleFont[0]->attr.size.c_str());
        /* Check if scaling in relation to a reference width/height is needed */
        fontSize = fontSize * scaling;
        textY += int(fontSize);
        textY += imageDataWriter.drawImage.drawTextArea((int)(scaling * 6), textY, srvParam->cfg->WMS[0]->TitleFont[0]->attr.location.c_str(), fontSize, 0, srvParam->mapTitle.c_str(),
                                                        CColor(0, 0, 0, 255), textBGColor);
        // textY+=12;
      }
    }
    if (srvParam->mapSubTitle.length() > 0) {
      if (srvParam->cfg->WMS[0]->SubTitleFont.size() > 0) {
        float fontSize = parseFloat(srvParam->cfg->WMS[0]->SubTitleFont[0]->attr.size.c_str());
        fontSize = fontSize * scaling;
        // textY+=int(fontSize)/5;
        textY += imageDataWriter.drawImage.drawTextArea((int)(scaling * 6), textY, srvParam->cfg->WMS[0]->SubTitleFont[0]->attr.location.c_str(), fontSize, 0, srvParam->mapSubTitle.c_str(),
                                                        CColor(0, 0, 0, 255), textBGColor);
        // textY+=8;
      }
    }

    if (srvParam->showDimensionsInImage) {
      textY += 4 * (int)scaling;
      CDataSource *dataSource = dataSources[dataSourceToUse];
      size_t nDims = dataSource->requiredDims.size();

      for (size_t d = 0; d < nDims; d++) {
        CT::string message;
        float fontSize = parseFloat(srvParam->cfg->WMS[0]->DimensionFont[0]->attr.size.c_str());
        fontSize = fontSize * scaling;
        textY += int(fontSize * 1.2);
        message.print("%s: %s", dataSource->requiredDims[d]->name.c_str(), dataSource->requiredDims[d]->value.c_str());
        imageDataWriter.drawImage.drawText(6, textY, srvParam->cfg->WMS[0]->DimensionFont[0]->attr.location.c_str(), fontSize, 0, message.c_str(), CColor(0, 0, 0, 255), textBGColor);
        textY += 4 * (int)scaling;
      }
    }

    if (!srvParam->showLegendInImage.equals("false") && !srvParam->showLegendInImage.empty()) {
      // Draw legend
      bool drawAllLegends = srvParam->showLegendInImage.equals("true");

      /* List of specified legends */
      std::vector<CT::string> legendLayerList = srvParam->showLegendInImage.split(",");

      //          int numberOfLegendsDrawn = 0;
      int legendOffsetX = 0;
      for (size_t d = 0; d < dataSources.size(); d++) {
        if (dataSources[d]->dLayerType != CConfigReaderLayerTypeGraticule) {
          bool drawThisLegend = false;

          if (!drawAllLegends) {
            for (size_t li = 0; li < legendLayerList.size(); li++) {
              CDBDebug("Comparing [%s] == [%s]", dataSources[d]->layerName.c_str(), legendLayerList[li].c_str());
              if (dataSources[d]->layerName.toLowerCase().equals(legendLayerList[li])) {
                drawThisLegend = true;
              }
            }
          } else {
            if (!dataSources[d]->cfgLayer->attr.hidden.equals("true")) {
              drawThisLegend = true;
            }
          }
          if (drawThisLegend) {
            /* Draw the legend */
            int padding = 4;
            int minimumLegendWidth = 100;
            CDrawImage legendImage;
            int legendWidth = LEGEND_WIDTH * scaling;
            if (legendWidth < minimumLegendWidth) legendWidth = minimumLegendWidth;
            imageDataWriter.drawImage.enableTransparency(true);
            legendImage.createImage(&imageDataWriter.drawImage, legendWidth, (imageDataWriter.drawImage.geoParams.height / 3) - padding * 2 + 2);

            CStyleConfiguration *styleConfiguration = dataSources[d]->getStyle();
            if (styleConfiguration != NULL && styleConfiguration->legendIndex != -1) {
              legendImage.createPalette(srvParam->cfg->Legend[styleConfiguration->legendIndex]);
            }

            status = imageDataWriter.createLegend(dataSources[d], &legendImage);
            if (status != 0) throw(__LINE__);
            // legendImage.rectangle(0,0,10000,10000,240);
            int posX = imageDataWriter.drawImage.geoParams.width - (legendImage.geoParams.width + padding) - legendOffsetX;
            // int posY=imageDataWriter.drawImage.Geo.dHeight-(legendImage.Geo.dHeight+padding);
            // int posX=padding*scaling;//imageDataWriter.drawImage.Geo.dWidth-(scaleBarImage.Geo->dWidth+padding);
            int posY = imageDataWriter.drawImage.geoParams.height - (legendImage.geoParams.height + padding * scaling);
            imageDataWriter.drawImage.draw(posX, posY, 0, 0, &legendImage);
            //                numberOfLegendsDrawn++;
            legendOffsetX += legendImage.geoParams.width + padding;
          }
        }
      }
    }

    if (srvParam->showScaleBarInImage) {
      // Draw legend

      int padding = 4;

      CDrawImage scaleBarImage;

      imageDataWriter.drawImage.enableTransparency(true);
      // scaleBarImage.setBGColor(1,0,0);

      scaleBarImage.createImage(&imageDataWriter.drawImage, 200 * scaling, 30 * scaling);

      // scaleBarImage.rectangle(0,0,scaleBarImage.Geo->dWidth,scaleBarImage.Geo->dHeight,CColor(0,0,0,0),CColor(0,0,0,255));
      status = imageDataWriter.createScaleBar(dataSources[0]->srvParams->geoParams, &scaleBarImage, scaling);
      if (status != 0) throw(__LINE__);
      int posX = padding * scaling; // imageDataWriter.drawImage.Geo.dWidth-(scaleBarImage.Geo->dWidth+padding);
      int posY = imageDataWriter.drawImage.geoParams.height - (scaleBarImage.geoParams.height + padding * scaling);
      // posY-=50;
      // imageDataWriter.drawImage.rectangle(posX,posY,scaleBarImage.Geo->dWidth+posX+1,scaleBarImage.Geo->dHeight+posY+1,CColor(255,255,255,180),CColor(255,255,255,0));
      imageDataWriter.drawImage.draw(posX, posY, 0, 0, &scaleBarImage);
    }

    if (srvParam->showNorthArrow) {
    }
    status = imageDataWriter.end();
    if (status != 0) throw(__LINE__);
    fclose(stdout);
  }
  return status;
}

int CRequest::handleGetCoverageRequest(CDataSource *firstDataSource) {
  int status = 0;

  if (srvParam->requestType != REQUEST_WCS_GETCOVERAGE) {
    return status;
  }

  CBaseDataWriterInterface *wcsWriter = NULL;
  CT::string driverName = "ADAGUCNetCDF";
  setDimValuesForDataSource(firstDataSource, srvParam);

  for (const auto &WCSFormat : srvParam->cfg->WCS[0]->WCSFormat) {
    if (srvParam->Format.equals(WCSFormat->attr.name.c_str())) {
      driverName.copy(WCSFormat->attr.driver.c_str());
      break;
    }
  }

  if (driverName.equals("ADAGUCNetCDF")) {
    CDBDebug("Creating CNetCDFDataWriter");
    wcsWriter = new CNetCDFDataWriter();
  }

#ifdef ADAGUC_USE_GDAL
  if (wcsWriter == NULL) {
    wcsWriter = new CGDALDataWriter();
  }
#endif
  if (wcsWriter == NULL) {
    CDBError("No WCS Writer found");
    return 1;
  }
  try {
    try {
      for (auto dataSource : dataSources) {
        status = wcsWriter->init(srvParam, dataSource, firstDataSource->getNumTimeSteps());
        if (status != 0) throw(__LINE__);
      }
    } catch (int e) {
      CDBError("Exception code %d", e);

      throw(__LINE__);
    }

    for (int k = 0; k < firstDataSource->getNumTimeSteps(); k++) {
      for (auto dataSource : dataSources) {
        dataSource->setTimeStep(k);
      }

      try {
        status = wcsWriter->addData(dataSources);
      } catch (int e) {
        CDBError("Exception code %d", e);
        throw(__LINE__);
      }
      if (status != 0) throw(__LINE__);
    }
    try {
      status = wcsWriter->end();
      if (status != 0) throw(__LINE__);
    } catch (int e) {
      CDBError("Exception code %d", e);
      throw(__LINE__);
    }
  } catch (int line) {
    CDBDebug("%d", line);
    delete wcsWriter;
    wcsWriter = NULL;
    throw(__LINE__);
  }

  delete wcsWriter;
  wcsWriter = NULL;
  return status;
}