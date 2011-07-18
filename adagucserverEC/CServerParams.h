#ifndef CServerParams_H
#define CServerParams_H
#include "netcdf.h"
#include "CDebugger.h"
#include "CTypes.h"
#include "Definitions.h"
#include "CServerConfig_CPPXSD.h"
#include "COGCDims.h"
#include "CGeoParams.h"
#include "CPGSQLDB.h"


class CServerParams{
  DEF_ERRORFUNCTION();
  public:
    double dfResX,dfResY;
    int dWCS_RES_OR_WH;
    int dX,dY;
    CT::string *WMSLayers;
    CT::string Format;
    CT::string InfoFormat;
    int imageFormat;
    int imageMode;
    CT::string BGColor;
    bool Transparent;
    CGeoParams * Geo;
    CT::string Styles;
    CT::string Style;
    CT::string OpenDAPSource,OpenDapVariable;
    COGCDims OGCDims[NC_MAX_DIMS];
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
  //    dataSources = NULL;
    }
    ~CServerParams(){
      if(WMSLayers!=NULL){delete[] WMSLayers;WMSLayers=NULL;}
    //  if(dataSources!=NULL){delete[] dataSources;dataSources=NULL;}
      if(configObj!=NULL){delete configObj;configObj=NULL;}
      if(Geo!=NULL){delete Geo;Geo=NULL;}
    }
    void makeUniqueLayerName(CT::string *layerName,CServerConfig::XMLE_Layer *cfgLayer){
      layerName->copy("");
      if(cfgLayer->Group.size()==1){
        if(cfgLayer->Group[0]->attr.value.c_str()!=NULL){
          layerName->copy(cfgLayer->Group[0]->attr.value.c_str());
          layerName->concat("/");
        }
      }
      if(cfgLayer->Name.size()==0){
        CServerConfig::XMLE_Name *name=new CServerConfig::XMLE_Name();
        cfgLayer->Name.push_back(name);
        name->value.copy(cfgLayer->Variable[0]->value.c_str());
      }  
      layerName->concat(cfgLayer->Name[0]->value.c_str());
    }
    void encodeTableName(CT::string *tableName){
      tableName->replace("/","_");
      tableName->toLowerCase();
    }
    
    int lookupTableName(CT::string *tableName,const char *path,const char *filter);


    void getCacheFileName(CT::string *cacheFileName);
    void getCacheDirectory(CT::string *cacheFileName);
    //Table names need to be different between time and height.
    // Therefore create unique tablenames like tablename_time and tablename_height
    static void makeCorrectTableName(CT::string *tableName,CT::string *dimName);
    static void showWCSNotEnabledErrorMessage();
};



#endif
