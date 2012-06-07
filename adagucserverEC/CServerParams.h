#ifndef CServerParams_H
#define CServerParams_H
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
  int autoOpenDAPEnabled;
  public:
    double dfResX,dfResY;
    int dWCS_RES_OR_WH;
    double dX,dY;
    CT::string *WMSLayers;
    CT::string Format;
    CT::string InfoFormat;
    int imageFormat;
    int imageMode;
    
    //figWidth and figHeight override normal width and height to shape a getfeatureinfo graph
    int figWidth,figHeight;
    CT::string BGColor;
    bool Transparent;
    CGeoParams * Geo;
    CT::string Styles;
    CT::string Style;
    CT::string OpenDAPSource,OpenDapVariable;
    
    CT::string mapTitle;
    CT::string mapSubTitle;
    bool showDimensionsInImage;
    bool showLegendInImage;
    bool showNorthArrow;
    
    COGCDims OGCDims[MAX_DIMS];
    int NumOGCDims;
    int serviceType;
    int requestType;
    int OGCVersion;
    int WCS_GoNative;
    bool enableDocumentCache;
   
    //int skipErrorsSilently;
// CDataSource *dataSources;
    CServerConfig *configObj;
    CServerConfig::XMLE_Configuration *cfg;
    CT::string configFileName;
    CServerParams(){
      //skipErrorsSilently=1;
      WMSLayers=NULL;
      serviceType=-1;
      requestType=-1;
      OGCVersion=-1;
      NumOGCDims=0;
      Transparent=false;
      enableDocumentCache=true;
      configObj = new CServerConfig();
      Geo = new CGeoParams;
      imageFormat=IMAGEFORMAT_IMAGEPNG8;
      imageMode=SERVERIMAGEMODE_8BIT;
      autoOpenDAPEnabled=-1;
      showDimensionsInImage = false;
      showLegendInImage = false;
      figWidth=-1;
      figHeight=-1;
  //    dataSources = NULL;
    }
    ~CServerParams(){
      if(WMSLayers!=NULL){delete[] WMSLayers;WMSLayers=NULL;}
    //  if(dataSources!=NULL){delete[] dataSources;dataSources=NULL;}
      if(configObj!=NULL){delete configObj;configObj=NULL;}
      if(Geo!=NULL){delete Geo;Geo=NULL;}
    }
    
    int makeUniqueLayerName(CT::string *layerName,CServerConfig::XMLE_Layer *cfgLayer);
    
    void encodeTableName(CT::string *tableName){
      tableName->replace("/","_");
      tableName->toLowerCase();
    }
    
    /**
     * Makes use of a lookup table to find the tablename belonging to the filter and path combinations.
     * 
     * @param tableName The CT::string which will be filled with new data after completion
     * @param path The path of the layer
     * @param filter The filter of the layer
     * @return Zero on succes, non-zero on failure.
     */
    int lookupTableName(CT::string *tableName,const char *path,const char *filter);


    void getCacheFileName(CT::string *cacheFileName);
    void getCacheDirectory(CT::string *cacheFileName);
    bool isAutoOpenDAPEnabled(){
      if(autoOpenDAPEnabled==-1){
        autoOpenDAPEnabled = 0;
        if(cfg->OpenDAP.size()>0){
          if(cfg->OpenDAP[0]->attr.enableautoopendap.equals("true"))autoOpenDAPEnabled = 1;
        }
      }
      if(autoOpenDAPEnabled==0)return false;else return true;
      return false;
    }
    //Table names need to be different between time and height.
    // Therefore create unique tablenames like tablename_time and tablename_height
    static void makeCorrectTableName(CT::string *tableName,CT::string *dimName);
    static void showWCSNotEnabledErrorMessage();
};



#endif
