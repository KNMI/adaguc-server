#ifndef CServerParams_H
#define CServerParams_H
#include "netcdf.h"
#include "CDebugger.h"
#include "CTypes.h"
#include "Definitions.h"
#include "CServerConfig_CPPXSD.h"
#include "COGCDims.h"
#include "CGeoParams.h"

class CServerParams{
  public:
    double dfResX,dfResY;
    int dWCS_RES_OR_WH;
    int dX,dY;
    CT::string *WMSLayers;
    CT::string Format;
    CT::string BGColor;
    bool Transparent;
    CGeoParams * Geo;
    CT::string Styles;
    CT::string Style;
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
      layerName->concat(cfgLayer->Name[0]->value.c_str());
    }
    void getCacheFileName(CT::string *cacheFileName);
    void getCacheDirectory(CT::string *cacheFileName);
};




#endif
