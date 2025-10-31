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
#include <algorithm>
#include <cstdio>
#include <random>
#include "CReadFile.h"
#include "CServerParams.h"
#include "CStopWatch.h"
#include <traceTimings/traceTimings.h>
#include <cstring>

const char *CServerParams::className = "CServerParams";

CServerParams::CServerParams() {

  serviceType = -1;
  requestType = -1;
  OGCVersion = -1;

  Transparent = false;
  cfg = NULL;
  configObj = new CServerConfig();
  Geo = new CGeoParams;
  imageFormat = IMAGEFORMAT_IMAGEPNG8;
  imageMode = SERVERIMAGEMODE_8BIT;
  autoOpenDAPEnabled = -1;
  autoLocalFileResourceEnabled = -1;
  showDimensionsInImage = false;
  showLegendInImage = "false";
  showScaleBarInImage = false;
  figWidth = -1;
  figHeight = -1;
  imageQuality = 85;
  dfResX = 0;
  dfResY = 0;
}

CServerParams::~CServerParams() {
  if (configObj != NULL) {
    delete configObj;
    configObj = NULL;
  }
  if (Geo != NULL) {
    delete Geo;
    Geo = NULL;
  }
  for (size_t j = 0; j < requestDims.size(); j++) {
    delete requestDims[j];
    requestDims[j] = NULL;
  }
  requestDims.clear();
}

std::string CServerParams::randomString(const int length) {
#ifdef MEASURETIME
  StopWatch_Stop(">CServerParams::randomString");
#endif

  const char charset[] = "0123456789"
                         "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                         "abcdefghijklmnopqrstuvwxyz";
  // -1 for the \0 and -1 because uniform_int_distribution uses closed bounds
  const size_t max_index = (sizeof(charset) - 2);

  static std::mt19937 engine = []() {
    std::random_device rd;
    return std::mt19937(rd());
  }();
  static std::uniform_int_distribution<int> dist(0, max_index);

  auto randChar = [&charset]() -> char { return charset[dist(engine)]; };

  std::string str(length, 0);
  std::generate_n(str.begin(), length, randChar);

#ifdef MEASURETIME
  StopWatch_Stop("<CServerParams::randomString");
#endif
  return str;
}

// Table names need to be different between dims like time and height.
//  Therefor create unique tablenames like tablename_time and tablename_height
CT::string CServerParams::makeCorrectTableName(CT::string tableName, CT::string dimName) {
  CT::string correctedTableName;
  correctedTableName.print("%s_%s", tableName.c_str(), dimName.c_str());
  correctedTableName.replaceSelf("-", "_m_");
  correctedTableName.replaceSelf("+", "_p_");
  correctedTableName.replaceSelf(".", "_");
  correctedTableName.toLowerCaseSelf();
  return correctedTableName;
}

void CServerParams::showWCSNotEnabledErrorMessage() { CDBError("WCS is not enabled because GDAL was not compiled into the server. "); }

int CServerParams::makeLayerGroupName(CT::string *groupName, CServerConfig::XMLE_Layer *cfgLayer) {
  /*
  if(cfgLayer->Variable.size()!=0){
  _layerName=cfgLayer->Variable[0]->value.c_str();
}*/

  CT::string layerName;
  groupName->copy("");
  if (cfgLayer->Group.size() == 1) {
    if (cfgLayer->Group[0]->attr.value.c_str() != NULL) {
      CT::string layerName(cfgLayer->Group[0]->attr.value.c_str());
      auto groupElements = layerName.splitToStack("/");
      if (groupElements.size() > 0) {
        groupName->copy(groupElements[0].c_str());
      }
    }
  }

  return 0;
}

bool CServerParams::isAutoResourceCacheEnabled() const {

  if (cfg->AutoResource.size() > 0) return cfg->AutoResource[0]->attr.enablecache.equals("true");
  return false;
}

char CServerParams::debugLoggingIsEnabled = -1; // Not configured yet, 1 means enabled, 0 means disabled

bool CServerParams::isDebugLoggingEnabled() const {
  if (debugLoggingIsEnabled == 0)
    return false;
  else if (debugLoggingIsEnabled == 1)
    return true;
  else if (cfg && cfg->Logging.size() > 0) {
    if (cfg->Logging[cfg->Logging.size() - 1]->attr.debug.equals("false")) {
      debugLoggingIsEnabled = 0;
      return false;
    }
  }
  debugLoggingIsEnabled = 1;
  return true;
}

bool CServerParams::isAutoOpenDAPResourceEnabled() {
  if (this->datasetLocation.empty() == false) {
    return false;
  }
  if (autoOpenDAPEnabled == -1) {
    autoOpenDAPEnabled = 0;
    if (cfg->AutoResource.size() > 0) {
      if (cfg->AutoResource[0]->attr.enableautoopendap.equals("true")) autoOpenDAPEnabled = 1;
    }
  }
  if (autoOpenDAPEnabled == 1) return true;
  return false;
}

bool CServerParams::isAutoLocalFileResourceEnabled() {
  /* When a dataset is configured, autoscan should be disabled */
  if (this->datasetLocation.empty() == false) {
    return false;
  }
  if (autoLocalFileResourceEnabled == -1) {
    autoLocalFileResourceEnabled = 0;
    if (cfg->AutoResource.size() > 0) {
      if (cfg->AutoResource[0]->attr.enablelocalfile.equals("true")) autoLocalFileResourceEnabled = 1;
    }
  }
  if (autoLocalFileResourceEnabled == 1) return true;
  return false;
}

bool CServerParams::isAutoResourceEnabled() {
  if (isAutoOpenDAPResourceEnabled() || isAutoLocalFileResourceEnabled()) return true;
  return false;
}

bool CServerParams::checkIfPathHasValidTokens(const char *path) { return checkForValidTokens(path, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789/_-+:. ,[]"); }

bool CServerParams::checkForValidTokens(const char *path, const char *validPATHTokens) {
  // Check for valid tokens
  size_t pathLength = strlen(path);
  size_t allowedTokenLength = strlen(validPATHTokens);
  for (size_t j = 0; j < pathLength; j++) {
    bool isInvalid = true;
    for (size_t i = 0; i < allowedTokenLength; i++) {
      if (path[j] == validPATHTokens[i]) {
        isInvalid = false;
        break;
      }
    }
    if (isInvalid) {
      CDBDebug("Invalid token '%c' in '%s'", path[j], path);
      return false;
    }

    // Check for sequences
    if (j > 0) {
      if (path[j - 1] == '.' && path[j] == '.') {
        CDBDebug("Invalid sequence in '%s'", path);
        return false;
      }
    }
  }
  return true;
}

bool CServerParams::checkResolvePath(const char *path, CT::string *resolvedPath) {
  if (cfg->AutoResource.size() > 0) {
    // Needs to be configured otherwise it will be denied.
    if (cfg->AutoResource[0]->Dir.size() == 0) {
      CDBDebug("No Dir elements defined");
      return false;
    }

    if (checkIfPathHasValidTokens(path) == false) return false;

    for (size_t d = 0; d < cfg->AutoResource[0]->Dir.size(); d++) {
      const char *_baseDir = cfg->AutoResource[0]->Dir[d]->attr.basedir.c_str();
      const char *dirPrefix = cfg->AutoResource[0]->Dir[d]->attr.prefix.c_str();

      char baseDir[PATH_MAX];
      if (realpath(_baseDir, baseDir) == NULL) {
        CDBError("Skipping AutoResource[0]->Dir[%d]->basedir: Configured value is not a valid realpath", d);
        continue;
      }

      if (strlen(baseDir) > 0 && strlen(dirPrefix) > 0) {
        // Prepend the prefix to make the absolute path
        CT::string pathToCheck;
        pathToCheck.print("%s/%s", dirPrefix, path);
        // Make a realpath
        char szResolvedPath[PATH_MAX];
        if (realpath(pathToCheck.c_str(), szResolvedPath) == NULL) {
          // CDBDebug("basedir='%s', prefix='%s', inputpath='%s', absolutepath='%s'",baseDir,dirPrefix,path,pathToCheck.c_str());
          CDBDebug("LOCALFILEACCESS: Invalid path '%s'", pathToCheck.c_str());
        } else {
          // Check if the resolved path is within the basedir
          // CDBDebug("basedir='%s', prefix='%s', inputpath='%s', absolutepath='%s'",baseDir,dirPrefix,path,pathToCheck.c_str());
          CDBDebug("szResolvedPath: %s", szResolvedPath);
          CDBDebug("baseDir       : %s", baseDir);
          CT::string resolvedPathStr = szResolvedPath;
          if (resolvedPathStr.indexOf(baseDir) == 0) {
            resolvedPath->copy(szResolvedPath);
            return true;
          }
        }
      } else {
        if (strlen(baseDir)) {
          CDBDebug("basedir not defined");
        }
        if (dirPrefix == NULL) {
          CDBDebug("prefix not defined");
        }
      }
    }
  } else {
    CDBDebug("No AutoResource enabled");
  }
  return false;
}

void CServerParams::encodeTableName(CT::string *tableName) {
  tableName->replaceSelf("/", "_");
  tableName->toLowerCaseSelf();
}

void CServerParams::setOnlineResource(CT::string r) { _onlineResource = r; };

CT::string CServerParams::getOnlineResource() {
  if (_onlineResource.length() > 0) {
    return _onlineResource;
  }
  if (cfg->OnlineResource.size() == 0) {
    // No Online resource is given.
    const char *pszADAGUCOnlineResource = getenv("ADAGUC_ONLINERESOURCE");
    if (pszADAGUCOnlineResource == NULL) {
      // CDBDebug("Warning: No OnlineResources configured. Unable to get from config OnlineResource or from environment ADAGUC_ONLINERESOURCE");
      _onlineResource = "";
      return "";
    }
    CT::string onlineResource = pszADAGUCOnlineResource;
    _onlineResource = onlineResource;
    return onlineResource;
  }

  CT::string onlineResource = cfg->OnlineResource[0]->attr.value.c_str();

  // A full path is given in the configuration
  if (onlineResource.indexOf("http", 4) == 0) {
    _onlineResource = onlineResource;
    return onlineResource;
  }

  // Only the last part is given, we need to prepend the HTTP_HOST environment variable.
  const char *pszHTTPHost = getenv("HTTP_HOST");
  if (pszHTTPHost == NULL) {
    CDBError("Unable to determine HTTP_HOST");
    _onlineResource = "";
    return "";
  }
  CT::string httpHost = "http://";
  httpHost.concat(pszHTTPHost);
  httpHost.concat(&onlineResource);
  _onlineResource = httpHost;
  return httpHost;
}

bool CServerParams::checkBBOXXYOrder(const char *projName) {
  if (OGCVersion == WMS_VERSION_1_3_0) {
    CT::string projNameString;
    if (projName == NULL) {
      projNameString = Geo->CRS.c_str();
    } else {
      projNameString = projName;
    }
    if (projNameString.equals("EPSG:4326"))
      return true;
    else if (projNameString.equals("EPSG:4258"))
      return true;
    else if (projNameString.equals("CRS:84"))
      return true;
  }
  return false;
}

/**
 * Returns a stringlist with all possible legends available for this Legend config object.
 * This is usually a configured legend element in a layer, or a configured legend element in a style.
 * @param Legend a XMLE_Legend object configured in a style or in a layer
 * @return Pointer to a new stringlist with all possible legend names, must be deleted with delete. Is NULL on failure.
 */
std::vector<CT::string> CServerParams::getLegendNames(std::vector<CServerConfig::XMLE_Legend *> Legend) {
  if (Legend.size() == 0) {
    std::vector<CT::string> legendList;
    legendList.push_back("rainbow");
    return legendList;
  }
  std::vector<CT::string> stringList;
  for (size_t j = 0; j < Legend.size(); j++) {
    CT::string legendValue = Legend[j]->value.c_str();
    CT::StackList<CT::string> l1 = legendValue.splitToStack(",");
    for (auto li : l1) {
      if (li.length() > 0) {
        stringList.push_back(li);
      }
    }
  }
  return stringList;
}

int CServerParams::dataRestriction = -1;
int CServerParams::checkDataRestriction() {
  if (dataRestriction != -1) return dataRestriction;

  // By default no restrictions
  int dr = ALLOW_WCS | ALLOW_GFI | ALLOW_METADATA;
  const char *data = getenv("ADAGUC_DATARESTRICTION");
  if (data != NULL) {
    dr = ALLOW_NONE;
    CT::string temp(data);
    temp.toUpperCaseSelf();
    if (temp.equals("TRUE")) {
      dr = ALLOW_NONE;
    }
    if (temp.equals("FALSE")) {
      dr = ALLOW_WCS | ALLOW_GFI | ALLOW_METADATA;
    }
    // Decompose into stringlist and check each item
    CT::StackList<CT::string> items = temp.splitToStack("|");
    for (size_t j = 0; j < items.size(); j++) {
      items[j].replaceSelf("\"", "");
      if (items[j].equals("ALLOW_GFI")) dr |= ALLOW_GFI;
      if (items[j].equals("ALLOW_WCS")) dr |= ALLOW_WCS;
      if (items[j].equals("ALLOW_METADATA")) dr |= ALLOW_METADATA;
      if (items[j].equals("SHOW_QUERYINFO")) dr |= SHOW_QUERYINFO;
    }
  }

  dataRestriction = dr;
  return dataRestriction;
}

const char *timeFormatAllowedChars = "0123456789:TZ-/. _ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz()*";
bool CServerParams::checkTimeFormat(CT::string &timeToCheck) {
  if (timeToCheck.length() < 1) return false;
  //  bool isValidTime = false;
  // First test wether invalid characters are in this string
  int numValidChars = strlen(timeFormatAllowedChars);
  const char *timeChars = timeToCheck.c_str();
  int numTimeChars = strlen(timeChars);
  for (int j = 0; j < numTimeChars; j++) {
    int i = 0;
    for (i = 0; i < numValidChars; i++)
      if (timeChars[j] == timeFormatAllowedChars[i]) break;
    if (i == numValidChars) return false;
  }
  return true;
  /*for(int j=0;j<NUMTIMEFORMATS&&isValidTime==false;j++){
    isValidTime = timeToCheck.testRegEx(timeFormats[j].pattern);
  }
  return isValidTime;*/
}

int CServerParams::parseConfigFile(CT::string &pszConfigFile) {
  // Find variables to substitute
  std::vector<CServerConfig::XMLE_Environment> extraEnvironment;
  CServerParams tempServerParam;
  tempServerParam._parseConfigFile(pszConfigFile, nullptr);

  if (tempServerParam.configObj != nullptr && tempServerParam.configObj->Configuration.size() > 0 && tempServerParam.configObj->Configuration[0]->Environment.size() > 0) {
    for (size_t j = 0; j < tempServerParam.configObj->Configuration[0]->Environment.size(); j++) {
      CServerConfig::XMLE_Environment tmpEnv;
      tmpEnv.attr.name = (*tempServerParam.configObj->Configuration[0]->Environment[j]).attr.name;
      tmpEnv.attr.defaultVal = (*tempServerParam.configObj->Configuration[0]->Environment[j]).attr.defaultVal;
      extraEnvironment.push_back(tmpEnv);
    }
  }

  return _parseConfigFile(pszConfigFile, &extraEnvironment);
}

int CServerParams::_parseConfigFile(CT::string &pszConfigFile, std::vector<CServerConfig::XMLE_Environment> *extraEnvironment) {
  CT::string configFileData = "";

  try {
    try {
      configFileData = CReadFile::open(pszConfigFile.c_str());
    } catch (int e) {
      CDBError("Unable to open configuration file [%s], error %d", pszConfigFile.c_str(), e);
      return 1;
    }

    /* Substitute ADAGUC_PATH */
    const char *pszADAGUC_PATH = getenv("ADAGUC_PATH");
    if (pszADAGUC_PATH != NULL) {
      CT::string adagucPath = CDirReader::makeCleanPath(pszADAGUC_PATH);
      adagucPath = adagucPath + "/";
      configFileData.replaceSelf("{ADAGUC_PATH}", adagucPath.c_str());
    }

    /* Substitute ADAGUC_TMP */
    const char *pszADAGUC_TMP = getenv("ADAGUC_TMP");
    if (pszADAGUC_TMP != NULL) configFileData.replaceSelf("{ADAGUC_TMP}", pszADAGUC_TMP);

    /* Substitute ADAGUC_DB */
    const char *pszADAGUC_DB = getenv("ADAGUC_DB");
    if (pszADAGUC_DB != NULL) configFileData.replaceSelf("{ADAGUC_DB}", pszADAGUC_DB);

    /* Substitute ADAGUC_DATASET_DIR */
    const char *pszADAGUC_DATASET_DIR = getenv("ADAGUC_DATASET_DIR");
    if (pszADAGUC_DATASET_DIR != NULL) configFileData.replaceSelf("{ADAGUC_DATASET_DIR}", pszADAGUC_DATASET_DIR);

    /* Substitute ADAGUC_DATA_DIR */
    const char *pszADAGUC_DATA_DIR = getenv("ADAGUC_DATA_DIR");
    if (pszADAGUC_DATA_DIR != NULL) configFileData.replaceSelf("{ADAGUC_DATA_DIR}", pszADAGUC_DATA_DIR);

    /* Substitute ADAGUC_AUTOWMS_DIR */
    const char *pszADAGUC_AUTOWMS_DIR = getenv("ADAGUC_AUTOWMS_DIR");
    if (pszADAGUC_AUTOWMS_DIR != NULL) configFileData.replaceSelf("{ADAGUC_AUTOWMS_DIR}", pszADAGUC_AUTOWMS_DIR);

    if (extraEnvironment != nullptr) {
      /* Substitute any others as specified in env */
      if (extraEnvironment->size() > 0) {
        for (size_t j = 0; j < (*extraEnvironment).size(); j++) {
          CServerConfig::XMLE_Environment *env = &(*extraEnvironment)[j];
          if (env != nullptr) {
            if (!env->attr.name.empty() && !env->attr.defaultVal.empty()) {

              if (env->attr.name.startsWith(CSERVERPARAMS_ADAGUCENV_PREFIX)) {
                const char *environmentVarName = env->attr.name.c_str();
                const char *environmentVarDefault = env->attr.defaultVal.c_str();
                const char *environmentValue = getenv(environmentVarName);
                CT::string substituteName;
                substituteName.print("{%s}", environmentVarName);
                const char *environmentSubstituteName = substituteName.c_str();

                if (environmentValue != NULL) {
                  if (verbose) {
                    CDBDebug("Replacing %s with environment value %s", environmentSubstituteName, environmentValue);
                  }
                  configFileData.replaceSelf(environmentSubstituteName, environmentValue);
                } else {
                  if (verbose) {
                    CDBDebug("Replacing %s with default value %s", environmentSubstituteName, environmentVarDefault);
                  }
                  configFileData.replaceSelf(environmentSubstituteName, environmentVarDefault);
                }
              } else {
                CDBWarning("Environment element found, but it is not prefixed with [%s]", CSERVERPARAMS_ADAGUCENV_PREFIX);
              }
            } else {
              CDBWarning("Environment element found, but either name or default are not set");
            }
          }
        }
      }
    }
  } catch (int e) {
    CDBError("Exception %d in substituting", e);
  }

  int status = configObj->parse(configFileData.c_str(), configFileData.length());

  if (status == 0 && configObj->Configuration.size() == 1) {
    return 0;
  } else {
    // cfg=NULL;
    CDBError("Invalid XML file %s", pszConfigFile.c_str());
    return 1;
  }
}

CT::string CServerParams::getResponseHeaders(int mode) {
  auto tracingHeaders = traceTimingsGetHeader();
  if (cfg != nullptr && cfg->Settings.size() == 1) {
    CT::string cacheString = "\r\nCache-Control:max-age=";
    if (mode == CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE) {
      if (!cfg->Settings[0]->attr.cache_age_volatileresources.empty()) {
        if (cfg->Settings[0]->attr.cache_age_volatileresources.toInt() != 0) {
          cacheString.printconcat("%d", cfg->Settings[0]->attr.cache_age_volatileresources.toInt());
          return cacheString + tracingHeaders;
        }
      }
    } else if (mode == CSERVERPARAMS_CACHE_CONTROL_OPTION_FULLYCACHEABLE) {
      if (!cfg->Settings[0]->attr.cache_age_cacheableresources.empty()) {
        if (cfg->Settings[0]->attr.cache_age_cacheableresources.toInt() != 0) {
          cacheString.printconcat("%d", cfg->Settings[0]->attr.cache_age_cacheableresources.toInt());
          return cacheString + tracingHeaders;
        }
      }
    }
  }
  return tracingHeaders;
}

std::tuple<float, std::string> CServerParams::getContourFont() {
  float contourFontSize = 8;
  std::string legendfontLocation;
  for (size_t wmsNr = 0; wmsNr < this->cfg->WMS.size(); wmsNr += 1) {
    for (size_t fontNr = 0; fontNr < this->cfg->WMS[wmsNr]->ContourFont.size(); fontNr += 1) {
      if (!this->cfg->WMS[wmsNr]->ContourFont[fontNr]->attr.size.empty()) {
        contourFontSize = this->cfg->WMS[wmsNr]->ContourFont[fontNr]->attr.size.toDouble();
      }
      if (!this->cfg->WMS[wmsNr]->ContourFont[fontNr]->attr.location.empty()) {
        legendfontLocation = this->cfg->WMS[wmsNr]->ContourFont[fontNr]->attr.location;
      }
    }
  }

  return std::make_tuple(contourFontSize, legendfontLocation);
}

std::tuple<float, std::string> CServerParams::getLegendFont() {
  float legendFontSize;
  std::string legendfontLocation;
  std::tie(legendFontSize, legendfontLocation) = this->getContourFont();

  for (size_t wmsNr = 0; wmsNr < this->cfg->WMS.size(); wmsNr += 1) {
    for (size_t fontNr = 0; fontNr < this->cfg->WMS[wmsNr]->LegendFont.size(); fontNr += 1) {
      if (!this->cfg->WMS[wmsNr]->LegendFont[fontNr]->attr.size.empty()) {
        legendFontSize = this->cfg->WMS[wmsNr]->LegendFont[fontNr]->attr.size.toDouble();
      }
      if (!this->cfg->WMS[wmsNr]->LegendFont[fontNr]->attr.location.empty()) {
        legendfontLocation = this->cfg->WMS[wmsNr]->LegendFont[fontNr]->attr.location;
      }
    }
  }

  return std::make_tuple(legendFontSize, legendfontLocation);
}

bool CServerParams::useMetadataTable() {
  size_t numSettings = this->cfg->Settings.size();
  if (numSettings > 0 && this->cfg->Settings[numSettings - 1]) {
    auto settings = this->cfg->Settings[numSettings - 1];
    if (settings->attr.enablemetadatacache.equalsIgnoreCase("false")) {
      return false;
    }
  }
  return true;
}

bool CServerParams::isEdrEnabled() {
  size_t numSettings = this->cfg->Settings.size();
  if (numSettings > 0 && this->cfg->Settings[numSettings - 1]) {
    auto settings = this->cfg->Settings[numSettings - 1];
    if (settings->attr.enable_edr.equalsIgnoreCase("false")) {
      return false;
    } else if (settings->attr.enable_edr.equalsIgnoreCase("true")) {
      return true;
    }
  }
  return true;
}

int CServerParams::getServerLegendIndexByName(CT::string legendName) {
  auto comp = [legendName](CServerConfig::XMLE_Legend *a) { return a->attr.name.equals(legendName); };
  auto it = std::find_if(cfg->Legend.begin(), cfg->Legend.end(), comp);
  return it == cfg->Legend.end() ? -1 : it - cfg->Legend.begin();
}

int CServerParams::getServerStyleIndexByName(CT::string styleName) {
  if (styleName.empty()) {
    CDBError("No style name provided");
    return -1;
  }
  if (styleName.equals("default")) {
    return -1;
  }
  // Remove last slash (/). E.g. windbarbs/shaded => windbarbs
  CT::string sanitizedStyleName = styleName.substring(0, styleName.indexOf("/"));

  auto comp = [sanitizedStyleName](CServerConfig::XMLE_Style *a) { return a->attr.name.equals(sanitizedStyleName); };
  auto it = std::find_if(cfg->Style.begin(), cfg->Style.end(), comp);
  int index = it == cfg->Style.end() ? -1 : it - cfg->Style.begin();

  return index;
}
