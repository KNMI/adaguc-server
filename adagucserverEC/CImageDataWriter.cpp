#include "CImageDataWriter.h"
//#define CIMAGEDATAWRITER_DEBUG

void doJacoIntoLatLon(double &u, double &v, double lo, double la, float deltaX, float deltaY, CImageWarper *warper);
std::map<std::string,CImageDataWriter::ProjCacheInfo> CImageDataWriter::projCacheMap;
std::map<std::string,CImageDataWriter::ProjCacheInfo>::iterator CImageDataWriter::projCacheIter;

class PlotObject{
      public:
        PlotObject(){
          elements = NULL;
          values = NULL;
          length = 0;
        }
        ~PlotObject(){
          freePoints();
        }

        CImageDataWriter::GetFeatureInfoResult::Element **elements;
        size_t length;
        CT::string name;
        CT::string units;
        
        double minValue,maxValue,*values;
        
        void freePoints(){
          //First remove pointers, otherwise the elements are also destructed.
          if(elements==NULL)return;
          for(size_t j=0;j<length;j++){
            elements[j] = NULL;
          }
          delete[] elements;
          delete[] values;
          elements = NULL;
          values = NULL;
        }
        
        void allocateLength(size_t numPoints){
          length=numPoints;
          freePoints();
          elements = new CImageDataWriter::GetFeatureInfoResult::Element *[numPoints];
          values = new double[numPoints];
        }
      };

const char * CImageDataWriter::className = "CImageDataWriter";
const char * CImageDataWriter::RenderMethodStringList="nearest";//, bilinear, contour, vector, barb, barbcontour, shaded,shadedcontour,vectorcontour,vectorcontourshaded,nearestcontour,bilinearcontour";
CImageDataWriter::CImageDataWriter(){
  currentStyleConfiguration = NULL;
  
  //Mode can be "uninitialized"0 "initialized"(1) and "finished" (2)
  writerStatus = uninitialized;
  currentDataSource = NULL;
}

CImageDataWriter::RenderMethod CImageDataWriter::getRenderMethodFromString(CT::string *renderMethodString){
  RenderMethod renderMethod = RM_UNDEFINED;
  if(renderMethodString->indexOf("nearest" )!=-1)renderMethod|=RM_NEAREST;
  if(renderMethodString->indexOf("bilinear")!=-1)renderMethod|=RM_BILINEAR;
  if(renderMethodString->indexOf("shaded"  )!=-1)renderMethod|=RM_SHADED;
  if(renderMethodString->indexOf("contour" )!=-1)renderMethod|=RM_CONTOUR;
  if(renderMethodString->indexOf("point"   )!=-1)renderMethod|=RM_POINT;
  if(renderMethodString->indexOf("vector"  )!=-1)renderMethod|=RM_VECTOR;
  if(renderMethodString->indexOf("barb"    )!=-1)renderMethod|=RM_BARB;
  if(renderMethodString->indexOf("thin")!=-1)renderMethod|=RM_THIN;
  if(renderMethodString->indexOf("rgba")!=-1)renderMethod|=RM_RGBA;

  return renderMethod;
}

void CImageDataWriter::getRenderMethodAsString(CT::string *renderMethodString, RenderMethod renderMethod){
  
  if(renderMethod == RM_UNDEFINED){renderMethodString->copy("undefined");return;}
  renderMethodString->copy("");
  if(renderMethod & RM_NEAREST)renderMethodString->concat("nearest");
  if(renderMethod & RM_BILINEAR)renderMethodString->concat("bilinear");
  if(renderMethod & RM_SHADED)renderMethodString->concat("shaded");
  if(renderMethod & RM_CONTOUR)renderMethodString->concat("contour");
  if(renderMethod & RM_POINT)renderMethodString->concat("point");
  if(renderMethod & RM_VECTOR)renderMethodString->concat("vector");
  if(renderMethod & RM_BARB)renderMethodString->concat("barb");
  if(renderMethod & RM_THIN)renderMethodString->concat("thin");
  if(renderMethod & RM_RGBA)renderMethodString->concat("rgba");
  

}

int CImageDataWriter::_setTransparencyAndBGColor(CServerParams *srvParam,CDrawImage* drawImage){
  //drawImage->setTrueColor(true);
  //Set transparency
  if(srvParam->Transparent==true){
    drawImage->enableTransparency(true);
  }else{
    drawImage->enableTransparency(false);
    //Set BGColor
    if(srvParam->BGColor.length()>0){
      if(srvParam->BGColor.length()!=8){
        CDBError("BGCOLOR does not comply to format 0xRRGGBB");
        return 1;
      }
      if(srvParam->BGColor.c_str()[0]!='0'||srvParam->BGColor.c_str()[1]!='x'){
        CDBError("BGCOLOR does not comply to format 0xRRGGBB");
        return 1;
      }
      unsigned char hexa[8];
      memcpy(hexa,srvParam->BGColor.c_str()+2,6);
      for(size_t j=0;j<6;j++){
        hexa[j]-=48;
        if(hexa[j]>16)hexa[j]-=7;
      }
      drawImage->setBGColor(hexa[0]*16+hexa[1],hexa[2]*16+hexa[3],hexa[4]*16+hexa[5]);
    }else{
      drawImage->setBGColor(255,255,255);
    }
  }
  return 0;
}

int CImageDataWriter::drawCascadedWMS(CDataSource * dataSource, const char *service,const char *layers,bool transparent){

#ifndef ENABLE_CURL
  CDBError("CURL not enabled");
  return 1;
#endif

#ifdef ENABLE_CURL
  bool trueColor=drawImage.getTrueColor();
  transparent=true;
  CT::string url=service;
  url.concat("SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&STYLES=&");
  if(trueColor==false)url.concat("FORMAT=image/gif");
  else url.concat("FORMAT=image/png");
  //&BBOX=50.943396226415075,-4.545656817372752,118.8679245283019,57.6116945532218
  if(transparent)url.printconcat("&TRANSPARENT=TRUE");
  url.printconcat("&WIDTH=%d",drawImage.Geo->dWidth);
  url.printconcat("&HEIGHT=%d",drawImage.Geo->dHeight);
  url.printconcat("&BBOX=%0.4f,%0.4f,%0.4f,%0.4f",drawImage.Geo->dfBBOX[0],
                  drawImage.Geo->dfBBOX[1],
                  drawImage.Geo->dfBBOX[2],
                  drawImage.Geo->dfBBOX[3]);
  url.printconcat("&SRS=%s",drawImage.Geo->CRS.c_str());
  url.printconcat("&LAYERS=%s",layers);
  for(size_t k=0;k<srvParam->requestDims.size();k++){
    url.printconcat("&%s=%s",srvParam->requestDims[k]->name.c_str(),srvParam->requestDims[k]->value.c_str());
  }
  //CDBDebug(url.c_str());
  gdImagePtr gdImage;
  

  
  MyCURL myCURL;
  int status  = myCURL.getGDImageField(url.c_str(),gdImage);
  if(status==0){
    if(gdImage){
      int w=gdImageSX(gdImage);
      int h=gdImageSY(gdImage);
      
      int offsetx=0;
      int offsety=0;
      if(dataSource->cfgLayer->Position.size()>0){
        CServerConfig::XMLE_Position * pos=dataSource->cfgLayer->Position[0];
        if(pos->attr.right.c_str()!=NULL)offsetx=(drawImage.Geo->dWidth-w)-parseInt(pos->attr.right.c_str());
        if(pos->attr.bottom.c_str()!=NULL)offsety=(drawImage.Geo->dHeight-h)-parseInt(pos->attr.bottom.c_str());
        if(pos->attr.left.c_str()!=NULL)offsetx=parseInt(pos->attr.left.c_str());
        if(pos->attr.top.c_str()!=NULL)offsety=parseInt(pos->attr.top.c_str());
        
      }
      
      /*if(drawImage.Geo->dHeight!=h||drawImage.Geo->dWidth!=w){
        gdImageDestroy(gdImage);
        CDBError("Returned cascaded WMS image size is not the same as requested image size");
        return 1;
      }*/
      
      int transpColor=gdImageGetTransparent(gdImage);
      for(int y=0;y<drawImage.Geo->dHeight&&y<h;y++){
        for(int x=0;x<drawImage.Geo->dWidth&&x<w;x++){
          int color = gdImageGetPixel(gdImage, x, y);
          if(color!=transpColor&&127!=gdImageAlpha(gdImage,color)){
            if(transparent){
              drawImage.setPixelTrueColor(x+offsetx,y+offsety,gdImageRed(gdImage,color),gdImageGreen(gdImage,color),gdImageBlue(gdImage,color),255-gdImageAlpha(gdImage,color)*2);
            }
            else
              drawImage.setPixelTrueColor(x+offsetx,y+offsety,gdImageRed(gdImage,color),gdImageGreen(gdImage,color),gdImageBlue(gdImage,color));
          }
        }
      }
      gdImageDestroy(gdImage);
    }else{
      CT::string u=url.c_str();u.encodeURLSelf();
      CDBError("Invalid image %s",u.c_str());
      return 1;
    }
  }else{
    CT::string u=url.c_str();u.encodeURLSelf();
    CDBError("Unable to get image %s",u.c_str());
    return 1;
  }
  return 0;
#endif

  return 0;
}

int CImageDataWriter::init(CServerParams *srvParam,CDataSource *dataSource, int NrOfBands){
  #ifdef CIMAGEDATAWRITER_DEBUG    
  CDBDebug("init");
  #endif
  if(writerStatus!=uninitialized){CDBError("Already initialized");return 1;}
  this->srvParam=srvParam;
  if(_setTransparencyAndBGColor(this->srvParam,&drawImage)!=0)return 1;
  if(srvParam->imageMode==SERVERIMAGEMODE_RGBA||srvParam->Styles.indexOf("HQ")>0){
    drawImage.setTrueColor(true);
    //drawImage.setAntiAliased(true);
  }
  
  if(dataSource->dLayerType!=CConfigReaderLayerTypeCascaded){
      if(initializeLegend(srvParam,dataSource)!=0)return 1;
  }
  if(currentStyleConfiguration!=NULL){
    if(currentStyleConfiguration->renderMethod&RM_RGBA){
      drawImage.setTrueColor(true);
    }
  }
  
  // WMS Format in layer always overrides all
  if(dataSource->cfgLayer->WMSFormat.size()>0){
    if(dataSource->cfgLayer->WMSFormat[0]->attr.name.equals("image/png32")){
      drawImage.setTrueColor(true);
    }
    if(dataSource->cfgLayer->WMSFormat[0]->attr.format.equals("image/png32")){
      drawImage.setTrueColor(true);
    }
  }
  //Set font location
  if(srvParam->cfg->WMS[0]->ContourFont.size()!=0){
    if(srvParam->cfg->WMS[0]->ContourFont[0]->attr.location.c_str()!=NULL){
      drawImage.setTTFFontLocation(srvParam->cfg->WMS[0]->ContourFont[0]->attr.location.c_str());
      
      if(srvParam->cfg->WMS[0]->ContourFont[0]->attr.size.c_str()!=NULL){
        CT::string fontSize=srvParam->cfg->WMS[0]->ContourFont[0]->attr.size.c_str();
        drawImage.setTTFFontSize(fontSize.toFloat());
      }
      //CDBError("Font %s",srvParam->cfg->WMS[0]->ContourFont[0]->attr.location.c_str());
      //return 1;

    }else {
      CDBError("In <Font>, attribute \"location\" missing");
      return 1;
    }
  }
  
  //Set background opacity, if applicable
  if(srvParam->wmsExtensions.opacity<100){
    drawImage.setBackGroundAlpha((unsigned char )(float(srvParam->wmsExtensions.opacity)*2.55));
  }
  
  int status;
  if(dataSource->dLayerType!=CConfigReaderLayerTypeCascaded){
    //Open the data of this dataSource
    #ifdef CIMAGEDATAWRITER_DEBUG  
    CDBDebug("opening %s",dataSource->getFileName());
    #endif  
    CDataReader reader;
    status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_HEADER);
    #ifdef CIMAGEDATAWRITER_DEBUG  
    CDBDebug("Has opened %s",dataSource->getFileName());
    #endif    
    if(status!=0){
      CDBError("Could not open file: %s",dataSource->getFileName());
      return 1;
    }
    #ifdef CIMAGEDATAWRITER_DEBUG  
    CDBDebug("opened");
    #endif  
    reader.close();
  }
  //drawImage.setTrueColor(true);
  //drawImage.setAntiAliased(true);
  /*drawImage.setTrueColor(true);
  drawImage.setAntiAliased(true);*/
  writerStatus=initialized;
  animation = 0;
  nrImagesAdded = 0;
  requestType=srvParam->requestType;
  if(requestType==REQUEST_WMS_GETMAP){
    status = drawImage.createImage(srvParam->Geo);
    if(status != 0) return 1;
  }
  if(requestType==REQUEST_WMS_GETLEGENDGRAPHIC){
    //drawImage.setAntiAliased(false);
    //drawImage.setTrueColor(false);
    int w = LEGEND_WIDTH;
    int h = LEGEND_HEIGHT;
    if(srvParam->Geo->dWidth!=1)w=srvParam->Geo->dWidth;
    if(srvParam->Geo->dHeight!=1)h=srvParam->Geo->dHeight;
    
    status = drawImage.createImage(w,h);
    if(status != 0) return 1;
  }
  if(requestType==REQUEST_WMS_GETFEATUREINFO){
    status = drawImage.createImage(512,256);
    drawImage.Geo->copy(srvParam->Geo);
    //return 0;
  }
  
  //Create 6-8-5 palette for cascaded layer
  if(dataSource->dLayerType==CConfigReaderLayerTypeCascaded){
    #ifdef CIMAGEDATAWRITER_DEBUG    
    CDBDebug("create685Palette");
    #endif
    status = drawImage.create685Palette();
    if(status != 0){
      CDBError("Unable to create standard 6-8-5 palette");
      return 1;
    }
  }

  //Create palette for internal WNS layer
  if(dataSource->dLayerType!=CConfigReaderLayerTypeCascaded){
//    if(initializeLegend(srvParam,dataSource)!=0)return 1;
    status = drawImage.createGDPalette(srvParam->cfg->Legend[currentStyleConfiguration->legendIndex]);
    if(status != 0){
      CDBError("Unknown palette type for %s",srvParam->cfg->Legend[currentStyleConfiguration->legendIndex]->attr.name.c_str());
      return 1;
    }
  }
  if(requestType==REQUEST_WMS_GETMAP){
    /*---------Add cascaded background map now------------------------------------*/
    //drawCascadedWMS("http://geoservices.knmi.nl/cgi-bin/worldmaps.cgi?","world_raster",false);
    //drawCascadedWMS("http://bhlbontw.knmi.nl/rcc/download/ensembles/cgi-bin/basemaps.cgi?","world_eca,country",true);
    /*----------------------------------------------------------------------------*/
  }
  
  #ifdef CIMAGEDATAWRITER_DEBUG    
  CDBDebug("/init");
  #endif
  return 0;
}


/**
* 
*/
void CImageDataWriter::calculateScaleAndOffsetFromMinMax(float &scale, float &offset,float min,float max,float log){
  if(log!=0.0f){
    //CDBDebug("LOG = %f",log);
    min=log10(min)/log10(log);
    max=log10(max)/log10(log);
  }
    
  scale=240/(max-min);
  offset=min*(-scale);
}

/**
* Fills in the styleConfig object based on datasource,stylename, legendname and rendermethod
* 
* @param styleConfig
* 
* 
*/
int CImageDataWriter::makeStyleConfig(StyleConfiguration *styleConfig,CDataSource *dataSource,const char *styleName,const char *legendName,const char *renderMethod){
  CT::string errorMessage;
  CT::string renderMethodString = renderMethod;
  CT::StackList<CT::string> sl = renderMethodString.splitToStack("/");
  if(sl.size()==2){
    renderMethodString.copy(&sl[0]);
    //if(sl[1].equals("HQ")){CDBDebug("32bitmode");}
  }

  styleConfig->renderMethod = getRenderMethodFromString(&renderMethodString);
  if(styleConfig->renderMethod == RM_UNDEFINED){errorMessage.print("rendermethod %s",renderMethod); }
  styleConfig->styleIndex   = getServerStyleIndexByName(styleName,dataSource->cfg->Style);
  //if(styleConfig->styleIndex == -1){errorMessage.print("styleIndex %s",styleName); }
  styleConfig->legendIndex  = getServerLegendIndexByName(legendName,dataSource->cfg->Legend);
  if(styleConfig->legendIndex == -1){errorMessage.print("legendIndex %s",legendName); }
  
  if(errorMessage.length()>0){
    CDBError("Unable to configure style: %s is invalid",errorMessage.c_str());
    return -1;
  }
  
  //Set defaults
  StyleConfiguration * s = styleConfig;
  s->shadeInterval=0.0f;
  s->contourIntervalL=0.0f;
  s->contourIntervalH=0.0f;
  s->legendScale = 0.0f;
  s->legendOffset = 0.0f;
  s->legendLog = 0.0f;
  s->legendLowerRange = 0.0f;
  s->legendUpperRange = 0.0f;
  s->smoothingFilter = 0;
  s->hasLegendValueRange = false;
  
  
  float min =0.0f;
  float max=0.0f;
  bool minMaxSet = false;
  
  if(s->styleIndex!=-1){
    //Get info from style
    CServerConfig::XMLE_Style* style = dataSource->cfg->Style[s->styleIndex];
    if(style->Scale.size()>0)s->legendScale=parseFloat(style->Scale[0]->value.c_str());
    if(style->Offset.size()>0)s->legendOffset=parseFloat(style->Offset[0]->value.c_str());
    if(style->Log.size()>0)s->legendLog=parseFloat(style->Log[0]->value.c_str());
    
    if(style->ContourIntervalL.size()>0)s->contourIntervalL=parseFloat(style->ContourIntervalL[0]->value.c_str());
    if(style->ContourIntervalH.size()>0)s->contourIntervalH=parseFloat(style->ContourIntervalH[0]->value.c_str());
    s->shadeInterval=s->contourIntervalL;
    if(style->ShadeInterval.size()>0)s->shadeInterval=parseFloat(style->ShadeInterval[0]->value.c_str());
    if(style->SmoothingFilter.size()>0)s->smoothingFilter=parseInt(style->SmoothingFilter[0]->value.c_str());
    
    if(style->ValueRange.size()>0){
      s->hasLegendValueRange=true;
      s->legendLowerRange=parseFloat(style->ValueRange[0]->attr.min.c_str());
      s->legendUpperRange=parseFloat(style->ValueRange[0]->attr.max.c_str());
    }
    
    
    if(style->Min.size()>0){min=parseFloat(style->Min[0]->value.c_str());minMaxSet=true;}
    if(style->Max.size()>0){max=parseFloat(style->Max[0]->value.c_str());minMaxSet=true;}
    
    s->contourLines=&style->ContourLine;
    s->shadeIntervals=&style->ShadeInterval;
    
    if(style->Legend.size()>0){
      if(style->Legend[0]->attr.tickinterval.c_str() != NULL){
        styleConfig->legendTickInterval = parseDouble(style->Legend[0]->attr.tickinterval.c_str());
      }
      if(style->Legend[0]->attr.tickround.c_str() != NULL){
        styleConfig->legendTickRound = parseDouble(style->Legend[0]->attr.tickround.c_str());
      }
      if(style->Legend[0]->attr.fixedclasses.equals("true")){
        styleConfig->legendHasFixedMinMax=true;
      }
    }
    
    
  }
  
  //Legend settings can always be overriden in the layer itself!
  CServerConfig::XMLE_Layer* layer = dataSource->cfgLayer;
  if(layer->Scale.size()>0)s->legendScale=parseFloat(layer->Scale[0]->value.c_str());
  if(layer->Offset.size()>0)s->legendOffset=parseFloat(layer->Offset[0]->value.c_str());
  if(layer->Log.size()>0)s->legendLog=parseFloat(layer->Log[0]->value.c_str());
  
  if(layer->ContourIntervalL.size()>0)s->contourIntervalL=parseFloat(layer->ContourIntervalL[0]->value.c_str());
  if(layer->ContourIntervalH.size()>0)s->contourIntervalH=parseFloat(layer->ContourIntervalH[0]->value.c_str());
  if(s->shadeInterval == 0.0f)s->shadeInterval = s->contourIntervalL;
  if(layer->ShadeInterval.size()>0)s->shadeInterval=parseFloat(layer->ShadeInterval[0]->value.c_str());
  if(layer->SmoothingFilter.size()>0)s->smoothingFilter=parseInt(layer->SmoothingFilter[0]->value.c_str());
  
  if(layer->ValueRange.size()>0){
    s->hasLegendValueRange=true;
    s->legendLowerRange=parseFloat(layer->ValueRange[0]->attr.min.c_str());
    s->legendUpperRange=parseFloat(layer->ValueRange[0]->attr.max.c_str());
  }
  
  if(layer->Min.size()>0){min=parseFloat(layer->Min[0]->value.c_str());minMaxSet=true;}
  if(layer->Max.size()>0){max=parseFloat(layer->Max[0]->value.c_str());minMaxSet=true;}

  if(layer->ContourLine.size()>0){
    s->contourLines=&layer->ContourLine;
  }
  if(layer->ShadeInterval.size()>0){
    s->shadeIntervals=&layer->ShadeInterval;
  }
  
  if(layer->Legend.size()>0){
    if(layer->Legend[0]->attr.tickinterval.c_str() != NULL){
      styleConfig->legendTickInterval = parseDouble(layer->Legend[0]->attr.tickinterval.c_str());
    }
    if(layer->Legend[0]->attr.tickround.c_str() != NULL){
      styleConfig->legendTickRound = parseDouble(layer->Legend[0]->attr.tickround.c_str());
    }
    if(layer->Legend[0]->attr.fixedclasses.equals("true")){
      styleConfig->legendHasFixedMinMax=true;
    }
  }
  
  //Min and max can again be overriden by WMS extension settings
  if( dataSource->srvParams->wmsExtensions.colorScaleRangeSet){
    minMaxSet=true;
    min=dataSource->srvParams->wmsExtensions.colorScaleRangeMin;
    max=dataSource->srvParams->wmsExtensions.colorScaleRangeMax;
  }
  //Log can again be overriden by WMS extension settings
  if(dataSource->srvParams->wmsExtensions.logScale){
    s->legendLog=10;
  }
      
  if(dataSource->srvParams->wmsExtensions.numColorBands !=-1){
    s->shadeInterval=dataSource->srvParams->wmsExtensions.numColorBands;
    s->contourIntervalL=dataSource->srvParams->wmsExtensions.numColorBands;
  }
        
  
  
  //When min and max are given, calculate the scale and offset according to min and max.
  if(minMaxSet){
    #ifdef CIMAGEDATAWRITER_DEBUG          
    CDBDebug("Found min and max in layer configuration");
    #endif      
    calculateScaleAndOffsetFromMinMax(s->legendScale,s->legendOffset,min,max,s->legendLog);
    //s->legendScale=240/(max-min);
    //s->legendOffset=min*(-s->legendScale);
  }
    
  //Some safety checks, we cannot create contourlines with negative values.
  /*if(s->contourIntervalL<=0.0f||s->contourIntervalH<=0.0f){
    if(s->renderMethod==contour||
      s->renderMethod==bilinearcontour||
      s->renderMethod==nearestcontour){
      s->renderMethod=nearest;
      }
  }*/
  CT::string styleDump;
  styleConfig->printStyleConfig(&styleDump);
  #ifdef CIMAGEDATAWRITER_DEBUG          
  CDBDebug("FOUND legend %s, rendermethod %s, style %s.",legendName,renderMethod,styleName);
  CDBDebug("styleDump:\n%s",styleDump.c_str());
  #endif
  return 0;
}


/**
* Returns a stringlist with all available legends for this datasource and chosen style.
* @param dataSource pointer to the datasource 
* @param style pointer to the style to find the legends for
* @return stringlist with the list of available legends.
*/


CT::PointerList<CT::string*> *CImageDataWriter::getLegendListForDataSource(CDataSource *dataSource, CServerConfig::XMLE_Style* style){
  
  if(dataSource->cfgLayer->Legend.size()>0){
    return getLegendNames(dataSource->cfgLayer->Legend);
  }else{
    if(style!=NULL){
      return getLegendNames(style->Legend);
    }
  }
  CDBError("No legendlist for layer %s",dataSource->layerName.c_str());
  return NULL;
}


/**
* Returns a stringlist with all available rendermethods for this datasource and chosen style.
* @param dataSource pointer to the datasource 
* @param style pointer to the style to find the rendermethods for
* @return stringlist with the list of available rendermethods.
*/
CT::PointerList<CT::string*> *CImageDataWriter::getRenderMethodListForDataSource(CDataSource *dataSource, CServerConfig::XMLE_Style* style){
  //List all the desired rendermethods
  CT::string renderMethodList;
  
  if(style!=NULL){
    if(style->RenderMethod.size()==1){
      renderMethodList.copy(style->RenderMethod[0]->value.c_str());
    }
  }
  
  //rendermethods defined in the layers override rendermethods defined in the style
  if(dataSource->cfgLayer->RenderMethod.size()==1){
    renderMethodList.copy(dataSource->cfgLayer->RenderMethod[0]->value.c_str());
  }
  //If still no list of rendermethods is found, use the default list
  if(renderMethodList.length()==0){
    renderMethodList.copy(CImageDataWriter::RenderMethodStringList);
  }
  return  renderMethodList.splitToPointer(",");;
}

/**
* This function calls getStyleListForDataSource in mode (1).
* 
* @param dataSource pointer to the datasource to find the stylelist for
* @return the stringlist with all possible stylenames. Pointer should be deleted with delete!
*/
CT::PointerList<CT::string*> *CImageDataWriter::getStyleListForDataSource(CDataSource *dataSource){
  return getStyleListForDataSource(dataSource,NULL);
}

/**
* Returns a new StyleConfiguration object which contains all settings for the corresponding styles. This function calls getStyleListForDataSource in mode(2).
* @param styleName
* @param serverCFG
* @return A new StyleConfiguration which must be deleted with delete.
*/
CImageDataWriter::StyleConfiguration *CImageDataWriter::getStyleConfigurationByName(const char *styleName,CDataSource *dataSource){
  #ifdef CIMAGEDATAWRITER_DEBUG    
  CDBDebug("getStyleConfigurationByName for layer %s with name %s",dataSource->layerName.c_str(),styleName);
  #endif
  
  //CServerConfig::XMLE_Configuration *serverCFG = dataSource->cfg;
  StyleConfiguration *styleConfig = new StyleConfiguration ();
  styleConfig->styleCompositionName=styleName;
  getStyleListForDataSource(dataSource,styleConfig);
  return styleConfig;
}

/**
* This function has two modes, return a string list (1) or (2) configure a StyleConfiguration object.
*   (1) Returns a stringlist with all possible style names for a datasource, when styleConfig is set to NULL.
*   (2) When a styleConfig is provided, this function fills in the provided StyleConfiguration object, 
*   the styleCompositionName needs to be set in advance (The stylename usually given in the request string)
* 
* @param dataSource pointer to the datasource to find the stylelist for
* @param styleConfig pointer to the StyleConfiguration object to be filled in. 
* @return the stringlist with all possible stylenames
*/
CT::PointerList<CT::string*> *CImageDataWriter::getStyleListForDataSource(CDataSource *dataSource,StyleConfiguration *styleConfig){
//  CDBDebug("getStyleListForDataSource");
  CT::PointerList<CT::string*> *stringList = new CT::PointerList<CT::string*>();
  CServerConfig::XMLE_Configuration *serverCFG = dataSource->cfg;
  CT::string styleToSearchString;
  bool isDefaultStyle = false;
  bool returnStringList = true;
  
  if(styleConfig!=NULL){
    styleConfig->hasError=false;
    returnStringList=false;
    styleConfig->contourLines = NULL;
    styleConfig->shadeIntervals=NULL;
    delete stringList;stringList = NULL;
    styleToSearchString.copy(&styleConfig->styleCompositionName);
    if(styleToSearchString.equals("default")||styleToSearchString.equals("default/HQ")){
      isDefaultStyle = true;
    }
  }
  

  //Auto configure styles, if no legends or styles are defined
  if(dataSource->cfgLayer->Styles.size()==0&&dataSource->cfgLayer->Legend.size()==0){
    CDataReader::autoConfigureStyles(dataSource);
  }
    
  CT::PointerList<CT::string*> *styleNames = getStyleNames(dataSource->cfgLayer->Styles);

  //We always skip the style "default" if there are more styles.
  size_t start=0;if(styleNames->size()>1)start=1;
  
  CT::PointerList<CT::string*> *renderMethods = NULL;
  CT::PointerList<CT::string*> *legendList = NULL;
  //Loop over the styles.
  try{
    for(size_t i=start;i<styleNames->size();i++){
      //Lookup the style index in the servers configuration
      int dStyleIndex=getServerStyleIndexByName(styleNames->get(i)->c_str(),serverCFG->Style);
      if(dStyleIndex==-1){
        if(returnStringList){
          if(!styleNames->get(i)->equals("default")){
            CDBError("Style %s not found for layer %s",styleNames->get(i)->c_str(),dataSource->layerName.c_str());
            delete styleNames;styleNames = NULL;
            delete stringList;stringList = NULL;
            return NULL;
          }else{
            CT::string * styleName = new CT::string();
            styleName->copy("default");
            stringList->push_back(styleName);
            delete styleNames;styleNames = NULL;
            return stringList;
          }
        }
      }
      CServerConfig::XMLE_Style* style = NULL;
      if(dStyleIndex!=-1)style=serverCFG->Style[dStyleIndex];
      
    
      
      renderMethods = getRenderMethodListForDataSource(dataSource,style);
      legendList = getLegendListForDataSource(dataSource,style);
      
      if(legendList==NULL){
        CDBError("No legends defined for layer %s",dataSource->layerName.c_str());
        delete styleNames;styleNames = NULL;
        delete stringList;stringList = NULL;
        delete renderMethods;renderMethods= NULL;
        if(styleConfig!=NULL){styleConfig->hasError=true;}
        return NULL;
      }
      for(size_t l=0;l<legendList->size();l++){
        for(size_t r=0;r<renderMethods->size();r++){
          if(renderMethods->get(r)->length()>0){
            CT::string * styleName = new CT::string();
            if(legendList->size()>1){
              styleName->print("%s_%s/%s",styleNames->get(i)->c_str(),legendList->get(l)->c_str(),renderMethods->get(r)->c_str());
            }else{
              styleName->print("%s/%s",styleNames->get(i)->c_str(),renderMethods->get(r)->c_str());
            }
            

            //StyleConfiguration mode, try to find which stylename we want our StyleConfiguration for.
            if(styleConfig!=NULL){
              #ifdef CIMAGEDATAWRITER_DEBUG    
              CDBDebug("Matching '%s' == '%s'",styleName->c_str(),styleToSearchString.c_str());
              #endif
              if(styleToSearchString.equals(styleName)||isDefaultStyle==true){
                // We found the correspondign legend/style and rendermethod corresponding with the requested stylename!
                // Now fill in the StyleConfiguration Object.
                
                
                if(style!=NULL){
                  
                  for(size_t j=0;j<style->NameMapping.size();j++){
                    if(renderMethods->get(r)->equals(style->NameMapping[j]->attr.name.c_str())){
                      styleConfig->styleTitle.copy(style->NameMapping[j]->attr.title.c_str());
                      styleConfig->styleAbstract.copy(style->NameMapping[j]->attr.abstract.c_str());
                      break;
                    }
                  }
                  
                }
                
                
                int status = makeStyleConfig(styleConfig,dataSource,styleNames->get(i)->c_str(),legendList->get(l)->c_str(),renderMethods->get(r)->c_str());
                delete styleName;
                if(status == -1){
                  styleConfig->hasError=true;
                }
                //Stop with iterating:
                throw(__LINE__);
              }
            }
            if(returnStringList)stringList->push_back(styleName);else delete styleName;
          }
        }
      }
      delete legendList;legendList =NULL;
      delete renderMethods;renderMethods = NULL;
    }
    delete styleNames; styleNames = NULL;
    
    // We have been through the loop, but the styleConfig has not been created. This is an error.
    if(styleConfig!=NULL){
      CDBError("Unable to find style %s",styleToSearchString.c_str());
      styleConfig->hasError=true;
    }
  }catch(int e){
    delete legendList;
    delete renderMethods;
    delete styleNames;
    delete stringList;stringList=NULL;
  }
    
  return stringList;
}


/**
* Returns a stringlist with all possible legends available for this Legend config object.
* This is usually a configured legend element in a layer, or a configured legend element in a style.
* @param Legend a XMLE_Legend object configured in a style or in a layer
* @return Pointer to a new stringlist with all possible legend names, must be deleted with delete. Is NULL on failure.
*/
CT::PointerList<CT::string*> *CImageDataWriter::getLegendNames(std::vector <CServerConfig::XMLE_Legend*> Legend){
  if(Legend.size()==0){CDBError("No legends defined");return NULL;}
  CT::PointerList<CT::string*> *stringList = new CT::PointerList<CT::string*>();
  
  for(size_t j=0;j<Legend.size();j++){
    CT::string legendValue=Legend[j]->value.c_str();
    CT::StackList<CT::string> l1=legendValue.splitToStack(",");
    for(size_t i=0;i<l1.size();i++){
      if(l1[i].length()>0){
        CT::string * val = new CT::string();
        stringList->push_back(val);
        val->copy(&l1[i]);
      }
    }
  }
  return stringList;
}

/**
* Returns a stringlist with all possible styles available for this style config object.
* @param Style a pointer to XMLE_Style vector configured in a layer
* @return Pointer to a new stringlist with all possible style names, must be deleted with delete. Is NULL on failure.
*/
CT::PointerList<CT::string*> *CImageDataWriter::getStyleNames(std::vector <CServerConfig::XMLE_Styles*> Styles){
  CT::PointerList<CT::string*> *stringList = new CT::PointerList<CT::string*>();
  CT::string * val = new CT::string();
  stringList->push_back(val);
  val->copy("default");
  for(size_t j=0;j<Styles.size();j++){
    if(Styles[j]->value.c_str()!=NULL){
      CT::string StyleValue=Styles[j]->value.c_str();
      if(StyleValue.length()>0){
        CT::StackList<CT::string>  l1=StyleValue.splitToStack(",");
        for(size_t i=0;i<l1.size();i++){
          if(l1[i].length()>0){
            CT::string * val = new CT::string();
            stringList->push_back(val);
            val->copy(&l1[i]);
          }
        }
      }
    }
  }

  return stringList;
}



/**
* Retrieves the position of for the requested style name in the servers configured style elements.
* @param styleName The name of the style to locate
* @param serverStyles Pointer to the servers configured styles.
* @return The style index as integer, points to the position in the servers configured styles. Is -1 on failure.
*/
int  CImageDataWriter::getServerStyleIndexByName(const char * styleName,std::vector <CServerConfig::XMLE_Style*> serverStyles){
  if(styleName==NULL){
    CDBError("No style name provided");
    return -1;
  }
  CT::string styleString = styleName;
  if(styleString.equals("default")||styleString.equals("default/HQ"))return -1;
  for(size_t j=0;j<serverStyles.size();j++){
    if(serverStyles[j]->attr.name.c_str()!=NULL){
      if(styleString.equals(serverStyles[j]->attr.name.c_str())){
        return j;
      }
    }
  }
  CDBError("No style found with name [%s]",styleName);
  return -1;
}

/**
* Retrieves the position of for the requested legend name in the servers configured legend elements.
* @param legendName The name of the legend to locate
* @param serverLegends Pointer to the servers configured legends.
* @return The legend index as integer, points to the position in the servers configured legends. Is -1 on failure.
*/
int  CImageDataWriter::getServerLegendIndexByName(const char * legendName,std::vector <CServerConfig::XMLE_Legend*> serverLegends){
  int dLegendIndex=-1;
  CT::string legendString = legendName;
  if(legendName==NULL)return -1;
  for(size_t j=0;j<serverLegends.size()&&dLegendIndex==-1;j++){
    if(legendString.equals(serverLegends[j]->attr.name.c_str())){
      dLegendIndex=j;
      break;
    }
  }
  return dLegendIndex;
}

int CImageDataWriter::initializeLegend(CServerParams *srvParam,CDataSource *dataSource){
  if(currentDataSource != NULL){
    if(dataSource==currentDataSource){
      return 0;
    }
  }
  #ifdef CIMAGEDATAWRITER_DEBUG    
  CDBDebug("initializeLegend");
  #endif
  if(srvParam==NULL){
    CDBError("srvParam==NULL");
    return -1;
  }
  
  if(_setTransparencyAndBGColor(srvParam,&drawImage)!=0){
    CDBError("Unable to do setTransparencyAndBGColor");
    return -1;
  }
  
  CT::string styleName="default";
  CT::string styles(srvParam->Styles.c_str());
  #ifdef CIMAGEDATAWRITER_DEBUG    
  CDBDebug("Server Styles=%s",srvParam->Styles.c_str());
  #endif
  CT::StackList<CT::string> layerstyles = styles.splitToStack(",");
  int layerIndex=dataSource->datasourceIndex;
  if(layerstyles.size()!=0){
    //Make sure default layer index is within the right bounds.
    if(layerIndex<0)layerIndex=0;
    if(layerIndex>((int)layerstyles.size())-1)layerIndex=layerstyles.size()-1;
    styleName=layerstyles[layerIndex].c_str();
    if(styleName.length()==0){
      styleName.copy("default");
    }
  }

  delete currentStyleConfiguration;
  
  dataSource->styleName=&styleName;
  currentStyleConfiguration=CImageDataWriter::getStyleConfigurationByName(styleName.c_str(),dataSource);
  if(currentStyleConfiguration->hasError){
    CDBError("Unable to configure style %s for layer %s\n",styleName.c_str(),dataSource->layerName.c_str());
  
    return -1;
  }
  
  dataSource->legendScale = currentStyleConfiguration->legendScale;
  dataSource->legendOffset = currentStyleConfiguration->legendOffset;
  dataSource->legendLog = currentStyleConfiguration->legendLog;
  dataSource->legendLowerRange = currentStyleConfiguration->legendLowerRange;
  dataSource->legendUpperRange = currentStyleConfiguration->legendUpperRange;
  dataSource->legendValueRange = currentStyleConfiguration->hasLegendValueRange;
  
  #ifdef CIMAGEDATAWRITER_DEBUG    
  CDBDebug("/initializeLegend");
  #endif
  
  currentDataSource = dataSource;
  return 0;
}


double CImageDataWriter::convertValue(CDFType type,void *data,size_t ptr){
  double pixel = 0.0f;
  if(type==CDF_CHAR)pixel=((char*)data)[ptr];
  if(type==CDF_UBYTE)pixel=((unsigned char*)data)[ptr];
  if(type==CDF_SHORT)pixel=((short*)data)[ptr];
  if(type==CDF_USHORT)pixel=((unsigned short*)data)[ptr];
  if(type==CDF_INT)pixel=((int*)data)[ptr];
  if(type==CDF_UINT)pixel=((unsigned int*)data)[ptr];
  if(type==CDF_FLOAT)pixel=((float*)data)[ptr];
  if(type==CDF_DOUBLE)pixel=((double*)data)[ptr];
  return pixel;
}
void CImageDataWriter::setValue(CDFType type,void *data,size_t ptr,double pixel){
  if(type==CDF_CHAR)((char*)data)[ptr]=(char)pixel;
  if(type==CDF_UBYTE)((unsigned char*)data)[ptr]=(unsigned char)pixel;
  if(type==CDF_SHORT)((short*)data)[ptr]=(short)pixel;
  if(type==CDF_USHORT)((unsigned short*)data)[ptr]=(unsigned short)pixel;
  if(type==CDF_INT)((int*)data)[ptr]=(int)pixel;
  if(type==CDF_UINT)((unsigned int*)data)[ptr]=(unsigned int)pixel;
  if(type==CDF_FLOAT)((float*)data)[ptr]=(float)pixel;
  if(type==CDF_DOUBLE)((double*)data)[ptr]=(double)pixel;
}


int CImageDataWriter::getFeatureInfo(std::vector<CDataSource *>dataSources,int dataSourceIndex,int dX,int dY){
  
  #ifdef MEASURETIME
  StopWatch_Stop("getFeatureInfo");
  #endif
  
  #ifdef CIMAGEDATAWRITER_DEBUG    
  CDBDebug("[getFeatureInfo] %d, %d, [%d,%d]", dataSources.size(), dataSourceIndex, dX, dY);
  #endif
  // Create a new getFeatureInfoResult object and push it into the vector.
  int status = 0;
  
  for(size_t d=0;d<dataSources.size();d++){
    GetFeatureInfoResult  *getFeatureInfoResult = new GetFeatureInfoResult();
    getFeatureInfoResultList.push_back(getFeatureInfoResult);
    
    for(int step=0;step<dataSources[d]->getNumTimeSteps();step++){
      dataSources[d]->setTimeStep(step);
      #ifdef CIMAGEDATAWRITER_DEBUG    
      CDBDebug("Processing dataSource %d with step %d of %d timesteps",d,step,dataSources[d]->getNumTimeSteps());
      #endif
    
      bool headerIsAvailable = false;
      bool openAll = false;
      
      if(dataSources[d]->dataObject[0]->cdfVariable!=NULL){
        headerIsAvailable=true;
        if(dataSources[d]->dataObject[0]->cdfVariable->getAttributeNE("ADAGUC_VECTOR")!=NULL){
          openAll =true;
        }  
      
        if(dataSources[d]->dataObject[0]->cdfVariable->getAttributeNE("ADAGUC_POINT")!=NULL){
          openAll =true;
        }  
      }
    
      CDataSource *dataSource=dataSources[d];
      if(dataSource==NULL){
        CDBError("dataSource == NULL");
        return 1;
      }
    
      //Copy layer name
      getFeatureInfoResult->layerName.copy(&dataSource->layerName);
      getFeatureInfoResult->dataSourceIndex=dataSourceIndex;
      
      CDataReader reader;
      //if(!headerIsAvailable)
      {
        if(openAll){
          //CDBDebug("OPEN ALL");
          status = reader.open(dataSources[d],CNETCDFREADER_MODE_OPEN_ALL);
        }else{
          //CDBDebug("OPEN HEADER");
          if(!headerIsAvailable){
            status = reader.open(dataSources[d],CNETCDFREADER_MODE_OPEN_HEADER);
          }
        }
      
        
        if(status!=0){
          CDBError("Could not open file: %s",dataSource->getFileName());
          return 1;
        }
      }
      
      
      //(89,26)       (5.180666,52.101790)    (5.180666,52.101790)
      
      //double CoordX=5.180666,CoordY=52.101790;
      //double nativeCoordX=5.180666,nativeCoordY=52.101790;
      //double lonX=5.180666,lonY=52.101790;
      //int imx=89,imy=26;
      
      CT::string ckey;
      ckey.print("%d%d%s",dX,dY,dataSource->nativeProj4.c_str());
      std::string key=ckey.c_str();
      ProjCacheInfo projCacheInfo ;
      
      #ifdef MEASURETIME
      StopWatch_Stop("projCacheInfo");
      #endif
      
      try{
        projCacheIter=projCacheMap.find(key);
        if(projCacheIter==projCacheMap.end()){
          throw 1;
        }
        projCacheInfo = (*projCacheIter).second;
        
        #ifdef MEASURETIME
        StopWatch_Stop("found cache projCacheInfo");
        #endif
      }catch(int e){
  
        #ifdef CIMAGEDATAWRITER_DEBUG  
        CDBDebug("initreproj %s",dataSource->nativeProj4.c_str());
        #endif
        status = imageWarper.initreproj(dataSource,drawImage.Geo,&srvParam->cfg->Projection);
        if(status!=0){CDBError("initreproj failed");reader.close();return 1;  }

        //getFeatureInfoHeader.copy("");
        double x,y,sx,sy;
        sx=dX;
        sy=dY;

        x=double(sx)/double(drawImage.Geo->dWidth);
        y=double(sy)/double(drawImage.Geo->dHeight);
        x*=(drawImage.Geo->dfBBOX[2]-drawImage.Geo->dfBBOX[0]);
        y*=(drawImage.Geo->dfBBOX[1]-drawImage.Geo->dfBBOX[3]);
        x+=drawImage.Geo->dfBBOX[0];
        y+=drawImage.Geo->dfBBOX[3];

        projCacheInfo.CoordX=x;
        projCacheInfo.CoordY=y;

        imageWarper.reprojpoint(x,y);
        projCacheInfo.nativeCoordX=x;
        projCacheInfo.nativeCoordY=y;

        x-=dataSource->dfBBOX[0];
        y-=dataSource->dfBBOX[1];
        x/=(dataSource->dfBBOX[2]-dataSource->dfBBOX[0]);
        y/=(dataSource->dfBBOX[3]-dataSource->dfBBOX[1]);
        x*=double(dataSource->dWidth);
        y*=double(dataSource->dHeight);
        //CDBDebug("%f %f",x,y);
        projCacheInfo.dWidth=dataSource->dWidth;
        projCacheInfo.dHeight=dataSource->dHeight;

        if(x<0){
          projCacheInfo.imx=-1;
        }else{
          projCacheInfo.imx=int(x);
        }
        if(y<0){
          projCacheInfo.imy=-1;
        }else{
          projCacheInfo.imy=dataSource->dHeight-(int)y-1;
        }

        projCacheInfo.lonX=projCacheInfo.CoordX;
        projCacheInfo.lonY=projCacheInfo.CoordY;
        //Get lat/lon
        imageWarper.reprojToLatLon(projCacheInfo.lonX,projCacheInfo.lonY);
        imageWarper.closereproj();
        projCacheMap[key]=projCacheInfo;
      }
      
      #ifdef MEASURETIME
      StopWatch_Stop("/projCacheInfo");
      #endif
      //CDBDebug("ProjRes = (%d,%d)(%f,%f)(%f,%f)(%f,%f)",projCacheInfo.imx,projCacheInfo.imy,projCacheInfo.CoordX,projCacheInfo.CoordY,projCacheInfo.nativeCoordX,projCacheInfo.nativeCoordY,projCacheInfo.lonX,projCacheInfo.lonY);
      
      // Projections coordinates in latlon
      getFeatureInfoResult->lon_coordinate=projCacheInfo.lonX;
      getFeatureInfoResult->lat_coordinate=projCacheInfo.lonY;
      
      // Pixel X and Y on the image
      getFeatureInfoResult->x_imagePixel=dX;
      getFeatureInfoResult->y_imagePixel=dY;
    
      // Projection coordinates X and Y on the image
      getFeatureInfoResult->x_imageCoordinate=projCacheInfo.CoordX;
      getFeatureInfoResult->y_imageCoordinate=projCacheInfo.CoordY;
      
      // Projection coordinates X and Y in the raster
      getFeatureInfoResult->x_rasterCoordinate=projCacheInfo.nativeCoordX;
      getFeatureInfoResult->y_rasterCoordinate=projCacheInfo.nativeCoordY;
      
      // Pixel X and Y on the raster
      getFeatureInfoResult->x_rasterIndex=projCacheInfo.imx;
      getFeatureInfoResult->y_rasterIndex=projCacheInfo.imy;
      
      if(projCacheInfo.imx>=0&&projCacheInfo.imy>=0&&projCacheInfo.imx<projCacheInfo.dWidth&&projCacheInfo.imy<projCacheInfo.dHeight){
        if(!openAll){
          #ifdef CIMAGEDATAWRITER_DEBUG 
          CDBDebug("Reading datasource %d for %d,%d",d,projCacheInfo.imx,projCacheInfo.imy);
          #endif
          
          status = reader.open(dataSources[d],CNETCDFREADER_MODE_OPEN_ALL,projCacheInfo.imx,projCacheInfo.imy);
          
          #ifdef CIMAGEDATAWRITER_DEBUG 
          CDBDebug("Done");
          #endif
          
        }

        if(status!=0){
          CDBError("Could not open file: %s",dataSource->getFileName());
          return 1;
        }
      }

      //TODO find raster projection units and find image projection units.
      
      //Determine if this is a GridRelative vector product
      //bool windVectorProduct=false;
      bool gridRelative=false;
      if (dataSource->dataObject.size()>1){
        //windVectorProduct=true;
        // Check standard_name/var_name for first vector component
        // if x_wind/grid_east_wind of y_wind/grid_northward_wind then gridRelative=true
        // if eastward_wind/northward_wind then gridRelative=false
        // default is gridRelative=true
        CT::string standard_name;
        standard_name=dataSource->dataObject[0]->variableName;
        try {
          dataSource->dataObject[0]->cdfVariable->getAttribute("standard_name")->getDataAsString(&standard_name);
        } catch (CDFError e) {}
        if (standard_name.equals("x_wind")||standard_name.equals("grid_eastward_wind")||
          standard_name.equals("y_wind")||standard_name.equals("grid_northward_wind")) {
          gridRelative=true;
        } else {
          gridRelative=false;
        }
        #ifdef CIMAGEDATAWRITER_DEBUG 
        CDBDebug("Grid propery gridRelative=%d", gridRelative);
        #endif
      }
          

    //Retrieve variable names
    for(size_t o=0;o<dataSource->dataObject.size();o++){
      //size_t j=d+o*dataSources.size();
//      CDBDebug("j = %d",j);
      //Create a new element and at it to the elements list.
      
      GetFeatureInfoResult::Element * element = new GetFeatureInfoResult::Element();
      getFeatureInfoResult->elements.push_back(element);
      element->dataSource=dataSource;
      //Get variable name
      element->var_name.copy(&dataSources[d]->dataObject[o]->variableName);
      //Get variable units
      element->units.copy(&dataSources[d]->dataObject[o]->units);

      //Get variable standard name
      CDF::Attribute * attr_standard_name=dataSources[d]->dataObject[o]->cdfVariable->getAttributeNE("standard_name");
      if(attr_standard_name!=NULL){
        CT::string standardName;attr_standard_name->getDataAsString(&standardName);
        element->standard_name.copy(&standardName);
        // Make a more clean standard name.
        standardName.replaceSelf("_"," ");standardName.replaceSelf(" status flag","");
        element->feature_name.copy(&standardName);
      }
      if(element->standard_name.c_str()==NULL){
        element->standard_name.copy(&element->var_name);
        element->feature_name.copy(&element->var_name);
      }

      // Get variable long name
      CDF::Attribute * attr_long_name=dataSources[d]->dataObject[o]->cdfVariable->getAttributeNE("long_name");
      if(attr_long_name!=NULL){
        attr_long_name->getDataAsString(&element->long_name);
      }else element->long_name.copy(&element->var_name);
      
      // Assign CDF::Variable Pointer
      element->variable = dataSources[d]->dataObject[o]->cdfVariable;
      element->value="nodata";
    
      element->time.copy(&dataSources[d]->timeSteps[dataSources[d]->getCurrentTimeStep()]->timeString);
      
      CCDFDims * cdfDims = dataSources[d]->getCDFDims();
      CT::string value,name;
      for(size_t j=0;j<dataSources[d]->requiredDims.size();j++){
        value=cdfDims->getDimensionValue(j);
        name=cdfDims->getDimensionName(j);
        if(name.indexOf("time")==0){
          value=element->time.c_str();
        }
        element->cdfDims.addDimension(name.c_str(),value.c_str(),cdfDims->getDimensionIndex(j));
      }
  
      #ifdef CIMAGEDATAWRITER_DEBUG  
      CDBDebug("getFeatureInfoResult->elements has %d elements\n", getFeatureInfoResult->elements.size());
      #endif
      // Retrieve corresponding values.
      #ifdef CIMAGEDATAWRITER_DEBUG  
      CDBDebug("imx:%d imy:%d projCacheInfo.dWidth:%d projCacheInfo.dHeight:%d",projCacheInfo.imx,projCacheInfo.imy,projCacheInfo.dWidth,projCacheInfo.dHeight);
      #endif
      if(projCacheInfo.imx>=0&&projCacheInfo.imy>=0&&projCacheInfo.imx<projCacheInfo.dWidth&&projCacheInfo.imy<projCacheInfo.dHeight){
        #ifdef CIMAGEDATAWRITER_DEBUG 
        CDBDebug("Accessing element %d",j);
        #endif
        
        //GetFeatureInfoResult::Element * element=getFeatureInfoResult->elements[j];

        size_t ptr=0;
        if(openAll){
          ptr=projCacheInfo.imx+projCacheInfo.imy*projCacheInfo.dWidth;
        }
        
        #ifdef CIMAGEDATAWRITER_DEBUG 
        CDBDebug("ptr = %d",ptr);
        #endif
        double pixel=convertValue(dataSource->dataObject[o]->cdfVariable->type,dataSource->dataObject[o]->cdfVariable->data,ptr);

        #ifdef CIMAGEDATAWRITER_DEBUG 
        CDBDebug("pixel value = %f",pixel);
        #endif
        //Fill in the actual data value
        //Check whether this is a NoData value:
        if((pixel!=dataSource->dataObject[o]->dfNodataValue&&dataSource->dataObject[o]->hasNodataValue==true&&pixel==pixel)||
          dataSource->dataObject[o]->hasNodataValue==false){
          if(dataSource->dataObject[o]->hasStatusFlag){
            //Add status flag
            CT::string flagMeaning;
            CDataSource::getFlagMeaningHumanReadable(&flagMeaning,&dataSource->dataObject[o]->statusFlagList,pixel);
            element->value.print("%s (%d)",flagMeaning.c_str(),(int)pixel);
            element->units="";
          }else{
            //Add raster value
            char szTemp[1024];
            floatToString(szTemp,1023,pixel);
            element->value=szTemp;
          }
          #ifdef CIMAGEDATAWRITER_DEBUG 
          CDBDebug("Element value == %s",element->value.c_str());
          #endif
        }else {
          element->value="nodata";
          #ifdef CIMAGEDATAWRITER_DEBUG 
          CDBDebug("Element value == %s",element->value.c_str());
          #endif
        }
       }
      }

      //reader.close();
      #ifdef CIMAGEDATAWRITER_DEBUG 
      CDBDebug("dataSource->dataObject.size()==%d",dataSource->dataObject.size());
      #endif
      
      //For vectors, we will calculate angle and strength
      if(dataSource->dataObject.size()==2){
        size_t ptr=0;
        if(openAll){
          ptr=projCacheInfo.imx+projCacheInfo.imy*projCacheInfo.dWidth;
        }

        double pi=3.141592;
        double pixel1=convertValue(dataSource->dataObject[0]->cdfVariable->type,dataSource->dataObject[0]->cdfVariable->data,ptr);
        double pixel2=convertValue(dataSource->dataObject[1]->cdfVariable->type,dataSource->dataObject[1]->cdfVariable->data,ptr);
        if (gridRelative)  {
    //Add raster value
          #ifdef CIMAGEDATAWRITER_DEBUG 
          CDBDebug("Let's do the Jacobian!!!");
          #endif
          status = imageWarper.initreproj(dataSource,drawImage.Geo,&srvParam->cfg->Projection);
          #ifdef CIMAGEDATAWRITER_DEBUG 
          CDBDebug("doJacoIntoLatLon(%f,%f,%f, %f, %f, %f)", pixel1,pixel2, projCacheInfo.lonX,projCacheInfo.lonY,0.01,0.01);
          #endif
          doJacoIntoLatLon(pixel1, pixel2, projCacheInfo.lonX, projCacheInfo.lonY, 0.01, 0.01, &imageWarper);
          imageWarper.closereproj();
    
          char szTemp[1024];
          floatToString(szTemp, 1023, pixel1); //New val
          getFeatureInfoResult->elements[getFeatureInfoResult->elements.size()-2]->value=szTemp;
          floatToString(szTemp, 1023, pixel2); //New val
          getFeatureInfoResult->elements[getFeatureInfoResult->elements.size()-1]->value=szTemp;
        }
        
        GetFeatureInfoResult::Element *element2=new GetFeatureInfoResult::Element();
        CCDFDims * cdfDims = dataSources[d]->getCDFDims();
        CT::string value,name;
        element2->time.copy(&dataSources[d]->timeSteps[dataSources[d]->getCurrentTimeStep()]->timeString);
        for(size_t j=0;j<dataSources[d]->requiredDims.size();j++){
          value=cdfDims->getDimensionValue(j);
          name=cdfDims->getDimensionName(j);
          if(name.indexOf("time")==0){
            value=element2->time.c_str();
          }
          element2->cdfDims.addDimension(name.c_str(),value.c_str(),cdfDims->getDimensionIndex(j));
        }
        element2->dataSource= dataSource;
        getFeatureInfoResult->elements.push_back(element2);
        double angle=270-atan2(pixel2, pixel1)*180/pi;
        if (angle>360) angle-=360;
        if (angle<0) angle=angle+360;
        element2->long_name="wind direction";
        element2->var_name="wind direction";
        element2->standard_name="dir";
        element2->feature_name="wind direction";
        element2->value.print("%3.0f",angle);
        element2->units="degrees";
        #ifdef CIMAGEDATAWRITER_DEBUG 
        CDBDebug("pushed wind dir %f for step %d [%d]", angle, step, getFeatureInfoResult->elements.size());
        #endif
        element2->time.copy(&dataSources[d]->timeSteps[dataSources[d]->getCurrentTimeStep()]->timeString);
        
        double windspeed=hypot(pixel1, pixel2);
        GetFeatureInfoResult::Element *windspeedOrigElement=new GetFeatureInfoResult::Element();
        windspeedOrigElement->dataSource= dataSource;
        windspeedOrigElement->time.copy(&dataSources[d]->timeSteps[dataSources[d]->getCurrentTimeStep()]->timeString);
        for(size_t j=0;j<dataSources[d]->requiredDims.size();j++){
          value=cdfDims->getDimensionValue(j);
          name=cdfDims->getDimensionName(j);
          if(name.indexOf("time")==0){
            value=windspeedOrigElement->time.c_str();
          }
          windspeedOrigElement->cdfDims.addDimension(name.c_str(),value.c_str(),cdfDims->getDimensionIndex(j));
        }
        getFeatureInfoResult->elements.push_back(windspeedOrigElement);
        windspeedOrigElement->long_name="wind speed";
        windspeedOrigElement->var_name="wind speed";
        windspeedOrigElement->standard_name="speed1";
        windspeedOrigElement->feature_name="wind speed";
        windspeedOrigElement->value.print("%3.1f",windspeed);
        windspeedOrigElement->units=dataSource->dataObject[0]->units;
        windspeedOrigElement->time.copy(&dataSources[d]->timeSteps[dataSources[d]->getCurrentTimeStep()]->timeString);
        #ifdef CIMAGEDATAWRITER_DEBUG 
        CDBDebug("pushed wind speed %f for step %d [%d]", windspeed, step, getFeatureInfoResult->elements.size());
        #endif

        //Skip KTS calculation if input data is not u and v vectors in m/s.
        bool skipKTSCalc = true;
        try{
          if(dataSource->dataObject[0]->units.indexOf("m/s")>=0){
            skipKTSCalc =false;
          }
        }catch(int e){}
      
        if(!skipKTSCalc){
          GetFeatureInfoResult::Element *element3=new GetFeatureInfoResult::Element();
          element3->dataSource= dataSource;
          element3->time.copy(&dataSources[d]->timeSteps[dataSources[d]->getCurrentTimeStep()]->timeString);
          CCDFDims * cdfDims = dataSources[d]->getCDFDims();CT::string value,name;
          for(size_t j=0;j<dataSources[d]->requiredDims.size();j++){
            value=cdfDims->getDimensionValue(j);
            name=cdfDims->getDimensionName(j);
            if(name.indexOf("time")==0){
              value=windspeedOrigElement->time.c_str();
            }
            element3->cdfDims.addDimension(name.c_str(),value.c_str(),cdfDims->getDimensionIndex(j));
          }
          getFeatureInfoResult->elements.push_back(element3);
          double windspeedKTS=windspeed*(3600./1852.);
          element3->long_name="wind speed";
          element3->var_name="wind speed";
          element3->standard_name="speed2";
          element3->feature_name="wind speed kts";
          element3->value.print("%3.1f",windspeedKTS);
          element3->units="kts";
          element3->time.copy(&dataSources[d]->timeSteps[dataSources[d]->getCurrentTimeStep()]->timeString);
          #ifdef CIMAGEDATAWRITER_DEBUG 
          CDBDebug("pushed wind speed KTS %f for step %d [%d]\n", windspeedKTS, step, getFeatureInfoResult->elements.size());
          #endif
        }
      }
    }
  }
  #ifdef CIMAGEDATAWRITER_DEBUG 
  CDBDebug("[/getFeatureInfo %d]",getFeatureInfoResultList.size());
  #endif
  return 0;
}


int CImageDataWriter::createAnimation(){
  printf("%s%c%c\n","Content-Type:image/gif",13,10);
  drawImage.beginAnimation();
  animation = 1;
  return 0;
}

void CImageDataWriter::setDate(const char *szTemp){
  drawImage.setTextStroke(szTemp, strlen(szTemp),drawImage.Geo->dWidth-170,5,240,254,0);
}

int CImageDataWriter::warpImage(CDataSource *dataSource,CDrawImage *drawImage){
  //Open the data of this dataSource
  #ifdef CIMAGEDATAWRITER_DEBUG  
  CDBDebug("opening %s",dataSource->getFileName());
  #endif  
  CDataReader reader;
  status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_ALL);
  #ifdef CIMAGEDATAWRITER_DEBUG  
  CDBDebug("Has opened %s",dataSource->getFileName());
  #endif    
  if(status!=0){
    CDBError("Could not open file: %s",dataSource->getFileName());
    return 1;
  }
  #ifdef CIMAGEDATAWRITER_DEBUG  
  CDBDebug("opened");
  #endif  
  //Initialize projection algorithm
  #ifdef CIMAGEDATAWRITER_DEBUG  
  CDBDebug("initreproj %s",dataSource->nativeProj4.c_str());
  #endif
  status = imageWarper.initreproj(dataSource,drawImage->Geo,&srvParam->cfg->Projection);
  if(status!=0){
    CDBError("initreproj failed");
    reader.close();
    return 1;
  }
  
#ifdef MEASURETIME
  StopWatch_Stop("warp start");
#endif

/*if(renderMethod==nearest){CDBDebug("nearest");}
if(renderMethod==bilinear){CDBDebug("bilinear");}
if(renderMethod==bilinearcontour){CDBDebug("bilinearcontour");}
if(renderMethod==nearestcontour){CDBDebug("nearestcontour");}
if(renderMethod==contour){CDBDebug("contour");}*/

  CImageWarperRenderInterface *imageWarperRenderer;
  RenderMethod renderMethod = currentStyleConfiguration->renderMethod;
  /**
  * Use fast nearest neighbourrenderer
  */
  if(renderMethod&RM_NEAREST){
    imageWarperRenderer = new CImgWarpNearestNeighbour();
    imageWarperRenderer->render(&imageWarper,dataSource,drawImage);
    delete imageWarperRenderer;
  }
  
  /**
  * Use RGBA renderer
  */
    if(renderMethod&RM_RGBA){
    imageWarperRenderer = new CImgWarpNearestRGBA();
    imageWarperRenderer->render(&imageWarper,dataSource,drawImage);
    delete imageWarperRenderer;
  }
  
  /**
  * Use bilinear renderer
  */
  /*if(renderMethod==nearestcontour||
    renderMethod==bilinear||
    renderMethod==bilinearcontour||
    renderMethod==contour||
    renderMethod==shaded||
    renderMethod==shadedcontour||
    renderMethod==vector||renderMethod==vectorshaded||renderMethod==vectorcontour||renderMethod==vectorcontourshaded||
    renderMethod==barb||renderMethod==barbshaded||renderMethod==barbcontour||renderMethod==barbcontourshaded||
    renderMethod==thinvector||renderMethod==thinvectorshaded||renderMethod==thinvectorcontour||renderMethod==thinvectorcontourshaded||
    renderMethod==thinbarb||renderMethod==thinbarbshaded||renderMethod==thinbarbcontour||renderMethod==thinbarbcontourshaded
    )*/
  if(renderMethod&RM_CONTOUR||renderMethod&RM_BILINEAR||renderMethod&RM_SHADED||renderMethod&RM_VECTOR||renderMethod&RM_BARB||renderMethod&RM_THIN)  {
    if(dataSource->dataObject[0]->points.size()==0){
      imageWarperRenderer = new CImgWarpBilinear();
      CT::string bilinearSettings;
      bool drawMap=false;
      bool drawContour=false;
      bool drawVector=false;
      bool drawBarb=false;
      bool drawShaded=false;
      bool drawGridVectors=false;
      
      if(renderMethod&RM_BILINEAR)drawMap=true;
      if(renderMethod&RM_CONTOUR)drawContour=true;
      if(renderMethod&RM_VECTOR)drawVector=true;
      if(renderMethod&RM_SHADED)drawShaded=true;
      if(renderMethod&RM_BARB)drawBarb=true;
      if(renderMethod&RM_THIN)drawGridVectors=true;
      
      
      
      
      /*if(renderMethod==bilinear||renderMethod==bilinearcontour)drawMap=true;
      if(renderMethod==bilinearcontour)drawContour=true;
      if(renderMethod==nearestcontour)drawContour=true;
      if(renderMethod==contour||renderMethod==shadedcontour||renderMethod==vectorcontour||renderMethod==vectorcontourshaded)drawContour=true;
      if(renderMethod==vector||renderMethod==vectorcontour||renderMethod==vectorshaded||renderMethod==vectorcontourshaded)drawVector=true;
      if(renderMethod==thinvector||renderMethod==thinvectorcontour||renderMethod==thinvectorshaded||renderMethod==thinvectorcontourshaded){ drawVector=true;drawGridVectors=true;}
      if(renderMethod==thinbarb||renderMethod==thinbarbcontour||renderMethod==thinbarbshaded||renderMethod==thinbarbcontourshaded) {drawBarb=true; drawGridVectors=true;}
      if(renderMethod==thinbarbcontour||renderMethod==thinbarbcontourshaded) {drawContour=true;}
      if(renderMethod==shaded||renderMethod==shadedcontour||renderMethod==vectorcontourshaded||renderMethod==barbcontourshaded)drawShaded=true;
      if(renderMethod==vectorshaded||renderMethod==thinvectorshaded||renderMethod==thinvectorcontourshaded)drawShaded=true;
      if(renderMethod==barbshaded||renderMethod==thinbarbshaded||renderMethod==thinbarbcontourshaded)drawShaded=true;
      if(renderMethod==barbcontour) { drawContour=true; drawBarb=true; }
      if(renderMethod==vectorcontour) { drawContour=true; drawVector=true; }
      if(renderMethod==vectorcontourshaded||renderMethod==thinvectorcontourshaded) { drawShaded=true; drawContour=true; drawVector=true; }
      if(renderMethod==vectorcontour||renderMethod==thinvectorcontour) { drawVector=true; drawContour=true;}
      if(renderMethod==barbcontourshaded||renderMethod==thinbarbcontourshaded) { drawShaded=true; drawContour=true; drawBarb=true; }
      if((renderMethod==barb)||(renderMethod==barbshaded)) drawBarb=true;*/
      
      
      if(drawMap==true)bilinearSettings.printconcat("drawMap=true;");
      if(drawVector==true)bilinearSettings.printconcat("drawVector=true;");
      if(drawBarb==true)bilinearSettings.printconcat("drawBarb=true;");
      if(drawShaded==true)bilinearSettings.printconcat("drawShaded=true;");
      if(drawContour==true)bilinearSettings.printconcat("drawContour=true;");
      if (drawGridVectors)bilinearSettings.printconcat("drawGridVectors=true;");
      bilinearSettings.printconcat("smoothingFilter=%d;",currentStyleConfiguration->smoothingFilter);
      if(drawShaded==true){
        bilinearSettings.printconcat("shadeInterval=%0.12f;contourBigInterval=%0.12f;contourSmallInterval=%0.12f;",
                                    currentStyleConfiguration->shadeInterval,currentStyleConfiguration->contourIntervalH,currentStyleConfiguration->contourIntervalL);
        
        if(currentStyleConfiguration->shadeIntervals!=NULL){
          for(size_t j=0;j<currentStyleConfiguration->shadeIntervals->size();j++){
            CServerConfig::XMLE_ShadeInterval *shadeInterval=((*currentStyleConfiguration->shadeIntervals)[j]);
            if(shadeInterval->attr.min.c_str()!=NULL&&shadeInterval->attr.max.c_str()!=NULL){
              bilinearSettings.printconcat("shading=min(%s)$max(%s)$",shadeInterval->attr.min.c_str(),shadeInterval->attr.max.c_str());
              if(shadeInterval->attr.fillcolor.c_str()!=NULL){bilinearSettings.printconcat("$fillcolor(%s)$",shadeInterval->attr.fillcolor.c_str());}
              bilinearSettings.printconcat(";");
            }
          }
        }
      }
      if(drawContour==true){
        
        if(currentStyleConfiguration->contourLines!=NULL){
          for(size_t j=0;j<currentStyleConfiguration->contourLines->size();j++){
            CServerConfig::XMLE_ContourLine * contourLine=((*currentStyleConfiguration->contourLines)[j]);
            //Check if we have a interval contour line or a contourline with separate classes
            if(contourLine->attr.interval.c_str()!=NULL){
              //ContourLine interval
              bilinearSettings.printconcat("contourline=");
              if(contourLine->attr.width.c_str()!=NULL){bilinearSettings.printconcat("width(%s)$",contourLine->attr.width.c_str());}
              if(contourLine->attr.linecolor.c_str()!=NULL){bilinearSettings.printconcat("linecolor(%s)$",contourLine->attr.linecolor.c_str());}
              if(contourLine->attr.textcolor.c_str()!=NULL){bilinearSettings.printconcat("textcolor(%s)$",contourLine->attr.textcolor.c_str());}
              if(contourLine->attr.interval.c_str()!=NULL){bilinearSettings.printconcat("interval(%s)$",contourLine->attr.interval.c_str());}
              if(contourLine->attr.textformatting.c_str()!=NULL){bilinearSettings.printconcat("textformatting(%s)$",contourLine->attr.textformatting.c_str());}
              bilinearSettings.printconcat(";");
            }
            if(contourLine->attr.classes.c_str()!=NULL){
              //ContourLine classes
              bilinearSettings.printconcat("contourline=");
              if(contourLine->attr.width.c_str()!=NULL){bilinearSettings.printconcat("width(%s)$",contourLine->attr.width.c_str());}
              if(contourLine->attr.linecolor.c_str()!=NULL){bilinearSettings.printconcat("linecolor(%s)$",contourLine->attr.linecolor.c_str());}
              if(contourLine->attr.textcolor.c_str()!=NULL){bilinearSettings.printconcat("textcolor(%s)$",contourLine->attr.textcolor.c_str());}
              if(contourLine->attr.classes.c_str()!=NULL){bilinearSettings.printconcat("classes(%s)$",contourLine->attr.classes.c_str());}
              if(contourLine->attr.textformatting.c_str()!=NULL){bilinearSettings.printconcat("textformatting(%s)$",contourLine->attr.textformatting.c_str());}
              bilinearSettings.printconcat(";");
            }
            
          }
          //bilinearSettings.printconcat("%s","contourline=width(3.5)$color(#0000FF)$interval(6)$textformat(%f);");
          //bilinearSettings.printconcat("%s","contourline=width(1.0)$color(#00FF00)classes(5.5,7.7)$textformat(%2.1f);");
        }
        
        //bilinearSettings.printconcat("textScaleFactor=%f;textOffsetFactor=%f;",textScaleFactor,textOffsetFactor);
      }
      //CDBDebug("bilinearSettings.c_str() %s",bilinearSettings.c_str());
      imageWarperRenderer->set(bilinearSettings.c_str());
      imageWarperRenderer->render(&imageWarper,dataSource,drawImage);
      delete imageWarperRenderer;
    }
  }
  
  /**
  * Use point renderer
  */
  //if(renderMethod==barb||renderMethod==vector||renderMethod==point){
  if(renderMethod&RM_BARB||renderMethod&RM_VECTOR||renderMethod&RM_POINT||renderMethod==RM_NEAREST){
    if(dataSource->dataObject[0]->points.size()!=0){
      //CDBDebug("CImgRenderPoints()");
      
      imageWarperRenderer = new CImgRenderPoints();
      CT::string renderMethodAsString;
      getRenderMethodAsString(&renderMethodAsString,renderMethod);
      imageWarperRenderer->set(renderMethodAsString.c_str());
      imageWarperRenderer->render(&imageWarper,dataSource,drawImage);
      delete imageWarperRenderer;
    }
  }
  
#ifdef MEASURETIME
  StopWatch_Stop("warp finished");
#endif
  imageWarper.closereproj();
  reader.close();
  return 0;
}

// Virtual functions
int CImageDataWriter::calculateData(std::vector <CDataSource*>&dataSources){
  /**
  This style has a special *custom* non WMS syntax:
  First style: represents how the boolean results must be combined
  Keywords: "and","or"
  Example: "and" for two layers, "and_and" for three layers
  Second and N+2 style: represents how the boolean map is created and which time is required
  Keywords: "and","between","notbetween","lessthan","greaterthan","time","|"
  Example: between_10.0_and_20.0|time_1990-01-01T00:00:00Z
  Note: after "|" always a time is specified with time_
  */

//  int status;

  CDBDebug("calculateData");

  if(animation==1&&nrImagesAdded>1){
    drawImage.addImage(30);
  }
  nrImagesAdded++;
  // draw the Image
  //for(size_t j=1;j<dataSources.size();j++)
  {
    CDataSource *dataSource;
    
    
    /**************************************************/
    int status;
    bool hasFailed=false;
    //Open the corresponding data of this dataSource, with the datareader
    std::vector <CDataReader*> dataReaders;
    for(size_t i=0;i<dataSources.size();i++){
      dataSource=dataSources[i];
      CDataReader *reader = new CDataReader ();
      dataReaders.push_back(reader);
      status = reader->open(dataSource,CNETCDFREADER_MODE_OPEN_ALL);
      CDBDebug("Opening %s",dataSource->getFileName());
      if(status!=0){CDBError("Could not open file: %s",dataSource->getFileName());  hasFailed=true; }
    }
    //Initialize projection algorithm
    dataSource=dataSources[0];
    
    if(hasFailed==false){
      #ifdef CIMAGEDATAWRITER_DEBUG  
      CDBDebug("initreproj %s",dataSource->nativeProj4.c_str());
      #endif
      status = imageWarper.initreproj(dataSource,drawImage.Geo,&srvParam->cfg->Projection);
      if(status!=0){
        CDBError("initreproj failed");
        hasFailed=true;
      }
    }
    if(hasFailed==false){
      //Start modifying the data using the specific style
      
      enum ConditionalOperator{ myand,myor,between,notbetween,lessthan,greaterthan};
      
      ConditionalOperator combineBooleanMapExpression[dataSources.size()-1];
      ConditionalOperator inputMapExpression[dataSources.size()];
      float inputMapExprValuesLow[dataSources.size()];
      float inputMapExprValuesHigh[dataSources.size()];
      
      CT::string *layerStyles = srvParam->Styles.splitToArray(",");
      CT::string style;
//      bool errorOccured=false;
      for(size_t j=0;j<dataSources.size();j++){
        size_t numberOfValues = 1;
        CT::string *_style = layerStyles[j].splitToArray("|");
        style.copy(&_style[0]);
        CDBDebug("STYLE == %s",style.c_str());
        if(j==0){
          //Find the conditional expression for the first layer (the boolean map)
          CT::string *conditionals = style.splitToArray("_");
          if(!conditionals[0].equals("default")&&conditionals->count!=dataSources.size()-2){
            CDBError("Incorrect number of conditional operators specified: %d  (expected %d)",
                    conditionals->count,dataSources.size()-2);
            hasFailed=true;
          }else{
            for(size_t i=0;i<conditionals->count;i++){
              combineBooleanMapExpression[i]=myand;
              if(conditionals[i].equals("and"))combineBooleanMapExpression[i]=myand;
              if(conditionals[i].equals("or"))combineBooleanMapExpression[i]=myor;
            }
            
          }
          delete[] conditionals;
        }else{
          inputMapExpression[j]=between;
          CT::string exprVal("0.0");
          //Find the expressin types:
          if(style.indexOf("between_")==0){
            inputMapExpression[j]=between;
            exprVal.copy(style.c_str()+8);
            numberOfValues=2;
          }
          if(style.indexOf("notbetween_")==0){
            inputMapExpression[j]=notbetween;
            exprVal.copy(style.c_str()+11);
            numberOfValues=2;
          }
          if(style.indexOf("lessthan_")==0){
            inputMapExpression[j]=lessthan;
            exprVal.copy(style.c_str()+9);
            numberOfValues=1;
          }
          if(style.indexOf("greaterthan_")==0){
            inputMapExpression[j]=greaterthan;
            exprVal.copy(style.c_str()+12);
            numberOfValues=1;
          }
          CT::string *LH=exprVal.splitToArray("_and_");
          if(LH->count!=numberOfValues){
            CDBError("Invalid number of values in expression '%s'",style.c_str());
            hasFailed=true;
          }else{
            inputMapExprValuesLow[j]=LH[0].toFloat();
            if(numberOfValues==2){
              inputMapExprValuesHigh[j]=LH[1].toFloat();
            }
          }
          delete[] LH;
          if(numberOfValues==1){
            CDBDebug("'%f'",inputMapExprValuesLow[j]);
          }
          if(numberOfValues==2){
            CDBDebug("'%f' and '%f'",inputMapExprValuesLow[j],inputMapExprValuesHigh[j]);
          }
        }
        delete [] _style;
      }
      
      CDBDebug("Start creating the boolean map");
      double pixel[dataSources.size()];
      bool conditialMap[dataSources.size()];
      for(int y=0;y<dataSource->dHeight;y++){
        for(int x=0;x<dataSource->dWidth;x++){
          size_t ptr=x+y*dataSource->dWidth;
          for(size_t j=1;j<dataSources.size();j++){
            CDataSource *dsj = dataSources[j];
            int xj=int((float(x)/float(dataSource->dWidth))*float(dsj->dWidth));
            int yj=int((float(y)/float(dataSource->dHeight))*float(dsj->dHeight));
            if(dsj->dfBBOX[1]>dsj->dfBBOX[3])yj=dsj->dHeight-yj-1;
            size_t ptrj=xj+yj*dsj->dWidth;
            
            pixel[j] = convertValue(dsj->dataObject[0]->cdfVariable->type,dsj->dataObject[0]->cdfVariable->data,ptrj);
            
            if(inputMapExpression[j]==between){
              if(pixel[j]>=inputMapExprValuesLow[j]&&pixel[j]<=inputMapExprValuesHigh[j])
                conditialMap[j]=true;else conditialMap[j]=false;
            }
            if(inputMapExpression[j]==notbetween){
              if(pixel[j]<inputMapExprValuesLow[j]||pixel[j]>inputMapExprValuesHigh[j])
                conditialMap[j]=true;else conditialMap[j]=false;
            }
            if(inputMapExpression[j]==lessthan){
              if(pixel[j]<inputMapExprValuesLow[j])
                conditialMap[j]=true;else conditialMap[j]=false;
            }
            if(inputMapExpression[j]==greaterthan){
              if(pixel[j]>inputMapExprValuesLow[j])
                conditialMap[j]=true;else conditialMap[j]=false;
            }
          }
          bool result = conditialMap[1];
          for(size_t j=2;j<dataSources.size();j++){
            if(combineBooleanMapExpression[j-2]==myand){
              if(result==true&&conditialMap[j]==true)result=true;else result=false;
            }
            if(combineBooleanMapExpression[j-2]==myor){
              if(result==true||conditialMap[j]==true)result=true;else result=false;
            }
          }
          if(result==true)pixel[0]=1;else pixel[0]=0;
          setValue(dataSources[0]->dataObject[0]->cdfVariable->type,dataSources[0]->dataObject[0]->cdfVariable->data,ptr,pixel[0]);
        }
      }
      CDBDebug("Warping with style %s",srvParam->Styles.c_str());
      CImageWarperRenderInterface *imageWarperRenderer;
      imageWarperRenderer = new CImgWarpNearestNeighbour();
      imageWarperRenderer->render(&imageWarper,dataSource,&drawImage);
      delete imageWarperRenderer;
      imageWarper.closereproj();
      delete [] layerStyles;
    }
    for(size_t j=0;j<dataReaders.size();j++){
      if(dataReaders[j]!=NULL){
        dataReaders[j]->close();
        delete dataReaders[j];
        dataReaders[j]=NULL;
      }
    }
    if(hasFailed==true)return 1;
    return 0;
    /**************************************************/
    
    if(status != 0)return status;
    
    if(status == 0){
      
      
      if(dataSource->cfgLayer->ImageText.size()>0){
        if(dataSource->cfgLayer->ImageText[0]->value.c_str()!=NULL){
          size_t len=strlen(dataSource->cfgLayer->ImageText[0]->value.c_str());
          drawImage.setTextStroke(dataSource->cfgLayer->ImageText[0]->value.c_str(),
                                  len,
                                  int(drawImage.Geo->dWidth/2-len*3),
                                  drawImage.Geo->dHeight-16,240,254,-1);
        }
      }
    }
  
  }
  return 0;
}
int CImageDataWriter::addData(std::vector <CDataSource*>&dataSources){
#ifdef CIMAGEDATAWRITER_DEBUG  
  CDBDebug("addData");
#endif   
  int status = 0;
  
  if(animation==1&&nrImagesAdded>0){
    drawImage.addImage(25);
  }
  //CDBDebug("Draw Data");
  nrImagesAdded++;
  // draw the Image
  //drawCascadedWMS("http://geoservices.knmi.nl/cgi-bin/restricted/MODIS_Netherlands.cgi?","modis_250m_netherlands_8bit",true);
#ifdef CIMAGEDATAWRITER_DEBUG    
  CDBDebug("Draw data. dataSources.size() =  %d",dataSources.size());
#endif  
  
  for(size_t j=0;j<dataSources.size();j++){
    CDataSource *dataSource=dataSources[j];

    
    
        
    if(dataSource->dLayerType==CConfigReaderLayerTypeCascaded){
      //CDBDebug("Drawing cascaded WMS (grid/logo/external");
      if(dataSource->cfgLayer->WMSLayer.size()==1){
        status = drawCascadedWMS(dataSource,dataSource->cfgLayer->WMSLayer[0]->attr.service.c_str(),dataSource->cfgLayer->WMSLayer[0]->attr.layer.c_str(),true);
        if(status!=0){
          CDBError("drawCascadedWMS for layer %s failed",dataSource->layerName.c_str());
        }
      }
    }
    
    
  
    /*drawImage.line(0,0,200,200,0.1,248);
    drawImage.line(200,200,500,201,0.1,248);
    
    for(float ty=10;ty<60;ty++){
      drawImage.line(50+ty*20,20+ty/2,70+ty*20,20+ty/2,0.1,248);
    }*/
  
      
    if(dataSource->dLayerType!=CConfigReaderLayerTypeCascaded){
      #ifdef CIMAGEDATAWRITER_DEBUG    
      CDBDebug("Drawingnormal legend");
      #endif
      if(j!=0){
        /*
        * Reinitialize legend for other type of legends, if possible (in true color mode it is always the case
        * For j==0, the legend is already initialized previously
        */
        #ifdef CIMAGEDATAWRITER_DEBUG
        CDBDebug("REINITLEGEND");
        #endif

        if(initializeLegend(srvParam,dataSource)!=0)return 1;
        
        status = drawImage.createGDPalette(srvParam->cfg->Legend[currentStyleConfiguration->legendIndex]);
        if(status != 0){
          CDBError("Unknown palette type for %s",srvParam->cfg->Legend[currentStyleConfiguration->legendIndex]->attr.name.c_str());
          return 1;
        }
        
        
      }
      
#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("Start warping");
#endif
      status = warpImage(dataSource,&drawImage);
#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("Finished warping");
#endif      
      if(status != 0){
        CDBError("warpImage for layer %s failed",dataSource->layerName.c_str());
        return status;
      }
      
      if(j==dataSources.size()-1){
        if(status == 0){
          
          if(dataSource->cfgLayer->ImageText.size()>0){
          
            CT::string imageText = "";
            if(dataSource->cfgLayer->ImageText[0]->value.c_str()!=NULL){
              imageText.copy(dataSource->cfgLayer->ImageText[0]->value.c_str());
            }
          
            //Determine ImageText based on configured netcdf attribute
            const char *attrToSearch=dataSource->cfgLayer->ImageText[0]->attr.attribute.c_str();
            if(attrToSearch!=NULL){
              //CDBDebug("Determining ImageText based on netcdf attribute %s",attrToSearch);
              try{
                CDF::Attribute *attr=dataSource->dataObject[0]->cdfObject->getAttribute(attrToSearch);
                if(attr->length>0){
                  imageText.copy(attrToSearch);
                  imageText.concat(": ");
                  imageText.concat(attr->toString().c_str());
                }
              }catch(int e){
              }
            }
            
            if(imageText.length()>0){
              size_t len=imageText.length();
              //CDBDebug("Watermark: %s",imageText.c_str());
              drawImage.setTextStroke(imageText.c_str(),len,int(drawImage.Geo->dWidth/2-len*3),drawImage.Geo->dHeight-16,240,254,-1);
            }
          }
        }
      }
    }
    
    //draw a grid in lat/lon coordinates.
    if(dataSource->cfgLayer->Grid.size()==1){
      double gridSize=10;
      double precision=0.25;
      double numTestSteps = 5;
      CColor textColor(0,0,0,128);
      float lineWidth=0.25;
      int lineColor= 247;
      
      if(dataSource->cfgLayer->Grid[0]->attr.resolution.c_str()!=NULL){
        gridSize = parseFloat(dataSource->cfgLayer->Grid[0]->attr.resolution.c_str());
      }
      precision=gridSize/10;
      if(dataSource->cfgLayer->Grid[0]->attr.precision.c_str()!=NULL){
        precision = parseFloat(dataSource->cfgLayer->Grid[0]->attr.precision.c_str());
      }
      
      bool useProjection = true;
      
      if(srvParam->Geo->CRS.equals("EPSG:4326")){
        //CDBDebug("Not using projection");
        useProjection = false;
      }
      
      if(useProjection){
        #ifdef CIMAGEDATAWRITER_DEBUG  
        CDBDebug("initreproj latlon");
        #endif
        int status = imageWarper.initreproj(LATLONPROJECTION,drawImage.Geo,&srvParam->cfg->Projection);
        if(status!=0){CDBError("initreproj failed");return 1;  }
      }
      
      CPoint topLeft;
      CBBOX latLonBBOX;
      //Find lat lon BBox;
      topLeft.x    =srvParam->Geo->dfBBOX[0];
      
      topLeft.y    =srvParam->Geo->dfBBOX[1];
      if(useProjection){
        imageWarper.reprojpoint(topLeft);
      }
      
      
      
      latLonBBOX.left=topLeft.x;
      latLonBBOX.right=topLeft.x;
      latLonBBOX.top=topLeft.y;
      latLonBBOX.bottom=topLeft.y;
      
    
      double numStepsX=(srvParam->Geo->dfBBOX[2]-srvParam->Geo->dfBBOX[0])/numTestSteps;
      double numStepsY=(srvParam->Geo->dfBBOX[3]-srvParam->Geo->dfBBOX[1])/numTestSteps;
      #ifdef CIMAGEDATAWRITER_DEBUG    
      CDBDebug("dfBBOX: %f, %f, %f, %f",srvParam->Geo->dfBBOX[0],srvParam->Geo->dfBBOX[1],srvParam->Geo->dfBBOX[2],srvParam->Geo->dfBBOX[3]);
      #endif
      for(double y=srvParam->Geo->dfBBOX[1];y<srvParam->Geo->dfBBOX[3]+numStepsY;y=y+numStepsY){
        for(double x=srvParam->Geo->dfBBOX[0];x<srvParam->Geo->dfBBOX[2]+numStepsX;x=x+numStepsX){
          #ifdef CIMAGEDATAWRITER_DEBUG    
          CDBDebug("xy: %f, %f",x,y);
          #endif
          topLeft.x=x;
          topLeft.y=y;
          if(useProjection){
            imageWarper.reprojpoint(topLeft);
          }
          if(topLeft.x<latLonBBOX.left)latLonBBOX.left=topLeft.x;
          if(topLeft.x>latLonBBOX.right)latLonBBOX.right=topLeft.x;
          if(topLeft.y<latLonBBOX.top)latLonBBOX.top=topLeft.y;
          if(topLeft.y>latLonBBOX.bottom)latLonBBOX.bottom=topLeft.y;
        }
      }
      
      #ifdef CIMAGEDATAWRITER_DEBUG    
      CDBDebug("SIZE: %f, %f, %f, %f",latLonBBOX.left,latLonBBOX.right,latLonBBOX.top,latLonBBOX.bottom);
      #endif
      
    
      latLonBBOX.left=double(int(latLonBBOX.left/gridSize))*gridSize-gridSize;
      latLonBBOX.right=double(int(latLonBBOX.right/gridSize))*gridSize+gridSize;
      latLonBBOX.top=double(int(latLonBBOX.top/gridSize))*gridSize-gridSize;
      latLonBBOX.bottom=double(int(latLonBBOX.bottom/gridSize))*gridSize+gridSize;
      
      int numPointsX = int((latLonBBOX.right - latLonBBOX.left)/precision);
      int numPointsY = int((latLonBBOX.bottom - latLonBBOX.top)/precision);
      numPointsX++;
      numPointsY++;
      
      size_t numPoints = numPointsX*numPointsY;
      
      #ifdef CIMAGEDATAWRITER_DEBUG    
      CDBDebug("numPointsX = %d, numPointsY = %d",numPointsX,numPointsY);
      #endif
      
      CPoint *gridP = new CPoint[numPoints];
  
      for(int y=0;y<numPointsY;y++){
        for(int x=0;x<numPointsX;x++){
          double gx=latLonBBOX.left+precision*double(x);
          double gy=latLonBBOX.top+precision*double(y);
          size_t p=x+y*numPointsX;
          gridP[p].x=gx;
          gridP[p].y=gy;
          if(useProjection){
            imageWarper.reprojpoint_inv(gridP[p]);
          }
          CoordinatesXYtoScreenXY(gridP[p],srvParam->Geo);
          
        }
      }
      
      #ifdef CIMAGEDATAWRITER_DEBUG    
      CDBDebug("Drawing horizontal lines");
      #endif

      bool drawText = false;
      const char *fontLoc = NULL;
      float fontSize = 6.0;
      if(srvParam->cfg->WMS[0]->GridFont.size()==1){
        
        fontLoc = srvParam->cfg->WMS[0]->GridFont[0]->attr.location.c_str();
        fontSize = parseFloat(srvParam->cfg->WMS[0]->GridFont[0]->attr.size.c_str());
        drawText = true;
      }
      
      int s=int(gridSize/precision);
      if(s<=0)s=1;
      CT::string message;
      for(int y=0;y<numPointsY;y=y+s){
        bool drawnTextLeft = false;
        bool drawnTextRight = false;
        for(int x=0;x<numPointsX-1;x++){
          size_t p=x+y*numPointsX;
          if(p<numPoints){
            drawImage.line(gridP[p].x,gridP[p].y,gridP[p+1].x,gridP[p+1].y,lineWidth,lineColor);
            if(drawnTextRight==false){
              if(gridP[p].x>srvParam->Geo->dWidth&&gridP[p].y>0){
                drawnTextRight=true;
                double gy=latLonBBOX.top+precision*double(y);
                message.print("%2.1f",gy);
                int ty=int(gridP[p].y);
                int tx=int(gridP[p].x);if(ty<8){ty=8;}if(tx>srvParam->Geo->dWidth-30)tx=srvParam->Geo->dWidth-1;
                tx-=17;
                
                if(drawText)drawImage.drawText(tx,ty-2,fontLoc,fontSize,0,message.c_str(),textColor);
              }
            }
            if(drawnTextLeft==false){
              if(gridP[p].x>0&&gridP[p].y>0){
                drawnTextLeft=true;
                double gy=latLonBBOX.top+precision*double(y);
                message.print("%2.1f",gy);
                int ty=int(gridP[p].y);
                int tx=int(gridP[p].x);if(ty<8){ty=0;}if(tx<15)tx=0;tx+=2;
                if(drawText)drawImage.drawText(tx,ty-2,fontLoc,fontSize,0,message.c_str(),textColor);
              }
            }
          }
        }
      }
      
      
      #ifdef CIMAGEDATAWRITER_DEBUG    
      CDBDebug("Drawing vertical lines");
      #endif        
      for(int x=0;x<numPointsX;x=x+s){
        bool drawnTextTop = false;
        bool drawnTextBottom = false;
        for(int y=numPointsY-2;y>=0;y--){
          size_t p=x+y*numPointsX;
          if(p<numPoints){
            drawImage.line(gridP[p].x,gridP[p].y,gridP[p+numPointsX].x,gridP[p+numPointsX].y,lineWidth,lineColor);
            
            if(drawnTextBottom==false){
              if(gridP[p].x>0&&gridP[p].y>srvParam->Geo->dHeight){
                drawnTextBottom=true;
                double gx=latLonBBOX.left+precision*double(x);
                message.print("%2.1f",gx);
                int ty=int(gridP[p].y);if(ty<15)ty=0;
                if(ty>srvParam->Geo->dHeight){
                  ty=srvParam->Geo->dHeight;
                }
                ty-=2;
                int tx=int((gridP[p]).x+2);
                if(drawText)drawImage.drawText(tx,ty,fontLoc,fontSize,0,message.c_str(),textColor);
              }
            }    
            
            if(drawnTextTop==false){
              if(gridP[p].x>0&&gridP[p].y>0){
                drawnTextTop=true;
                double gx=latLonBBOX.left+precision*double(x);
                message.print("%2.1f",gx);
                int ty=int(gridP[p].y);if(ty<15)ty=0;ty+=7;
                int tx=int(gridP[p].x)+2;//if(tx<8){tx=8;ty+=4;}if(ty<15)tx=1;
                if(drawText)drawImage.drawText(tx,ty,fontLoc,fontSize,0,message.c_str(),textColor);
              }
            }
          }
        }
      }

      #ifdef CIMAGEDATAWRITER_DEBUG    
      CDBDebug("Delete gridp");
      #endif
      
      delete[] gridP;
    }
  }
  
  //drawCascadedWMS("http://bhlbontw.knmi.nl/rcc/download/ensembles/cgi-bin/basemaps.cgi?","country_lines",true);
  return status;
}

CColor getColorForPlot(int plotNr,int nrOfPlots){

  CColor color = CColor(255,255,255,255);
  if(nrOfPlots<6){
    if(plotNr==0){color=CColor(0,0,255,255);}
    if(plotNr==1){color=CColor(0,255,0,255);}
    if(plotNr==2){color=CColor(255,0,0,255);}
    if(plotNr==3){color=CColor(255,128,0,255);}
    if(plotNr==4){color=CColor(0,255,128,255);}
    if(plotNr==5){color=CColor(255,0,128,255);}
    if(plotNr==6){color=CColor(0,0,128,255);}
    if(plotNr==7){color=CColor(128,0,0,255);}
    if(plotNr==8){color=CColor(0,128,0,255);}
    if(plotNr==9){color=CColor(0,128,0,255);}
    if(plotNr==10){color=CColor(0,128,128,255);}
    if(plotNr==11){color=CColor(128,128,0,255);}
  }else{
    color=CColor(0,255,0,255);
  }
  return color;
}


int CImageDataWriter::getTextForValue(CT::string *tv,float v,StyleConfiguration *currentStyleConfiguration){
  
  int textRounding=0;
  if(currentStyleConfiguration==NULL){
    return 1;
  }
  float legendInterval=currentStyleConfiguration->shadeInterval;
  if(legendInterval!=0){
    float fracPart=legendInterval-int(legendInterval);
    textRounding=-int(log10(fracPart)-0.9999999f);
  }
  if(textRounding<=0)tv->print("%2.0f",v);
  if(textRounding==1)tv->print("%2.1f",v);
  if(textRounding==2)tv->print("%2.2f",v);
  if(textRounding==3)tv->print("%2.3",v);
  if(textRounding==4)tv->print("%2.4f",v);
  if(textRounding==5)tv->print("%2.5f",v);
  if(textRounding==5)tv->print("%2.6f",v);
  if(textRounding>6)tv->print("%f",v);
  return 0;
}

int CImageDataWriter::end(){
  #ifdef CIMAGEDATAWRITER_DEBUG    
  CDBDebug("end, number of GF results: %d",getFeatureInfoResultList.size());
  #endif
  if(writerStatus==uninitialized){CDBError("Not initialized");return 1;}
  if(writerStatus==finished){CDBError("Already finished");return 1;}
  writerStatus=finished;
  if(requestType==REQUEST_WMS_GETFEATUREINFO){
    enum ResultFormats {textplain,texthtml,textxml, applicationvndogcgml,imagepng,imagegif};
    ResultFormats resultFormat=texthtml;
    
    if(srvParam->InfoFormat.equals("text/plain"))resultFormat=textplain;
    if(srvParam->InfoFormat.equals("text/xml"))resultFormat=textxml;
    if(srvParam->InfoFormat.equals("image/png"))resultFormat=imagepng;
    if(srvParam->InfoFormat.equals("image/gif"))resultFormat=imagegif;
    
    if(srvParam->InfoFormat.equals("application/vnd.ogc.gml"))resultFormat=textxml;//applicationvndogcgml;
    

    /* Text plain and text html */
    if(resultFormat==textplain||resultFormat==texthtml){
      CT::string resultHTML;
      if(resultFormat==textplain){
        resultHTML.print("%s%c%c\n","Content-Type:text/plain",13,10);
      }else{
        resultHTML.print("%s%c%c\n","Content-Type:text/html",13,10);
      }
      
      if(resultFormat==texthtml)resultHTML.printconcat("<html>\n");
      
      if(getFeatureInfoResultList.size()==0){
        resultHTML.printconcat("Query returned no results");
      }else{
        GetFeatureInfoResult *g = getFeatureInfoResultList[0];
        if(resultFormat==texthtml){
          //resultHTML.printconcat("coordinates (%0.2f , %0.2f)<br>\n",g->x_imageCoordinate,g->y_imageCoordinate);
          resultHTML.printconcat("<b>Coordinates</b> - (lon=%0.2f; lat=%0.2f)<br>\n",g->lon_coordinate,g->lat_coordinate);
          
        }else{
          //resultHTML.printconcat("coordinates (%0.2f , %0.2f)\n",g->x_imageCoordinate,g->y_imageCoordinate);
          resultHTML.printconcat("Coordinates - (lon=%0.2f; lat=%0.2f)\n",g->lon_coordinate,g->lat_coordinate);
        }
        
      
        /*for(size_t j=0;j<getFeatureInfoResultList.size();j++){
          GetFeatureInfoResult *g = getFeatureInfoResultList[j];
          int elNR = 0;
          GetFeatureInfoResult::Element * e=g->elements[elNR];
          if(g->elements.size()>1){
            resultHTML.printconcat("%d: ",elNR);
          }
          if(resultFormat==texthtml){
            resultHTML.printconcat("<b>%s</b> - %s<br>\n",e->var_name.c_str(),e->feature_name.c_str());
          }else{
            resultHTML.printconcat("%s - %s\n",e->var_name.c_str(),e->feature_name.c_str());
          }
        }*/
        //if(resultFormat==texthtml)resultHTML.printconcat("<hr>\n");
        //CDBDebug("getFeatureInfoResultList.size() %d",getFeatureInfoResultList.size());
        for(size_t j=0;j<getFeatureInfoResultList.size();j++){
          
          GetFeatureInfoResult *g = getFeatureInfoResultList[j];
          
          
          if(resultFormat==texthtml){
            resultHTML.printconcat("<hr/><b>%s</b><br/>\n",g->layerName.c_str());
          }
          else{
            resultHTML.printconcat("%s\n",g->layerName.c_str());
          }
          for(size_t elNR=0;elNR<g->elements.size();elNR++){
            GetFeatureInfoResult::Element * e=g->elements[elNR];
            if(g->elements.size()>1){
              resultHTML.printconcat("%d: ",elNR);
            }
            CDBDebug(" %d elements.size() %d value '%s'",j,g->elements.size(),e->value.c_str());
            if(e->value.length()>0){
              if(resultFormat==texthtml){
                resultHTML.printconcat("%s: %s <b>%s</b>",e->time.c_str(),e->var_name.c_str(),e->value.c_str());
              }else{
                resultHTML.printconcat("%s: %s %s",e->time.c_str(),e->var_name.c_str(),e->value.c_str());
              }
              if(e->units.length()>0){
                if(!e->value.equals("nodata")&&!e->value.equals("")){
                  resultHTML.printconcat(" %s",e->units.c_str());
                }
              }
            }/*else{
              resultHTML.printconcat("%s: -",e->time.c_str());
            }*/
            if(resultFormat==texthtml)resultHTML.printconcat("<br>\n");else resultHTML.printconcat("\n");
          }
        }

      }
      
      if(resultFormat==texthtml)resultHTML.printconcat("</html>\n");else resultHTML.printconcat("\n");
      resetErrors();
      
      
      printf("%s",resultHTML.c_str());
    }
    
    /* Text XML */
    if(resultFormat==textxml){
      CDBDebug("CREATING GML");
      CT::string resultXML;
      resultXML.print("%s%c%c\n","Content-Type:text/xml",13,10);
      resultXML.printconcat("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
      resultXML.printconcat(" <GMLOutput\n");
      resultXML.printconcat("          xmlns:gml=\"http://www.opengis.net/gml\"\n");
      resultXML.printconcat("          xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n");
      resultXML.printconcat("          xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n");

      if(getFeatureInfoResultList.size()==0){
        CDBError("Query returned no results");
        return 1;
      }else{
        for(size_t j=0;j<getFeatureInfoResultList.size();j++){
          GetFeatureInfoResult *g = getFeatureInfoResultList[j];
          CT::string layerName=g->layerName.c_str();
          layerName.replaceSelf(" ","_");
          layerName.replaceSelf("/","_");
          layerName.replaceSelf(":","-");
          
          resultXML.printconcat("  <%s_layer>\n",layerName.c_str());
          for(size_t elNR=0;elNR<g->elements.size();elNR++){
            GetFeatureInfoResult::Element * e=g->elements[elNR];
            CT::string featureName=e->feature_name.c_str();featureName.replaceSelf(" ","_");
            resultXML.printconcat("    <%s_feature>\n",featureName.c_str());
            resultXML.printconcat("      <gml:location>\n");
            
            resultXML.printconcat("        <gml:Point srsName=\"EPSG:4326\">\n");
            resultXML.printconcat("          <gml:pos>%f,%f</gml:pos>\n",g->lon_coordinate,g->lat_coordinate);
            resultXML.printconcat("        </gml:Point>\n");
            
            if(!srvParam->Geo->CRS.equals("EPSG:4326")){
              resultXML.printconcat("        <gml:Point srsName=\"%s\">\n",srvParam->Geo->CRS.c_str());
              resultXML.printconcat("          <gml:pos>%f %f</gml:pos>\n",g->x_imageCoordinate,g->y_imageCoordinate);
              resultXML.printconcat("        </gml:Point>\n");
            }

            /*resultXML.printconcat("        <gml:Point srsName=\"%s\">\n","image:xyindices");
            resultXML.printconcat("          <gml:pos>%d %d</gml:pos>\n",g->x_imagePixel,g->y_imagePixel);
            resultXML.printconcat("        </gml:Point>\n");*/

            resultXML.printconcat("        <gml:Point srsName=\"%s\">\n","raster:coordinates");
            resultXML.printconcat("          <gml:pos>%f %f</gml:pos>\n",g->x_rasterCoordinate,g->y_rasterCoordinate);
            resultXML.printconcat("        </gml:Point>\n");

            resultXML.printconcat("        <gml:Point srsName=\"%s\">\n","raster:xyindices");
            resultXML.printconcat("          <gml:pos>%d %d</gml:pos>\n",g->x_rasterIndex,g->y_rasterIndex);
            resultXML.printconcat("        </gml:Point>\n");

            
            resultXML.printconcat("      </gml:location>\n");
            resultXML.printconcat("      <FeatureName>%s</FeatureName>\n",featureName.c_str());
            resultXML.printconcat("      <StandardName>%s</StandardName>\n",e->standard_name.c_str());
            resultXML.printconcat("      <LongName>%s</LongName>\n",e->long_name.c_str());
            resultXML.printconcat("      <VarName>%s</VarName>\n",e->var_name.c_str());
            resultXML.printconcat("      <Value units=\"%s\">%s</Value>\n",e->units.c_str(),e->value.c_str());
            //resultXML.printconcat("      <Dimension name=\"time\">%s</Dimension>\n",e->time.c_str());
            for(size_t d=0;d<e->cdfDims.dimensions.size();d++){
              //TODO MUST BECOME THE OGC DIMNAME
              //resultXML.printconcat("      <Dimension name=\"%s\" index=\"%d\">%s</Dimension>\n",e->cdfDims.dimensions[d]->name.c_str(),e->cdfDims.dimensions[d]->index,e->cdfDims.dimensions[d]->value.c_str());
              resultXML.printconcat("      <Dimension name=\"%s\" index=\"%d\">%s</Dimension>\n",e->dataSource->requiredDims[d]->name.c_str(),e->cdfDims.dimensions[d]->index,e->cdfDims.dimensions[d]->value.c_str());
            }
            
            resultXML.printconcat("    </%s_feature>\n",featureName.c_str());
          }
          resultXML.printconcat("  </%s_layer>\n",layerName.c_str());
        }
      }
      resultXML.printconcat(" </GMLOutput>\n");
      resetErrors();
      printf("%s",resultXML.c_str());
      
    }
    
    /*************************************************************************************************************************************/
    /* image/png image/png image/png image/png image/png image/png image/png image/png image/png image/png image/png image/png image/png */
    /*************************************************************************************************************************************/
    
    if(resultFormat==imagepng||resultFormat==imagegif){
      #ifdef MEASURETIME
      StopWatch_Stop("Start creating image");
      #endif
      
    
      if(getFeatureInfoResultList.size()==0){
        CDBError("Query returned no results");
        return 1;
      }
      
      #ifdef CIMAGEDATAWRITER_DEBUG        
      CDBDebug("GetFeatureInfo Format image/png");
      #endif
      float width=srvParam->Geo->dWidth,height=srvParam->Geo->dHeight;
      if(srvParam->figWidth>1)width=srvParam->figWidth;
      if(srvParam->figHeight>1)height=srvParam->figHeight;
      
   
    
      
      
      //Set font location
      const char *fontLocation = NULL;
      if(srvParam->cfg->WMS[0]->ContourFont.size()!=0){
        if(srvParam->cfg->WMS[0]->ContourFont[0]->attr.location.c_str()!=NULL){
          fontLocation=srvParam->cfg->WMS[0]->ContourFont[0]->attr.location.c_str();
        }else {
          CDBError("In <Font>, attribute \"location\" missing");
          return 1;
        }
      }
      
      
    
      
      
      size_t nrOfLayers = getFeatureInfoResultList.size();
//      size_t nrOfElements = getFeatureInfoResultList[0]->elements.size();
      
      std::vector<PlotObject*> plotObjects;
      
      
      std::vector<CT::string> features[nrOfLayers];
      std::vector<int> numDims[nrOfLayers]; 
      //Find number of features per layer
      for(size_t layerNr=0;layerNr<nrOfLayers;layerNr++){
        size_t nrOfElements = getFeatureInfoResultList[layerNr]->elements.size();
        
        for(size_t elNr=0;elNr<nrOfElements;elNr++){
          GetFeatureInfoResult::Element * element = getFeatureInfoResultList[layerNr]->elements[elNr];
          bool featureNameFound=false;
          for(size_t j=0;j<features[layerNr].size();j++){
            if(features[layerNr][j].equals(&element->feature_name)){featureNameFound=true;break;}
          }
          if(!featureNameFound){
            features[layerNr].push_back(element->feature_name.c_str());
            numDims[layerNr].push_back(element->cdfDims.dimensions.size());
          }else{
            /*for(size_t j=0;j<features[layerNr].size();j++){
              CDBDebug("%d %d %s\tDims:%d",layerNr,j,features[layerNr][j].c_str(),numDims[layerNr][j]);
            }*/

            break;
          }
        }
      }
      for(size_t layerNr=0;layerNr<nrOfLayers;layerNr++){
        if(numDims[layerNr].size()>0){
          CDataSource *ds=getFeatureInfoResultList[layerNr]->elements[0]->dataSource;
          size_t nrOfElements = getFeatureInfoResultList[layerNr]->elements.size();
          size_t nrOfFeatures = features[layerNr].size();
          size_t nrOfElementSteps = nrOfElements/(nrOfFeatures);
          
          size_t numDimStepsPerTime = 1;
          CT::string dimname = "";
          for(size_t j=1;j<ds->requiredDims.size();j++){
            //TODO
            numDimStepsPerTime*=ds->requiredDims[j]->uniqueValues.size();
          }

          nrOfElementSteps=nrOfElementSteps/numDimStepsPerTime;
          
          
          for(size_t featureNr=0;featureNr<nrOfFeatures;featureNr++){
            for(size_t dimIter=0;dimIter<numDimStepsPerTime;dimIter++){
              PlotObject *plotObject = new PlotObject();plotObjects.push_back(plotObject);
              plotObject->allocateLength(nrOfElementSteps);
              
              size_t elNr = dimIter*nrOfFeatures+featureNr;
              GetFeatureInfoResult::Element * element = getFeatureInfoResultList[layerNr]->elements[elNr];
              //plotObject->name.copy(getFeatureInfoResultList[layerNr]->layerName.c_str());
              //plotObject->name.concat("/");
              plotObject->name.copy(features[layerNr][featureNr].c_str());
              if(element->units.length()>0){
                plotObject->name.printconcat(" (%s)",element->units.c_str());
              }
              for(size_t j=1;j<element->cdfDims.dimensions.size();j++){
                plotObject->name.concat(" @");
                plotObject->name.concat(element->cdfDims.dimensions[j]->value.c_str());
              }

              plotObject->units=&element->units;
              for(size_t elStep=0;elStep<nrOfElementSteps;elStep++){
                
                //CDBDebug("Iterating %s",ds->requiredDims[j]->allValues[
                
                  
                  
                  size_t elNr = elStep*nrOfFeatures*numDimStepsPerTime + featureNr + dimIter*nrOfFeatures;
                  
                // CDBDebug("%d = %d/%d %d/%d - %d/%d - %d/%d",elNr,layerNr,nrOfLayers,featureNr,nrOfFeatures,dimIter,numDimStepsPerTime,elStep,nrOfElementSteps);
                  
                  GetFeatureInfoResult::Element * element = getFeatureInfoResultList[layerNr]->elements[elNr];
                  plotObject->elements[elStep]=element;
              
                  /*
                  CT::string dims = "";
                  for(size_t d=1;d<element->cdfDims.dimensions.size();d++){
                    dims.printconcat("%s ",element->cdfDims.dimensions[d]->value.c_str());
                  }
                  CDBDebug("%s %s",features[layerNr][featureNr].c_str(),dims.c_str());
                */
                
              }
            }
          }
        }
      }
      
      
      
      //Find min max for values and time
      

      
      
      CTime *ctime = new CTime();
      ctime->init("seconds since 1950");
      
      double startTimeValue=0;
      double stopTimeValue=0;
      bool firstDateDone = false;
      
      float overallMinValue = 0,overallMaxValue = 1;
      bool overallMinMaxValueDone = false;
      bool overallMinMaxValueWasEstimated = false;
      
      for(size_t j=0;j<plotObjects.size();j++){
        PlotObject *plotObject = plotObjects[j];
        //CDBDebug("%d) %s in %s",j,plotObject->name.c_str(),plotObject->units.c_str());
        
        //Find min and max dates
        double minDate;
        double maxDate;
        try{
          minDate=ctime->ISOStringToDate(plotObject->elements[0]->time.c_str()).offset;
        }catch(int e){
          CDBError("Time startTimeValue error %s",plotObject->elements[0]->time.c_str());
        }
        
        try{
          maxDate=ctime->ISOStringToDate(plotObject->elements[plotObject->length-1]->time.c_str()).offset;
        }catch(int e){
          CDBError("Time stopTimeValue error %s",plotObject->elements[plotObject->length-1]->time.c_str());
        }
        
        if(!firstDateDone){
          startTimeValue = minDate;
          stopTimeValue  = maxDate;
          firstDateDone = true;
        }else{
          if(startTimeValue>minDate)startTimeValue=minDate;
          if(stopTimeValue<maxDate)stopTimeValue=maxDate;
        }
        
        
        bool firstDone = false;
        plotObject->minValue = 0;
        plotObject->maxValue = 1;
        
        //Find min and max values
        for(size_t i=0;i<plotObject->length;i++){
          GetFeatureInfoResult::Element * element = plotObject->elements[i];
          double value = element->value.toFloat();
          if(element->value.c_str()[0]>60)value=NAN;;
          if(element->value.equals("nodata"))value=NAN;
          plotObject->values[i]=value;
          
          if(value == value){
            if(firstDone == false){
              plotObject->minValue = value;
              plotObject->maxValue = value;
              firstDone = true;
            }
            
            if(plotObject->minValue>value)plotObject->minValue = value;
            if(plotObject->maxValue<value)plotObject->maxValue = value;
            
          }
        }
        
        //Minmax is fixed by layer settings:
        if(plotObject->elements[0]->dataSource != NULL){
          if(!plotObject->elements[0]->dataSource->stretchMinMax){
            //Determine min max based on given datasource settings (scale/offset/log or min/max/log in config file)
            plotObject->minValue=getValueForColorIndex(plotObject->elements[0]->dataSource,0);
            plotObject->maxValue=getValueForColorIndex(plotObject->elements[0]->dataSource,240);
          }else{
            overallMinMaxValueWasEstimated = true;
          }
        }
        
        //Increase minmax if they are the same.
        if(plotObject->minValue==plotObject->maxValue){
          plotObject->minValue=plotObject->minValue+0.01;
          plotObject->maxValue=plotObject->maxValue-0.01;
        }
        
        if(!overallMinMaxValueDone){
          overallMinValue = plotObject->minValue;
          overallMaxValue = plotObject->maxValue;
        }else{
          overallMinMaxValueDone=true;
          if(overallMinValue>plotObject->minValue)overallMinValue = plotObject->minValue;
          if(overallMaxValue<plotObject->maxValue)overallMaxValue = plotObject->maxValue;
        }
        //CDBDebug("%f %f",plotObject->minValue,plotObject->maxValue);
      }
      
      float significantDigits = 0.1;
     
        float range = overallMaxValue-overallMinValue;
        float order = log10(range);
        float orderRounded=floor(order);
        significantDigits = pow(10,orderRounded);
        //CDBDebug("significantDigits = %f",significantDigits);
       if(overallMinMaxValueWasEstimated){ 
        overallMinValue=floor(overallMinValue/significantDigits)*significantDigits;
        overallMaxValue=ceil(overallMaxValue/significantDigits)*significantDigits;

      }
      
      CDBDebug("OverallMinMax = %f %f",overallMinValue,overallMaxValue);
      
      
      
      CT::string startDateString = ctime->dateToISOString(ctime->getDate(startTimeValue));
      CT::string stopDateString = ctime->dateToISOString(ctime->getDate(stopTimeValue));
      startDateString.setChar(19,'Z' );startDateString.setSize(20);
      stopDateString.setChar(19,'Z' );stopDateString.setSize(20);
      CDBDebug("Dates: %s/%s",startDateString.c_str(),stopDateString.c_str());

      
    
      float classes=((overallMaxValue-overallMinValue)/significantDigits)*2;
      int tickRound=0;

      if(currentStyleConfiguration->legendTickInterval>0.0f){
        classes=(plotObjects[0]->minValue-plotObjects[0]->maxValue)/currentStyleConfiguration->legendTickInterval;
      }
      if(currentStyleConfiguration->legendTickRound>0){
        tickRound = int(round(log10(currentStyleConfiguration->legendTickRound))+3);
      }
      CDataSource * dataSource=getFeatureInfoResultList[0]->elements[0]->dataSource;
      
      float scale=dataSource->legendScale;
      float offset=dataSource->legendOffset;
      
 
      scale=240.0f/(overallMaxValue-overallMinValue);
      offset=-overallMinValue*scale;
      
      //Init title
      size_t nrOfPlotObjectsForTitle = plotObjects.size();
      if(nrOfPlotObjectsForTitle>9)nrOfPlotObjectsForTitle=9;
      int cols=1,rows=1;
      
      if(nrOfPlotObjectsForTitle>1)cols=2;
      if(nrOfPlotObjectsForTitle>5)cols=3;
      rows=int(float(nrOfPlotObjectsForTitle)/float(cols)+0.5);
      
      //Init canvas
         
      float plotOffsetX=(width*0.05);
      float plotOffsetY = rows*10+10;
      if(plotOffsetX<50)plotOffsetX=50;

      float plotHeight=((height-plotOffsetY-30));
      float plotWidth=((width-plotOffsetX)*0.98);
      
      CDrawImage plotCanvas;
      CDrawImage lineCanvas;
      if(resultFormat==imagepng){
        plotCanvas.setTrueColor(true);
        lineCanvas.setTrueColor(true);
        lineCanvas.enableTransparency(true);
      }
      if(resultFormat==imagegif){
        plotCanvas.setTrueColor(false);
        plotCanvas.setBGColor(255,255,255);
        lineCanvas.setTrueColor(false);
        lineCanvas.enableTransparency(true);
      }
      plotCanvas.createImage(int(width),int(height));
      plotCanvas.create685Palette();
      lineCanvas.createImage(plotWidth,plotHeight);
      lineCanvas.create685Palette();
      //lineCanvas.rectangle(0,0,int(plotWidth/2),int(plotHeight),CColor(255,255,255,255),CColor(255,255,255,255));
      lineCanvas.rectangle(-1,-1,int(plotWidth+1),int(plotHeight+1),CColor(255,255,255,255),CColor(255,255,255,255));
      plotCanvas.rectangle(-1,-1,int(width+1),int(height+1),CColor(255,255,255,255),CColor(0,0,0,255));
      
         
      //TODO
      plotCanvas.rectangle(int(plotOffsetX-1),int(plotOffsetY-1),int(plotWidth+plotOffsetX),int(plotHeight+plotOffsetY),CColor(255,255,255,255),CColor(0,0,0,128));
   
      
      //Draw Title
      CT::string title;
    
      for(size_t j=0;j<nrOfPlotObjectsForTitle;j++){
        int tx=j%cols;
        int ty=j/cols;
        size_t tp=tx*rows+ty;
        if(tp<nrOfPlotObjectsForTitle){
          CT::string title=plotObjects[tp]->name.c_str();
          int x = tx*((width-80)/cols)+80;
          int y=12+ty*10;
          plotCanvas.rectangle(x-30,y-7,x-5,y,getColorForPlot(tp,plotObjects.size()),CColor(0,0,0,128));
          plotCanvas.drawText(x,y,fontLocation,7,0,title.c_str(),CColor(0,0,0,255),CColor(255,255,255,0));
        }
      }

      
      
      for(int j=0;j<=classes;j++){
        char szTemp[256];
        float c=((float(classes-j)/classes))*(plotHeight)-1;
        float v=((float(j)/classes))*(240.0f);
        v-=offset;
        v/=scale;
        if(dataSource->legendLog!=0){v=pow(dataSource->legendLog,v);}
    
        //if(j!=0)
        lineCanvas.line(0,(int)c,plotWidth,(int)c,0.5,CColor(0,0,128,128));
        if(tickRound==0){floatToString(szTemp,255,1/significantDigits+1,v);}else{
          floatToString(szTemp,255,tickRound,v);
        }
        plotCanvas.drawText(5,int(c+plotOffsetY+3),fontLocation,8,0,szTemp,CColor(0,0,0,255),CColor(255,255,255,0));
      }
  
      float lineWidth = 2.0;
      for(size_t plotNr=0;plotNr<plotObjects.size();plotNr++){
        PlotObject *plotObject = plotObjects[plotNr];
        CColor color=getColorForPlot(plotNr,plotObjects.size());
        

      if(plotObjects.size()>5)lineWidth=0.3;
        
        
      double timeWidth=(stopTimeValue-startTimeValue);
        for(size_t i=0;i<plotObject->length;i++){
          CTime::Date timePos1=ctime->ISOStringToDate(plotObject->elements[i]->time.c_str());
          double x1=((timePos1.offset-startTimeValue)/timeWidth)*plotWidth;
          
          if(plotNr==0){
            if(timePos1.hour==0&&timePos1.minute==0&&timePos1.second==0){
              lineCanvas.line(x1,0,x1,plotHeight,1.5,CColor(0,0,0,255));
              char szTemp[256];snprintf(szTemp,255,"%d",timePos1.day);
              plotCanvas.drawText(x1-strlen(szTemp)*4+plotOffsetX+1,int(plotOffsetY+plotHeight+12),fontLocation,9,0,szTemp,CColor(0,0,0,255),CColor(255,255,255,0));
            }else if(timePos1.minute==0&&timePos1.second==0){
              lineCanvas.line(x1,0,x1,plotHeight,0.5,CColor(0,0,128,128));
              char szTemp[256];snprintf(szTemp,255,"%d",timePos1.hour);
              plotCanvas.drawText(x1-strlen(szTemp)*2+plotOffsetX+1,int(plotOffsetY+plotHeight+8),fontLocation,5,0,szTemp,CColor(0,0,0,192),CColor(255,255,255,0));
            }else{
              lineCanvas.line(x1,0,x1,plotHeight,0.5,CColor(128,128,128,128));
            }
          }
          if(i<plotObject->length-1){
            CTime::Date timePos2=ctime->ISOStringToDate(plotObject->elements[i+1]->time.c_str());
          
            double x2=((timePos2.offset-startTimeValue)/timeWidth)*plotWidth;
        
            float v1=plotObject->values[i];
            float v2=plotObject->values[i+1];
            if(v1==v1&&v2==v2){
              //if(v1>minValue[elNr]&&v1<maxValue[elNr]&&v2>minValue[elNr]&&v2<maxValue[elNr]){
              //}

              float v1l=v1;
              float v2l=v2;
              bool noData=false;
              if(dataSource->legendLog!=0){
                if ((v1>0)&&(v2>0)){
                  v1l=log10(v1l)/log10(dataSource->legendLog);
                  v2l=log10(v2l)/log10(dataSource->legendLog);
                } else {
                  noData=true;
                }
              }

              v1l*=scale;
              v1l+=offset;
              v1l/=240.0;
              v2l*=scale;
              v2l+=offset;
              v2l/=240.0;
                int y1=int((1-v1l)*plotHeight);
                int y2=int((1-v2l)*plotHeight);
                
              if (!noData) {
                  lineCanvas.line(x1,y1,x2,y2,lineWidth,color);
              }
            }
          }
        }
      }
  
      delete ctime;
      
      
      //GetFeatureInfoResult::Element * e2=getFeatureInfoResultList[getFeatureInfoResultList.size()-1]->elements[0];
      title.print("Dates: %s till %s",startDateString.c_str(),stopDateString.c_str());
      plotCanvas.drawText(int(plotWidth/2-float(title.length())*2.5),int(height-5),fontLocation,8,0,title.c_str(),CColor(0,0,0,255),CColor(255,255,255,0));
        plotCanvas.draw(plotOffsetX, plotOffsetY,0,0,&lineCanvas);
      if(resultFormat==imagepng){
        printf("%s%c%c\n","Content-Type:image/png",13,10);
        plotCanvas.printImagePng();
      }
      if(resultFormat==imagegif){
        printf("%s%c%c\n","Content-Type:image/gif",13,10);
        plotCanvas.printImageGif();
      }
      #ifdef MEASURETIME
      StopWatch_Stop("/Start creating image");
      #endif
    
      for(size_t j=0;j<plotObjects.size();j++)delete plotObjects[j];plotObjects.clear();
      //CDBDebug("Done!");
    }
    
    
    
    for(size_t j=0;j<getFeatureInfoResultList.size();j++){delete getFeatureInfoResultList[j];getFeatureInfoResultList[j]=NULL; } getFeatureInfoResultList.clear();
  }
  if(requestType!=REQUEST_WMS_GETMAP&&requestType!=REQUEST_WMS_GETLEGENDGRAPHIC)return 0;
  
  
  //Output WMS getmap results
  if(errorsOccured()){
    CDBError("Errors occured during image data writing");
    return 1;
  }
  
  //Animation image:
  if(animation==1){
    drawImage.addImage(100);
    drawImage.endAnimation();
    return 0;
  }

#ifdef MEASURETIME
StopWatch_Stop("Drawing finished");
#endif

  //Static image
  int status=0;
  if(srvParam->imageFormat==IMAGEFORMAT_IMAGEPNG8||srvParam->imageFormat==IMAGEFORMAT_IMAGEPNG32){
    
    //printf("%s%c%c","Cache-Control: max-age=3600, must-revalidate",13,10);
    //printf("%s%c%c","Pragma: no-cache",13,10);
    //printf("%s%c%c","Cache-Control:no-store,no-cache,must-revalidate,post-check=0,pre-check=0",13,10);
    //printf("%s%c%c","Expires: Fri, 17 Aug 2012:19:41 GMT");
    //printf("%s%c%c","Last-Modified: Thu, 01 Jan 1970 00:00:00 GMT",13,10);
    //printf("%s\n","ETag: \"3e86-410-3596fbbc\"");
    printf("%s%c%c\n","Content-Type:image/png",13,10);
    
    
    

    
    
    status=drawImage.printImagePng();
  }else if(srvParam->imageFormat==IMAGEFORMAT_IMAGEGIF){
    printf("%s%c%c\n","Content-Type:image/gif",13,10);
    status=drawImage.printImageGif();
  }else {
    printf("%s%c%c\n","Content-Type:image/png",13,10);
    status=drawImage.printImagePng();
  }
  
  #ifdef MEASURETIME
  StopWatch_Stop("Image printed");
  #endif
  if(status!=0){
    CDBError("Errors occured during image printing");
  }
  return status;
}
float CImageDataWriter::getValueForColorIndex(CDataSource *dataSource,int index){
  /*if(dataSource->stretchMinMax){
    if(dataSource->statistics==NULL){
      dataSource->statistics = new CDataSource::Statistics();
      dataSource->statistics->calculate(dataSource);
    }
    float minValue=(float)dataSource->statistics->getMinimum();
    float maxValue=(float)dataSource->statistics->getMaximum();
    //maxValue+=10;
    float ls=240/(maxValue-minValue);
    float lo=-(minValue*ls);
    dataSource->legendScale=ls;
    dataSource->legendOffset=lo;
    CDBDebug("max=%f; min=%f",maxValue,minValue);
    CDBDebug("scale=%f; offset=%f",ls,lo);
  }  */
  float v=index;
  v-=dataSource->legendOffset;
  v/=dataSource->legendScale;
  if(dataSource->legendLog!=0){
    v=pow(dataSource->legendLog,v);
  }
  return v;
}
int CImageDataWriter::getColorIndexForValue(CDataSource *dataSource,float value){
  float val=value;
  if(dataSource->legendLog!=0)val=log10(val+.000001)/log10(dataSource->legendLog);
  val*=dataSource->legendScale;
  val+=dataSource->legendOffset;
  if(val>=239)val=239;else if(val<0)val=0;
  return int(val);
}



int CImageDataWriter::createLegend(CDataSource *dataSource,CDrawImage *legendImage){
  
  enum LegendType { undefined,continous,discrete,statusflag,cascaded};
  LegendType legendType=undefined;
  bool estimateMinMax=false;

  int legendPositiveUp = 1;
  //float legendWidth = legendImage->Geo->dWidth;
  float legendHeight = legendImage->Geo->dHeight;
  int pLeft=4;
  int pTop=0;
  char szTemp[256];
  
  if(dataSource->dLayerType==CConfigReaderLayerTypeCascaded){
    legendType = cascaded;
    CDBDebug("GetLegendGraphic for cascaded WMS is not yet supported");
    legendImage->crop(4);
    return 0;
  }
  
  CDataReader reader;
  RenderMethod renderMethod = currentStyleConfiguration->renderMethod;

  //if(renderMethod==shadedcontour||renderMethod==shaded||renderMethod==contour){
  if(renderMethod&RM_SHADED||renderMethod&RM_CONTOUR){
    //We need to open all the data, because we need to estimate min/max for legend drawing
    estimateMinMax = true;
    if(currentStyleConfiguration->legendHasFixedMinMax==true){
      estimateMinMax=false;
    }
  }else {
    //When the scale factor is zero (0.0f) we need to open the data too, because we want to estimate min/max in this case.
    //When the scale factor is given, we only need to open the header, for displaying the units.
    if(dataSource->legendScale==0.0f){
      estimateMinMax = true;
    }else{
      estimateMinMax = false;
    }
  }
  
  if(dataSource->dataObject[0]->cdfVariable->data==NULL){
  if(estimateMinMax){
      status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_ALL);
    }else{
      status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_HEADER);
    }
    if(status!=0){
      CDBError("Unable to open file");
      return 1;
    }
  }
  
  //Determine legendtype.
  if(dataSource->dataObject[0]->hasStatusFlag){
    legendType = statusflag;
  }else if(!(renderMethod&RM_SHADED||renderMethod&RM_CONTOUR)){
    legendType = continous;
  }else{
    legendType = discrete;
  }
  
  if(renderMethod&RM_RGBA){
    legendType=cascaded;
  }

if(legendType == cascaded){
    legendImage->crop(1,1);
}
  
  
  //Create a legend based on status flags.
  if( legendType == statusflag ){
    int dH=30;
    //cbW=LEGEND_WIDTH/3;cbW/=3;cbW*=3;cbW+=3;
    float cbW = 20;//legendWidth/8;
    float cbH = legendHeight-13-13-30;
  
    int blockHeight = 12;
    int blockDistance=18;
  
    size_t numFlags=dataSource->dataObject[0]->statusFlagList.size();
    while(numFlags*blockDistance>legendHeight-14&&blockDistance>5){
      blockDistance--;
      if(blockHeight>5){
        blockHeight--;
      }
    }
    
  
    for(size_t j=0;j<numFlags;j++){
      float y=j*blockDistance+(cbH-numFlags*blockDistance+8);
      double value=dataSource->dataObject[0]->statusFlagList[j]->value;
      int c=getColorIndexForValue(dataSource,value);
      legendImage->rectangle(1+pLeft,int(2+dH+y)+pTop,(int)cbW+9+pLeft,(int)y+2+dH+blockHeight+pTop,c,248);
      CT::string flagMeaning;
      CDataSource::getFlagMeaningHumanReadable(&flagMeaning,&dataSource->dataObject[0]->statusFlagList,value);
      CT::string legendMessage;
      legendMessage.print("%d %s",(int)value,flagMeaning.c_str());
      legendImage->setText(legendMessage.c_str(),legendMessage.length(),(int)cbW+15+pLeft,(int)y+dH+2+pTop,248,-1);  
    }
    CT::string units="status flag";
    legendImage->setText(units.c_str(),units.length(),2+pLeft,int(legendHeight)-14+pTop,248,-1);
    legendImage->crop(4,4);
  }
  
  //Draw a continous legend
  if( legendType == continous ){
    float cbW = 20;//legendWidth/8;
    float cbH = legendHeight-13-13;
    int dH=0;
    
    for(int j=0;j<cbH;j++){
      for(int i=0;i<cbW+3;i++){
        float c=(float(cbH*legendPositiveUp-j)/cbH)*240.0f;
        legendImage->setPixelIndexed(i+pLeft,j+7+dH+pTop,int(c));
      }
    }
    legendImage->rectangle(pLeft,7+dH+pTop,(int)cbW+3+pLeft,(int)cbH+7+dH+pTop,248);
  

    float classes=6;
    int tickRound=0;
    double min=getValueForColorIndex(dataSource,0);
    double max=getValueForColorIndex(dataSource,240);
    if(currentStyleConfiguration->legendTickInterval>0.0f){
      classes=(max-min)/double(currentStyleConfiguration->legendTickInterval);
      
    }
    if(currentStyleConfiguration->legendTickRound>0){
      tickRound = int(round(log10(currentStyleConfiguration->legendTickRound))+3);
    }
    
    //CDBDebug("LEGEND: scale %f offset %f",dataSource->legendScale,dataSource->legendOffset);
    for(int j=0;j<=classes+1;j++){
      float c=((float(classes*legendPositiveUp-j)/classes))*(cbH);
      float v=((float(j)/classes))*(240.0f);
      v-=dataSource->legendOffset;
      v/=dataSource->legendScale;
      if(dataSource->legendLog!=0){v=pow(dataSource->legendLog,v);}
      if(v<=max){
        float lineWidth=1;
        legendImage->line((int)cbW-1+pLeft,(int)c+7+dH+pTop,(int)cbW+6+pLeft,(int)c+7+dH+pTop,lineWidth,248);
        if(tickRound==0){floatToString(szTemp,255,v);}else{
          floatToString(szTemp,255,tickRound,v);
        }
        legendImage->setText(szTemp,strlen(szTemp),(int)cbW+10+pLeft,(int)c+dH+pTop+1,248,-1);
      }
    }
    
    //Get units
    CT::string units;
    if(dataSource->dataObject[0]->units.length()>0){
      units.concat(&dataSource->dataObject[0]->units);
    }
    if(units.length()>0)legendImage->setText(units.c_str(),units.length(),2+pLeft,int(legendHeight)-14+pTop,248,-1);
    legendImage->crop(4,-1);    
  }
  
  //Draw legend with fixed intervals
  if( legendType == discrete ){
    float cbW = 20;//legendWidth/8;
    float cbH = legendHeight-13-13;
    //int dH=0;
    //cbW = 90.0/3.0;
    // We always need to have the min/max of the data
    // Always to show only the occuring data values in the legend,
    // and in some cases to stretch the colors over min max
    
    //Get the min/max values
    float minValue = getValueForColorIndex(dataSource,0);
    float maxValue = getValueForColorIndex(dataSource,240);

    if(estimateMinMax){
      if(dataSource->statistics==NULL){
        dataSource->statistics = new CDataSource::Statistics();
        dataSource->statistics->calculate(dataSource);
      }
      minValue=(float)dataSource->statistics->getMinimum();
      maxValue=(float)dataSource->statistics->getMaximum();
      
    }
    
    
    //Calculate the number of classes
    float legendInterval=currentStyleConfiguration->shadeInterval;
    int numClasses=(int((maxValue-minValue)/legendInterval));
    
    /*
    // and reduce the number of classes when required...
    if(!dataSource->stretchMinMax){
      while(numClasses>15){
        legendInterval*=2;//(maxValue-minValue);
        numClasses=int((maxValue-minValue)/legendInterval);
      }
    }*/
#ifdef CIMAGEDATAWRITER_DEBUG    
CDBDebug("LayerName = %s",dataSource->layerName.c_str());
CDBDebug("minValue=%f maxValue=%f",minValue,maxValue);
CDBDebug("scale=%f offset=%f",dataSource->legendScale,dataSource->legendOffset);    
#endif
    float iMin=convertValueToClass(minValue,legendInterval);
    float iMax=convertValueToClass(maxValue,legendInterval)+legendInterval;
#ifdef CIMAGEDATAWRITER_DEBUG        
CDBDebug("iMin=%f iMax=%f",iMin,iMax);
#endif

    //In case of auto scale and autooffset we will stretch the colors over the min/max values
    //Calculate new scale and offset for the new min/max:
    //When using min/max stretching, the classes need to be extended according to its shade itnerval
    if(dataSource->stretchMinMax==true){
      //Calculate new scale and offset for the new min/max:
      float ls=240/((iMax-iMin));
      float lo=-(iMin*ls);
      dataSource->legendScale=ls;
      dataSource->legendOffset=lo;
    }
    
    
    bool discreteLegendOnInterval=false;
    bool definedLegendOnShadeClasses=false;
    
    if(currentStyleConfiguration->shadeIntervals!=NULL){
      if(currentStyleConfiguration->shadeIntervals->size()>0){
        definedLegendOnShadeClasses=true;
      }
    }
    if(legendInterval!=0){
      discreteLegendOnInterval=true;
    }
    
    
    /**
    * Defined blocks based on defined interval
    */
    
    if(definedLegendOnShadeClasses){
      char szTemp[1024];
      for(size_t j=0;j<currentStyleConfiguration->shadeIntervals->size();j++){
        CServerConfig::XMLE_ShadeInterval *s=(*currentStyleConfiguration->shadeIntervals)[j];
        if(s->attr.min.c_str()!=NULL&&s->attr.max.c_str()!=NULL){
          int cY1 = int(cbH-(j*12));
          int cY2 = int(cbH-(((j+1)*12)-2));
          CColor color;
          if(s->attr.fillcolor.c_str()!=NULL){
            color=CColor(s->attr.fillcolor.c_str());
          }else{
            color=legendImage->getColorForIndex(getColorIndexForValue(dataSource,parseFloat(s->attr.min.c_str())));
          }
          
          legendImage->rectangle(4+pLeft,cY2+pTop,int(cbW)+7+pLeft,cY1+pTop,color,CColor(0,0,0,255));
          
          if(s->attr.label.c_str()==NULL){
            snprintf(szTemp,1000,"%s - %s",s->attr.min.c_str(),s->attr.max.c_str());
          }else{
            snprintf(szTemp,1000,"%s",s->attr.label.c_str());
          }
          
          legendImage->setText(szTemp,strlen(szTemp),int(cbW)+12+pLeft,cY2+pTop,248,-1);
        }
      }
    }
    
    

    if(discreteLegendOnInterval){
      /**
      * Discrete blocks based on continous interval
      */
      
      numClasses=int((iMax-iMin)/legendInterval);
      int classSizeY=(180/(numClasses));
      if(classSizeY>14)classSizeY=14;
      
      
      // Rounding of legend text depends on legendInterval
      if(legendInterval==0){
        CDBError("legendInterval is zero");
        return 1;
      }
      int textRounding=0;
      if(legendInterval!=0){
        float fracPart=legendInterval-int(legendInterval);
        textRounding=-int(log10(fracPart)-0.9999999f);
      }
      
        
      int classNr=0;
      for(float j=iMin;j<iMax+legendInterval;j=j+legendInterval){
        float v=j;
        int cY= int((cbH-(classNr-5))+6);
        
        int dDistanceBetweenClasses=(classSizeY-10);
        if(dDistanceBetweenClasses<4){dDistanceBetweenClasses=2;};
        if(dDistanceBetweenClasses>4)dDistanceBetweenClasses=4;
        cY-=dDistanceBetweenClasses;
        int cY2=int((cbH-(classNr+classSizeY-5))+6);
        classNr+=classSizeY;

        if(j<iMax)
        {
          int y=getColorIndexForValue(dataSource,v);
          legendImage->rectangle(4+pLeft,cY2+pTop,int(cbW)+7+pLeft,cY+pTop,(y),248);
          if(textRounding<=0)sprintf(szTemp,"%2.0f - %2.0f",v,v+legendInterval);
          if(textRounding==1)sprintf(szTemp,"%2.1f - %2.1f",v,v+legendInterval);
          if(textRounding==2)sprintf(szTemp,"%2.2f - %2.2f",v,v+legendInterval);
          if(textRounding==3)sprintf(szTemp,"%2.3f - %2.3f",v,v+legendInterval);
          if(textRounding==4)sprintf(szTemp,"%2.4f - %2.4f",v,v+legendInterval);
          if(textRounding==5)sprintf(szTemp,"%2.5f - %2.5f",v,v+legendInterval);
          if(textRounding==5)sprintf(szTemp,"%2.6f - %2.6f",v,v+legendInterval);
          if(textRounding>6)sprintf(szTemp,"%f - %f",v,v+legendInterval);
          int l=strlen(szTemp);
          legendImage->setText(szTemp,l,(int)cbW+10+pLeft,((cY+cY2)/2)-7+pTop,248,-1);
        }

      }
    }
  
    //Get units
    CT::string units;
    if(dataSource->dataObject[0]->units.length()>0){
      units.concat(&dataSource->dataObject[0]->units);
    }
    if(units.length()>0)legendImage->setText(units.c_str(),units.length(),2+pLeft,int(legendHeight)-14+pTop,248,-1);
    legendImage->crop(4,4);
  }
  

  reader.close();

  


  
  return 0;
}


int CImageDataWriter::drawText(int x,int y,const char * fontlocation, float size,float angle,const char *text,unsigned char colorIndex){
  drawImage.drawText(x,y, fontlocation,size, angle,text, colorIndex);
  return 0;
}

    void doJacoIntoLatLon(double &u, double &v, double lo, double la, float deltaX, float deltaY, CImageWarper *warper) {
            double modelXLat, modelYLat;
            double modelXLon, modelYLon;
            double VJaa,VJab,VJba,VJbb;
            int signLon=(deltaX<0)?-1:1;
            int signLat=(deltaY<0)?-1:1;


              double modelX=lo;
              double modelY=la;
              warper->reprojModelFromLatLon(modelX, modelY); // model to vis proj.
              modelXLon=modelX+deltaX;
              modelYLon=modelY;
              modelXLat=modelX;
              modelYLat=modelY+deltaY;
//              warper->reprojpoint_inv(lo, la); // model to vis proj.
//              warper->reprojpoint_inv(modelXLon, modelYLon);
//              warper->reprojpoint_inv(modelXLat, modelYLat);
//              warper->reprojModelToLatLon(lo, la); // model to vis proj.
              warper->reprojModelToLatLon(modelXLon, modelYLon);
              warper->reprojModelToLatLon(modelXLat, modelYLat);
              double distLon=hypot(modelXLon-lo, modelYLon-la);
              double distLat=hypot(modelXLat-lo, modelYLat-la);
              
              VJaa=signLon*(modelXLon-lo)/distLon;
              VJab=signLon*(modelXLat-lo)/distLat;
              VJba=signLat*(modelYLon-la)/distLon;
              VJbb=signLat*(modelYLat-la)/distLat;
              double magnitude=hypot(u, v);
              double uu;
              double vv;
              uu = VJaa*u+VJab*v;
              vv = VJba*u+VJbb*v;
              
              double newMagnitude = hypot(uu, vv);
              u=uu*magnitude/newMagnitude;
              v=vv*magnitude/newMagnitude;
              
}
