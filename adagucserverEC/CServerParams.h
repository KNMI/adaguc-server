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

#ifndef CServerParams_H
#define CServerParams_H
#include <climits>
#include <cstdlib>
#include "CDebugger.h"
#include "CTString.h"
#include "CDirReader.h"
#include "Definitions.h"
#include "CServerConfig_CPPXSD.h"
#include "COGCDims.h"
#include "./Types/GeoParameters.h"
#include <map>
#include <tuple>
#include <string>

// #define MAX_DIMS 10

#define CSERVERPARAMS_ADAGUCENV_PREFIX "ADAGUCENV_"

/**
 * See http://www.resc.rdg.ac.uk/trac/ncWMS/wiki/WmsExtensions
 */
class CWMSExtensions {
public:
  CWMSExtensions() {
    opacity = 100;
    colorScaleRangeSet = false;
    numColorBands = -1;
    numColorBandsSet = false;
    logScale = false;
  }
  double opacity; // 0 = fully transparent, 100 = fully opaque (default). Only applies to image formats that support partial pixel transparency (e.g. PNG). This parameter is redundant if the client
                  // application can set image opacity (e.g. Google Earth).
  double colorScaleRangeMin;
  double colorScaleRangeMax;
  bool colorScaleRangeSet;
  float numColorBands;
  bool numColorBandsSet;
  bool logScale;
};

#define CSERVERPARAMS_CACHE_CONTROL_OPTION_NOCACHE 0
#define CSERVERPARAMS_CACHE_CONTROL_OPTION_FULLYCACHEABLE 1
#define CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE 2

/**
 * Global server settings, initialized at the start, accesible from almost everywhere
 */
class CServerParams {

private:
  int autoOpenDAPEnabled = -1;
  int autoLocalFileResourceEnabled = -1;
  int cacheControlOption = CSERVERPARAMS_CACHE_CONTROL_OPTION_NOCACHE;
  std::string _onlineResource;
  int _parseConfigFile(const std::string &pszConfigFile, std::vector<CServerConfig::XMLE_Environment> *extraEnvironment);

public:
  bool Transparent = false;
  bool verbose = false;
  bool showDimensionsInImage = false;
  bool showScaleBarInImage = false;
  bool showNorthArrow = false;
  int dFound_BBOX = 0;
  int imageFormat = IMAGEFORMAT_IMAGEPNG8;
  int imageQuality = 85; // 0-100
  int imageMode = SERVERIMAGEMODE_8BIT;
  int figWidth = -1; // figWidth and figHeight override normal width and height to shape a getfeatureinfo graph
  int figHeight = -1;
  int serviceType = -1;
  int requestType = -1;
  int OGCVersion = -1;
  int WCS_GoNative = -1;
  double dfResX = 0;
  double dfResY = 0;
  double dX = 0;
  double dY = 0;
  std::string Format;
  std::string InfoFormat;
  std::string BGColor;
  std::string responceCrs;
  std::string Styles;
  std::string Style;
  std::string autoResourceLocation; // given location by the KVP key source=<value> parameter
  std::string datasetLocation;
  std::string internalAutoResourceLocation; // internalAutoResourceLocation is the internal location used and can differ from the given location by the KVP key source=<value> parameter
  std::string autoResourceVariable;         // autoResourceVariable is given by the KVP key variable=<value> parameter.
  std::string mapTitle;
  std::string mapSubTitle;
  std::string showLegendInImage = "false";
  std::string configFileName;
  std::string JSONP;
  std::vector<OGCURIDims> requestDims;
  std::vector<std::string> requestedLayerNames;

  CWMSExtensions wmsExtensions;
  GeoParameters geoParams;
  CServerConfig configObj;                          // The parsed config object representing the xml structure
  CServerConfig::XMLE_Configuration *cfg = nullptr; // Pointer into configObj when configured.

  /**
   * Function which checks whether logging should be done
   * @return true if logging is enabled
   */
  bool isDebugLoggingEnabled() const;

  /**
   * Function which can be used to check whether automatic resources have been enabled or not
   * The resource can be provided to the ADAGUC service via the KVP parameter "SOURCE=OPeNDAPURL/FILE"
   * The function read the config file once, for consecutive checks the value is stored in a variable
   *
   * @return true: AutoResource is enabled
   */
  bool isAutoResourceEnabled();

  /**
   * Function which can be used to check whether automatic OPeNDAP URL reading has been enabled or not
   * The OPeNDAP URL can be provided to the ADAGUC service via the KVP parameter "SOURCE=OPeNDAPURL"
   * The function read the config file once, for consecutive checks the value is stored in a variable
   *
   * @return true: OPeNDAP URL's are supported
   */
  bool isAutoOpenDAPResourceEnabled();

  /**
   * Function which can be used to check whether automatic local file reading has been enabled or not
   * The OPeNDAP URL can be provided to the ADAGUC service via the KVP parameter "SOURCE=OPeNDAPURL"
   * The function read the config file once, for consecutive checks the value is stored in a variable
   *
   * @return true: File locations are supported
   */
  bool isAutoLocalFileResourceEnabled();

  /**
   * Check wether the resourcelocation is whithin the servers configured realpath. In the servers configuration a comma separated list of realpaths can be configured.
   * @param resourceLocation The location to check for
   * @param resolvedPath The resolvedPath if a instantiated std::string pointer is given.
   * @return true means valid location
   */
  bool checkResolvePath(const std::string &path, std::string &outputtedResolvedPath);

  /**
   * Get configured online resource
   */
  std::string getOnlineResource();

  /**
   * Set online resource
   */
  void setOnlineResource(std::string onlineResource);

  /**
   * Determine whether boundingbox y and x are swapped, for example the case with WMS 1.3.0 and EPSG:4326
   * @param projName Optional, can be set to NULL, in that case the global CRS/SRS server setting is used.
   * @return true means that BBOX order is reversed, e.g. y1,x1,y2,x2
   */
  bool checkBBOXXYOrder(const char *projName);

  /**
   * Parses the provided configuration file. Can be called consecutively to extend the internal configuration object.
   * @param pszConfigFile The config file to parse
   * returns zero on success       *
   */
  int parseConfigFile(const std::string &pszConfigFile);

  /**
   * Returns cache control header
   * mode CSERVERPARAMS_CACHE_CONTROL_OPTION_NOCACHE returns empty string ("")
   * mode CSERVERPARAMS_CACHE_CONTROL_OPTION_SHORTCACHE is for urls which response might change often (shorter max-age)
   * mode CSERVERPARAMS_CACHE_CONTROL_OPTION_FULLYCACHEABLE is fully specified urls
   */
  std::string getResponseHeaders(int mode);

  void setCacheControlOption(int mode) { cacheControlOption = mode; }
  int getCacheControlOption() { return cacheControlOption; }

  /**
   * Returns the fontsize in px for contour lines and classes in the legend
   */
  std::tuple<float, std::string> getContourFont();

  /**
   * Returns the fontsize in px for legend
   */
  std::tuple<float, std::string> getLegendFont();

  bool useMetadataTable();

  bool isEdrEnabled();

  /**
   * Retrieves the position of for the requested legend name in the servers configured legend elements.
   * @param legendName The name of the legend to locate
   * @return The legend index as integer, points to the position in the servers configured legends. Is -1 on failure.
   */
  int getServerLegendIndexByName(std::string legendName);

  /**
   * Retrieves the style index in the server configuration by stylename.
   * Returns -1 in case of:
   *  - empty name
   *  - if the name ="default".
   *  - If the style name is not found.
   * @param styleName The name of the style to locate
   * @return The style index as integer, points to the position in the servers configured styles. Is -1 on failure.
   */
  int getServerStyleIndexByName(std::string styleName);
};

/**
 * Check whether a filepath or urlpath contains valid tokens or not
 * @param path The filepath or urlpath to check
 * @return true on valid, false on invalid
 */
bool checkIfPathHasValidTokens(const std::string &path);

/**
 * Generic function which checks for custom tokens
 * @param path The string sequence to check
 * @param validTokens The string with a list of allowed tokens
 * @return true on valid, false on invalid
 */
bool checkForValidTokens(const std::string &path, const std::string &validTokens);
/**
 * Generic function which will be showed when a WCS is requested while it is not compiled in
 */
void showWCSNotEnabledErrorMessage();

/**
 * Returns a stringlist with all possible legends available for this Legend config object.
 * This is usually a configured legend element in a layer, or a configured legend element in a style.
 * @param Legend a XMLE_Legend object configured in a style or in a layer
 * @return Pointer to a new stringlist with all possible legend names, must be deleted with delete. Is NULL on failure.
 */
std::vector<std::string> getLegendNames(const std::vector<CServerConfig::XMLE_Legend *> Legend);

/**
 * Checks whether data is restricted or not based on the environment variable ADAGUC_DATARESTRICTION.
 * If not defined or set to FALSE, there are no dataset restrictions. If set to TRUE, dataaccess is restricted for WCS, GFI, metadata and queryinfo.
 * Possible values are: ALLOW_GFI, ALLOW_WCS, ALLOW_METADATA and SHOW_QUERYINFO. Values can be combined by using the | sign without any space.
 *
 * ALLOW_GFI: Allows getFeatureInfo request to work.
 *
 * ALLOW_WCS: Allows dataaccess by the Web Coverage Service.
 *
 * ALLOW_METADATA: Allows to display detailed netcdf header information.
 *
 * SHOW_QUERYINFO: Displays failed queries, if not set "hidden" is shown instead.
 */
int checkDataRestriction();

/**
 * Checks if the string contains valid dimension tokens *
 * returns true if valid, otherwise false
 */
bool checkTimeFormat(const std::string &timeToCheck);
#endif
