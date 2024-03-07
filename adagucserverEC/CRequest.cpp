
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
const char *CRequest::className = "CRequest";
int CRequest::CGI = 0;

// Entry point for all runs
int CRequest::runRequest() {
  int status = process_querystring();
  CDFObjectStore::getCDFObjectStore()->clear();
  CConvertGeoJSON::clearFeatureStore();
  CDFStore::clear();
  ProjectionStore::getProjectionStore()->clear();
  CDBFactory::clear();
  return status;
}

void writeLogFile3(const char *msg) {
  char *logfile = getenv("ADAGUC_LOGFILE");
  if (logfile == NULL) {
    return;
  }

  FILE *pFile = fopen(logfile, "a");
  if (pFile == NULL) {
    return;
  }

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
  // else CDBError("Unable to write logfile %s",logfile);
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
  CT::StackList<CT::string> configFileList = configFile.splitToStack(",");

  // Parse the main configuration file
  int status = srvParam->parseConfigFile(configFileList[0]);

  if (status != 0 || srvParam->configObj->Configuration.size() != 1) {
    srvParam->cfg = NULL;
    CDBError("Invalid XML file %s", pszConfigFile);
    return 1;
  }

  srvParam->configFileName.copy(pszConfigFile);
  srvParam->cfg = srvParam->configObj->Configuration[0];

  // Include additional config files given as argument
  if (configFileList.size() > 1) {
    for (size_t j = 1; j < configFileList.size() - 1; j++) {
      // CDBDebug("Include '%s'",configFileList[j].c_str());
      status = srvParam->parseConfigFile(configFileList[j]);
      if (status != 0) {
        CDBError("There is an error with include '%s'", configFileList[j].c_str());
        return 1;
      }
    }

    // The last configration file is considered the dataset one, strip path and extension and give it to configurer
    srvParam->datasetLocation.copy(configFileList[configFileList.size() - 1].basename().c_str());
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

  const char *pszQueryString = getenv("QUERY_STRING");
  if (pszQueryString != NULL) {
    CT::string queryString(pszQueryString);
    queryString.decodeURLSelf();
    CT::string *parameters = queryString.splitToArray("&");
    for (size_t j = 0; j < parameters->count; j++) {
      CT::string value0Cap;
      CT::string values[2];
      int equalPos = parameters[j].indexOf("="); // splitToArray("=");
      if (equalPos != -1) {
        values[0] = parameters[j].substring(0, equalPos);
        values[1] = parameters[j].c_str() + equalPos + 1;
        values[0].count = 2;
      } else {
        values[0] = parameters[j].c_str();
        values[1] = "";
        values[0].count = 1;
      }
      value0Cap.copy(&values[0]);
      value0Cap.toUpperCaseSelf();
      if (value0Cap.equals("DATASET") && srvParam->datasetLocation.empty()) {
        srvParam->datasetLocation.copy(values[1].c_str());
        status = CAutoResource::configureDataset(srvParam, false);
        if (status != 0) {
          CDBError("CAutoResource::configureDataset failed");
          return status;
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
        status = csld.processSLDUrl(values[1]);

        if (status != 0) {
          CDBError("Processing SLD failed");
          return status;
        }
      }
    }
    delete[] parameters;
  }

  // Include additional config files given in the include statement of the config file
  // Last config file is included first
  size_t includeSize = srvParam->cfg->Include.size();
  for (size_t j = 0; j < includeSize; j++) {
    if (srvParam->cfg->Include[j]->attr.location.empty() == false) {
      int index = (includeSize - 1) - j;
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

  if (srvParam->cfg->CacheDocs.size() == 1) {
    srvParam->enableDocumentCache = srvParam->cfg->CacheDocs[0]->attr.enabled.equals("true");
  }

#ifdef MEASURETIME
  StopWatch_Stop("Config file parsed");
#endif

  // Check for mandatory attributes
  for (size_t j = 0; j < srvParam->cfg->Layer.size(); j++) {
    if (srvParam->cfg->Layer[j]->attr.type.equals("database")) {
      if (srvParam->cfg->Layer[j]->Variable.size() == 0) {
        CDBError("Configuration error at layer %d: <Variable> not defined", j);
        return 1;
      }
      if (srvParam->cfg->Layer[j]->FilePath.size() == 0) {
        CDBError("Configuration error at layer %d: <FilePath> not defined", j);
        return 1;
      }
    }
  }

  // Check for autoscan elements
  for (size_t j = 0; j < srvParam->cfg->Layer.size(); j++) {
    auto &layer = srvParam->cfg->Layer[j];

    if (!layer->attr.type.equals("autoscan")) {
      continue; // Do nothing if layer not of type autoscam
    }

    if (layer->FilePath.size() == 0) {
      CDBError("Configuration error at layer %d: <FilePath> not defined", j);
      return 1;
    }

    // bool layerConfigCacheAvailable = false;
    try {
      /* Create the list of layers from a directory list */
      const char *baseDir = layer->FilePath[0]->value.c_str();

      CDBDebug("autoscan");
      std::vector<std::string> fileList;
      try {
        fileList = CDBFileScanner::searchFileNames(baseDir, layer->FilePath[0]->attr.filter.c_str(), NULL);
      } catch (int linenr) {
        CDBError("Could not find any file in directory '%s'", baseDir);
        throw(__LINE__);
      }

      if (fileList.size() == 0) {
        CDBError("Could not find any file in directory '%s'", baseDir);
        throw(__LINE__);
      }
      size_t nrOfFileErrors = 0;
      for (const auto &fileName : fileList) {
        try {
          CT::string baseDirStr = baseDir;
          CT::string groupName = fileName.c_str();
          groupName.substringSelf(baseDirStr.length(), -1);

          // Open file
          // CDBDebug("Opening file %s",fileList[j].c_str());
          CDFObject *cdfObject = CDFObjectStore::getCDFObjectStore()->getCDFObjectHeader(NULL, srvParam, groupName);
          if (cdfObject == NULL) {
            CDBError("Unable to read file %s", groupName);
            throw(__LINE__);
          }

          // std::vector<CT::string> variables;
          // List variables
          for (auto &var : cdfObject->variables) {
            if (!var->isDimension && var->dimensionlinks.size() >= 2) {
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

        } catch (int e) {
          nrOfFileErrors++;
        }
      }
      if (nrOfFileErrors != 0) {
        CDBError("%d files are not readable", nrOfFileErrors);
      }

    } catch (int line) {
      return 1;
    }
  }
#ifdef MEASURETIME
  StopWatch_Stop("Config file checked");
#endif

  return status;
}

int CRequest::process_wms_getmetadata_request() {
  if (srvParam->WMSLayers != NULL) {
    for (size_t j = 0; j < srvParam->WMSLayers->count; j++) {
      CDBDebug("WMS GETMETADATA %s", srvParam->WMSLayers[j].c_str());
    }
  }
  return process_all_layers();
}

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
      CT::string ymd = store->getRecord(k)->get(0);
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

int CRequest::process_wms_getstyles_request() {
  //     int status;
  //     if(srvParam->serviceType==SERVICE_WMS){
  //       if(srvParam->Geo->dWidth>MAX_IMAGE_WIDTH){
  //         CDBError("Parameter WIDTH must be smaller than %d",MAX_IMAGE_WIDTH);
  //         return 1;
  //       }
  //       if(srvParam->Geo->dHeight>MAX_IMAGE_HEIGHT){
  //       CDBError("Parameter HEIGHT must be smaller than %d",MAX_IMAGE_HEIGHT);
  //       return 1;
  //       }
  //     }
  //     CDrawImage plotCanvas;
  //     plotCanvas.setTrueColor(true);
  //     plotCanvas.createImage(int(srvParam->Geo->dWidth),int(srvParam->Geo->dHeight));
  //     plotCanvas.create685Palette();
  //
  //     CImageDataWriter imageDataWriter;
  //
  //     CDataSource * dataSource = new CDataSource();
  //     //dataSource->setCFGLayer(srvParam,srvParam->configObj->Configuration[0],srvParam->cfg->Layer[0],"prediction",0);
  //     dataSource->addStep("",NULL);
  //     dataSource->getCDFDims()->addDimension("time","0",0);
  //     dataSource->setTimeStep(0);
  //     dataSource->srvParams=srvParam;
  //     dataSource->cfg=srvParam->configObj->Configuration[0];
  //     dataSource->cfgLayer=srvParam->cfg->Layer[0];
  //     CDataSource::DataObject *newDataObject = new CDataSource::DataObject();
  //     newDataObject->variableName.copy("test");
  //     dataSource->getDataObjectsVector()->push_back(newDataObject);
  //     dataSource->dLayerType=CConfigReaderLayerTypeStyled;
  //
  //
  //
  //
  //     plotCanvas.rectangle(0,0,plotCanvas.Geo->dWidth,plotCanvas.Geo->dHeight,CColor(255,255,255,255),CColor(255,255,255,255));
  //
  //     int legendWidth = 200;
  //     int legendHeight = 600;
  //
  //
  //
  //
  //
  //
  //     int posX=0;
  //     int posY=0;
  //
  //     bool errorOccured = false;
  //
  //     bool legendOnlyMode = true;
  //     try{
  //
  //       for(size_t j=0;j<srvParam->cfg->Style.size();j++){
  //         CServerConfig::XMLE_Style* style = srvParam->cfg->Style[j];
  //         CDBDebug("style %s",style->attr.name.c_str());
  //
  //         CT::PointerList<CT::string*> *legendList = NULL;
  //
  //         if(legendOnlyMode == false){
  //           legendList = CServerParams::getLegendNames(style->Legend);
  //         }else{
  //           legendList = new CT::PointerList<CT::string*>();
  //           for(size_t j=0;j<srvParam->cfg->Legend.size();j++){
  //
  //             legendList->push_back(new CT::string(srvParam->cfg->Legend[j]->attr.name.c_str()));
  //
  //           }
  //         }
  //
  //         CDBDebug("Found %d legends",legendList->size());
  //         for(size_t i=0;i<legendList->size();i++){
  //           CDBDebug("legend %s",(*legendList)[i]->c_str());
  //
  //           int legendIndex =
  //           CImageDataWriter::getServerLegendIndexByName((*legendList)[i]->c_str(),srvParam->cfg->Legend);
  //           if(legendIndex == -1){
  //             CDBError("Legend %s is not configured");
  //             delete legendList;
  //             throw (__LINE__);
  //           }
  //           CDBDebug("Found legend index %d",legendIndex);
  //
  //           CT::PointerList<CT::string*> *renderMethodList =
  //           CImageDataWriter::getRenderMethodListForDataSource(dataSource,style);
  //
  //           CDBDebug("Found %d rendermethods",renderMethodList->size());
  // //           for(size_t r=0;r<renderMethodList->size();r++){
  // //             CDBDebug("Using
  // %s->%s->%s",style->attr.name.c_str(),(*legendList)[i]->c_str(),(*renderMethodList)[r]->c_str());
  // //             CT::string styleName;
  // //             styleName.print("%s/%s",style->attr.name.c_str(),(*renderMethodList)[r]->c_str());
  // //             if(legendOnlyMode){
  // //               styleName.print("%s",(*legendList)[i]->c_str());
  // //             }
  // //
  // //
  // //
  // CImageDataWriter::makeStyleConfig(dataSource->styleConfiguration,dataSource);//,style->attr.name.c_str(),(*legendList)[i]->c_str(),(*renderMethodList)[r]->c_str());
  // //
  // //             CDrawImage legendImage;
  // //             legendImage.enableTransparency(true);
  // //             legendImage.createImage(&plotCanvas,legendWidth,legendHeight);
  // //             status = legendImage.createGDPalette(srvParam->cfg->Legend[legendIndex]);if(status !=
  // 0)throw(__LINE__);
  // //
  // //
  // //
  // legendImage.rectangle(0,0,legendImage.Geo->dWidth-1,legendImage.Geo->dHeight-1,CColor(255,255,255,255),CColor(0,0,255,255));
  // //             status = imageDataWriter.createLegend(dataSource,&legendImage);if(status != 0)throw(__LINE__);
  // //             //posX = (legendNr++)*legendWidth;
  // //
  // //             plotCanvas.draw(posX,posY,0,0,&legendImage);
  // //
  // plotCanvas.drawText(posX+4,posY+legendHeight-4,srvParam->cfg->WMS[0]->SubTitleFont[0]->attr.location.c_str(),8,0,styleName.c_str(),CColor(0,0,0,255),textBGColor);
  // //
  // //             posX+=legendWidth;
  // //             if(posX>plotCanvas.Geo->dWidth){
  // //               posX=0;
  // //               posY+=legendHeight;
  // //             }
  // //             if(legendOnlyMode)break;
  // //           }
  //           delete renderMethodList;
  //         }
  //         delete legendList;
  //         if(legendOnlyMode)break;
  //       }
  //     }catch(int e){
  //       errorOccured = true;
  //     }
  //
  //
  //
  //
  //     delete dataSource;
  //
  //     if(errorOccured){
  //       return 1;
  //     }
  //     printf("%s%c%c\n","Content-Type:image/png",13,10);
  //     plotCanvas.printImagePng();
  return 0;
}

int CRequest::process_wms_getlegendgraphic_request() {
  if (srvParam->WMSLayers != NULL)
    for (size_t j = 0; j < srvParam->WMSLayers->count; j++) {
      CDBDebug("WMS GETLEGENDGRAPHIC %s", srvParam->WMSLayers[j].c_str());
    }
  return process_all_layers();
}
int CRequest::process_wms_getfeatureinfo_request() {
  if (srvParam->WMSLayers != NULL)
    for (size_t j = 0; j < srvParam->WMSLayers->count; j++) {
      CDBDebug("WMS GETFEATUREINFO %s", srvParam->WMSLayers[j].c_str());
    }
  return process_all_layers();
}

int CRequest::process_wcs_getcoverage_request() {
#ifndef ADAGUC_USE_GDAL
  CServerParams::showWCSNotEnabledErrorMessage();
  return 1;
#else
  if (srvParam->WMSLayers != NULL)
    for (size_t j = 0; j < srvParam->WMSLayers->count; j++) {
      CDBDebug("WCS GETCOVERAGE %s", srvParam->WMSLayers[j].c_str());
    }
  return process_all_layers();
#endif
}
const char *CSimpleStore::className = "CSimpleStore";

int CRequest::storeDocumentCache(CSimpleStore *simpleStore) {
  // Store the store
  CT::string cacheBuffer;
  simpleStore->getBuffer(&cacheBuffer);
  CT::string cacheFileName;
  srvParam->getCacheFileName(&cacheFileName);
  FILE *pFile = fopen(cacheFileName.c_str(), "wb");
  if (!pFile) {
    CDBError("Unable to write cachefile %s", cacheFileName.c_str());
    return 1;
  }
  fputs(cacheBuffer.c_str(), pFile);
  fclose(pFile);
  if (chmod(cacheFileName.c_str(), 0777) < 0) {
    CDBError("Unable to change permissions of cachefile %s", cacheFileName.c_str());
    return 1;
  }

  return 0;
}

int CRequest::getDocFromDocCache(CSimpleStore *simpleStore, CT::string *docName, CT::string *document) {
  if (document == NULL || docName == NULL) return 0;
  // If CacheDocs is true and the configuration is unmodified
  // the XML can be retrieved from the disk
  bool cacheNeedsRefresh = false;
  // Check if the configuration file is modified
  // Get configration file modification time
  CT::string configModificationDate;
  struct tm *clock;                                // create a time structure
  struct stat attrib;                              // create a file attribute structure
  stat(srvParam->configFileName.c_str(), &attrib); // get the attributes of afile.txt
  clock = gmtime(&(attrib.st_mtime));              // Get the last modified time and put it into the time structure
  char buffer[80];
  // strftime (buffer,80,"%I:%M%p.",clock);
  strftime(buffer, 80, "%Y-%m-%dT%H:%M:%SZ", clock);
  configModificationDate.copy(buffer);

  // Get a filename suited for diskstorage
  CT::string cacheFileName;
  srvParam->getCacheFileName(&cacheFileName);
  // CDBDebug("cacheFileName: %s",cacheFileName.c_str());
  // Check wether the cache file exists
  struct stat stFileInfo;
  CT::string cacheBuffer;
  // If the file does not exist, the cache needs to be created
  if (stat(cacheFileName.c_str(), &stFileInfo) != 0) {
    CDBDebug("The cache file %s does not exist", cacheFileName.c_str());
    cacheNeedsRefresh = true;
  }
  if (cacheNeedsRefresh == false) {
    // Try to open the cache file for reading
    FILE *cacheF = fopen(cacheFileName.c_str(), "r");
    if (cacheF == NULL) {
      CDBDebug("Unable to open cachefile %s", cacheFileName.c_str());
      cacheNeedsRefresh = true;
    }
    // obtain file size:
    fseek(cacheF, 0, SEEK_END);
    size_t fileSize = ftell(cacheF);
    rewind(cacheF);
    char *pszCacheBuffer = new char[fileSize + 1];
    // copy the file into the buffer:
    size_t bytesRead = fread(pszCacheBuffer, 1, fileSize, cacheF);
    if (bytesRead != fileSize) {
      CDBError("Reading error of cache file %s", cacheFileName.c_str());
      delete[] pszCacheBuffer;
      return 1;
    }
    fclose(cacheF);
    // Store the data in the CT::string object
    cacheBuffer.copy(pszCacheBuffer);
    delete[] pszCacheBuffer;
  }
  if (cacheNeedsRefresh == false) {
    // Now compare compare file modification date from last known file modification date
    CT::string oldConfigModificationDate;
    simpleStore->parse(cacheBuffer.c_str());
    if (simpleStore->getCTStringAttribute("configModificationDate", &oldConfigModificationDate) != 0) {
      CDBDebug("configModificationDate not available in cachefile %s", cacheFileName.c_str());
      // simpleStore->setStringAttribute("configModificationDate",configModificationDate.c_str());
      cacheNeedsRefresh = true;
    } else {
      if (oldConfigModificationDate.equals(&configModificationDate)) {
        // The modification date of the configuration file is the same as the stored one.
        // CDBDebug("Cache needs no update");
      } else {
        CDBDebug("Cache needs update because %s!=%s", oldConfigModificationDate.c_str(), configModificationDate.c_str());
        // simpleStore->setStringAttribute("configModificationDate",configModificationDate.c_str());
        cacheNeedsRefresh = true;
      }
    }
  }
  if (cacheNeedsRefresh == false) {
    // Read and Provide the xml document!
    int status = simpleStore->getCTStringAttribute(docName->c_str(), document);
    if (status != 0) {
      // CDBDebug("Unable to get document %s from cache",docName->c_str());
      cacheNeedsRefresh = true;
    }
  }
  if (cacheNeedsRefresh == true) {
    simpleStore->setStringAttribute("configModificationDate", configModificationDate.c_str());
    // CDBDebug("cacheNeedsRefresh==true");
    return 2;
  }
  return 0;
}

int CRequest::generateOGCGetCapabilities(CT::string *XMLdocument) {
  CXMLGen XMLGen;
  // Set WMSLayers:
  srvParam->WMSLayers = new CT::string[srvParam->cfg->Layer.size()];
  for (size_t j = 0; j < srvParam->cfg->Layer.size(); j++) {
    srvParam->makeUniqueLayerName(&srvParam->WMSLayers[j], srvParam->cfg->Layer[j]);
    srvParam->WMSLayers[j].count = srvParam->cfg->Layer.size();
  }
  return XMLGen.OGCGetCapabilities(srvParam, XMLdocument);
}

int CRequest::generateGetReferenceTimes(CDataSource *dataSource) {
  CT::string XMLdocument;
  if (srvParam->enableDocumentCache == true) {
    CSimpleStore simpleStore;
    CT::string documentName;
    int status = getDocumentCacheName(&documentName, srvParam);
    if (status != 0) return 1;
    // Try to get the getcapabilities document from the store:
    status = getDocFromDocCache(&simpleStore, &documentName, &XMLdocument);
    if (status == 1) return 1;
    // if(status==2, the store is ok, but not up to date
    if (status == 2) {
      // CDBDebug("Generating a new document with name %s",documentName.c_str());
      if (generateGetReferenceTimesDoc(&XMLdocument, dataSource) == CXMLGEN_FATAL_ERROR_OCCURED) return 1;
      // Store this document
      simpleStore.setStringAttribute(documentName.c_str(), XMLdocument.c_str());
      if (storeDocumentCache(&simpleStore) != 0) return 1;

    } else {
      CDBDebug("Providing document from store with name %s", documentName.c_str());
    }
  } else {
    // Do not use cache
    if (generateGetReferenceTimesDoc(&XMLdocument, dataSource) == CXMLGEN_FATAL_ERROR_OCCURED) return 1;
  }
  if (srvParam->JSONP.length() == 0) {
    printf("%s%c%c\n", "Content-Type: application/json ", 13, 10);
    printf("%s", XMLdocument.c_str());
  } else {
    printf("%s%c%c\n", "Content-Type: application/javascript ", 13, 10);
    printf("%s(%s)", srvParam->JSONP.c_str(), XMLdocument.c_str());
  }

  return 0;
}

int CRequest::generateOGCDescribeCoverage(CT::string *XMLdocument) {
  CXMLGen XMLGen;
  for (size_t j = 0; j < srvParam->WMSLayers->count; j++) {
    CDBDebug("WCS_DESCRIBECOVERAGE %s", srvParam->WMSLayers[j].c_str());
  }
  return XMLGen.OGCGetCapabilities(srvParam, XMLdocument);
}

int CRequest::process_wms_getcap_request() {
  CDBDebug("WMS GETCAPABILITIES");

  CT::string XMLdocument;
  if (srvParam->enableDocumentCache == true) {
    CSimpleStore simpleStore;
    CT::string documentName;
    int status = getDocumentCacheName(&documentName, srvParam);
    if (status != 0) return 1;
    // Try to get the getcapabilities document from the store:
    status = getDocFromDocCache(&simpleStore, &documentName, &XMLdocument);
    if (status == 1) return 1;
    // if(status==2, the store is ok, but not up to date
    if (status == 2) { // Store needs update
      // CDBDebug("Generating a new document with name %s",documentName.c_str());
      int status = generateOGCGetCapabilities(&XMLdocument);
      if (status == CXMLGEN_FATAL_ERROR_OCCURED) return 1;
      // Store this document
      if (status == 0) {
        simpleStore.setStringAttribute(documentName.c_str(), XMLdocument.c_str());
        if (storeDocumentCache(&simpleStore) != 0) return 1;
      }
    } else {
      CT::string cacheFileName;
      srvParam->getCacheFileName(&cacheFileName);
      CDBDebug("Providing document from store '%s' with name '%s'", cacheFileName.c_str(), documentName.c_str());
    }
  } else {
    // Do not use cache
    int status = generateOGCGetCapabilities(&XMLdocument);
    ;
    if (status == CXMLGEN_FATAL_ERROR_OCCURED) return 1;
  }
  const char *pszADAGUCWriteToFile = getenv("ADAGUC_WRITETOFILE");
  if (pszADAGUCWriteToFile != NULL) {
    CReadFile::write(pszADAGUCWriteToFile, XMLdocument.c_str(), XMLdocument.length());
  } else {
    printf("%s%s%c%c\n", "Content-Type:text/xml", srvParam->getCacheControlHeader(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
    printf("%s", XMLdocument.c_str());
  }

  return 0;
}

int CRequest::process_wms_getreferencetimes_request() {
  CDBDebug("WMS GETREFERENCETIMES");
  return process_all_layers();
}

int CRequest::process_wcs_getcap_request() {
  CDBDebug("WCS GETCAPABILITIES");
  return process_wms_getcap_request();
}

int CRequest::process_wcs_describecov_request() { return process_all_layers(); }

int CRequest::process_wms_getmap_request() {
  if (srvParam->WMSLayers != NULL) {
    CT::string message = "WMS GETMAP ";
    for (size_t j = 0; j < srvParam->WMSLayers->count; j++) {
      if (j > 0) message.concat(",");
      message.printconcat("(%d) %s", j, srvParam->WMSLayers[j].c_str());
    }
    CDBDebug(message.c_str());
  } else {
    CDBDebug("WMS GETMAP no layers");
  }
  return process_all_layers();
}

int CRequest::process_wms_gethistogram_request() {
  if (srvParam->WMSLayers != NULL) {
    CT::string message = "WMS GETHISTOGRAM ";
    for (size_t j = 0; j < srvParam->WMSLayers->count; j++) {
      if (j > 0) message.concat(",");
      message.printconcat("(%d) %s", j, srvParam->WMSLayers[j].c_str());
    }
    CDBDebug(message.c_str());
  } else {
    CDBDebug("WMS GETMAP no layers");
  }
  return process_all_layers();
}

int CRequest::setDimValuesForDataSource(CDataSource *dataSource, CServerParams *srvParam) {
#ifdef CREQUEST_DEBUG
  CDBDebug("setDimValuesForDataSource");
#endif
  int status = fillDimValuesForDataSource(dataSource, srvParam);
  if (status != 0) return status;
  status = queryDimValuesForDataSource(dataSource, srvParam);
  if (status != 0) return status;
  return 0;
};

int CRequest::fillDimValuesForDataSource(CDataSource *dataSource, CServerParams *srvParam) {

#ifdef CREQUEST_DEBUG
  StopWatch_Stop("### [fillDimValuesForDataSource]");
#endif
  try {
    /*
     * Check if all tables are available, if not update the db
     */
    if (srvParam->isAutoResourceEnabled()) {
      int status = CDBFactory::getDBAdapter(srvParam->cfg)->autoUpdateAndScanDimensionTables(dataSource);
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
        if (dataSource->requiredDims[l]->name.equals(&dimName)) {
          alreadyAdded = true;
          break;
        }
      }

#ifdef CREQUEST_DEBUG
      CDBDebug("alreadyAdded = %d", alreadyAdded);
#endif
      if (alreadyAdded == false) {
        for (size_t k = 0; k < srvParam->requestDims.size(); k++) {
          if (srvParam->requestDims[k]->name.equals(&dimName)) {
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
                  CTime ctime;
                  ctime.init("seconds since 1970", NULL);
                  double currentTimeAsEpoch;

                  try {
                    currentTimeAsEpoch = ctime.dateToOffset(ctime.freeDateStringToDate(ogcDim->value.c_str()));
                    CT::string currentDateConverted = ctime.dateToISOString(ctime.getDate(currentTimeAsEpoch));
                    ogcDim->value = currentDateConverted;
                  } catch (int e) {
                    CDBDebug("Unable to convert %s to epoch", ogcDim->value.c_str());
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
        if (dataSource->requiredDims[k]->name.equals(&dimName)) {
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
          CDBDebug("Invalid dimension value for layer %s", dataSource->cfgLayer->Name[0]->value.c_str());
          throw InvalidDimensionValue;
          //           setExceptionType(InvalidDimensionValue);
          //           CDBError("Invalid dimension value for layer %s",dataSource->cfgLayer->Name[0]->value.c_str());
          //           CDBError("query failed");
          //           return 1;
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
          if (requiredDim->name.equals(&dimName)) {
            CDBDebug("Forcing dimension %s from %s to %s", dimName.c_str(), requiredDim->value.c_str(), fixedValue.c_str());
            requiredDim->value = fixedValue;
            requiredDim->hasFixedValue = true;
            break;
          }
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

int findExtent(const char *srcProj4Str, CServerParams *srvParam, double nativeViewPortBBOX[4]) {
  CImageWarper warper;
  int status = warper.initreproj(srcProj4Str, srvParam->Geo, &srvParam->cfg->Projection);
  if (status != 0) {
    warper.closereproj();
    return 1;
  }

  double bbStepX = (nativeViewPortBBOX[2] - nativeViewPortBBOX[0]) / 100.;
  double bbStepY = (nativeViewPortBBOX[3] - nativeViewPortBBOX[1]) / 100.;

  double xLow = nativeViewPortBBOX[0];
  double yLow = nativeViewPortBBOX[1];
  double xHigh = nativeViewPortBBOX[2];
  double yHigh = nativeViewPortBBOX[3];

  bool first_already_processed = false;
  for (double y = yLow; y < yHigh; y += bbStepY) {
    for (double x = xLow; x < xHigh; x += bbStepX) {

      double x1 = x, y1 = y;
      status = warper.reprojpoint(x1, y1);
      if (status == 0) {
        // CDBDebug("Testing %f %f" ,x,y);
        if (first_already_processed == false) {
          nativeViewPortBBOX[0] = x1;
          nativeViewPortBBOX[1] = y1;
          nativeViewPortBBOX[2] = x1;
          nativeViewPortBBOX[3] = y1;
        } else {
          if (nativeViewPortBBOX[0] > x1) nativeViewPortBBOX[0] = x1;
          if (nativeViewPortBBOX[1] > y1) nativeViewPortBBOX[1] = y1;
          if (nativeViewPortBBOX[2] < x1) nativeViewPortBBOX[2] = x1;
          if (nativeViewPortBBOX[3] < y1) nativeViewPortBBOX[3] = y1;
        }
        first_already_processed = true;
      }
    }
  }
  warper.closereproj();
  return 0;
}

int CRequest::queryDimValuesForDataSource(CDataSource *dataSource, CServerParams *srvParam) {
#ifdef CREQUEST_DEBUG
  CDBDebug("queryDimValuesForDataSource");
#endif
  try {
    CDBStore::Store *store = NULL;

    // If query on bbox = enabled, and CRS is set, set the current viewport bbox
    dataSource->queryBBOX = dataSource->cfgLayer->TileSettings.size() == 1 && !srvParam->Geo->CRS.empty();

    //     CDBDebug("queryDimValuesForDataSource dataSource->queryBBOX=%d %s for step
    //     %d/%d",(int)dataSource->queryBBOX,dataSource->layerName.c_str(),dataSource->getCurrentTimeStep(),dataSource->getNumTimeSteps());
    //     CDBDebug("queryDimValuesForDataSource cfgLayer->Name = %s",dataSource->cfgLayer->Name[0]->value.c_str());
    //     CDBDebug("queryDimValuesForDataSource cfgLayer->Title = %s",dataSource->cfgLayer->Title[0]->value.c_str());
    //     CDBDebug("queryDimValuesForDataSource cfgLayer->FilePath =
    //     %s",dataSource->cfgLayer->FilePath[0]->value.c_str()); CDBDebug("queryDimValuesForDataSource
    //     dataSource->cfgLayer->TileSettings.size()= %d",dataSource->cfgLayer->TileSettings.size());
    //

    if (dataSource->queryBBOX && dataSource->cfgLayer->TileSettings.size() == 1) {
      // CDBDebug("queryDimValuesForDataSource dataSource->queryBBOX %s for step
      // %d/%d",dataSource->layerName.c_str(),dataSource->getCurrentTimeStep(),dataSource->getNumTimeSteps());
      CT::string nativeProj4 = dataSource->cfgLayer->TileSettings[0]->attr.tileprojection.c_str();

      nativeProj4 = dataSource->cfgLayer->TileSettings[0]->attr.tileprojection.c_str();

      double nativeViewPortBBOX[4];
      nativeViewPortBBOX[0] = srvParam->Geo->dfBBOX[0];
      nativeViewPortBBOX[1] = srvParam->Geo->dfBBOX[1];
      nativeViewPortBBOX[2] = srvParam->Geo->dfBBOX[2];
      nativeViewPortBBOX[3] = srvParam->Geo->dfBBOX[3];

      if (nativeViewPortBBOX[0] == nativeViewPortBBOX[2]) {
        CDBDebug("View port BBOX is wrong: %f %f %f %f", nativeViewPortBBOX[0], nativeViewPortBBOX[1], nativeViewPortBBOX[2], nativeViewPortBBOX[3]);
        nativeViewPortBBOX[0] = -180;
        nativeViewPortBBOX[1] = -90;
        nativeViewPortBBOX[2] = 180;
        nativeViewPortBBOX[3] = 90;
      }
      findExtent(nativeProj4.c_str(), srvParam, nativeViewPortBBOX);

      int tilewidth = dataSource->cfgLayer->TileSettings[0]->attr.tilewidthpx.toInt();
      int tileheight = dataSource->cfgLayer->TileSettings[0]->attr.tileheightpx.toInt();
      double tilecellsizex = dataSource->cfgLayer->TileSettings[0]->attr.tilecellsizex.toDouble();
      double tilecellsizey = dataSource->cfgLayer->TileSettings[0]->attr.tilecellsizey.toDouble();
      double level1BBOXWidth = fabs(tilecellsizex * double(tilewidth));
      double level1BBOXHeight = fabs(tilecellsizey * double(tileheight));
      bool tileSettingsDebug = dataSource->cfgLayer->TileSettings[0]->attr.debug.equals("true");

      //       if(dataSource->cfgLayer->TileSettings[0]->attr.tilebboxwidth.empty()==false){
      //         level1BBOXWidth = dataSource->cfgLayer->TileSettings[0]->attr.tilebboxwidth.toDouble();
      //       }
      //       if(dataSource->cfgLayer->TileSettings[0]->attr.tilebboxwidth.empty()==false){
      //         level1BBOXHeight = dataSource->cfgLayer->TileSettings[0]->attr.tilebboxheight.toDouble();
      //       }

#ifdef CREQUEST_DEBUG
      CDBDebug("level1BBOXHeight,level1BBOXHeight %f,%f", level1BBOXHeight, level1BBOXHeight);
#endif

      int maxlevel = dataSource->cfgLayer->TileSettings[0]->attr.maxlevel.toInt();
      int minlevel = 1;
      if (dataSource->cfgLayer->TileSettings[0]->attr.minlevel.empty() == false) {
        minlevel = dataSource->cfgLayer->TileSettings[0]->attr.minlevel.toInt() - 1;
        if (minlevel <= 1) minlevel = 1;
      }

      double screenCellSize = -1;
      // if(!nativeProj4.equals(srvParam->Geo->CRS))

      // Find cellsize at parts of window

      double viewportCellsizeX = (srvParam->Geo->dfBBOX[2] - srvParam->Geo->dfBBOX[0]) / double(srvParam->Geo->dWidth);
      double viewportCellsizeY = (srvParam->Geo->dfBBOX[3] - srvParam->Geo->dfBBOX[1]) / double(srvParam->Geo->dHeight);

      CImageWarper warper;
      int status = warper.initreproj(nativeProj4.c_str(), srvParam->Geo, &srvParam->cfg->Projection);
      if (status != 0) {
        warper.closereproj();
        CDBError("Unable to initialize projection ");
        return 1;
      }

      for (double wy = 0.2; wy < 0.8; wy += 0.1) {
        for (double wx = 0.2; wx < 0.8; wx += 0.1) {
          double viewPortMX = srvParam->Geo->dfBBOX[2] * (1 - wx) + srvParam->Geo->dfBBOX[0] * wx;
          double viewPortMY = srvParam->Geo->dfBBOX[3] * (1 - wy) + srvParam->Geo->dfBBOX[1] * wy;
          // CDBDebug("viewPortMX, viewPortMY at (%f %f) = (%f,%f)",wx, wy, viewPortMX,viewPortMY);
          double x1 = viewPortMX;
          double y1 = viewPortMY;
          double x2 = viewPortMX + viewportCellsizeX;
          double y2 = viewPortMY + viewportCellsizeY;
          ;
          status = 0;
          status += warper.reprojpoint(x1, y1);
          status += warper.reprojpoint(x2, y2);

          //             CDBDebug("%f %f",x1,y1);
          //             CDBDebug("%f %f",x2,y2);
          if (status == 0) {
            double calcCellsize = sqrt(((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1)));
            if (screenCellSize < 0) {
              screenCellSize = calcCellsize;
            } else {
              if (calcCellsize < screenCellSize) screenCellSize = calcCellsize;
            }
            // CDBDebug("screenCellSize (%f,%f)= %f",viewPortMX,viewPortMY,calcCellsize);
          }
        }
      }

#ifdef CREQUEST_DEBUG
      CDBDebug("Cellsize screen %f", screenCellSize);
      CDBDebug("Cellsize basetile %f", level1BBOXWidth / double(tilewidth));
#endif

      warper.closereproj();

      dataSource->queryLevel = 0;
      //       int numResults = 0;
      store = NULL;

      for (int queryLevel = minlevel; queryLevel < maxlevel; queryLevel++) {
        double levelXBBOXWidth = level1BBOXWidth * pow(2, queryLevel) * 1;
        double tileCellSize = levelXBBOXWidth / double(tilewidth);
        if (tileCellSize < screenCellSize) {
          dataSource->queryLevel = queryLevel;
          // break;
        }
      }

      CDBDebug("dataSource->queryLevel%d", dataSource->queryLevel);
      // if(dataSource->queryLevel < minlevel ) {
      //   dataSource->queryLevel = minlevel ;
      // }

      /*  while(((numResults*tilewidth*tileheight)/2>srvParam->Geo->dWidth*srvParam->Geo->dHeight&&numResults>3)||numResults==0||numResults>60)
        {

          if(dataSource->queryLevel>(maxlevel-1)){dataSource->queryLevel--;break;}
       *///   delete store;store=NULL;
      dataSource->queryLevel++;

      if (maxlevel == 0) {
        dataSource->queryLevel = 0;
        dataSource->queryBBOX = false;
      }

      double levelXBBOXWidth = level1BBOXWidth * pow(2, dataSource->queryLevel - 1) * 1;
      double levelXBBOXHeight = level1BBOXHeight * pow(2, dataSource->queryLevel - 1) * 1;
      // CDBDebug("levelXBBOXWidth = %f, levelXBBOXHeight = %f
      // queryLevel=%d",levelXBBOXWidth,levelXBBOXHeight,dataSource->queryLevel);
      dataSource->nativeViewPortBBOX[0] = nativeViewPortBBOX[0] - levelXBBOXWidth;
      dataSource->nativeViewPortBBOX[1] = nativeViewPortBBOX[1] - levelXBBOXHeight;
      dataSource->nativeViewPortBBOX[2] = nativeViewPortBBOX[2] + levelXBBOXWidth;
      dataSource->nativeViewPortBBOX[3] = nativeViewPortBBOX[3] + levelXBBOXHeight;

      // CDBDebug(" dataSource->nativeViewPortBBOX: [%f,%f,%f,%f]", dataSource->nativeViewPortBBOX[0],
      // dataSource->nativeViewPortBBOX[1], dataSource->nativeViewPortBBOX[2], dataSource->nativeViewPortBBOX[3]);
      int maxTilesInImage = 300;
      if (!dataSource->cfgLayer->TileSettings[0]->attr.maxtilesinimage.empty()) {
        maxTilesInImage = dataSource->cfgLayer->TileSettings[0]->attr.maxtilesinimage.toInt();
      }
      store = CDBFactory::getDBAdapter(srvParam->cfg)->getFilesAndIndicesForDimensions(dataSource, maxTilesInImage);
      if (store == NULL) {
        CDBError("Unable to query bbox for tiles");
        return 1;
      }

      if (store->getSize() == 0) {
        CDBDebug("Found no tiles, trying level %d", maxlevel);
        delete store;
        dataSource->queryLevel = maxlevel;
        dataSource->nativeViewPortBBOX[0] = -2000000;
        dataSource->nativeViewPortBBOX[1] = -2000000;
        dataSource->nativeViewPortBBOX[2] = 2000000;
        dataSource->nativeViewPortBBOX[3] = 2000000;
        dataSource->queryBBOX = true;
        store = CDBFactory::getDBAdapter(srvParam->cfg)->getFilesAndIndicesForDimensions(dataSource, 1);
        if (store == NULL) {
          CDBError("Unable to query bbox for tiles");
          return 1;
        }
      }

      //         if(store!=NULL){
      //           numResults=store->getSize();
      //           CDBDebug("Found %d tiles",store->getSize());
      //         }else {
      //           numResults = 0;
      //         }
      //         double tileCellSize = levelXBBOXWidth/tilewidth;

      //       }

      double tileCellSize = levelXBBOXWidth / double(tilewidth);
      // CDBDebug("level %d, tiles %0d cellsize %f",dataSource->queryLevel,store->getSize(),tileCellSize);
      if (tileSettingsDebug) {
        srvParam->mapTitle.print("level %d, tiles %d", dataSource->queryLevel, store->getSize());
        srvParam->mapSubTitle.print("level %d, tiles %0d, tileCellSize %f, screenCellSize %f", dataSource->queryLevel, store->getSize(), tileCellSize, screenCellSize);
      }

    } else {
      /* Do queries without tiling and boundingbox */
      dataSource->queryBBOX = false;

      /*
            dataSource->queryBBOX = true;
            dataSource->nativeViewPortBBOX[0] =111615;
            dataSource->nativeViewPortBBOX[1] = 9.19318e+06;
            dataSource->nativeViewPortBBOX[2] = 666176;
            dataSource->nativeViewPortBBOX[3] =  9.24119e+06;
            dataSource->queryLevel = 0;*/
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
      CDBDebug("Using maxquerylimit %d", maxQueryResultLimit);
      store = CDBFactory::getDBAdapter(srvParam->cfg)->getFilesAndIndicesForDimensions(dataSource, maxQueryResultLimit);
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
        CDBDebug("dataSource->requiredDims.size() %d", dataSource->requiredDims.size());
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
      dataSource->addStep(record->get(0)->c_str(), NULL);
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
        // dataSource->requiredDims[i]->allValues.push_back(sDims[l].c_str());
      }
    }

    //     for(size_t i=0;i<dataSource->requiredDims.size();i++){
    //       CDBDebug("%d There are %d values for dimension
    //       %s",i,dataSource->requiredDims[i]->uniqueValues.size(),dataSource->requiredDims[i]->netCDFDimName.c_str());
    //     }

    delete store;
  } catch (int i) {
    CDBError("%d", i);
    return 2;
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
  if (srvParam->WMSLayers == NULL) {
    if (srvParam->requestType == REQUEST_WCS_DESCRIBECOVERAGE) {
      // Therefore we read all the coverages, we do this by defining WMSLayers, as if the user entered in the coverages
      // section all layers.
      srvParam->requestType = REQUEST_WCS_DESCRIBECOVERAGE;
      size_t layerCount = srvParam->cfg->Layer.size();
      srvParam->WMSLayers = new CT::string[layerCount];
      for (size_t j = 0; j < layerCount; j++) {
        srvParam->makeUniqueLayerName(&srvParam->WMSLayers[j], srvParam->cfg->Layer[j]);
        srvParam->WMSLayers[j].count = layerCount;
      }
    } else {
      CDBError("No layers/coverages defined");
      return 1;
    }
  } else {
    // Otherwise WMSLayers are defined by the user, so we need only the selection the user has made
    if (srvParam->WMSLayers->count == 0) {
      CDBError("No layers/coverages defined");
      return 1;
    }

    // dataSources = new CDataSource[srvParam->WMSLayers->count];
    // Now set the properties of these sourceimages
    CT::string layerName;

    for (size_t j = 0; j < srvParam->WMSLayers->count; j++) {
      size_t layerNo = 0;
      for (layerNo = 0; layerNo < srvParam->cfg->Layer.size(); layerNo++) {

        srvParam->makeUniqueLayerName(&layerName, srvParam->cfg->Layer[layerNo]);
        // CDBError("comparing (%d) %s==%s",j,layerName.c_str(),srvParam->WMSLayers[j].c_str());
        if (layerName.equals(srvParam->WMSLayers[j].c_str())) {
          CDataSource *dataSource = new CDataSource();

          dataSources.push_back(dataSource);

          if (dataSource->setCFGLayer(srvParam, srvParam->configObj->Configuration[0], srvParam->cfg->Layer[layerNo], layerName.c_str(), j) != 0) {
            return 1;
          }

          // Check if layer has an additional layer
          for (size_t additionalLayerNr = 0; additionalLayerNr < srvParam->cfg->Layer[layerNo]->AdditionalLayer.size(); additionalLayerNr++) {
            CServerConfig::XMLE_AdditionalLayer *additionalLayer = srvParam->cfg->Layer[layerNo]->AdditionalLayer[additionalLayerNr];
            bool replacePreviousDataSource = (additionalLayer->attr.replace.equals("true") || additionalLayer->attr.replace.equals("previous"));
            bool replaceAllDataSource = additionalLayer->attr.replace.equals("all");

            CT::string additionalLayerName = additionalLayer->value.c_str();
            for (size_t additionalLayerNo = 0; additionalLayerNo < srvParam->cfg->Layer.size(); additionalLayerNo++) {
              CT::string additional;
              srvParam->makeUniqueLayerName(&additional, srvParam->cfg->Layer[additionalLayerNo]);
              // CDBDebug("comparing for additionallayer %s==%s", additionalLayerName.c_str(), additional.c_str());
              if (additionalLayerName.equals(additional)) {
                // CDBDebug("Found additionalLayer [%s]", additional.c_str());
                CDataSource *additionalDataSource = new CDataSource();

                // CDBDebug("setCFGLayer for additionallayer %s", additionalLayerName.c_str());
                if (additionalDataSource->setCFGLayer(srvParam, srvParam->configObj->Configuration[0], srvParam->cfg->Layer[additionalLayerNo], additionalLayerName.c_str(), j) != 0) {
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

                CDBDebug("add = %d replaceAllDataSource = %d replacePreviousDataSource = %d", add, replaceAllDataSource, replacePreviousDataSource);
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
                  if (additionalLayer->attr.style.empty()) {
                    additionalDataSource->setStyle("default");
                  } else {
                    additionalDataSource->setStyle(additionalLayer->attr.style.c_str());
                  }
                  dataSources.push_back(additionalDataSource);
                } else {
                  delete additionalDataSource;
                }

                break;
              }
            } /* End of looping additionallayers */
          }
          break;
        }
      }
      if (layerNo == srvParam->cfg->Layer.size()) {
        CDBError("Layer [%s] not found", srvParam->WMSLayers[j].c_str());
        return 1;
      }
    }
  }

  int status;
  if (srvParam->serviceType == SERVICE_WMS) {
    if (srvParam->Geo->dWidth > MAX_IMAGE_WIDTH) {
      CDBError("Parameter WIDTH must be smaller than %d", MAX_IMAGE_WIDTH);
      return 1;
    }
    if (srvParam->Geo->dHeight > MAX_IMAGE_HEIGHT) {
      CDBError("Parameter HEIGHT must be smaller than %d", MAX_IMAGE_HEIGHT);
      return 1;
    }
  }

  if (srvParam->serviceType == SERVICE_WCS && srvParam->requestType == REQUEST_WCS_DESCRIBECOVERAGE) {
    CT::string XMLDocument;
    status = generateOGCDescribeCoverage(&XMLDocument);
    if (status == CXMLGEN_FATAL_ERROR_OCCURED) return 1;
    const char *pszADAGUCWriteToFile = getenv("ADAGUC_WRITETOFILE");
    if (pszADAGUCWriteToFile != NULL) {
      CReadFile::write(pszADAGUCWriteToFile, XMLDocument.c_str(), XMLDocument.length());
    } else {
      printf("%s%s%c%c\n", "Content-Type:text/xml", srvParam->getCacheControlHeader(CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE).c_str(), 13, 10);
      printf("%s", XMLDocument.c_str());
    }
    return 0;
  }

  for (size_t j = 0; j < dataSources.size(); j++) {

    if (dataSources[j]->dLayerType == CConfigReaderLayerTypeUnknown) {
      CDBError("Unknow layer type");
      return 0;
    }

    /***************************/
    /* Type = Database layer   */
    /***************************/
    if (dataSources[j]->dLayerType == CConfigReaderLayerTypeDataBase || dataSources[j]->dLayerType == CConfigReaderLayerTypeStyled || dataSources[j]->dLayerType == CConfigReaderLayerTypeBaseLayer) {

      if (dataSources[j]->cfgLayer->Dimension.size() == 0) {

        if (CAutoConfigure::autoConfigureDimensions(dataSources[j]) != 0) {
          CDBError("Unable to configure dimensions automatically");
          return 1;
        }
      } else {
        try {
          if (setDimValuesForDataSource(dataSources[j], srvParam) != 0) {
            CDBError("Error in setDimValuesForDataSource: Unable to find data for layer %s", dataSources[j]->layerName.c_str());
            return 1;
          }
        } catch (ServiceExceptionCode e) {
          CDBError("Invalid dimensions values: No data available for layer %s", dataSources[j]->layerName.c_str());
          setExceptionType(e);
          return 1;
        }
        /*delete[] values_path;
        delete[] values_dim;
        delete[] date_time;*/
      }
    }

    if (dataSources[j]->dLayerType == CConfigReaderLayerTypeCascaded) {
      // This layer has no dimensions, but we need to add one timestep with data in order to make the next code work.
      CDBDebug("Addstep");
      dataSources[j]->addStep("", NULL);
      // dataSources[j]->getCDFDims()->addDimension("none","0",0);
    }
    if (dataSources[j]->dLayerType == CConfigReaderLayerTypeLiveUpdate) {
      layerTypeLiveUpdateConfigureDimensionsInDataSource(dataSources[j]);
    }
  }

  // Try to find BBOX automatically, when not provided.
  if (srvParam->requestType == REQUEST_WMS_GETMAP && srvParam->dFound_BBOX == 0) {
    for (size_t d = 0; d < dataSources.size(); d++) {
      if (dataSources[d]->dLayerType != CConfigReaderLayerTypeCascaded && dataSources[d]->dLayerType != CConfigReaderLayerTypeBaseLayer &&
          dataSources[d]->dLayerType != CConfigReaderLayerTypeLiveUpdate) {
        CImageWarper warper;
        CDataReader reader;
        status = reader.open(dataSources[d], CNETCDFREADER_MODE_OPEN_HEADER);
        reader.close();
        status = warper.initreproj(dataSources[d], srvParam->Geo, &srvParam->cfg->Projection);
        if (status != 0) {
          warper.closereproj();
          CDBDebug("Unable to initialize projection ");
        }
        srvParam->Geo->dfBBOX[0] = -180;
        srvParam->Geo->dfBBOX[1] = -90;
        srvParam->Geo->dfBBOX[2] = 180;
        srvParam->Geo->dfBBOX[3] = 90;
        warper.findExtent(dataSources[d], srvParam->Geo->dfBBOX);
        warper.closereproj();
        CDBDebug("Found bbox %s %f %f %f %f", srvParam->Geo->CRS.c_str(), srvParam->Geo->dfBBOX[0], srvParam->Geo->dfBBOX[1], srvParam->Geo->dfBBOX[2], srvParam->Geo->dfBBOX[3]);
        srvParam->dFound_BBOX = 1;
        break;
      }
    }
  }

  /**************************************/
  /* Handle WMS Getmap database request */
  /**************************************/
  if (dataSources[0]->dLayerType == CConfigReaderLayerTypeDataBase || dataSources[0]->dLayerType == CConfigReaderLayerTypeStyled || dataSources[0]->dLayerType == CConfigReaderLayerTypeCascaded ||
      dataSources[0]->dLayerType == CConfigReaderLayerTypeBaseLayer) {
    try {
      for (size_t d = 0; d < dataSources.size(); d++) {
        dataSources[d]->setTimeStep(0);
      }
      if (srvParam->requestType == REQUEST_WMS_GETMAP) {

        CImageDataWriter imageDataWriter;

        /**
          We want like give priority to our own internal layers, instead to external cascaded layers. This is because
          our internal layers have an exact customized legend, and we would like to use this always.
        */

        bool imageDataWriterIsInitialized = false;
        int dataSourceToUse = 0;
        for (size_t d = 0; d < dataSources.size() && imageDataWriterIsInitialized == false; d++) {
          if (dataSources[d]->dLayerType != CConfigReaderLayerTypeCascaded) {
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

          // When we have multiple timesteps, we will create an animation.
          if (dataSources[dataSourceToUse]->getNumTimeSteps() > 1) imageDataWriter.createAnimation();
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

            if (dataSources[0]->dLayerType == CConfigReaderLayerTypeDataBase || dataSources[0]->dLayerType == CConfigReaderLayerTypeCascaded ||
                dataSources[0]->dLayerType == CConfigReaderLayerTypeBaseLayer) {
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
            if (dataSources[0]->dLayerType == CConfigReaderLayerTypeDataBase || dataSources[0]->dLayerType == CConfigReaderLayerTypeCascaded ||
                dataSources[0]->dLayerType == CConfigReaderLayerTypeBaseLayer) {

              status = imageDataWriter.addData(dataSources);
              if (status != 0) {
                /**
                 * Adding data failed:
                 * Do not ruin an animation if one timestep fails to load.
                 * If there is a single time step then throw an exception otherwise continue.
                 */
                if (dataSources[dataSourceToUse]->getNumTimeSteps() == 1) {
                  // Not an animation, so throw an exception
                  CDBError("Unable to convert datasource %s to image", dataSources[0]->layerName.c_str());
                  throw(__LINE__);
                } else {
                  // This is an animation, report an error and continue with adding images.
                  CDBError("Unable to load datasource %s at line %d", dataSources[dataSourceToUse]->getDataObject(0)->variableName.c_str(), __LINE__);
                }
              }
            }
            if (dataSources[0]->dLayerType == CConfigReaderLayerTypeStyled) {
              // Special styled layer for GEOMON project
              status = imageDataWriter.calculateData(dataSources);
              if (status != 0) throw(__LINE__);
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
          CT::StackList<CT::string> legendLayerList = srvParam->showLegendInImage.splitToStack(",");

          //          int numberOfLegendsDrawn = 0;
          int legendOffsetX = 0;
          for (size_t d = 0; d < dataSources.size(); d++) {
            if (dataSources[d]->dLayerType != CConfigReaderLayerTypeCascaded) {
              bool drawThisLegend = false;

              if (!drawAllLegends) {
                for (size_t li = 0; li < legendLayerList.size(); li++) {
                  CDBDebug("Comparing [%s] == [%s]", dataSources[d]->layerName.c_str(), legendLayerList[li].c_str());
                  if (dataSources[d]->layerName.toLowerCase().equals(legendLayerList[li])) {
                    drawThisLegend = true;
                  }
                }
              } else {
                drawThisLegend = true;
              }
              if (drawThisLegend) {
                /* Draw the legend */
                int padding = 4;
                int minimumLegendWidth = 100;
                CDrawImage legendImage;
                int legendWidth = LEGEND_WIDTH * scaling;
                if (legendWidth < minimumLegendWidth) legendWidth = minimumLegendWidth;
                imageDataWriter.drawImage.enableTransparency(true);
                legendImage.createImage(&imageDataWriter.drawImage, legendWidth, (imageDataWriter.drawImage.Geo->dHeight / 3) - padding * 2 + 2);

                CStyleConfiguration *styleConfiguration = dataSources[d]->getStyle();
                if (styleConfiguration != NULL && styleConfiguration->legendIndex != -1) {
                  legendImage.createGDPalette(srvParam->cfg->Legend[styleConfiguration->legendIndex]);
                }

                status = imageDataWriter.createLegend(dataSources[d], &legendImage);
                if (status != 0) throw(__LINE__);
                // legendImage.rectangle(0,0,10000,10000,240);
                int posX = imageDataWriter.drawImage.Geo->dWidth - (legendImage.Geo->dWidth + padding) - legendOffsetX;
                // int posY=imageDataWriter.drawImage.Geo->dHeight-(legendImage.Geo->dHeight+padding);
                // int posX=padding*scaling;//imageDataWriter.drawImage.Geo->dWidth-(scaleBarImage.Geo->dWidth+padding);
                int posY = imageDataWriter.drawImage.Geo->dHeight - (legendImage.Geo->dHeight + padding * scaling);
                imageDataWriter.drawImage.draw(posX, posY, 0, 0, &legendImage);
                //                numberOfLegendsDrawn++;
                legendOffsetX += legendImage.Geo->dWidth + padding;
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
          status = imageDataWriter.createScaleBar(dataSources[0]->srvParams->Geo, &scaleBarImage, scaling);
          if (status != 0) throw(__LINE__);
          int posX = padding * scaling; // imageDataWriter.drawImage.Geo->dWidth-(scaleBarImage.Geo->dWidth+padding);
          int posY = imageDataWriter.drawImage.Geo->dHeight - (scaleBarImage.Geo->dHeight + padding * scaling);
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

      if (srvParam->requestType == REQUEST_WCS_GETCOVERAGE) {
        CBaseDataWriterInterface *wcsWriter = NULL;
        CT::string driverName = "ADAGUCNetCDF";
        setDimValuesForDataSource(dataSources[0], srvParam);

        for (size_t i = 0; i < srvParam->cfg->WCS[0]->WCSFormat.size(); i++) {
          if (srvParam->Format.equals(srvParam->cfg->WCS[0]->WCSFormat[i]->attr.name.c_str())) {
            driverName.copy(srvParam->cfg->WCS[0]->WCSFormat[i]->attr.driver.c_str());
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
            status = wcsWriter->init(srvParam, dataSources[0], dataSources[0]->getNumTimeSteps());
            if (status != 0) throw(__LINE__);
          } catch (int e) {
            CDBError("Exception code %d", e);

            throw(__LINE__);
          }

          for (int k = 0; k < dataSources[0]->getNumTimeSteps(); k++) {
            dataSources[0]->setTimeStep(k);
            CDBDebug("WCS dataset %d/ timestep %d of %d", 0, k, dataSources[0]->getNumTimeSteps());

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
      }

      if (srvParam->requestType == REQUEST_WMS_GETFEATUREINFO) {
        CImageDataWriter imageDataWriter;
        status = imageDataWriter.init(srvParam, dataSources[0], dataSources[0]->getNumTimeSteps());
        if (status != 0) throw(__LINE__);
        status = imageDataWriter.getFeatureInfo(dataSources, 0, int(srvParam->dX), int(srvParam->dY));
        if (status != 0) throw(__LINE__);
        status = imageDataWriter.end();
        if (status != 0) throw(__LINE__);
      }

      // WMS Getlegendgraphic
      if (srvParam->requestType == REQUEST_WMS_GETLEGENDGRAPHIC) {
        CImageDataWriter imageDataWriter;
        status = imageDataWriter.init(srvParam, dataSources[0], 1);
        if (status != 0) throw(__LINE__);
        bool rotate = srvParam->Geo->dWidth > srvParam->Geo->dHeight;
        CDBDebug("creatinglegend %dx%d %d", srvParam->Geo->dWidth, srvParam->Geo->dHeight, rotate);
        status = imageDataWriter.createLegend(dataSources[0], &imageDataWriter.drawImage, rotate);
        if (status != 0) throw(__LINE__);
        status = imageDataWriter.end();
        if (status != 0) throw(__LINE__);
      }

      // WMS GETHISTOGRAM
      if (srvParam->requestType == REQUEST_WMS_GETHISTOGRAM) {
        CCreateHistogram histogramCreator;
        CDBDebug("REQUEST_WMS_GETHISTOGRAM");

        try {
          status = histogramCreator.init(srvParam, dataSources[0], dataSources[0]->getNumTimeSteps());
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
        printf("%s%c%c\n", "Content-Type:text/plain", 13, 10);
        CDataReader reader;
        status = reader.open(dataSources[0], CNETCDFREADER_MODE_OPEN_HEADER);
        if (status != 0) {
          CDBError("Could not open file: %s", dataSources[0]->getFileName());
          throw(__LINE__);
        }
        CT::string dumpString = CDF::dump(dataSources[0]->getDataObject(0)->cdfObject);
        printf("%s", dumpString.c_str());
        reader.close();
      }

      if (srvParam->requestType == REQUEST_WMS_GETREFERENCETIMES) {
        status = generateGetReferenceTimes(dataSources[0]);
        if (status != 0) {
          throw(__LINE__);
        }
      }
    } catch (int i) {
      // Exception thrown: Cleanup and return;
      CDBError("Returning from line %d", i);
      return 1;
    }
  } else if (dataSources[0]->dLayerType == CConfigReaderLayerTypeLiveUpdate) {
    // Render the current time in an image for testing purpose / frontend development
    CDrawImage image;
    layerTypeLiveUpdateRenderIntoDrawImage(&image, srvParam);
    printf("%s%c%c\n", "Content-Type:image/png", 13, 10);
    image.printImagePng8(true);
  } else {
    CDBError("Unknown layer type");
  }
  //}

  return 0;
}

int CRequest::process_querystring() {

  /**
   * START Implementation of POST request.
   */
  //  char * method = getenv("REQUEST_METHOD");
  //
  //  //strcmp returns 0, means they are equal.
  //  if (!strcmp(method, "POST")) {
  //
  //    CT::string * post_body;
  //    long body_length = atoi(getenv("CONTENT_LENGTH"));
  //
  //    //Buffer size in memory
  //    char *buffer = (char*) malloc (sizeof(char)*body_length);
  //
  //    //Copy the content_body into the buffer:
  //    fread(buffer, body_length, 1, stdin);
  //    buffer[body_length] = 0;
  //
  //    //Copy Buffer to CT::string
  //    post_body->copy(buffer);
  //
  //    //Clear buffer
  //    free(buffer);
  //
  //    int status = CSLDPostRequest::startProcessing(post_body);
  //
  //    if(status != 0){
  //      CDBError("Something went wrong processing Post request");
  //    } else {
  //      #ifdef CSLD_POST_REQUEST_DEBUG
  //        CDBDebug("POST request is succesfully processed!");
  //      #endif
  //    }
  //  }
  /**
   * END Implementation of POST request.
   */

#ifdef MEASURETIME
  StopWatch_Stop("Start processing query string");
#endif
  //  StopWatch_Time("render()");
  // First try to find all possible dimensions
  // std::vector
  /* for(size_t j=0;j<srvParam->cfg->Layer.size();j++){
     for(size_t d=0;d<srvParam->cfg->Layer[j]->Dimension.size();d++){
       CT::string *dim = new CT::string(srvParam->cfg->Layer[j]->Dimension[d]->value.c_str());

       dim->toUpperCaseSelf();

       bool foundDim=false;
       for(size_t i=0;i<queryDims.size();i++){
         if(dim->equals(queryDims[i])){foundDim=true;break;}
       }
       if(foundDim==false){
         queryDims.push_back(dim);
       }else delete dim;
     }
   }
   */
  if (srvParam->cfg->WMS.size() != 1) {
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
  int dFound_WMSLAYERS = 0;
  int dFound_WMSLAYER = 0;
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
            CT::string *items = dapPath.splitToArray("?");
            COpenDAPHandler opendapHandler;
            opendapHandler.handleOpenDAPRequest(items[0].c_str(), pszQueryString, srvParam);
            delete[] items;
            return 0;
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
  // CDBDebug("QueryString: \"%s\"",queryString.c_str());
  CT::string *parameters = queryString.splitToArray("&");

#ifdef CREQUEST_DEBUG
  CDBDebug("Parsing query string parameters");
#endif
  for (size_t j = 0; j < parameters->count; j++) {
    CT::string uriKeyUpperCase;
    CT::string uriValue;

    int equalPos = parameters[j].indexOf("="); // splitToArray("=");

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
        return 1;
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
        return 1;
      }
    }
    if (!uriValue.empty()) {
      // BBOX Parameters
      if (uriKeyUpperCase.equals("BBOX")) {
        CT::string *bboxvalues = uriValue.replace("%2C", ",").splitToArray(",");
        if (bboxvalues->count == 4) {
          for (int j = 0; j < 4; j++) {
            srvParam->Geo->dfBBOX[j] = atof(bboxvalues[j].c_str());
          }
        } else {
          CDBError("ADAGUC Server: Invalid BBOX values");
          return 1;
        }
        delete[] bboxvalues;
        srvParam->dFound_BBOX = 1;
      }
      if (uriKeyUpperCase.equals("BBOXWIDTH")) {

        srvParam->Geo->dfBBOX[0] = 0;
        srvParam->Geo->dfBBOX[1] = 0;
        srvParam->Geo->dfBBOX[2] = uriValue.toDouble();
        srvParam->Geo->dfBBOX[3] = uriValue.toDouble();

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
        srvParam->Geo->dWidth = atoi(uriValue.c_str());
        if (srvParam->Geo->dWidth < 1) {
          CDBError("ADAGUC Server: Parameter Width should be at least 1");
          return 1;
        }
        dFound_Width = 1;
      }
      // Height Parameters
      if (uriKeyUpperCase.equals("HEIGHT")) {
        srvParam->Geo->dHeight = atoi(uriValue.c_str());
        if (srvParam->Geo->dHeight < 1) {
          CDBError("ADAGUC Server: Parameter Height should be at least 1");
          return 1;
        }

        dFound_Height = 1;
      }
      // RESX Parameters
      if (uriKeyUpperCase.equals("RESX")) {
        srvParam->dfResX = atof(uriValue.c_str());
        if (srvParam->dfResX == 0) {
          CDBError("ADAGUC Server: Parameter RESX should not be zero");
          return 1;
        }
        dFound_RESX = 1;
      }
      // RESY Parameters
      if (uriKeyUpperCase.equals("RESY")) {
        srvParam->dfResY = atof(uriValue.c_str());
        if (srvParam->dfResY == 0) {
          CDBError("ADAGUC Server: Parameter RESY should not be zero");
          return 1;
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
          srvParam->Geo->CRS.copy(uriValue);
          // srvParam->Geo->CRS.decodeURLSelf();
          dFound_SRS = 1;
        }
      }
      if (uriKeyUpperCase.equals("CRS")) {
        if (uriValue.length() > 2) {
          srvParam->Geo->CRS.copy(uriValue);
          dFound_CRS = 1;
        }
      }

      if (uriKeyUpperCase.equals("RESPONSE_CRS")) {
        if (uriValue.length() > 2) {
          srvParam->Geo->BBOX_CRS.copy(uriValue);
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
          return 1;
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
          return 1;
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
          return 1;
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
          return 1;
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
          return 1;
        }
      }

      // Opendap source parameter
      if (dFound_autoResourceLocation == 0) {
        if (uriKeyUpperCase.equals("SOURCE")) {
          if (srvParam->autoResourceLocation.empty()) {
            CT::string *hashList = uriValue.splitToArray("#");
            srvParam->autoResourceLocation.copy(hashList[0].c_str());
            delete[] hashList;
          }
          dFound_autoResourceLocation = 1;
        }
      }

      // WMS Layers parameter
      if (uriKeyUpperCase.equals("LAYERS")) {
        if (srvParam->WMSLayers != NULL) {
          delete[] srvParam->WMSLayers;
        }
        srvParam->WMSLayers = uriValue.splitToArray(",");
        dFound_WMSLAYERS = 1;
      }
      // WMS Layer parameter
      if (uriKeyUpperCase.equals("LAYER")) {
        if (srvParam->WMSLayers != NULL) {
          delete[] srvParam->WMSLayers;
        }
        srvParam->WMSLayers = uriValue.splitToArray(",");
        dFound_WMSLAYER = 1;
      }

      // WMS Layer parameter
      if (uriKeyUpperCase.equals("QUERY_LAYERS")) {
        if (srvParam->WMSLayers != NULL) {
          delete[] srvParam->WMSLayers;
        }
        srvParam->WMSLayers = uriValue.splitToArray(",");
        dFound_WMSLAYER = 1;
      }
      // WCS Coverage parameter
      if (uriKeyUpperCase.equals("COVERAGE")) {
        if (srvParam->WMSLayers != NULL) {
          CDBError("ADAGUC Server: COVERAGE already defined");
          return 1;
        } else {
          srvParam->WMSLayers = uriValue.splitToArray(",");
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
        CT::string *valuesC = uriValue.splitToArray(",");
        if (valuesC->count == 2) {
          srvParam->wmsExtensions.colorScaleRangeMin = valuesC[0].toDouble();
          srvParam->wmsExtensions.colorScaleRangeMax = valuesC[1].toDouble();
          srvParam->wmsExtensions.colorScaleRangeSet = true;
        }
        delete[] valuesC;
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
          return 1;
        }
      }
    }
  }
  delete[] parameters;

  if (dFound_Width == 0 && dFound_Height == 0) {
    if (srvParam->dFound_BBOX && dFound_RESX && dFound_RESY) {
      srvParam->Geo->dWidth = int(((srvParam->Geo->dfBBOX[2] - srvParam->Geo->dfBBOX[0]) / srvParam->dfResX));
      srvParam->Geo->dHeight = int(((srvParam->Geo->dfBBOX[1] - srvParam->Geo->dfBBOX[3]) / srvParam->dfResY));
      srvParam->Geo->dHeight = abs(srvParam->Geo->dHeight);
#ifdef CREQUEST_DEBUG
      CDBDebug("Calculated width height based on resx resy %d,%d", srvParam->Geo->dWidth, srvParam->Geo->dHeight);
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
    return 1;
  }
  if (dFound_Styles == 0) {
    srvParam->Styles.copy("");
  }
  if (SERVICE.equals("WMS")) srvParam->serviceType = SERVICE_WMS;
  if (SERVICE.equals("WCS")) srvParam->serviceType = SERVICE_WCS;
  if (SERVICE.equals("METADATA")) srvParam->serviceType = SERVICE_METADATA;

  if (CAutoResource::configure(srvParam, false) != 0) {
    CDBError("AutoResource failed");
    return 1;
  }

  if (srvParam->serviceType == SERVICE_WMS) {
#ifdef CREQUEST_DEBUG
    CDBDebug("Getting parameters for WMS service");
#endif

    // Check the version
    if (dFound_Version != 0) {
      srvParam->OGCVersion = -1; // WMS_VERSION_1_1_1;
      if (Version.equals("1.0.0"))
        srvParam->OGCVersion = WMS_VERSION_1_0_0;
      else if (Version.equals("1.1.1"))
        srvParam->OGCVersion = WMS_VERSION_1_1_1;
      else if (Version.equals("1.3.0"))
        srvParam->OGCVersion = WMS_VERSION_1_3_0;
      else {
        CDBError("Invalid version ('%s'): WMS 1.0.0, WMS 1.1.1 and WMS 1.3.0 are supported", Version.c_str());
        return 1;
      }
    }
    // Default is 1.3.0
    srvParam->OGCVersion = WMS_VERSION_1_3_0;

    if (dFound_Request == 0) {
      CDBError("ADAGUC Server: Parameter REQUEST missing");
      return 1;
    }
    if (REQUEST.equals("GETCAPABILITIES")) srvParam->requestType = REQUEST_WMS_GETCAPABILITIES;
    if (REQUEST.equals("GETMAP")) srvParam->requestType = REQUEST_WMS_GETMAP;
    if (REQUEST.equals("GETHISTOGRAM")) srvParam->requestType = REQUEST_WMS_GETHISTOGRAM;
    if (REQUEST.equals("GETSCALEBAR")) srvParam->requestType = REQUEST_WMS_GETSCALEBAR;
    if (REQUEST.equals("GETFEATUREINFO")) srvParam->requestType = REQUEST_WMS_GETFEATUREINFO;
    if (REQUEST.equals("GETPOINTVALUE")) srvParam->requestType = REQUEST_WMS_GETPOINTVALUE;
    if (REQUEST.equals("GETLEGENDGRAPHIC")) srvParam->requestType = REQUEST_WMS_GETLEGENDGRAPHIC;
    if (REQUEST.equals("GETMETADATA")) srvParam->requestType = REQUEST_WMS_GETMETADATA;
    if (REQUEST.equals("GETSTYLES")) srvParam->requestType = REQUEST_WMS_GETSTYLES;
    if (REQUEST.equals("GETREFERENCETIMES")) srvParam->requestType = REQUEST_WMS_GETREFERENCETIMES;

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
        // BBOX swap
        double dfBBOX[4];
        for (int j = 0; j < 4; j++) {
          dfBBOX[j] = srvParam->Geo->dfBBOX[j];
        }
        srvParam->Geo->dfBBOX[0] = dfBBOX[1];
        srvParam->Geo->dfBBOX[1] = dfBBOX[0];
        srvParam->Geo->dfBBOX[2] = dfBBOX[3];
        srvParam->Geo->dfBBOX[3] = dfBBOX[2];
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
      } else if (Exceptions.equals("application/vnd.ogc.se_inimage") || Exceptions.equals("INIMAGE")) {
        seterrormode(WMS_EXCEPTIONS_IMAGE);
      } else if (Exceptions.equals("application/vnd.ogc.se_blank") || Exceptions.equals("BLANK")) {
        seterrormode(WMS_EXCEPTIONS_BLANKIMAGE);
      } else if (Exceptions.equals("XML")) {
        if (srvParam->OGCVersion == WMS_VERSION_1_1_1)
          seterrormode(WMS_EXCEPTIONS_XML_1_1_1);
        else if (srvParam->OGCVersion == WMS_VERSION_1_3_0)
          seterrormode(WMS_EXCEPTIONS_XML_1_3_0);
      }
    } else {
      // EXCEPTIONS not set in request
    }

    if (srvParam->requestType == REQUEST_WMS_GETSTYLES) {
      return process_wms_getstyles_request() != 0;
    }

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
        return 1;
      }
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
        } else if (outputFormat.indexOf("gif") > 0) {
          srvParam->imageFormat = IMAGEFORMAT_IMAGEGIF;
          srvParam->imageMode = SERVERIMAGEMODE_8BIT;
        }
      }
    }

    if (srvParam->requestType == REQUEST_WMS_GETMAP || srvParam->requestType == REQUEST_WMS_GETFEATUREINFO || srvParam->requestType == REQUEST_WMS_GETPOINTVALUE ||
        srvParam->requestType == REQUEST_WMS_GETHISTOGRAM

    ) {

      if (srvParam->requestType == REQUEST_WMS_GETFEATUREINFO || srvParam->requestType == REQUEST_WMS_GETPOINTVALUE || srvParam->requestType == REQUEST_WMS_GETHISTOGRAM) {
        int status = CServerParams::checkDataRestriction();
        if ((status & ALLOW_GFI) == false) {
          CDBError("ADAGUC Server: This layer is not queryable.");
          return 1;
        }
      }

      if (dFound_Width == 0 && dFound_Height == 0) {
        CDBError("ADAGUC Server: Parameter WIDTH or HEIGHT missing");
        return 1;
      }
      // Check if styles is defined for WMS 1.1.1
      if (dFound_Styles == 0 && srvParam->requestType == REQUEST_WMS_GETMAP && srvParam->OGCVersion == WMS_VERSION_1_1_1) {
        // CDBError("ADAGUC Server: Parameter STYLES missing");TODO Google Earth does not provide this! Disabled this
        // check for the moment.
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
        srvParam->Geo->dfBBOX[0] = srvParam->dX;
        srvParam->Geo->dfBBOX[1] = srvParam->dY;
        srvParam->Geo->dfBBOX[2] = srvParam->Geo->dfBBOX[0];
        srvParam->Geo->dfBBOX[3] = srvParam->Geo->dfBBOX[1];
        srvParam->Geo->dWidth = 1;
        srvParam->Geo->dHeight = 1;
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

      if (dFound_Width == 0) {
        if (srvParam->Geo->dfBBOX[2] != srvParam->Geo->dfBBOX[0]) {
          float r = fabs(srvParam->Geo->dfBBOX[3] - srvParam->Geo->dfBBOX[1]) / fabs(srvParam->Geo->dfBBOX[2] - srvParam->Geo->dfBBOX[0]);
          srvParam->Geo->dWidth = int(float(srvParam->Geo->dHeight) / r);
          if (srvParam->Geo->dWidth > MAX_IMAGE_WIDTH) {
            srvParam->Geo->dWidth = srvParam->Geo->dHeight;
          }
        } else {
          srvParam->Geo->dWidth = srvParam->Geo->dHeight;
        }
      }

      if (dFound_Height == 0) {
        if (srvParam->Geo->dfBBOX[2] != srvParam->Geo->dfBBOX[0]) {
          float r = fabs(srvParam->Geo->dfBBOX[3] - srvParam->Geo->dfBBOX[1]) / fabs(srvParam->Geo->dfBBOX[2] - srvParam->Geo->dfBBOX[0]);
          CDBDebug("NNOX = %f, %f, %f, %f", srvParam->Geo->dfBBOX[0], srvParam->Geo->dfBBOX[1], srvParam->Geo->dfBBOX[2], srvParam->Geo->dfBBOX[3]);
          CDBDebug("R = %f", r);
          srvParam->Geo->dHeight = int(float(srvParam->Geo->dWidth) * r);
          if (srvParam->Geo->dHeight > MAX_IMAGE_HEIGHT) {
            srvParam->Geo->dHeight = srvParam->Geo->dWidth;
          }
        } else {
          srvParam->Geo->dHeight = srvParam->Geo->dWidth;
        }
        CDBDebug("Calculated height: %d", srvParam->Geo->dHeight);
        CDBDebug("dWidth: %d", srvParam->Geo->dWidth);
      }

      if (srvParam->Geo->dWidth < 0) srvParam->Geo->dWidth = 1;
      if (srvParam->Geo->dHeight < 0) srvParam->Geo->dHeight = 1;

      // When error is image, utilize full image size
      setErrorImageSize(srvParam->Geo->dWidth, srvParam->Geo->dHeight, srvParam->imageFormat, srvParam->Transparent);

      if ((srvParam->OGCVersion == WMS_VERSION_1_0_0 || srvParam->OGCVersion == WMS_VERSION_1_1_1) && dFound_SRS == 0) {
        CDBError("ADAGUC Server: Parameter SRS missing");
        return 1;
      }

      if (srvParam->OGCVersion == WMS_VERSION_1_3_0 && dFound_CRS == 0) {
        CDBError("ADAGUC Server: Parameter CRS missing");
        return 1;
      }

      if (dFound_WMSLAYERS == 0) {
        CDBError("ADAGUC Server: Parameter LAYERS missing");
        return 1;
      }

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
            return 1;
          }
          if (dFound_Y == 0) {
            CDBError("ADAGUC Server: Parameter Y missing");
            return 1;
          }
        }

        if (srvParam->OGCVersion == WMS_VERSION_1_3_0) {
          if (dFound_I == 0) {
            CDBError("ADAGUC Server: Parameter I missing");
            return 1;
          }
          if (dFound_J == 0) {
            CDBError("ADAGUC Server: Parameter J missing");
            return 1;
          }
        }

        int status = process_wms_getfeatureinfo_request();
        if (status != 0) {
          if (status != 2) {
            CDBError("WMS GetFeatureInfo Request failed");
          }
          return 1;
        }
        return 0;
      }
    } else {
      CDBError("ADAGUC Server: Invalid value for request. Supported requests are: getcapabilities, getmap, gethistogram, "
               "getfeatureinfo, getpointvalue, getmetadata, getReferencetimes, getstyles and getlegendgraphic");
      return 1;
    }

    // WMS GETSCALEBAR
    if (srvParam->requestType == REQUEST_WMS_GETSCALEBAR) {

      CDrawImage drawImage;

      drawImage.setCanvasColorType(CDRAWIMAGE_COLORTYPE_ARGB);
      drawImage.setRenderer(CDRAWIMAGERENDERER_CAIRO);
      drawImage.enableTransparency(true);

      // Set font location
      if (srvParam->cfg->WMS[0]->ContourFont.size() != 0) {
        if (srvParam->cfg->WMS[0]->ContourFont[0]->attr.location.empty() == false) {
          drawImage.setTTFFontLocation(srvParam->cfg->WMS[0]->ContourFont[0]->attr.location.c_str());
          if (srvParam->cfg->WMS[0]->ContourFont[0]->attr.size.empty() == false) {
            // srvParam->cfg->WMS[0]->ContourFont[0]->attr.size.c_str();
            drawImage.setTTFFontSize(7);
          }
        } else {
          CDBError("In <Font>, attribute \"location\" missing");
          return 1;
        }
      }
      drawImage.createImage(300, 30);
      drawImage.create685Palette();
      try {
        CCreateScaleBar::createScaleBar(&drawImage, srvParam->Geo, 1);
      } catch (int e) {
        CDBError("Exception %d", e);
        return 1;
      }
      drawImage.crop(1);
      const char *cacheControl = srvParam->getCacheControlHeader(CSERVERPARAMS_CACHE_CONTROL_OPTION_FULLYCACHEABLE).c_str();
      printf("%s%s%c%c\n", "Content-Type:image/png", cacheControl, 13, 10);
      drawImage.printImagePng8(true);
      return 0;
    }

    if (srvParam->requestType == REQUEST_WMS_GETLEGENDGRAPHIC) {
      if (dFound_WMSLAYER == 0) {
        CDBError("ADAGUC Server: Parameter LAYER missing");
        return 1;
      }
      int status = process_wms_getlegendgraphic_request();
      if (status != 0) {
        CDBError("WMS GetLegendGraphic Request failed");
        return 1;
      }
      return 0;
    }
  }

  if (srvParam->requestType == REQUEST_WMS_GETCAPABILITIES) {
    int status = process_wms_getcap_request();
    if (status != 0) {
      CDBError("GetCapabilities failed");
    }
    return status;
  }
  if (srvParam->requestType == REQUEST_WMS_GETMETADATA) {
    int status = CServerParams::checkDataRestriction();
    if ((status & ALLOW_METADATA) == false) {
      CDBError("ADAGUC Server: GetMetaData is restricted");
      return 1;
    }
    if (dFound_WMSLAYER == 0) {
      CDBError("ADAGUC Server: Parameter LAYER missing");
      return 1;
    }
    if (dFound_Format == 0) {
      CDBError("ADAGUC Server: Parameter FORMAT missing");
      return 1;
    }
    if (process_wms_getmetadata_request() != 0) {
      CDBError("WMS GetMetaData Request failed");
      return 1;
    }
    return 0;
  }

  if (srvParam->serviceType == SERVICE_WCS) {
    srvParam->OGCVersion = WCS_VERSION_1_0;
    if ((CServerParams::checkDataRestriction() & ALLOW_WCS) == false) {
      CDBError("ADAGUC Server: WCS Service is disabled.");
      return 1;
    }
    if (dFound_Request == 0) {
      CDBError("ADAGUC Server: Parameter REQUEST missing");
      return 1;
    }

    if (REQUEST.equals("DESCRIBECOVERAGE")) {
      srvParam->requestType = REQUEST_WCS_DESCRIBECOVERAGE;
      if (process_wcs_describecov_request() != 0) {
        CDBError("WCS DescribeCoverage Request failed");
      }
      return 0;
    } else if (REQUEST.equals("GETCOVERAGE")) {
      srvParam->requestType = REQUEST_WCS_GETCOVERAGE;
      CDBDebug("WCS");

      if (dFound_Width == 0 && dFound_Height == 0 && dFound_RESX == 0 && dFound_RESY == 0 && srvParam->dFound_BBOX == 0 && dFound_CRS == 0)
        srvParam->WCS_GoNative = 1;
      else {
        if (srvParam->dFound_BBOX == 0) {
          CDBError("ADAGUC Server: Parameter BBOX missing");
          return 1;
        }
        if (dFound_CRS == 0) {
          CDBError("ADAGUC Server: Parameter CRS missing");
          return 1;
        }
        if (dFound_Format == 0) {
          CDBError("ADAGUC Server: Parameter FORMAT missing");
          return 1;
        }
        if (dFound_WCSCOVERAGE == 0) {
          CDBError("ADAGUC Server: Parameter COVERAGE missing");
          return 1;
        }
        if (dFound_CRS == 1 && dFound_RESPONSE_CRS == 1) {
          CT::string CRS = srvParam->Geo->BBOX_CRS;
          CT::string RESPONSE_CRS = srvParam->Geo->CRS;
          srvParam->Geo->CRS = CRS;
          srvParam->Geo->BBOX_CRS = RESPONSE_CRS;
        } else {
          srvParam->Geo->BBOX_CRS = srvParam->Geo->CRS;
        }
        srvParam->WCS_GoNative = 0;
      }

      if (process_wcs_getcoverage_request() != 0) {
        CDBError("WCS GetCoverage Request failed");
        return 1;
      }
      return 0;
    } else if (REQUEST.equals("GETCAPABILITIES")) {
      srvParam->requestType = REQUEST_WCS_GETCAPABILITIES;
      return process_wcs_getcap_request();
    }
    CDBError("ADAGUC Server: Invalid value for request. Supported requests are: getcapabilities, describecoverage and "
             "getcoverage");
    return 1;
  }

  if (REQUEST.equals("GETMETADATA")) {
    srvParam->requestType = REQUEST_METADATA_GETMETADATA;
    if (srvParam->autoResourceLocation.empty()) {
      CDBError("No source defined for metadata request");
      return 1;
    }
    return CHandleMetadata().process(srvParam);
  }

  CDBError("ADAGUC Server: Unknown service");

#ifdef MEASURETIME
  StopWatch_Stop("End of query string");
#endif

  return 1;
}

int CRequest::updatedb(CT::string *tailPath, CT::string *layerPathToScan, int scanFlags, CT::string layerName) {
  int errorHasOccured = 0;
  int status;
  // Fill in all data sources from the configuration object
  size_t numberOfLayers = srvParam->cfg->Layer.size();

  for (size_t layerNo = 0; layerNo < numberOfLayers; layerNo++) {
    CDataSource *dataSource = new CDataSource();
    if (dataSource->setCFGLayer(srvParam, srvParam->configObj->Configuration[0], srvParam->cfg->Layer[layerNo], NULL, layerNo) != 0) {
      delete dataSource;
      return 1;
    }
    if (!layerName.empty() && srvParam->cfg->Layer[layerNo]->Name.size() == 1) {
      CT::string simpleLayerName = srvParam->cfg->Layer[layerNo]->Name[0]->value;
      if (layerName.equals(simpleLayerName)) {
        dataSources.push_back(dataSource);
      }
    } else {
      dataSources.push_back(dataSource);
    }
  }

  srvParam->requestType = REQUEST_UPDATEDB;
  bool file_was_added = false;

  for (size_t j = 0; j < dataSources.size(); j++) {
    if (dataSources[j]->dLayerType == CConfigReaderLayerTypeDataBase || dataSources[j]->dLayerType == CConfigReaderLayerTypeBaseLayer) {
      if (scanFlags & CDBFILESCANNER_UPDATEDB) {
        file_was_added = CDBFileScanner::updatedb(dataSources[j], tailPath, layerPathToScan, scanFlags);
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
  if (file_was_added == false && layerPathToScan->length() > 0) {
    CDBWarning("The specified file %s did not match to any of the layers", layerPathToScan->c_str());
  }
  if (srvParam->enableDocumentCache) {
    // invalidate cache
    CT::string cacheFileName;
    srvParam->getCacheFileName(&cacheFileName);

    CDBDebug("Invalidating cache file [%s]", cacheFileName.c_str());
    // Remove the cache file, but check first wether it exists or not.
    struct stat stFileInfo;
    // The file exists, so remove it.
    if (stat(cacheFileName.c_str(), &stFileInfo) == 0) {
      CDBDebug("Removing cachefile %s ", cacheFileName.c_str());
      if (remove(cacheFileName.c_str()) != 0) {
        CDBError("Unable to remove cachefile %s, please do it manually.", cacheFileName.c_str());
        return 1;
      }
    } else {
      CDBDebug("There is no cachefile");
    }
  }
  /*
  CSimpleStore simpleStore;
  status = getDocFromDocCache(&simpleStore,NULL,NULL);
  simpleStore.setStringAttribute("configModificationDate","needsupdate!");
  if(storeDocumentCache(&simpleStore)!=0)return 1;*/
  if (errorHasOccured) {
    CDBDebug("***** Finished DB Update with %d errors *****", errorHasOccured);
  } else {
    CDBDebug("***** Finished DB Update *****");
  }

  CDFObjectStore::getCDFObjectStore()->clear();
  CConvertGeoJSON::clearFeatureStore();
  CDFStore::clear();
  CDBFactory::clear();
  return errorHasOccured > 0;
}

int CRequest::getDocumentCacheName(CT::string *documentName, CServerParams *srvParam) {
  documentName->copy("none");
  if (srvParam->requestType == REQUEST_WMS_GETCAPABILITIES) {
    if (srvParam->OGCVersion == WMS_VERSION_1_0_0) {
      documentName->copy("WMS_1_0_0_GetCapabilities");
    } else if (srvParam->OGCVersion == WMS_VERSION_1_1_1) {
      documentName->copy("WMS_1_1_1_GetCapabilities");
    } else if (srvParam->OGCVersion == WMS_VERSION_1_3_0) {
      documentName->copy("WMS_1_3_0_GetCapabilities");
    }
  }
  if (srvParam->requestType == REQUEST_WMS_GETREFERENCETIMES) {
    documentName->copy("WMS_1_3_0_GetReferenceTimes");
  }
  if (srvParam->requestType == REQUEST_WCS_GETCAPABILITIES && srvParam->OGCVersion == WCS_VERSION_1_0) {
    documentName->copy("WCS_1_0_GetCapabilities");
  }
  if (srvParam->requestType == REQUEST_WCS_DESCRIBECOVERAGE && srvParam->OGCVersion == WCS_VERSION_1_0) {
    documentName->copy("WCS_1_0_DescribeCoverage");
  }

  if (documentName->equals("none")) {
    CDBError("Unknown cache request...");
    return 1;
  }
  return 0;
}

// pthread_mutex_t CImageDataWriter_addData_lock;
void *CImageDataWriter_addData(void *arg) {

  //   pthread_mutex_lock(&CImageDataWriter_addData_lock);
  CImageDataWriter_addData_args *imgdwArg = (CImageDataWriter_addData_args *)arg;
  imgdwArg->status = imgdwArg->imageDataWriter->addData(imgdwArg->dataSources);

  imgdwArg->finished = true;
  //   pthread_mutex_unlock(&CImageDataWriter_addData_lock);
  imgdwArg->running = false;
  return NULL;
}
