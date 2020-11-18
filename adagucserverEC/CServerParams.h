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
#include <limits.h> 
#include <stdlib.h>
#include "CDebugger.h"
#include "CTypes.h"
#include "CDirReader.h"
#include "Definitions.h"
#include "CServerConfig_CPPXSD.h"
#include "COGCDims.h"
#include "CGeoParams.h"
#include "CCache.h"
#include <map>
#include <string>

//#define MAX_DIMS 10

/**
 * See http://www.resc.rdg.ac.uk/trac/ncWMS/wiki/WmsExtensions
 */
class CWMSExtensions{
public:
  CWMSExtensions(){
    opacity=100;
    colorScaleRangeSet = false;
    numColorBands=-1;
    numColorBandsSet=false;
    logScale =false;
  }
  double opacity;//0 = fully transparent, 100 = fully opaque (default). Only applies to image formats that support partial pixel transparency (e.g. PNG). This parameter is redundant if the client application can set image opacity (e.g. Google Earth). 
  double colorScaleRangeMin;
  double colorScaleRangeMax;
  bool colorScaleRangeSet;
  float numColorBands;
  bool numColorBandsSet;
  bool logScale;
};

/**
 * Global server settings, initialized at the start, accesible from almost everywhere
 */
class CServerParams{
  DEF_ERRORFUNCTION();
  private:
    int autoOpenDAPEnabled,autoLocalFileResourceEnabled;
    
    CT::string _onlineResource;
    static int dataRestriction;
  public:
    double dfResX,dfResY;
    int dFound_BBOX;
    int dWCS_RES_OR_WH;
    double dX,dY;
    CT::string *WMSLayers;
    CT::string Format;
    CT::string InfoFormat;
    int imageFormat;
    int imageMode;
    CWMSExtensions wmsExtensions;
    

    
    
  
    
    /*
     * figWidth and figHeight override normal width and height to shape a getfeatureinfo graph
     */
    int figWidth,figHeight;
    CT::string BGColor;
    bool Transparent;
    CGeoParams * Geo;
    CT::string Styles;
    CT::string Style;
    
    //given location by the KVP key source=<value> parameter
    CT::string autoResourceLocation;
    
    CT::string datasetLocation;
    
    //internalAutoResourceLocation is the internal location used and can differ from the given location by the KVP key source=<value> parameter
    CT::string internalAutoResourceLocation;
    //autoResourceVariable is given by the KVP key variable=<value> parameter.
    CT::string autoResourceVariable;
    
    CT::string mapTitle;
    CT::string mapSubTitle;
    bool showDimensionsInImage;
    CT::string showLegendInImage;
    bool showScaleBarInImage;
    bool showNorthArrow;
    
    CT::string JSONP,queryStrURLParam;
    
    std::vector<COGCDims*> requestDims;
    int serviceType;
    int requestType;
    int OGCVersion;
    int WCS_GoNative;
    bool enableDocumentCache;
   
    CServerConfig *configObj;
    CServerConfig::XMLE_Configuration *cfg;
    CT::string configFileName;
    
    /**
     * Constructor
     */
    CServerParams();
    
    /** 
     * Destructor
     */
    ~CServerParams();
    
    
    
    /** 
     * Function which generates a unique layername from the Layer's configuration
     * @param layerName the returned name
     * @param cfgLayer the configuration object of the corresponding layer
     */
    int makeUniqueLayerName(CT::string *layerName,CServerConfig::XMLE_Layer *cfgLayer);
 
  
    
    /** 
     * Function which generates a group name from the Layer's configuration
     * @param groupName the returned name
     * @param cfgLayer the configuration object of the corresponding layer
     */
    int makeLayerGroupName(CT::string *groupName,CServerConfig::XMLE_Layer *cfgLayer);
    /**
     * Replaces illegal characters in a tableName
     * @param the tableName with the characters to be tested. Same string is filled with the new name
     */
    void encodeTableName(CT::string *tableName);
    
  

    /**
     * Get the filename of the cachefile used for XML caching. 
     * The filename is automatically constructed or can be set by the user in the configuration file alternatively.
     * 
     * @param cacheFileName The CT::string to be filled with the filename
     */
    void getCacheFileName(CT::string *cacheFileName);
    
    /**
     * Get the directory used for XML and netcdf caching. 
     * The filename is automatically constructed or can be set by the user in the configuration file alternatively.
     * 
     * @param cacheFileName The CT::string to be filled with the filename
     */
    void _getCacheDirectory(CT::string *cacheFileName);

    /**
     * Function which checks whether remote resources should be cached or not
     * @return true if enablecache attribute in AutoResource is undefined or set to true 
     */
    bool isAutoResourceCacheEnabled() const;
    
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
     * This function generates generic table names based on dimension names:
     * Table names need to be different between time and height, therefore create unique tablenames like tablename_time and tablename_height
     * The function also replaces illegal characters.
     * 
     * @param tableName The default table name which will be changed to a correct name
     * @param dimName The dimension name which will be appended to the table name
     */
    
    static void makeCorrectTableName(CT::string *tableName,CT::string *dimName);
    
    /**
     * Check whether a filepath or urlpath contains valid tokens or not
     * @param path The filepath or urlpath to check
     * @return true on valid, false on invalid
     */
    static bool checkIfPathHasValidTokens(const char *path);
    
    /**
     * Generic function which checks for custom tokens
     * @param path The string sequence to check
     * @param validTokens The string with a list of allowed tokens
     * @return true on valid, false on invalid
     */
    static bool checkForValidTokens(const char *path,const char *validTokens);
    
    /** 
     * Check wether the resourcelocation is whithin the servers configured realpath. In the servers configuration a comma separated list of realpaths can be configured.
     * @param resourceLocation The location to check for
     * @param resolvedPath The resolvedPath if a instantiated CT::string pointer is given. 
     * @return true means valid location
     */
    bool checkResolvePath(const char *path,CT::string *resolvedPath);
    
    /** 
     * Generic function which will be showed when a WCS is requested while it is not compiled in
     */    
    static void showWCSNotEnabledErrorMessage();
    
    
    
    /**
     * Get configured online resource
     */
    CT::string getOnlineResource();
    
    /**
     * Set online resource
     */
    void setOnlineResource(CT::string onlineResource);
    
    /**
     * Determine whether boundingbox y and x are swapped, for example the case with WMS 1.3.0 and EPSG:4326
     * @param projName Optional, can be set to NULL, in that case the global CRS/SRS server setting is used.
     * @return true means that BBOX order is reversed, e.g. y1,x1,y2,x2
     */
    bool checkBBOXXYOrder(const char *projName);
    
    
    
    /**
      * Returns a stringlist with all possible legends available for this Legend config object.
      * This is usually a configured legend element in a layer, or a configured legend element in a style.
      * @param Legend a XMLE_Legend object configured in a style or in a layer
      * @return Pointer to a new stringlist with all possible legend names, must be deleted with delete. Is NULL on failure.
      */
      static CT::PointerList<CT::string*> *getLegendNames(std::vector <CServerConfig::XMLE_Legend*> Legend);
      
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
      static int checkDataRestriction();
    
      
      /**
       * Checks if the string contains valid dimension tokens *
       * returns true if valid, otherwise false
       */
      static bool checkTimeFormat(CT::string& timeToCheck);
      
      /** 
       * Creates a random string of specified length
       * @param len The length of the string
       */
      static const CT::string randomString(const int len);
      
      /**
       * Parses the provided configuration file. Can be called consecutively to extend the internal configuration object.
       * @param pszConfigFile The config file to parse
       * returns zero on success       * 
       */
      int parseConfigFile(CT::string &pszConfigFile);
     
};



#endif
