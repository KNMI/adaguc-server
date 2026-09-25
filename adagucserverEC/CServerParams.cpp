/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl, GST - GeoSpatialTeam KNMI
 * Date:     2026-09-10
 *
 ******************************************************************************
 *
 * Copyright 2026, Royal Netherlands Meteorological Institute (KNMI)
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
#include "CDebugger.h"
#include "CTString.h"
#include "CDirReader.h"
#include "CStopWatch.h"
#include <traceTimings/traceTimings.h>
#include <cstring>
#include <algorithm>

void showWCSNotEnabledErrorMessage() { CDBError("WCS is not enabled because GDAL was not compiled into the server. "); }

CWMSExtensions::CWMSExtensions() {
  opacity = 100;
  colorScaleRangeSet = false;
  numColorBands = -1;
  numColorBandsSet = false;
  logScale = false;
}

char debugLoggingIsEnabled = -1; // Not configured yet, 1 means enabled, 0 means disabled

bool CServerParams::isDebugLoggingEnabled() const {
  if (debugLoggingIsEnabled == 0)
    return false;
  else if (debugLoggingIsEnabled == 1)
    return true;
  else if (cfg && cfg->Logging.size() > 0) {
    if (cfg->Logging[cfg->Logging.size() - 1].attr.debug == ("false")) {
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
      if (cfg->AutoResource[0].attr.enableautoopendap == ("true")) autoOpenDAPEnabled = 1;
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
      if (cfg->AutoResource[0].attr.enablelocalfile == ("true")) autoLocalFileResourceEnabled = 1;
    }
  }
  if (autoLocalFileResourceEnabled == 1) return true;
  return false;
}

bool CServerParams::isAutoResourceEnabled() {
  if (isAutoOpenDAPResourceEnabled() || isAutoLocalFileResourceEnabled()) return true;
  return false;
}

bool checkIfPathHasValidTokens(const std::string &path) { return checkForValidTokens(path, "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789/_-+:. ,[]"); }

bool checkForValidTokens(const std::string &path, const std::string &validPATHTokens) {
  // Check for valid tokens
  size_t pathLength = path.length();
  size_t allowedTokenLength = validPATHTokens.length();
  for (size_t j = 0; j < pathLength; j++) {
    bool isInvalid = true;
    for (size_t i = 0; i < allowedTokenLength; i++) {
      if (path[j] == validPATHTokens[i]) {
        isInvalid = false;
        break;
      }
    }
    if (isInvalid) {
      CDBDebug("Invalid token '%c' in '%s'", path[j], path.c_str());
      return false;
    }

    // Check for sequences
    if (j > 0) {
      if (path[j - 1] == '.' && path[j] == '.') {
        CDBDebug("Invalid sequence in '%s'", path.c_str());
        return false;
      }
    }
  }
  return true;
}

bool CServerParams::checkResolvePath(const std::string &path, std::string &outputtedResolvedPath) {
  if (cfg->AutoResource.size() > 0) {
    // Needs to be configured otherwise it will be denied.
    if (cfg->AutoResource[0].Dir.size() == 0) {
      CDBDebug("No Dir elements defined");
      return false;
    }

    if (checkIfPathHasValidTokens(path) == false) {
      CDBDebug("Invalid tokens in path");
      return false;
    }

    for (size_t d = 0; d < cfg->AutoResource[0].Dir.size(); d++) {
      const char *_baseDir = cfg->AutoResource[0].Dir[d].attr.basedir.c_str();
      const char *dirPrefix = cfg->AutoResource[0].Dir[d].attr.prefix.c_str();

      char baseDir[PATH_MAX];
      if (realpath(_baseDir, baseDir) == NULL) {
        CDBError("Skipping AutoResource[0]->Dir[%lu]->basedir: Configured value is not a valid realpath", d);
        continue;
      }

      if (strlen(baseDir) > 0 && strlen(dirPrefix) > 0) {
        // Prepend the prefix to make the absolute path
        std::string pathToCheck = CT::printf("%s/%s", dirPrefix, path.c_str());
        // Make a realpath
        char szResolvedPath[PATH_MAX];
        if (realpath(pathToCheck.c_str(), szResolvedPath) != NULL) {
          std::string resolvedPathStr = szResolvedPath;
          if (CT::startsWith(resolvedPathStr, baseDir)) {
            outputtedResolvedPath = resolvedPathStr;
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

void CServerParams::setOnlineResource(std::string r) { _onlineResource = r; };

std::string CServerParams::getOnlineResource() {
  if (_onlineResource.length() > 0) {
    return _onlineResource;
  }
  if (cfg->OnlineResource.size() == 0) {
    // No Online resource is given.
    const char *pszADAGUCOnlineResource = getenv("ADAGUC_ONLINERESOURCE");
    if (pszADAGUCOnlineResource == NULL) {
      _onlineResource = "";
      return "";
    }
    std::string onlineResource = pszADAGUCOnlineResource;
    _onlineResource = onlineResource;
    return onlineResource;
  }

  std::string onlineResource = cfg->OnlineResource[0].attr.value.c_str();

  // A full path is given in the configuration
  if (CT::indexOf(onlineResource, "http") == 0) {
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
  std::string httpHost = "http://";
  httpHost += pszHTTPHost;
  httpHost += onlineResource;
  _onlineResource = httpHost;
  return httpHost;
}

bool CServerParams::checkBBOXXYOrder(const char *projName) {
  if (OGCVersion == WMS_VERSION_1_3_0) {
    std::string projNameString;
    if (projName == NULL) {
      projNameString = geoParams.crs.c_str();
    } else {
      projNameString = projName;
    }
    auto comp = [projNameString](const CServerConfig::XMLE_Projection &a) { return a.attr.id == projNameString.c_str(); };
    auto it = std::find_if(cfg->Projection.begin(), cfg->Projection.end(), comp);
    if (it != cfg->Projection.end()) {
      return CT::equalsIgnoreCase((*it).attr.invertxyforwms130, "true");
    }
  }
  return false;
}

/**
 * Returns a stringlist with all possible legends available for this Legend config object.
 * This is usually a configured legend element in a layer, or a configured legend element in a style.
 * @param Legend a XMLE_Legend object configured in a style or in a layer
 * @return Pointer to a new stringlist with all possible legend names, must be deleted with delete. Is NULL on failure.
 */
std::vector<std::string> getLegendNames(const std::vector<CServerConfig::XMLE_Legend> &Legend) {
  if (Legend.size() == 0) {
    return {"rainbow"};
  }
  std::vector<std::string> stringList;
  for (size_t j = 0; j < Legend.size(); j++) {
    std::vector<std::string> l1 = CT::split(Legend[j].elementValue, ",");
    std::erase_if(l1, [](const std::string &s) { return s.empty(); });
    stringList.insert(stringList.end(), l1.begin(), l1.end());
  }
  return stringList;
}

int dataRestriction = -1;
int checkDataRestriction() {
  if (dataRestriction != -1) return dataRestriction;

  // By default no restrictions
  int dr = ALLOW_WCS | ALLOW_GFI | ALLOW_METADATA;
  const char *data = getenv("ADAGUC_DATARESTRICTION");
  if (data != NULL) {
    dr = ALLOW_NONE;
    std::string temp(data);
    temp = CT::toUpperCase(temp);
    if (temp == ("TRUE")) {
      dr = ALLOW_NONE;
    }
    if (temp == ("FALSE")) {
      dr = ALLOW_WCS | ALLOW_GFI | ALLOW_METADATA;
    }
    // Decompose into stringlist and check each item
    std::vector<std::string> items = CT::split(temp, "|");
    for (size_t j = 0; j < items.size(); j++) {
      CT::replaceSelf(items[j], "\"", "");
      if (items[j] == ("ALLOW_GFI")) dr |= ALLOW_GFI;
      if (items[j] == ("ALLOW_WCS")) dr |= ALLOW_WCS;
      if (items[j] == ("ALLOW_METADATA")) dr |= ALLOW_METADATA;
      if (items[j] == ("SHOW_QUERYINFO")) dr |= SHOW_QUERYINFO;
    }
  }

  dataRestriction = dr;
  return dataRestriction;
}

const std::string timeFormatAllowedCharsString = "0123456789:TZ-/. _ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz()*";
// Checks if time is an iso time format
// 2024-05-25T08:00:00Z or 2024-05-25T08:00:00, '*' is also allowed.
bool checkTimeFormat(const std::string &timeToCheck) {
  if (timeToCheck == "*") return true;
  if (timeToCheck.length() < 18 || timeToCheck.at(10) != 'T') return false;
  return timeToCheck.find_first_not_of(timeFormatAllowedCharsString) == std::string::npos;
}

// Substitutes the standard ADAGUC_* environment variables in the configuration data
static void substituteStandardEnvironment(std::string &configFileData) {
  /* Substitute ADAGUC_PATH */
  const char *pszADAGUC_PATH = getenv("ADAGUC_PATH");
  if (pszADAGUC_PATH != NULL) {
    std::string adagucPath = makeCleanPath(pszADAGUC_PATH) + "/";
    CT::replaceSelf(configFileData, "{ADAGUC_PATH}", adagucPath.c_str());
  }

  /* Substitute ADAGUC_TMP */
  const char *pszADAGUC_TMP = getenv("ADAGUC_TMP");
  CT::replaceSelf(configFileData, "{ADAGUC_TMP}", pszADAGUC_TMP == NULL ? "/tmp/" : pszADAGUC_TMP);

  /* Substitute the others only when set */
  for (const char *name : {"ADAGUC_DB", "ADAGUC_DATASET_DIR", "ADAGUC_DATA_DIR", "ADAGUC_AUTOWMS_DIR"}) {
    const char *value = getenv(name);
    if (value != NULL) CT::replaceSelf(configFileData, CT::printf("{%s}", name).c_str(), value);
  }
}

// Substitutes the variables declared by <Environment> elements, using the environment value or the default
static void substituteExtraEnvironment(std::string &configFileData, const std::vector<CServerConfig::XMLE_Environment> &environments, bool verbose) {
  for (const auto &env : environments) {
    const std::string &name = env.attr.name;
    const std::string &defaultVal = env.attr.defaultVal;
    if (name.empty() || defaultVal.empty()) {
      CDBWarning("Environment element found, but either name or default are not set [%s] [%s]", name.c_str(), defaultVal.c_str());
      continue;
    }
    if (!CT::startsWith(name, CSERVERPARAMS_ADAGUCENV_PREFIX)) {
      CDBWarning("Environment element found, but it is not prefixed with [%s]", CSERVERPARAMS_ADAGUCENV_PREFIX);
      continue;
    }
    const char *environmentValue = getenv(name.c_str());
    const char *value = environmentValue != NULL ? environmentValue : defaultVal.c_str();
    std::string substituteName = CT::printf("{%s}", name.c_str());
    if (verbose) {
      CDBDebug("Replacing %s with %s value %s", substituteName.c_str(), environmentValue != NULL ? "environment" : "default", value);
    }
    CT::replaceSelf(configFileData, substituteName.c_str(), value);
  }
}

int CServerParams::parseConfigFile(const std::string &pszConfigFile) {
  if (adagucMeasureTime) {
    StopWatch_Stop("CServerParams::parseConfigFile start %s", pszConfigFile.c_str());
  }
  std::string configFileData;
  try {
    configFileData = readFile(pszConfigFile);
  } catch (int e) {
    CDBError("Unable to open configuration file [%s], error %d", pszConfigFile.c_str(), e);
    return 1;
  }
  std::string datasetName = CT::basename(pszConfigFile.c_str());

  try {
    substituteStandardEnvironment(configFileData);

    // Environment elements declare extra variables to substitute. Finding them requires parsing the XML first,
    // so this extra pass is only done when the file can contain them.
    if (configFileData.find("Environment") != std::string::npos) {
      if (adagucMeasureTime) {
        StopWatch_Stop("CServerParams::parseConfigFile start extra substitutions");
      }
      CServerConfig environmentConfig;
      if (parseConfig(&environmentConfig, configFileData, datasetName) == 0 && !environmentConfig.Configuration.empty()) {
        substituteExtraEnvironment(configFileData, environmentConfig.Configuration[0].Environment, verbose);
      }
    }
  } catch (int e) {
    CDBError("Exception %d in substituting", e);
  }

  if (adagucMeasureTime) {
    StopWatch_Stop("CServerParams::parseConfigFile start parseConfig");
  }
  int status = parseConfig(&configObj, configFileData, datasetName);
  if (adagucMeasureTime) {
    StopWatch_Stop("CServerParams::parseConfigFile done parseConfig");
  }

  if (status == 0 && configObj.Configuration.size() == 1) {
    return 0;
  }
  CDBError("Invalid XML file %s", pszConfigFile.c_str());
  return 1;
}

std::string CServerParams::getResponseHeaders(int mode) {
  auto tracingHeaders = traceTimingsGetHeader();
  if (cfg != nullptr && cfg->Settings.size() > 0) {
    std::string cacheString = "\r\nCache-Control:max-age=";
    if (mode == CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE) {
      if (!cfg->Settings[0].attr.cache_age_volatileresources.empty()) {
        if (atoi(cfg->Settings[0].attr.cache_age_volatileresources.c_str()) != 0) {
          cacheString += std::to_string(atoi(cfg->Settings[0].attr.cache_age_volatileresources.c_str()));
          return cacheString + tracingHeaders;
        }
      }
    } else if (mode == CSERVERPARAMS_CACHE_CONTROL_OPTION_FULLYCACHEABLE) {
      if (!cfg->Settings[0].attr.cache_age_cacheableresources.empty()) {
        if (atoi(cfg->Settings[0].attr.cache_age_cacheableresources.c_str()) != 0) {
          cacheString += std::to_string(atoi(cfg->Settings[0].attr.cache_age_cacheableresources.c_str()));
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
    for (size_t fontNr = 0; fontNr < this->cfg->WMS[wmsNr].ContourFont.size(); fontNr += 1) {
      if (!this->cfg->WMS[wmsNr].ContourFont[fontNr].attr.size.empty()) {
        contourFontSize = atof(this->cfg->WMS[wmsNr].ContourFont[fontNr].attr.size.c_str());
      }
      if (!this->cfg->WMS[wmsNr].ContourFont[fontNr].attr.location.empty()) {
        legendfontLocation = this->cfg->WMS[wmsNr].ContourFont[fontNr].attr.location;
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
    for (size_t fontNr = 0; fontNr < this->cfg->WMS[wmsNr].LegendFont.size(); fontNr += 1) {
      if (!this->cfg->WMS[wmsNr].LegendFont[fontNr].attr.size.empty()) {
        legendFontSize = atof(this->cfg->WMS[wmsNr].LegendFont[fontNr].attr.size.c_str());
      }
      if (!this->cfg->WMS[wmsNr].LegendFont[fontNr].attr.location.empty()) {
        legendfontLocation = this->cfg->WMS[wmsNr].LegendFont[fontNr].attr.location;
      }
    }
  }

  return std::make_tuple(legendFontSize, legendfontLocation);
}

bool CServerParams::useMetadataTable() {
  size_t numSettings = this->cfg->Settings.size();
  if (numSettings > 0) {
    const auto &settings = this->cfg->Settings[numSettings - 1];
    if (CT::equalsIgnoreCase(settings.attr.enablemetadatacache, "false")) {
      return false;
    }
  }
  return true;
}

bool CServerParams::isEdrEnabled() {
  size_t numSettings = this->cfg->Settings.size();
  if (numSettings > 0) {
    const auto &settings = this->cfg->Settings[numSettings - 1];
    return CT::equalsIgnoreCase(settings.attr.enable_edr, "true");
  }
  return true;
}

int CServerParams::getServerLegendIndexByName(std::string legendName) {
  auto comp = [legendName](const CServerConfig::XMLE_Legend &a) { return a.attr.name == (legendName); };
  auto it = std::find_if(cfg->Legend.begin(), cfg->Legend.end(), comp);
  return it == cfg->Legend.end() ? -1 : it - cfg->Legend.begin();
}

int CServerParams::getServerStyleIndexByName(std::string styleName) {
  if (styleName.empty()) {
    CDBError("No style name provided");
    return -1;
  }
  if (styleName == ("default")) {
    return -1;
  }
  // Remove last slash (/). E.g. windbarbs/shaded => windbarbs
  std::string sanitizedStyleName = CT::substring(styleName, 0, CT::indexOf(styleName, "/"));

  auto comp = [sanitizedStyleName](const CServerConfig::XMLE_Style &a) { return a.attr.name == (sanitizedStyleName); };
  auto it = std::find_if(cfg->Style.begin(), cfg->Style.end(), comp);
  int index = it == cfg->Style.end() ? -1 : it - cfg->Style.begin();

  return index;
}
