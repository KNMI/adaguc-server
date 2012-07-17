#ifndef CServerParams_H
#define CServerParams_H
#include <limits.h> 
#include <stdlib.h>
#include "CDebugger.h"
#include "CTypes.h"
#include "Definitions.h"
#include "CServerConfig_CPPXSD.h"
#include "COGCDims.h"
#include "CGeoParams.h"
#include "CPGSQLDB.h"

#define MAX_DIMS 10

class CServerParams{
  DEF_ERRORFUNCTION();
  private:
    int autoOpenDAPEnabled,autoLocalFileResourceEnabled;
  public:
    double dfResX,dfResY;
    int dWCS_RES_OR_WH;
    double dX,dY;
    CT::string *WMSLayers;
    CT::string Format;
    CT::string InfoFormat;
    int imageFormat;
    int imageMode;
    
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
    //internalAutoResourceLocation is the internal location used and can differ from the given location by the KVP key source=<value> parameter
    CT::string internalAutoResourceLocation;
    //autoResourceVariable is given by the KVP key variable=<value> parameter.
    CT::string autoResourceVariable;
    
    CT::string mapTitle;
    CT::string mapSubTitle;
    bool showDimensionsInImage;
    bool showLegendInImage;
    bool showNorthArrow;
    
    
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
     * Replaces illegal characters in a tableName
     * @param the tableName with the characters to be tested. Same string is filled with the new name
     */
    void encodeTableName(CT::string *tableName);
    
    /**
     * Makes use of a lookup table to find the tablename belonging to the filter and path combinations.
     * @param tableName The CT::string which will be filled with new data after completion
     * @param path The path of the layer
     * @param filter The filter of the layer
     * @return Zero on succes, non-zero on failure.
     */
    int lookupTableName(CT::string *tableName,const char *path,const char *filter);


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
    void getCacheDirectory(CT::string *cacheFileName);

    
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
};



#endif
