#include "CImageDataWriter.h"

const char * CImageDataWriter::className = "CImageDataWriter";
const char * CImageDataWriter::RenderMethodStringList="nearest,bilinear,contour,shadedcontour,shaded,vector,vectorcontour,nearestcontour,bilinearcontour,vectorcontourshaded";
CImageDataWriter::CImageDataWriter(){
  mode = 0;
}

CImageDataWriter::RenderMethodEnum CImageDataWriter::getRenderMethodFromString(CT::string *renderMethodString){
  RenderMethodEnum renderMethod;
  if(renderMethodString->equals("bilinear"))renderMethod=bilinear;
  else if(renderMethodString->equals("nearest"))renderMethod=nearest;
  else if(renderMethodString->equals("vector"))renderMethod=vector;
  else if(renderMethodString->equals("shaded"))renderMethod=shaded;
  else if(renderMethodString->equals("contour"))renderMethod=contour;
  else if(renderMethodString->equals("shadedcontour"))renderMethod=contourshaded;
  else if(renderMethodString->equals("vectorcontour"))renderMethod=vectorcontour;
  else if(renderMethodString->equals("nearestcontour"))renderMethod=nearestcontour;
  else if(renderMethodString->equals("bilinearcontour"))renderMethod=bilinearcontour;
  else if(renderMethodString->equals("vectorcontourshaded"))renderMethod=vectorcontourshaded;
  
  return renderMethod;
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

int CImageDataWriter::drawCascadedWMS(const char *service,const char *layers,bool transparent){

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
  for(int k=0;k<srvParam->NumOGCDims;k++){
    url.printconcat("&%s=%s",srvParam->OGCDims[k].Name.c_str(),srvParam->OGCDims[k].Value.c_str());
  }
  CDBDebug(url.c_str());
  gdImagePtr gdImage;
  MyCURL myCURL;
  myCURL.getGDImageField(url.c_str(),gdImage);
  if(gdImage&&1==1){
    //int w=gdImageSX(gdImage);
    //int h=gdImageSY(gdImage);
    int transpColor=gdImageGetTransparent(gdImage);
    for(int y=0;y<drawImage.Geo->dHeight;y++){
      for(int x=0;x<drawImage.Geo->dWidth;x++){
        int color = gdImageGetPixel(gdImage, x, y);
        if(color!=transpColor&&127!=gdImageAlpha(gdImage,color)){
          if(transparent)
            drawImage.setPixelTrueColor(x,y,gdImageRed(gdImage,color),gdImageGreen(gdImage,color),gdImageBlue(gdImage,color),255-gdImageAlpha(gdImage,color)*2);
          else
            drawImage.setPixelTrueColor(x,y,gdImageRed(gdImage,color),gdImageGreen(gdImage,color),gdImageBlue(gdImage,color));
        }
      }
    }
    gdImageDestroy(gdImage);
  }
  return 0;
#endif

  return 0;
}

int CImageDataWriter::init(CServerParams *srvParam,CDataSource *dataSource, int NrOfBands){
  if(mode!=0){CDBError("Already initialized");return 1;}
  this->srvParam=srvParam;
  if(_setTransparencyAndBGColor(this->srvParam,&drawImage)!=0)return 1;
  if(srvParam->imageMode==SERVERIMAGEMODE_RGBA||srvParam->Styles.indexOf("HQ")>0){
    drawImage.setTrueColor(true);
    drawImage.setAntiAliased(true);
  }
  
  //Set font location
  if(srvParam->cfg->Font.size()!=0){
    if(srvParam->cfg->Font[0]->attr.location.c_str()!=NULL){
      drawImage.setTTFFontLocation(srvParam->cfg->Font[0]->attr.location.c_str());
      
      if(srvParam->cfg->Font[0]->attr.size.c_str()!=NULL){
        CT::string fontSize=srvParam->cfg->Font[0]->attr.size.c_str();
        drawImage.setTTFFontSize(fontSize.toInt());
      }
      //CDBError("Font %s",srvParam->cfg->Font[0]->attr.location.c_str());
      //return 1;

    }else {
      CDBError("In <Font>, attribute \"location\" missing");
      return 1;
    }
  }
  
  

  
  //drawImage.setTrueColor(true);
  //drawImage.setAntiAliased(true);
  /*drawImage.setTrueColor(true);
  drawImage.setAntiAliased(true);*/
  mode=1;
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
    status = drawImage.createImage(LEGEND_WIDTH,LEGEND_HEIGHT);
    if(status != 0) return 1;
  }
  if(requestType==REQUEST_WMS_GETFEATUREINFO){
    drawImage.Geo->copy(srvParam->Geo);
    return 0;
  }
  
  //Create 6-8-5 palette for cascaded layer
  if(dataSource->dLayerType==CConfigReaderLayerTypeCascaded){
    status = drawImage.create685Palette();
    if(status != 0){
      CDBError("Unable to create standard 6-8-5 palette");
      return 1;
    }
  }

  //Create palette for internal WNS layer
  if(dataSource->dLayerType!=CConfigReaderLayerTypeCascaded){
    int dLegendIndex = initializeLegend(srvParam,dataSource);
    if(dLegendIndex==-1){
      CDBError("Unable to initialize legend for dataSource %s",dataSource->cfgLayer->Name[0]->value.c_str());
      return 1;
    }
    status = drawImage.createGDPalette(srvParam->cfg->Legend[dLegendIndex]);
    if(status != 0){
      CDBError("Unknown palette type for %s",srvParam->cfg->Legend[dLegendIndex]->attr.name.c_str());
      return 1;
    }
  }
  if(requestType==REQUEST_WMS_GETMAP){
    /*---------Add cascaded background map now------------------------------------*/
    //drawCascadedWMS("http://geoservices.knmi.nl/cgi-bin/worldmaps.cgi?","world_raster",false);
    //drawCascadedWMS("http://bhlbontw.knmi.nl/rcc/download/ensembles/cgi-bin/basemaps.cgi?","world_eca,country",true);
    /*----------------------------------------------------------------------------*/
  }
  return 0;
}
int CImageDataWriter::initializeLegend(CServerParams *srvParam,CDataSource *dataSource){
  if(srvParam==NULL){
    CDBError("srvParam==NULL");
    return -1;
  }
  int dLegendIndex=-1;
  if(_setTransparencyAndBGColor(srvParam,&drawImage)!=0){
    CDBError("Unable to do setTransparencyAndBGColor");
    return -1;
  }
  

  /* GET LEGEND INFORMATION From the layer itself*/
  //Get legend for the layer by using layers legend
  if(dataSource->cfgLayer->Legend.size()!=0){
    if(srvParam->requestType==REQUEST_WMS_GETMAP||srvParam->requestType==REQUEST_WMS_GETLEGENDGRAPHIC){
      for(size_t j=0;j<srvParam->cfg->Legend.size()&&dLegendIndex==-1;j++){
        if(dataSource->cfgLayer->Legend[0]->value.equals(
           srvParam->cfg->Legend[j]->attr.name.c_str())){
          dLegendIndex=j;
          break;
           }
      }
    }
  }
  //Get Legend settings
  if(dataSource->cfgLayer->Scale.size()>0){
    dataSource->legendScale=parseFloat(dataSource->cfgLayer->Scale[0]->value.c_str());
  }
  if(dataSource->cfgLayer->Offset.size()>0){
    dataSource->legendOffset=parseFloat(dataSource->cfgLayer->Offset[0]->value.c_str());
  }
   //When min and max are given, calculate the scale and offset according to min and max.
    if(dataSource->cfgLayer->Min.size()>0&&dataSource->cfgLayer->Max.size()>0){
      float min=parseFloat(dataSource->cfgLayer->Min[0]->value.c_str());
      float max=parseFloat(dataSource->cfgLayer->Max[0]->value.c_str());
      dataSource->legendScale=240/(max-min);
      dataSource->legendOffset=min*(-dataSource->legendScale);
    }
  if(dataSource->cfgLayer->Log.size()>0){
    dataSource->legendLog=parseFloat(dataSource->cfgLayer->Log[0]->value.c_str());
  }
  if(dataSource->cfgLayer->ValueRange.size()>0){
    dataSource->legendValueRange=1;
    dataSource->legendLowerRange=parseFloat(dataSource->cfgLayer->ValueRange[0]->attr.min.c_str());
    dataSource->legendUpperRange=parseFloat(dataSource->cfgLayer->ValueRange[0]->attr.max.c_str());
  }

  /* GET STYLE INFORMATION (OPTIONAL) */
  //Get style information (if provided)
  int dLayerStyleIndex=-1;//Equals styles=default (Nearest neighbour rendering)
  CT::string layerStyleName;
  
  //Default rendermethod is nearest
  renderMethod=nearest;

  if(CDataReader::autoConfigureStyles(dataSource)!=0){
    CDBError("Unable to autoconfigure styles");
    return 1;
  }
  
  //Try to find the default rendermethod from the layers style object
  if(dataSource->cfgLayer->Styles.size()==1){
    CT::string styles(dataSource->cfgLayer->Styles[0]->value.c_str());
    CT::string *layerstyles = styles.split(",");
    if(layerstyles->count>0){
      dLayerStyleIndex=0;
      layerStyleName.copy(&layerstyles[0]);
      if(srvParam->cfg->Style.size()>0){
        size_t j=0;
        for(j=0;j<srvParam->cfg->Style.size();j++){
          if(layerStyleName.equals(srvParam->cfg->Style[j]->attr.name.c_str())){
            break;
          }
        }
//        CDBDebug("j=%d",j);
        if(srvParam->cfg->Style[j]->RenderMethod.size()==1){
          CT::string renderMethodList(srvParam->cfg->Style[j]->RenderMethod[0]->value.c_str());
          CT::string *renderMethods = renderMethodList.split(",");
          if(renderMethods->count>0){
            renderMethod=getRenderMethodFromString(&renderMethods[0]);
          }
          delete[] renderMethods;
        }
      }
    }
    delete[] layerstyles ;
  }else {
    // No legend or styles are defined for this layer
    return dLegendIndex;
  }
  
  //If a rendermethod is given in the layers config, use the first one as default
  if(dataSource->cfgLayer->RenderMethod.size()==1){
    CT::string tmp(dataSource->cfgLayer->RenderMethod[0]->value.c_str());
    CT::string *renderMethodList = tmp.split(",");
    renderMethod=getRenderMethodFromString(&renderMethodList[0]);
    delete[] renderMethodList;
  }
  //styles=temperature/nearestneighbour
  //Get legend for the layer by using layers style
  CT::string *requestStyle=srvParam->Styles.split(",");
  bool isDefaultStyle=false;
  if(requestStyle->count>0){
    if(requestStyle[0].length()>0){
      if(requestStyle[0].equals("default")||requestStyle[0].equals("default/HQ")){
        isDefaultStyle=true;
      }
      //if(!requestStyle[0].equals("default"))
      {
        //requestStyle[0].copy("temperature/shadedcontour/HQ");
        if(dataSource->cfgLayer->Styles.size()==1){
          CT::string *legendStyle=NULL;
          if(srvParam->requestType==REQUEST_WMS_GETMAP||srvParam->requestType==REQUEST_WMS_GETLEGENDGRAPHIC){
            CT::string styles(dataSource->cfgLayer->Styles[0]->value.c_str());
            CT::string *layerstyles = styles.split(",");
            //If default, take the first style...
            if(isDefaultStyle){
              requestStyle[0].copy(&layerstyles[0]);
            }
            legendStyle=requestStyle[0].split("/");
            //if(!requestStyle[0].equals("default")){
              for(size_t j=0;j<layerstyles->count;j++){
                if(layerstyles[j].equals(legendStyle[0].c_str())){
                  layerStyleName.copy(&layerstyles[j]);
                  dLayerStyleIndex=j;
                  break;
                }
              }
            /*}else {
              dLayerStyleIndex=0;
              layerStyleName.copy(&layerstyles[dLayerStyleIndex]);
              delete[] legendStyle;
              legendStyle=layerStyleName.split("/");
              CDBDebug("layerStyleName %s",layerStyleName.c_str());
          }*/
            delete[] layerstyles;
          }
          if(legendStyle!=NULL){
            //Find the render method:
            if(legendStyle->count==2||legendStyle->count==3){
              if(legendStyle[1].length()>0){
                //CDBDebug("Rendermethod = %s",legendStyle[1].c_str());
                renderMethod=getRenderMethodFromString(&legendStyle[1]);
              }
            }
          }
          
          delete[] legendStyle;
        }
        
        
        
        //The style was not found in the layer
        if(dLayerStyleIndex==-1&&dataSource->dLayerType!=CConfigReaderLayerTypeStyled){
          if(isDefaultStyle==true){
            dLayerStyleIndex=0;
            return 0;
          }
          CDBError("Style '%s' was not found in the layers configuration",requestStyle[0].c_str());
          return -1;
        }
      }
    }
  }
  /*if(dLayerStyleIndex==-1){
        //If no style information like legend, scale and offset is not given in the config
        //Than the use the first index of the style element instead as style=default
  if(dLegendIndex==-1){
  if(dataSource->cfgLayer->Styles.size()==1){
  CT::string styles(dataSource->cfgLayer->Styles[0]->value.c_str());
  CT::string *layerstyles = styles.split(",");
  if(layerstyles->count>0){
          //renderMethod=nearest;
  dLayerStyleIndex=0;
  layerStyleName.copy(&layerstyles[0]);
}
  delete[] layerstyles;
}else{
  CDBError("No style information could be found in the configuration or in the layers configuration for layer %s",dataSource->getLayerName());
  return 1;
}
}
}*/
  delete[] requestStyle;
  
  int dConfigStyleIndex=-1;//Equals styles=default (Nearest neighbour rendering)
  //Get the servers style index from the name
  if(dLayerStyleIndex>-1){
    //CDBDebug("dLayerStyleIndex %d - %d",dLayerStyleIndex,srvParam->cfg->Style.size());
    //Get the servers style index from the name
//    CDBDebug("Found style %s in the layers config, now searching the servers config.",layerStyleName.c_str());

    if(srvParam->cfg->Style.size()>0){
      if(srvParam->requestType==REQUEST_WMS_GETMAP||srvParam->requestType==REQUEST_WMS_GETLEGENDGRAPHIC){
        for(size_t j=0;j<srvParam->cfg->Style.size()&&dConfigStyleIndex==-1;j++){
          if(layerStyleName.equals(srvParam->cfg->Style[j]->attr.name.c_str())){
            dConfigStyleIndex=j;
            break;
          }
        }
      }
    }else{
      CDBError("Styles is configured in the Layer, but no Style is configured in the servers configuration file");
      return -1;
    }
    if(dConfigStyleIndex!=-1){
      if(isDefaultStyle){
        CServerConfig::XMLE_Style * cfgStyle = srvParam->cfg->Style[dConfigStyleIndex];
        if(cfgStyle->RenderMethod.size()==1){
          //renderMethod=getRenderMethodFromString(&renderMethods[0]);
          CT::string rm=cfgStyle->RenderMethod[0]->value.c_str();
          CT::string *rms=rm.split(",");
          if(rms->count>0){
            CT::string *rms2=rms[0].split("/");
            renderMethod=getRenderMethodFromString(&rms2[0]);
            delete[] rms2;
          }
          delete[] rms;
          //CDBDebug("Found style '%s' with index %d in configuration",layerStyleName.c_str(),dConfigStyleIndex);
        }
      }
    }else{
      CDBError("Unable to find style '%s' in configuration",layerStyleName.c_str());
      return -1;
    }
  }
   //If no legend is yet found, and no style is provided, we assume styles=default
  if(dConfigStyleIndex==-1&&dLegendIndex==-1&&srvParam->cfg->Style.size()>0){
    dConfigStyleIndex=0;
  }
  
  if(dConfigStyleIndex!=-1&&srvParam->cfg->Style.size()>0){
    CServerConfig::XMLE_Style * cfgStyle = srvParam->cfg->Style[dConfigStyleIndex];
    /* GET LEGEND INFORMATION FROM STYLE OBJECT */
    //Get legend for the layer by using layers legend
    if(dLegendIndex==-1){
      dLegendIndex = -1;
      if(srvParam->requestType==REQUEST_WMS_GETMAP||srvParam->requestType==REQUEST_WMS_GETLEGENDGRAPHIC){
        for(size_t j=0;j<srvParam->cfg->Legend.size()&&dLegendIndex==-1;j++){
          if(cfgStyle->Legend.size()==0){
            CDBError("No Legend found for style %s",cfgStyle->attr.name.c_str());
            return -1;
          }
          if(cfgStyle->Legend[0]->value.equals(srvParam->cfg->Legend[j]->attr.name.c_str())){
            dLegendIndex=j;
            break;
          }
        }
      }
    }
   //Get Legend settings
    
    if(cfgStyle->Scale.size()>0)dataSource->legendScale=parseFloat(cfgStyle->Scale[0]->value.c_str());
    if(cfgStyle->Offset.size()>0)dataSource->legendOffset=parseFloat(cfgStyle->Offset[0]->value.c_str());

    //When min and max are given, calculate the scale and offset according to min and max.
    if(cfgStyle->Min.size()>0&&cfgStyle->Max.size()>0){
      float min=parseFloat(cfgStyle->Min[0]->value.c_str());
      float max=parseFloat(cfgStyle->Max[0]->value.c_str());
      dataSource->legendScale=240/(max-min);
      dataSource->legendOffset=min*(-dataSource->legendScale);
    }
    
    
    if(cfgStyle->Log.size()>0)dataSource->legendLog=parseFloat(cfgStyle->Log[0]->value.c_str());
    
    if(cfgStyle->ValueRange.size()>0){
      dataSource->legendValueRange=1;
      dataSource->legendLowerRange=parseFloat(cfgStyle->ValueRange[0]->attr.min.c_str());
      dataSource->legendUpperRange=parseFloat(cfgStyle->ValueRange[0]->attr.max.c_str());
    }
    
    //Legend settings can always be overriden in the layer itself!
    if(dataSource->cfgLayer->Scale.size()>0){
      dataSource->legendScale=parseFloat(dataSource->cfgLayer->Scale[0]->value.c_str());
    }
    if(dataSource->cfgLayer->Offset.size()>0){
      dataSource->legendOffset=parseFloat(dataSource->cfgLayer->Offset[0]->value.c_str());
    }
    
    //When min and max are given, calculate the scale and offset according to min and max.
    if(dataSource->cfgLayer->Min.size()>0&&dataSource->cfgLayer->Max.size()>0){
      CDBDebug("Found min and max in layer configuration");
      float min=parseFloat(dataSource->cfgLayer->Min[0]->value.c_str());
      float max=parseFloat(dataSource->cfgLayer->Max[0]->value.c_str());
      dataSource->legendScale=240/(max-min);
      dataSource->legendOffset=min*(-dataSource->legendScale);
    }
    
    
    
    if(dataSource->cfgLayer->Log.size()>0){
      dataSource->legendLog=parseFloat(dataSource->cfgLayer->Log[0]->value.c_str());
    }
    if(dataSource->cfgLayer->ValueRange.size()>0){
      dataSource->legendValueRange=1;
      dataSource->legendLowerRange=parseFloat(dataSource->cfgLayer->ValueRange[0]->attr.min.c_str());
      dataSource->legendUpperRange=parseFloat(dataSource->cfgLayer->ValueRange[0]->attr.max.c_str());
    }

    
    //Get contour info
    shadeInterval=0.0f;contourIntervalL=0.0f;contourIntervalH=0.0f;smoothingFilter=1;
    if(cfgStyle->ContourIntervalL.size()>0)
      contourIntervalL=parseFloat(cfgStyle->ContourIntervalL[0]->value.c_str());
    if(cfgStyle->ContourIntervalH.size()>0)
      contourIntervalH=parseFloat(cfgStyle->ContourIntervalH[0]->value.c_str());
    shadeInterval=contourIntervalL;
    if(cfgStyle->ShadeInterval.size()>0)
      shadeInterval=parseFloat(cfgStyle->ShadeInterval[0]->value.c_str());
    if(cfgStyle->SmoothingFilter.size()>0)
      smoothingFilter=parseInt(cfgStyle->SmoothingFilter[0]->value.c_str());
    //Get contourtextinfo
    textScaleFactor=1.0f;textOffsetFactor=0.0f;
    
    if(cfgStyle->ContourText.size()>0){
      if(cfgStyle->ContourText[0]->attr.scale.c_str()!=NULL){
        textScaleFactor=parseFloat(cfgStyle->ContourText[0]->attr.scale.c_str());
      }
      if(cfgStyle->ContourText[0]->attr.offset.c_str()!=NULL)
        textOffsetFactor=parseFloat(cfgStyle->ContourText[0]->attr.offset.c_str());
    }
  }
  if(contourIntervalL<=0.0f||contourIntervalH<=0.0f){
    if(renderMethod==contour||
       renderMethod==bilinearcontour||
       renderMethod==nearestcontour){
      renderMethod=nearest;
       }
  }
  
  if(dLegendIndex==-1){
    CDBError("No legend found for layer %s", dataSource->getLayerName());
    return -1;
  }

  return dLegendIndex;
}
int CImageDataWriter::createLegend(CDataSource *dataSource){
  status = createLegend(dataSource,&drawImage);
  //if(status != 0)return 1;
  return status;//end();
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
int CImageDataWriter::getFeatureInfo(CDataSource *dataSource,int dX,int dY){
  // Create a new getFeatureInfoResult object and push it into the vector.
  GetFeatureInfoResult  *getFeatureInfoResult = new GetFeatureInfoResult();
  getFeatureInfoResultList.push_back(getFeatureInfoResult);
    
//  status  = imageWarper.getFeatureInfo(&getFeatureInfoHeader,&temp,dataSource,drawImage.Geo,dX,dY);
  
  //int CImageWarper::getFeatureInfo(CT::string *Header,CT::string *Result,CDataSource *dataSource,CGeoParams *GeoDest,int dX, int dY){

  char szTemp[MAX_STR_LEN+1];
  if(dataSource==NULL){
    CDBError("dataSource == NULL");
    return 1;
  }
  int status;
  CDataReader reader;
  
  CT::string cacheLocation;srvParam->getCacheDirectory(&cacheLocation);
  
  status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_ALL,cacheLocation.c_str());
  if(status!=0){
    CDBError("Could not open file: %s",dataSource->getFileName());
    return 1;
  }
  status = imageWarper.initreproj(dataSource,drawImage.Geo,&srvParam->cfg->Projection);
  if(status!=0){
    CDBError("initreproj failed");
    reader.close();
    return 1;
  }
  
  //getFeatureInfoHeader.copy("");
  double x,y,sx,sy,CoordX,CoordY;
  int imx,imy;
  sx=dX;
  sy=dY;
  //printf("Native XY (%f : %f)\n<br>",sx,sy);
  x=double(sx)/double(drawImage.Geo->dWidth);
  y=double(sy)/double(drawImage.Geo->dHeight);
  x*=(drawImage.Geo->dfBBOX[2]-drawImage.Geo->dfBBOX[0]);
  y*=(drawImage.Geo->dfBBOX[1]-drawImage.Geo->dfBBOX[3]);
  x+=drawImage.Geo->dfBBOX[0];
  y+=drawImage.Geo->dfBBOX[3];
  //printf("%s%c%c\n","Content-Type:text/html",13,10);
  CoordX=x;
  CoordY=y;
  imageWarper.reprojpoint(x,y);
  
  double nativeCoordX=x;
  double nativeCoordY=y;
  //printf("Projected XY (%f : %f)\n<br>",x,y);
  x-=dataSource->dfBBOX[0];
  y-=dataSource->dfBBOX[1];
  x/=(dataSource->dfBBOX[2]-dataSource->dfBBOX[0]);
  y/=(dataSource->dfBBOX[3]-dataSource->dfBBOX[1]);
  x*=double(dataSource->dWidth);
  y*=double(dataSource->dHeight);
  imx=(int)x;
  imy=dataSource->dHeight-(int)y-1;
  // printf("Image XY (%d : %d)\n<br>",imx,imy);
  //Get lat/lon

  // Projections coordinates in latlon
  getFeatureInfoResult->lon_coordinate=CoordX;
  getFeatureInfoResult->lat_coordinate=CoordY;

  
  imageWarper.reprojToLatLon(getFeatureInfoResult->lon_coordinate,getFeatureInfoResult->lat_coordinate);
  imageWarper.closereproj();

  // Pixel X and Y on the image
  getFeatureInfoResult->x_imagePixel=dX;
  getFeatureInfoResult->y_imagePixel=dY;
 
  // Projection coordinates X and Y on the image
  getFeatureInfoResult->x_imageCoordinate=CoordX;
  getFeatureInfoResult->y_imageCoordinate=CoordY;
  
  // Projection coordinates X and Y in the raster
  getFeatureInfoResult->x_rasterCoordinate=nativeCoordX;
  getFeatureInfoResult->y_rasterCoordinate=nativeCoordY;
  
  // Pixel X and Y on the raster
  getFeatureInfoResult->x_rasterIndex=imx;
  getFeatureInfoResult->y_rasterIndex=imy;
  

  //Copy layer name
  getFeatureInfoResult->layerName.copy(&dataSource->layerName);
  
  //TODO find raster projection units and find image projection units.
  
  //Retrieve variable names
  for(size_t j=0;j<dataSource->dataObject.size();j++){
    //Create a new element and at it to the elements list.
    GetFeatureInfoResult::Element * element = new GetFeatureInfoResult::Element();
    getFeatureInfoResult->elements.push_back(element);

    //Get variable name
    element->var_name.copy(&dataSource->dataObject[j]->variableName);
    //Get variable units
    element->units.copy(&dataSource->dataObject[j]->units);

    //Get variable standard name
    CDF::Attribute * attr_standard_name=dataSource->dataObject[j]->cdfVariable->getAttributeNE("standard_name");
    if(attr_standard_name!=NULL){
      CT::string standardName;attr_standard_name->getDataAsString(&standardName);
      element->standard_name.copy(&standardName);
      // Make a more clean standard name.
      standardName.replace("_"," ");standardName.replace(" status flag","");
      element->feature_name.copy(&standardName);
    }else element->standard_name.copy(&element->var_name);
    
    // Get variable long name
    CDF::Attribute * attr_long_name=dataSource->dataObject[j]->cdfVariable->getAttributeNE("long_name");
    if(attr_long_name!=NULL){
      attr_long_name->getDataAsString(&element->long_name);
    }else element->long_name.copy(&element->var_name);
    
    // Assign CDF::Variable Pointer
    element->variable = dataSource->dataObject[j]->cdfVariable;
    element->value="";
    status = reader.getTimeString(szTemp);
    if(status != 0){
      element->time.print("Time error: %d: ",status);
    }
    else{
      element->time=szTemp;
    }
  }
  
  // Retrieve corresponding values.
  if(imx>=0&&imy>=0&&imx<dataSource->dWidth&&imy<dataSource->dHeight){
    for(size_t j=0;j<dataSource->dataObject.size();j++){
      GetFeatureInfoResult::Element * element=getFeatureInfoResult->elements[j];
      size_t ptr=imx+imy*dataSource->dWidth;
      double pixel=convertValue(dataSource->dataObject[j]->dataType,dataSource->dataObject[j]->data,ptr);
      //Get time
      
      //Check wether this is a NoData value:
      if((pixel!=dataSource->dataObject[j]->dfNodataValue&&dataSource->dataObject[j]->hasNodataValue==true&&pixel==pixel)||dataSource->dataObject[0]->hasNodataValue==false){
        if(dataSource->dataObject[j]->hasStatusFlag){
          //Add status flag
          element->value.print("%s (%d)",dataSource->dataObject[j]->getFlagMeaning(pixel),(int)pixel);
          element->units="";
        }else{
          //Add raster value
          floatToString(szTemp,MAX_STR_LEN,pixel);
          element->value=szTemp;
        }
      }
      else {
        element->value="nodata";
      }
    }
  }
  reader.close();
  
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
  
  int status;
  //Open the data of this dataSource
  CDBDebug("opening:");
  CDataReader reader;
  CDBDebug("!");
  CT::string cacheLocation;srvParam->getCacheDirectory(&cacheLocation);
  CDBDebug("!");
  status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_ALL,cacheLocation.c_str());
  CDBDebug("!");
  if(status!=0){
    CDBError("Could not open file: %s",dataSource->getFileName());
    return 1;
  }
  CDBDebug("opened");
  //Initialize projection algorithm
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
  if(renderMethod==nearest||renderMethod==nearestcontour){
    imageWarperRenderer = new CImgWarpNearestNeighbour();
    imageWarperRenderer->render(&imageWarper,dataSource,drawImage);
    delete imageWarperRenderer;
  }
  
  if(renderMethod==nearestcontour||
     renderMethod==bilinear||
     renderMethod==bilinearcontour||
     renderMethod==contour||
     renderMethod==vector||
     renderMethod==vectorcontour||
     renderMethod==shaded||
     renderMethod==contourshaded||
     renderMethod==vectorcontourshaded
    )
  {
    imageWarperRenderer = new CImgWarpBilinear();
    CT::string bilinearSettings;
    bool drawMap=false;
    bool drawContour=false;
    bool drawVector=false;
    bool drawShaded=false;
    if(renderMethod==bilinear||renderMethod==bilinearcontour)drawMap=true;
    if(renderMethod==bilinearcontour)drawContour=true;
    if(renderMethod==nearestcontour)drawContour=true;
    if(renderMethod==contour||renderMethod==contourshaded||renderMethod==vectorcontour||renderMethod==vectorcontourshaded)drawContour=true;
    if(renderMethod==vector||renderMethod==vectorcontour||renderMethod==vectorcontourshaded)drawVector=true;
    if(renderMethod==shaded||renderMethod==contourshaded||renderMethod==vectorcontourshaded)drawShaded=true;
    if(drawMap==true)bilinearSettings.printconcat("drawMap=true;");
    if(drawVector==true)bilinearSettings.printconcat("drawVector=true;");
    if(drawShaded==true)bilinearSettings.printconcat("drawShaded=true;");
    if(drawContour==true)bilinearSettings.printconcat("drawContour=true;");
    bilinearSettings.printconcat("smoothingFilter=%d;",smoothingFilter);
    if(drawContour==true||drawShaded==true){
      bilinearSettings.printconcat("shadeInterval=%f;contourSmallInterval=%f;contourBigInterval=%f;",
                                   shadeInterval,contourIntervalL,contourIntervalH);
      bilinearSettings.printconcat("textScaleFactor=%f;textOffsetFactor=%f;",
                                   textScaleFactor,textOffsetFactor);
    }
//    CDBDebug("bilinearSettings.c_str() %s",bilinearSettings.c_str());
    imageWarperRenderer->set(bilinearSettings.c_str());
    imageWarperRenderer->render(&imageWarper,dataSource,drawImage);
    delete imageWarperRenderer;
  }
  
#ifdef MEASURETIME
  StopWatch_Stop("warp finished");
#endif
  CDBDebug("imageWarper.closereproj();");
  imageWarper.closereproj();
  CDBDebug("reader.close();");
  reader.close();
  CDBDebug("Ok");
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
      CT::string cacheLocation;srvParam->getCacheDirectory(&cacheLocation);
      status = reader->open(dataSource,CNETCDFREADER_MODE_OPEN_ALL,cacheLocation.c_str());
      CDBDebug("Opening %s",dataSource->getFileName());
      if(status!=0){CDBError("Could not open file: %s",dataSource->getFileName());  hasFailed=true; }
    }
    //Initialize projection algorithm
    dataSource=dataSources[0];
    
    if(hasFailed==false){
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
      
      CT::string *layerStyles = srvParam->Styles.split(",");
      CT::string style;
//      bool errorOccured=false;
      for(size_t j=0;j<dataSources.size();j++){
        size_t numberOfValues = 1;
        CT::string *_style = layerStyles[j].split("|");
        style.copy(&_style[0]);
        CDBDebug("STYLE == %s",style.c_str());
        if(j==0){
          //Find the conditional expression for the first layer (the boolean map)
          CT::string *conditionals = style.split("_");
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
          CT::string *LH=exprVal.split("_and_");
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
            
            pixel[j] = convertValue(dsj->dataObject[0]->dataType,dsj->dataObject[0]->data,ptrj);
            
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
          setValue(dataSources[0]->dataObject[0]->dataType,dataSources[0]->dataObject[0]->data,ptr,pixel[0]);
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
        size_t len=strlen(dataSource->cfgLayer->ImageText[0]->value.c_str());
        drawImage.setTextStroke(dataSource->cfgLayer->ImageText[0]->value.c_str(),
                                len,
                                int(drawImage.Geo->dWidth/2-len*3),
                                drawImage.Geo->dHeight-16,240,254,-1);
      }
    }
  
  }
  return 0;
}
int CImageDataWriter::addData(std::vector <CDataSource*>&dataSources){
   CDBDebug("addData");
  int status = 0;
  
  if(animation==1&&nrImagesAdded>0){
    drawImage.addImage(25);
  }
  //CDBDebug("Draw Data");
  nrImagesAdded++;
  // draw the Image
  //drawCascadedWMS("http://geoservices.knmi.nl/cgi-bin/restricted/MODIS_Netherlands.cgi?","modis_250m_netherlands_8bit",true);
  CDBDebug("Draw data. dataSources.size() =  %d",dataSources.size());
  
  for(size_t j=0;j<dataSources.size();j++){
    CDataSource *dataSource=dataSources[j];

    if(dataSource->dLayerType==CConfigReaderLayerTypeCascaded){
      CDBDebug("Drawing cascaded WMS");
      if(dataSource->cfgLayer->WMSLayer.size()!=1){CDBError("WMSLayer not configured.");return 1;}
    
      status = drawCascadedWMS(dataSource->cfgLayer->WMSLayer[0]->attr.service.c_str(),dataSource->cfgLayer->WMSLayer[0]->attr.layer.c_str(),true);
      if(status!=0){
        CDBError("drawCascadedWMS for layer %s failed",dataSource->layerName.c_str());
      }
    }
      
    if(dataSource->dLayerType!=CConfigReaderLayerTypeCascaded){
      if(j!=0){if(initializeLegend(srvParam,dataSource)==-1)return 1;}
      CDBDebug("Start warping");
      status = warpImage(dataSource,&drawImage);
      CDBDebug("Finished warping");
      if(status != 0){
        CDBError("warpImage for layer %s failed",dataSource->layerName.c_str());
        return status;
      }
      
      if(j==dataSources.size()-1){
        if(status == 0){
          if(dataSource->cfgLayer->ImageText.size()>0){
            size_t len=strlen(dataSource->cfgLayer->ImageText[0]->value.c_str());
            drawImage.setTextStroke(dataSource->cfgLayer->ImageText[0]->value.c_str(),
                                    len,
                                    int(drawImage.Geo->dWidth/2-len*3),
                                    drawImage.Geo->dHeight-16,240,254,-1);
          }
        }
      }
    }
  }
  
  //drawCascadedWMS("http://bhlbontw.knmi.nl/rcc/download/ensembles/cgi-bin/basemaps.cgi?","country_lines",true);
  return status;
}
int CImageDataWriter::end(){
  if(mode==0){CDBError("Not initialized");return 1;}
  if(mode==2){CDBError("Already finished");return 1;}
  mode=2;
  if(requestType==REQUEST_WMS_GETFEATUREINFO){
    enum ResultFormats {textplain,texthtml,textxml, applicationvndogcgml};
    ResultFormats resultFormat=texthtml;
    
    if(srvParam->InfoFormat.equals("text/plain"))resultFormat=textplain;
    if(srvParam->InfoFormat.equals("text/xml"))resultFormat=textxml;
    if(srvParam->InfoFormat.equals("application/vnd.ogc.gml"))resultFormat=textxml;//applicationvndogcgml;
    

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
          resultHTML.printconcat("coordinates (%0.2f , %0.2f)<br>\n",g->x_imageCoordinate,g->y_imageCoordinate);
        }else{
          resultHTML.printconcat("coordinates (%0.2f , %0.2f)\n",g->x_imageCoordinate,g->y_imageCoordinate);
        }
        
      
        for(size_t elNR=0;elNR<g->elements.size();elNR++){
          GetFeatureInfoResult::Element * e=g->elements[elNR];
          if(g->elements.size()>1){
            resultHTML.printconcat("%d: ",elNR);
          }
          if(resultFormat==texthtml){
            resultHTML.printconcat("<b>%s</b> - %s<br>\n",e->var_name.c_str(),e->feature_name.c_str());
          }else{
            resultHTML.printconcat("%s - %s\n",e->var_name.c_str(),e->feature_name.c_str());
          }
        }
        if(resultFormat==texthtml)resultHTML.printconcat("<hr>\n");
      
        for(size_t j=0;j<getFeatureInfoResultList.size();j++){
          GetFeatureInfoResult *g = getFeatureInfoResultList[j];
          for(size_t elNR=0;elNR<g->elements.size();elNR++){
            GetFeatureInfoResult::Element * e=g->elements[elNR];
            if(g->elements.size()>1){
              resultHTML.printconcat("%d: ",elNR);
            }
            if(e->value.length()>0){
              if(resultFormat==texthtml){
                resultHTML.printconcat("%s: <b>%s</b>",e->time.c_str(),e->value.c_str());}else{
                resultHTML.printconcat("%s: %s",e->time.c_str(),e->value.c_str());             
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
    
    if(resultFormat==textxml){
      
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
          layerName.replace(" ","_");
          layerName.replace("/","_");
          
          resultXML.printconcat("  <%s_layer>\n",layerName.c_str());
          for(size_t elNR=0;elNR<g->elements.size();elNR++){
            GetFeatureInfoResult::Element * e=g->elements[elNR];
            CT::string featureName=e->standard_name.c_str();featureName.replace(" ","_");
            resultXML.printconcat("    <%s_feature>\n",featureName.c_str());
            resultXML.printconcat("      <gml:location>\n");
            
            resultXML.printconcat("        <gml:Point srsName=\"EPSG:4326\">\n");
            resultXML.printconcat("          <gml:pos>%f,%f</gml:pos>\n",g->lon_coordinate,g->lat_coordinate);
            resultXML.printconcat("        </gml:Point>\n");
            
            resultXML.printconcat("        <gml:Point srsName=\"%s\">\n",srvParam->Geo->CRS.c_str());
            resultXML.printconcat("          <gml:pos>%f %f</gml:pos>\n",g->x_imageCoordinate,g->y_imageCoordinate);
            resultXML.printconcat("        </gml:Point>\n");

            resultXML.printconcat("        <gml:Point srsName=\"%s\">\n","image:xyindices");
            resultXML.printconcat("          <gml:pos>%d %d</gml:pos>\n",g->x_imagePixel,g->y_imagePixel);
            resultXML.printconcat("        </gml:Point>\n");

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
            resultXML.printconcat("      <Dimension name=\"time\">%s</Dimension>\n",e->time.c_str());
            
            
            resultXML.printconcat("    </%s_feature>\n",featureName.c_str());
          }
          resultXML.printconcat("  </%s_layer>\n",layerName.c_str());
        }
      }
      resultXML.printconcat(" </GMLOutput>\n");
      resetErrors();
      printf("%s",resultXML.c_str());
      
    }
    for(size_t j=0;j<getFeatureInfoResultList.size();j++){delete getFeatureInfoResultList[j];getFeatureInfoResultList[j]=NULL; } getFeatureInfoResultList.clear();
  }
  if(requestType!=REQUEST_WMS_GETMAP&&requestType!=REQUEST_WMS_GETLEGENDGRAPHIC)return 0;
  
  
  //Output WMS getmap results
  if(errorsOccured()){
    return 1;
  }
  
  //Animation image:
  if(animation==1){
    drawImage.addImage(100);
    drawImage.endAnimation();
    return 0;
  }

  //Static image
  int status=0;
  if(srvParam->imageFormat==IMAGEFORMAT_IMAGEPNG8||srvParam->imageFormat==IMAGEFORMAT_IMAGEPNG32){
    printf("%s%c%c\n","Content-Type:image/png",13,10);
    status=drawImage.printImagePng();
  }else if(srvParam->imageFormat==IMAGEFORMAT_IMAGEGIF){
    printf("%s%c%c\n","Content-Type:image/gif",13,10);
    status=drawImage.printImageGif();
  }else {
    printf("%s%c%c\n","Content-Type:image/png",13,10);
    status=drawImage.printImagePng();
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

int CImageDataWriter::createLegend(CDataSource *dataSource,CDrawImage *drawImage){
  int legendPositiveUp = 1;
  int dH=0;
  float cbH=LEGEND_HEIGHT-13-13;
  float cbW=LEGEND_WIDTH/7;
  char szTemp[256];
  CDataReader reader;
  
  //Get the cache location
  CT::string cacheLocation;srvParam->getCacheDirectory(&cacheLocation);
  /*if(renderMethod!=contourshaded&&renderMethod!=shaded&&renderMethod!=contour){
     //When the scale factor is zero (0.0f) we need to open the data too, because we want to estimate min/max in this case.
     // When the scale factor is given, we only need to open the header, for displaying the units.
    if(dataSource->legendScale==0.0f){
      status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_ALL,cacheLocation.c_str());
    }else{
      status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_HEADER,cacheLocation.c_str());
    }
  }else {
    status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_ALL,cacheLocation.c_str());
  }*/
  
  
  if(renderMethod==contourshaded||renderMethod==shaded||renderMethod==contour){
    //We need to open all the data, because we need to estimate min/max for legend drawing
     status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_ALL,cacheLocation.c_str());
  }else {
    //When the scale factor is zero (0.0f) we need to open the data too, because we want to estimate min/max in this case.
    // When the scale factor is given, we only need to open the header, for displaying the units.
    if(dataSource->legendScale==0.0f){
      status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_ALL,cacheLocation.c_str());
    }else{
      status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_HEADER,cacheLocation.c_str());
    }
  }
  //Create a legend based on status flags.
  if(dataSource->dataObject[0]->hasStatusFlag){
    dH=30;
    cbW=LEGEND_WIDTH/5;cbW/=3;cbW*=3;cbW+=3;
    cbH=LEGEND_HEIGHT-13-13-30;
   
    size_t numFlags=dataSource->dataObject[0]->statusFlagList.size();
    for(size_t j=0;j<numFlags;j++){
      float y=j*18+(cbH-numFlags*18+8);
      double value=dataSource->dataObject[0]->statusFlagList[j]->value;
      int c=getColorIndexForValue(dataSource,value);
      drawImage->rectangle(5,int(2+dH+y),(int)cbW+9,(int)y+2+dH+12,c,248);
      CT::string legendMessage;
      legendMessage.print("%d %s",(int)dataSource->dataObject[0]->statusFlagList[j]->value,dataSource->dataObject[0]->statusFlagList[j]->meaning.c_str());
      drawImage->setText(legendMessage.c_str(),legendMessage.length(),(int)cbW+16,(int)y+dH+1,248,-1);  
    }
  }else if(renderMethod!=contourshaded&&renderMethod!=shaded&&renderMethod!=contour){
    dH=30;
    cbW=LEGEND_WIDTH/5;
    cbH=LEGEND_HEIGHT-13-13-30;
    
    for(int j=0;j<cbH;j++){
      for(int i=0;i<cbW+2;i++){
        float c=(float(cbH*legendPositiveUp-j)/cbH)*240.0f;
        drawImage->setPixelIndexed(i+1,j+7+dH,int(c+1));
      }
    }
    drawImage->rectangle(2,7+dH,(int)cbW+3,(int)cbH+7+dH,248);
    float classes=8;
    for(int j=0;j<=classes;j++){
      float c=((float(classes*legendPositiveUp-j)/classes))*(cbH);
      float v=((float(j)/classes))*(240.0f);
      drawImage->line((int)cbW-4,(int)c+7+dH,(int)cbW+6,(int)c+7+dH,248);
  
      v-=dataSource->legendOffset;
      v/=dataSource->legendScale;
      if(dataSource->legendLog!=0){
        v=pow(dataSource->legendLog,v);
      }
      //snprintf(szTemp,255,"%0.2f",v);
      //floatToString(szTemp,255,v);
      // Rounding of legend text depends on legendInterval
      /*float legendInterval=0.5;
      if(legendInterval==0)return 1;
      int textRounding=0;
      if(legendInterval!=0){
        float fracPart=legendInterval-int(legendInterval);
        textRounding=-int(log10(fracPart)-0.9999999f);
      }
        if(textRounding<=0)sprintf(szTemp,"%d",int(v+0.5));
        if(textRounding==1)sprintf(szTemp,"%2.1f",v);
        if(textRounding==2)sprintf(szTemp,"%2.2f",v);
        if(textRounding==3)sprintf(szTemp,"%2.3f",v);
        if(textRounding==4)sprintf(szTemp,"%2.4f",v);
        if(textRounding==5)sprintf(szTemp,"%2.5f",v);
        if(textRounding==5)sprintf(szTemp,"%2.6f",v);
        if(textRounding>6)sprintf(szTemp,"%f",v);*/
      floatToString(szTemp,255,v);
      int l=strlen(szTemp);
      drawImage->setText(szTemp,l,(int)cbW+12,(int)c+dH,248,-1);
    }
  }else{

    // We always need to have the min/max of the data
    // Always to show only the occuring data values in the legend,
    // and in some cases to stretch the colors over min max
    if(dataSource->statistics==NULL){
      dataSource->statistics = new CDataSource::Statistics();
      dataSource->statistics->calculate(dataSource);
    }
    
    //Get the min/max values
    float minValue=(float)dataSource->statistics->getMinimum();
    float maxValue=(float)dataSource->statistics->getMaximum();

    //Calculate the number of classes
    float legendInterval=shadeInterval;
    int numClasses=(int((maxValue-minValue)/legendInterval));
    
    /*
    // and reduce the number of classes when required...
    if(!dataSource->stretchMinMax){
      while(numClasses>15){
        legendInterval*=2;//(maxValue-minValue);
        numClasses=int((maxValue-minValue)/legendInterval);
      }
    }*/
    
    CDBDebug("minValue=%f maxValue=%f",minValue,maxValue);
CDBDebug("scale=%f offset=%f",dataSource->legendScale,dataSource->legendOffset);    
    float iMin=convertValueToClass(minValue,legendInterval);
    float iMax=convertValueToClass(maxValue,legendInterval)+legendInterval;
CDBDebug("iMin=%f iMax=%f",iMin,iMax);

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
    CDBDebug("scale=%f offset=%f",dataSource->legendScale,dataSource->legendOffset);
    //floatToString(szTemp,255,iMax);
   
    
    numClasses=int((iMax-iMin)/legendInterval);
    //if(numClasses<=2)numClasses=2;
    
    //CDBDebug("numClasses = %d",numClasses);
    int classSizeY=(180/(numClasses));
    if(classSizeY>14)classSizeY=14;
    //for(float j=iMax+legendInterval;j>=iMin;j=j-legendInterval){
    
    // Rounding of legend text depends on legendInterval
    if(legendInterval==0)return 1;
    int textRounding=0;
    if(legendInterval!=0){
      float fracPart=legendInterval-int(legendInterval);
      textRounding=-int(log10(fracPart)-0.9999999f);
    }
    
      
    int classNr=0;
    for(float j=iMin;j<iMax+legendInterval;j=j+legendInterval){
      float v=j;
      
      //int y=getColorIndexForValue(dataSource,v);
      
      
      //int y2=getColorIndexForValue(dataSource,(v+legendInterval));
      int cY= int((cbH-(classNr-5))+6);
      
      int dDistanceBetweenClasses=(classSizeY-10);
      if(dDistanceBetweenClasses<4)dDistanceBetweenClasses=0;
      if(dDistanceBetweenClasses>4)dDistanceBetweenClasses=4;
      cY-=dDistanceBetweenClasses;
      int cY2=int((cbH-(classNr+classSizeY-5))+6);
      classNr+=classSizeY;
      //cY*=numClasses;
      //cY2*=numClasses;
      if(j<iMax)
      {
        int y=getColorIndexForValue(dataSource,v);
        drawImage->rectangle(4,cY2,int(cbW)+7,cY,(y),248);
        if(textRounding<=0)sprintf(szTemp,"%2.1f - %2.1f",v,v+legendInterval);
        if(textRounding==1)sprintf(szTemp,"%2.1f - %2.1f",v,v+legendInterval);
        if(textRounding==2)sprintf(szTemp,"%2.2f - %2.2f",v,v+legendInterval);
        if(textRounding==3)sprintf(szTemp,"%2.3f - %2.3f",v,v+legendInterval);
        if(textRounding==4)sprintf(szTemp,"%2.4f - %2.4f",v,v+legendInterval);
        if(textRounding==5)sprintf(szTemp,"%2.5f - %2.5f",v,v+legendInterval);
        if(textRounding==5)sprintf(szTemp,"%2.6f - %2.6f",v,v+legendInterval);
        if(textRounding>6)sprintf(szTemp,"%f - %f",v,v+legendInterval);
        //CT::string    //floatToString(szTemp,255,v);
        int l=strlen(szTemp);
        drawImage->setText(szTemp,l,(int)cbW+14,((cY+cY2)/2)-7,248,-1);
      }

    }
    
    /*for(int j=0;j<240;j++){
    float v=getValueForColorIndex(dataSource,j);
 
    float c=(float(cbH*legendPositiveUp-j)/cbH)*240.0f;
    if((int(v*1000)%1000)==0){
        
    drawImage->line((int)cbW-4,(int)c+7+dH,(int)cbW+6,(int)c+7+dH,248);
    floatToString(szTemp,255,v);
    int l=strlen(szTemp);
    drawImage->setText(szTemp,l,(int)cbW+8,(int)c+dH,248,0);
  }
      
  }*/
    
  }
  

  reader.close();
  if(dataSource->dataObject[0]->hasStatusFlag==false){
    //Get units
    CT::string units;
    if(status==0){
      if(dataSource->dataObject[0]->units.length()>0){
        units.concat(&dataSource->dataObject[0]->units);
      }
    }
    //Print the units under the legend:
    if(units.length()>0)drawImage->setText(units.c_str(),units.length(),5,LEGEND_HEIGHT-15,248,-1);
  }else{
    CT::string units="status flag";
    drawImage->setText(units.c_str(),units.length(),5,LEGEND_HEIGHT-15,248,-1);
  }
  return 0;
}
