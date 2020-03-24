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

#include <set>

#include <vector>
#include <sstream>
#include <algorithm>
#include "CCreateScaleBar.h"
#include "CCreateLegend.h"

#include "CImageDataWriter.h"
#include "CMakeJSONTimeSeries.h"
#include "CMakeEProfile.h"
#include "CReporter.h"
#ifndef M_PI
#define M_PI            3.14159265358979323846  // pi 
#endif

#ifndef  rad2deg
#define  rad2deg  (180./M_PI)   // conversion for rad to deg 
#endif

#ifndef  deg2rad
#define  deg2rad  (M_PI/180.)   // conversion for deg to rad 
#endif

CT::string months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};


//#define CIMAGEDATAWRITER_DEBUG
//  #define MEASURETIME



void doJacoIntoLatLon(double &u, double &v, double lo, double la, float deltaX, float deltaY, CImageWarper *warper);
void rotateUvNorth(double &u, double &v, double rlo, double rla, float deltaX, float deltaY, CImageWarper *warper);

std::map<std::string,CImageDataWriter::ProjCacheInfo> CImageDataWriter::projCacheMap;
std::map<std::string,CImageDataWriter::ProjCacheInfo>::iterator CImageDataWriter::projCacheIter;

CImageDataWriter::ProjCacheInfo CImageDataWriter::GetProjInfo(CT::string ckey, CDrawImage *drawImage, CDataSource *dataSource,CImageWarper *imageWarper,CServerParams *srvParam,int dX,int dY){
  std::string key=ckey.c_str();
  ProjCacheInfo projCacheInfo ;
  
  #ifdef MEASURETIME
  StopWatch_Stop("projCacheInfo");
  #endif
  
  //bool projInvertedFirst = true;
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
    projCacheInfo.isOutsideBBOX = false;
    #ifdef CIMAGEDATAWRITER_DEBUG  
    CDBDebug("initreproj with proj string '%s'",dataSource->nativeProj4.c_str());
    #endif
    int status = imageWarper->initreproj(dataSource,drawImage->Geo,&srvParam->cfg->Projection);
    if(status!=0){CDBError("initreproj failed");throw(1);  }

    //getFeatureInfoHeader.copy("");
    double x,y,sx,sy;
    sx=dX;
    sy=dY;

    x=double(sx)/double(drawImage->Geo->dWidth);
    y=double(sy)/double(drawImage->Geo->dHeight);
    x*=(drawImage->Geo->dfBBOX[2]-drawImage->Geo->dfBBOX[0]);
    y*=(drawImage->Geo->dfBBOX[1]-drawImage->Geo->dfBBOX[3]);
    x+=drawImage->Geo->dfBBOX[0];
    y+=drawImage->Geo->dfBBOX[3];
    
    projCacheInfo.isOutsideBBOX = false;
    //projInvertedFirst = false;

    //else
    {
      
      double y1=dataSource->dfBBOX[1];
      double y2=dataSource->dfBBOX[3];
      double x1=dataSource->dfBBOX[0];
      double x2=dataSource->dfBBOX[2];
      if(y2<y1){
        if(y1>-360&&y2<360&&x1>-720&&x2<720){
          //projInvertedFirst = true;
          double checkBBOX[4];
            for(int j=0;j<4;j++)checkBBOX[j]=dataSource->dfBBOX[j];
            
            //CDBDebug("Current BBOX:  %f %f %f %f",dataSource->dfBBOX[0],dataSource->dfBBOX[1],dataSource->dfBBOX[2],dataSource->dfBBOX[3]);
            bool hasError = false;
            if(imageWarper->reprojpoint_inv(checkBBOX[0],checkBBOX[1])!=0)hasError=true;  
            if(imageWarper->reprojpoint(checkBBOX[0],checkBBOX[1])!=0)hasError=true;  
            
            if(imageWarper->reprojpoint_inv(checkBBOX[2],checkBBOX[3])!=0)hasError=true;  
            if(imageWarper->reprojpoint(checkBBOX[2],checkBBOX[3])!=0)hasError=true;  
            
            if(checkBBOX[2]+200<dataSource->dfBBOX[2]){
              checkBBOX[2]+=360;
            }
            
            if(hasError == false){
              for(int j=0;j<4;j++)dataSource->dfBBOX[j] = checkBBOX[j];
            }
            
        }
      }
    }
    //while(sx>180)sx-=360;

    projCacheInfo.CoordX=x;
    projCacheInfo.CoordY=y;

//     CDBDebug("X is : %f Y is: %f",x,y);
    
    imageWarper->reprojpoint(x,y);
    if(  CGeoParams::isLonLatProjection(&dataSource->nativeProj4)){     
//       CDBDebug("Is latlon %f %f",dataSource->dfBBOX[0],dataSource->dfBBOX[2]);
      //if(dataSource->dfBBOX[2]>180||dataSource->dfBBOX[0]<-180){
//         CDBDebug("X is : %f %d %d Y is: %f",x,x>=-180,x<180,y);
        if(x>=-180&&x<180){
          
        //  while(x>=dataSource->dfBBOX[2])x-=360;
          while(x<dataSource->dfBBOX[0])x+=360;
        }else {
          projCacheInfo.isOutsideBBOX=true;
//                  everythingIsInBBOX = false;
        }
      //}

    }
    
    projCacheInfo.nativeCoordX=x;
    projCacheInfo.nativeCoordY=y;

    x-=dataSource->dfBBOX[0];
    y-=dataSource->dfBBOX[1];
    x/=(dataSource->dfBBOX[2]-dataSource->dfBBOX[0]);
    y/=(dataSource->dfBBOX[3]-dataSource->dfBBOX[1]);
    x*=double(dataSource->dWidth);
    y*=double(dataSource->dHeight);
#ifdef CIMAGEDATAWRITER_DEBUG
     CDBDebug("XY: %f %f and %d %d",x,y,dataSource->dWidth,dataSource->dHeight);
#endif
    projCacheInfo.dWidth=dataSource->dWidth;
    projCacheInfo.dHeight=dataSource->dHeight;
    projCacheInfo.dX=(dataSource->dfBBOX[2]-dataSource->dfBBOX[0])/double(dataSource->dWidth);
    projCacheInfo.dY=(dataSource->dfBBOX[3]-dataSource->dfBBOX[1])/double(dataSource->dHeight);

    if(x<0||x>=dataSource->dWidth){
      projCacheInfo.imx=-1;
      projCacheInfo.isOutsideBBOX=true;
    }else{
      projCacheInfo.imx=int(x);
    }
    if(y<0||y>=dataSource->dHeight){
      projCacheInfo.imy=-1;
      projCacheInfo.isOutsideBBOX=true;
    }else{
      projCacheInfo.imy=dataSource->dHeight-(int)y-1;
    }
//     CDBDebug("projCacheInfo %d,%d",int(projCacheInfo.imx),int(projCacheInfo.imy));
    projCacheInfo.lonX=projCacheInfo.CoordX;
    projCacheInfo.lonY=projCacheInfo.CoordY;
    //Get lat/lon
    imageWarper->reprojToLatLon(projCacheInfo.lonX,projCacheInfo.lonY);
    imageWarper->closereproj();
    projCacheMap[key]=projCacheInfo;
    
  }
  return projCacheInfo;
}

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}

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

CImageDataWriter::CImageDataWriter(){
  
  //Mode can be "uninitialized"0 "initialized"(1) and "finished" (2)
  writerStatus = uninitialized;

}



int CImageDataWriter::_setTransparencyAndBGColor(CServerParams *srvParam,CDrawImage* drawImage){
//  CDBDebug("_setTransparencyAndBGColor");
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
      int hexa[8];
      
      for(size_t j=0;j<6;j++){
        hexa[j]=srvParam->BGColor.charAt(j+2);
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

void CImageDataWriter::getFeatureInfoGetPointDataResults(CDataSource *dataSource, CImageDataWriter::GetFeatureInfoResult *getFeatureInfoResult, int dataObjectNrInDataSource, GetFeatureInfoResult::Element * element, int maxPixelDistance) {
  bool hasData = false;
  // Add info about point data
  if (dataSource->getDataObject(dataObjectNrInDataSource)->points.size()>0/*&&hasData==true*/){
    CDBDebug("GFI value = %s, %d", element->value.c_str(),dataSource->getDataObject(dataObjectNrInDataSource)->cdfVariable->getAttributeNE("ADAGUC_SKIP_POINTS")==NULL);
  }
  if(dataSource->getDataObject(dataObjectNrInDataSource)->points.size()>0/*&&hasData==true*/){
    if (dataSource->getDataObject(dataObjectNrInDataSource)->cdfVariable->getAttributeNE("ADAGUC_SKIP_POINTS")==NULL) {
      float closestDistance =0;
      int closestIndex =0;
      
      for(size_t j=0;j<dataSource->getDataObject(dataObjectNrInDataSource)->points.size();j++){
        PointDVWithLatLon point = dataSource->getDataObject(dataObjectNrInDataSource)->points[j];
        float distance = hypot(point.lon-getFeatureInfoResult->lon_coordinate,point.lat-getFeatureInfoResult->lat_coordinate);
        if(distance<closestDistance||j==0){
          closestIndex=j;
          closestDistance = distance;
        }
      }
      
      /* Now we have detected the closest point, check if it is in a certain distance in the map in pixel coordinates */
      double pointPX = dataSource->getDataObject(dataObjectNrInDataSource)->points[closestIndex].lon, pointPY = dataSource->getDataObject(dataObjectNrInDataSource)->points[closestIndex].lat;
      double gfiPX = getFeatureInfoResult->lon_coordinate, gfiPY = getFeatureInfoResult->lat_coordinate;
      CImageWarper warper;
      /* We can always use LATLONPROJECTION, because getFeatureInfoResult->lon_coordinate and getFeatureInfoResult->lat_coordinate are always converted to latlon in CImageDataWriter */
      int status = warper.initreproj(LATLONPROJECTION,srvParam->Geo,&srvParam->cfg->Projection);
      if (status != 0){CDBError("Unable to initialize projection");}
      status = warper.reprojpoint_inv_topx(pointPX, pointPY);
      status = warper.reprojpoint_inv_topx(gfiPX, gfiPY);
      float pixelDistance = hypot(pointPX-gfiPX,pointPY-gfiPY);
      warper.closereproj();

      if (pixelDistance>maxPixelDistance){
        hasData = false;
        return;
      }
      
      PointDVWithLatLon point = dataSource->getDataObject(dataObjectNrInDataSource)->points[closestIndex];
      
      if (!hasData) {
        double val=dataSource->getDataObject(dataObjectNrInDataSource)->points[closestIndex].v; 
        if(dataSource->getDataObject(dataObjectNrInDataSource)->hasStatusFlag){
          //Add status flag
          CT::string flagMeaning;
          CDataSource::getFlagMeaningHumanReadable(&flagMeaning,&dataSource->getDataObject(dataObjectNrInDataSource)->statusFlagList,(int)val);
          element->value.print("%s (%d)",flagMeaning.c_str(),(int)val);
          element->units="";
        }else{             
          element->value.print("%f",val);
        }
      }
      
      //Loop over point paramlist
      for(size_t p=0;p<point.paramList.size();p++){
        GetFeatureInfoResult::Element *pointID=new GetFeatureInfoResult::Element();
        pointID->dataSource= dataSource;
        //pointID->time=dataSources[d]->getDimensionValueForNameAndStep("time",dataSources[d]->getCurrentTimeStep());
        for(size_t j=0;j<dataSource->requiredDims.size();j++){
          CCDFDims * cdfDims = dataSource->getCDFDims();
          CT::string value=cdfDims->getDimensionValue(j);
          CT::string name=cdfDims->getDimensionName(j);
          pointID->cdfDims.addDimension(name.c_str(),value.c_str(),cdfDims->getDimensionIndex(j));
        }
        getFeatureInfoResult->elements.push_back(pointID);
        pointID->long_name=point.paramList[p].description;
        pointID->var_name=point.paramList[p].key;
        pointID->standard_name=point.paramList[p].key;
        pointID->feature_name=point.paramList[p].key;
        pointID->value=point.paramList[p].value;
        pointID->units="";
      }
    }      
  }
}

int CImageDataWriter::drawCascadedWMS(CDataSource * dataSource, const char *service,const char *layers, const char *styles,bool transparent, const char *bgcolor){

#ifndef ENABLE_CURL
  CDBError("CURL not enabled");
  return 1;
#endif

#ifdef ENABLE_CURL
  bool trueColor=drawImage.getTrueColor();
 // transparent=true;
  CT::string url=service;
  url.concat("SERVICE=WMS&VERSION=1.1.1&REQUEST=GetMap&");
  if(trueColor==false)url.concat("FORMAT=image/gif");
  else url.concat("FORMAT=image/png");
  //&BBOX=50.943396226415075,-4.545656817372752,118.8679245283019,57.6116945532218
  if(transparent){
    url.printconcat("&TRANSPARENT=TRUE");
  } else {
    url.printconcat("&TRANSPARENT=FALSE");
  }
  if (strlen(bgcolor)>0) {
     url.printconcat("&BGCOLOR=");
     url.printconcat(bgcolor);
  }
  url.printconcat("&WIDTH=%d",drawImage.Geo->dWidth);
  url.printconcat("&HEIGHT=%d",drawImage.Geo->dHeight);
  url.printconcat("&BBOX=%0.4f,%0.4f,%0.4f,%0.4f",drawImage.Geo->dfBBOX[0],
                  drawImage.Geo->dfBBOX[1],
                  drawImage.Geo->dfBBOX[2],
                  drawImage.Geo->dfBBOX[3]);
  url.printconcat("&SRS=%s",drawImage.Geo->CRS.c_str());
  url.printconcat("&LAYERS=%s",layers);
  if ((styles!=NULL)&&(strlen(styles)>0)){
    url.printconcat("&STYLES=%s", styles);
  } else {
    url.printconcat("&STYLES=");
  }
  for(size_t k=0;k<srvParam->requestDims.size();k++){
    url.printconcat("&%s=%s",srvParam->requestDims[k]->name.c_str(),srvParam->requestDims[k]->value.c_str());
  }
  CDBDebug(url.c_str());
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
        if(pos->attr.right.empty()==false)offsetx=(drawImage.Geo->dWidth-w)-parseInt(pos->attr.right.c_str());
        if(pos->attr.bottom.empty()==false)offsety=(drawImage.Geo->dHeight-h)-parseInt(pos->attr.bottom.c_str());
        if(pos->attr.left.empty()==false)offsetx=parseInt(pos->attr.left.c_str());
        if(pos->attr.top.empty()==false)offsety=parseInt(pos->attr.top.c_str());
        
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
  int status = 0;
  #ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("init");
  #endif
  if(writerStatus!=uninitialized){CDBError("Already initialized");return 1;}
  this->srvParam=srvParam;
  
  if(_setTransparencyAndBGColor(this->srvParam,&drawImage)!=0)return 1;
  
  CStyleConfiguration *styleConfiguration = NULL;
  
  if(dataSource!=NULL){
    styleConfiguration = dataSource->getStyle();
  }
  
  bool forceGDRenderer = false;

  if(styleConfiguration!=NULL && styleConfiguration->styleConfig!=NULL && styleConfiguration->styleConfig->RenderSettings.size() == 1) {
    // XMLE_RenderSettings
    if(styleConfiguration->styleConfig->RenderSettings[0]->attr.renderer.equals("gd")){
      forceGDRenderer = true;
    }
  }
  
  if(!forceGDRenderer){
    if(srvParam->imageFormat ==  IMAGEFORMAT_IMAGEPNG8 || 
      srvParam->imageFormat ==  IMAGEFORMAT_IMAGEPNG24 || 
      srvParam->imageFormat ==  IMAGEFORMAT_IMAGEPNG32) {
      drawImage.setRenderer(CDRAWIMAGERENDERER_CAIRO);
    }else{
      drawImage.setRenderer(CDRAWIMAGERENDERER_GD);
    }
  }else{
    CDBDebug("Forcing renderer to GD");
    drawImage.setRenderer(CDRAWIMAGERENDERER_GD);
  }
   
  
  if(srvParam->imageMode==SERVERIMAGEMODE_RGBA||srvParam->Styles.indexOf("HQ")>0){
    drawImage.setCanvasColorType(CDRAWIMAGE_COLORTYPE_ARGB);
    drawImage.setRenderer(CDRAWIMAGERENDERER_CAIRO);
    
  }


  
  if(styleConfiguration!=NULL){
  
    if(styleConfiguration->renderMethod&RM_RGBA){
    
      drawImage.setCanvasColorType(CDRAWIMAGE_COLORTYPE_ARGB);
      if(srvParam->requestType==REQUEST_WMS_GETLEGENDGRAPHIC){
        //CDBDebug("drawImage.create685Palette();");
        writerStatus=initialized;
        drawImage.createImage(40,20);
        drawImage.create685Palette();
        return 0;
      }
    }
  }
  
  // WMS Format in layer always overrides all
  if(dataSource!=NULL){
    if(dataSource->cfgLayer->WMSFormat.size()>0){
        if(dataSource->cfgLayer->WMSFormat[0]->attr.name.equals("image/png32")){
        drawImage.setCanvasColorType(CDRAWIMAGE_COLORTYPE_ARGB);
        }
        if(dataSource->cfgLayer->WMSFormat[0]->attr.format.equals("image/png32")){
        drawImage.setCanvasColorType(CDRAWIMAGE_COLORTYPE_ARGB);
        }
        if(dataSource->cfgLayer->WMSFormat[0]->attr.format.equals("image/png24")){
          drawImage.setCanvasColorType(CDRAWIMAGE_COLORTYPE_ARGB);
        }
         if(dataSource->cfgLayer->WMSFormat[0]->attr.format.equals("image/webp")){
          drawImage.setCanvasColorType(CDRAWIMAGE_COLORTYPE_ARGB);
          srvParam->imageFormat=IMAGEFORMAT_IMAGEWEBP;
        }
        
    }
  }
  //Set font location
  if(srvParam->cfg->WMS[0]->ContourFont.size()!=0){
    if(srvParam->cfg->WMS[0]->ContourFont[0]->attr.location.empty()==false){
      drawImage.setTTFFontLocation(srvParam->cfg->WMS[0]->ContourFont[0]->attr.location.c_str());
      
      if(srvParam->cfg->WMS[0]->ContourFont[0]->attr.size.empty()==false){
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
    //CDBDebug("srvParam->wmsExtensions.opacity %d",srvParam->wmsExtensions.opacity);
    drawImage.setBackGroundAlpha((unsigned char )(float(srvParam->wmsExtensions.opacity)*2.55));
  }
  


  writerStatus=initialized;
  animation = 0;
  nrImagesAdded = 0;

  if(srvParam->requestType==REQUEST_WMS_GETMAP){
//  CDBDebug("CREATING IMAGE FOR WMS GETMAP ---------------------------------------");
    status = drawImage.createImage(srvParam->Geo);
  
    if(status != 0) return 1;
  }
  if(srvParam->requestType==REQUEST_WMS_GETLEGENDGRAPHIC){
    //drawImage.setAntiAliased(false);
    //drawImage.setTrueColor(false);
    
    int w = LEGEND_WIDTH;
    int h = LEGEND_HEIGHT;
    if(srvParam->Geo->dWidth!=1)w=srvParam->Geo->dWidth;
    if(srvParam->Geo->dHeight!=1)h=srvParam->Geo->dHeight;
    
    if (w>h) {
      status = drawImage.createImage(h,w);
      CDBDebug("Init legend %dx%d", h,w);
    } else {
      status = drawImage.createImage(w,h);
      CDBDebug("Init legend %dx%d", h,w);
    }
    
    if(status != 0) return 1;
  }
  if(srvParam->requestType==REQUEST_WMS_GETFEATUREINFO||srvParam->requestType==REQUEST_WMS_GETHISTOGRAM){
    //status = drawImage.createImage(2,2);
    drawImage.Geo->copy(srvParam->Geo);
    
        #ifdef CIMAGEDATAWRITER_DEBUG    
    CDBDebug("/init");
        #endif
    return 0;
  }
  
  if(dataSource!=NULL){
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

    if(styleConfiguration!=NULL){
        if(styleConfiguration->legendIndex != -1){
        //Create palette for internal WNS layer
        if(dataSource->dLayerType!=CConfigReaderLayerTypeCascaded
        ){
            status = drawImage.createGDPalette(srvParam->cfg->Legend[styleConfiguration->legendIndex]);
            if(status != 0){
            CDBError("Unknown palette type for %s",srvParam->cfg->Legend[styleConfiguration->legendIndex]->attr.name.c_str());
            return 1;
            }
        }
        }
    }
  }
  #ifdef CIMAGEDATAWRITER_DEBUG    
  CDBDebug("/init");
  #endif
  return 0;
}






double CImageDataWriter::convertValue(CDFType type,void *data,size_t ptr){
  switch(type){
      case CDF_CHAR:  return ((char*)data)[ptr];
      case CDF_BYTE:  return ((char*)data)[ptr];
      case CDF_UBYTE: return ((unsigned char*)data)[ptr];
      case CDF_SHORT: return ((short*)data)[ptr];
      case CDF_USHORT:return ((ushort*)data)[ptr];
      case CDF_INT:   return ((int*)data)[ptr];
      case CDF_UINT:  return ((uint*)data)[ptr];
      case CDF_FLOAT: return ((float*)data)[ptr];
      case CDF_DOUBLE:return ((double*)data)[ptr];
      default:
          CDBError("Unknown type detected in convertValue, type = %d",type);
          break;
  }
  return 0;
}
void CImageDataWriter::setValue(CDFType type,void *data,size_t ptr,double pixel){
  if(type==CDF_CHAR||type==CDF_BYTE)((char*)data)[ptr]=(char)pixel;
  if(type==CDF_UBYTE)((unsigned char*)data)[ptr]=(unsigned char)pixel;
  if(type==CDF_SHORT)((short*)data)[ptr]=(short)pixel;
  if(type==CDF_USHORT)((unsigned short*)data)[ptr]=(unsigned short)pixel;
  if(type==CDF_INT)((int*)data)[ptr]=(int)pixel;
  if(type==CDF_UINT)((unsigned int*)data)[ptr]=(unsigned int)pixel;
  if(type==CDF_FLOAT)((float*)data)[ptr]=(float)pixel;
  if(type==CDF_DOUBLE)((double*)data)[ptr]=(double)pixel;
}






int CImageDataWriter::getFeatureInfo(std::vector<CDataSource *>dataSources,int dataSourceIndex,int dX,int dY){
  CImageWarper imageWarper;
  #ifdef MEASURETIME
  StopWatch_Stop("getFeatureInfo");
  #endif
  
  #ifdef CIMAGEDATAWRITER_DEBUG
  CDBDebug("[getFeatureInfo] %d, %d, [%d,%d]", dataSources.size(), dataSourceIndex, dX, dY);
  #endif
  // Create a new getFeatureInfoResult object and push it into the vector.
  int status = 0;
  isProfileData=false;
  for(size_t d=0;d<dataSources.size();d++){
    
    GetFeatureInfoResult  *getFeatureInfoResult = new GetFeatureInfoResult();
    getFeatureInfoResultList.push_back(getFeatureInfoResult);
    bool headerIsAvailable = false;
    bool sameHeaderForAll = false;
    bool openAll = false;
    
    bool everythingIsInBBOX = true;
    CDataReader reader;
    reader.open(dataSources[d],CNETCDFREADER_MODE_OPEN_HEADER); 
    if(dataSources[d]->getNumDataObjects()>0){ 
      if(dataSources[d]->getDataObject(0)->cdfVariable!=NULL){
        if(dataSources[d]->isConfigured){
          if(dataSources[d]->nativeProj4.length()>0){
            headerIsAvailable=true;
          }
        }
        if(dataSources[d]->getDataObject(0)->cdfVariable->getAttributeNE("ADAGUC_VECTOR")!=NULL){
          openAll =true;
        }  
      
        if(dataSources[d]->getDataObject(0)->cdfVariable->getAttributeNE("ADAGUC_POINT")!=NULL){
          openAll =true;
        }  
        
        if(dataSources[d]->getDataObject(0)->cdfVariable->getAttributeNE("UGRID_MESH")!=NULL){
          openAll =true;
        }  
        if(dataSources[d]->getDataObject(0)->cdfVariable->getAttributeNE("ADAGUC_PROFILE")!=NULL){
          if(!srvParam->InfoFormat.equals("application/json")){
            isProfileData =true;
          }
        }
        if(dataSources[d]->getDataObject(0)->cdfObject->getAttributeNE("ADAGUC_GEOJSON")!=NULL){
          openAll =true;
        }  
        if(dataSources[d]->cfgLayer->FilePath.size()==1 && dataSources[d]->cfgLayer->FilePath[0]->attr.gfi_openall.equals("true")){
          openAll =true;
        }
        if(dataSources[d]->cfgLayer->FilePath.size()==1 && dataSources[d]->cfgLayer->FilePath[0]->attr.gfi_openall.equals("headers")){
          sameHeaderForAll = true;
        }
        
        
      }
    }
    
    //CDBDebug("gfi_openall: %d %d",dataSources[d]->cfgLayer->FilePath.size(),openAll);
    
  
      
    if(dataSources[d]->cfgLayer->TileSettings.size()==1){
      openAll=true;
    }
    
    
    CDataSource *dataSource=dataSources[d];
    if(dataSource==NULL){
      CDBError("dataSource == NULL");
      return 1;
    }
    
    CDBDebug("isProfileData:[%d] openAll:[%d] sameHeaderForAll:[%d] infoFormat:[%s]",isProfileData,openAll,sameHeaderForAll,srvParam->InfoFormat.c_str());
    
    if(isProfileData){
      if(srvParam->InfoFormat.equals("image/png")){
        int status = CMakeEProfile::MakeEProfile(&drawImage,&imageWarper,dataSources, d,dX,dY);
        if(status != 0){
          CDBError("CMakeEProfile::MakeEProfile failed");
          return status;
        }      
        CDBDebug("CMakeEProfile::MakeEProfile done");
      }else{
        printf("%s%c%c\n","Content-Type:text/plain",13,10);
        printf("Not supported yet");
        return 0;
      }
    }else if(sameHeaderForAll==false && openAll == false && srvParam->InfoFormat.equals("application/json")){
      int status = CMakeJSONTimeSeries::MakeJSONTimeSeries(&drawImage,&imageWarper,dataSources, d,dX,dY,&gfiStructure);
      if(status != 0){
        CDBError("CMakeJSONTimeSeries::MakeJSONTimeSeries failed");
        return status;
      }
    }else{
    //return 1;
      CDBDebug("Num time steps for datasource %d",dataSources[d]->getNumTimeSteps());
      
      std::map<std::string, bool>dimensionKeyValueMap;//A map for every dimensionvalue linked to a value
      
      
      
      for(int step=0;step<dataSources[d]->getNumTimeSteps();step++){
        dataSources[d]->setTimeStep(step);
        
        CCDFDims * cdfDims = dataSources[d]->getCDFDims();
        
      
        
    
        

        //Copy layer name
        getFeatureInfoResult->layerName.copy(&dataSource->layerName);
        getFeatureInfoResult->layerTitle.copy(&dataSource->layerName);
        

        getFeatureInfoResult->dataSourceIndex=dataSourceIndex;
        
        #ifdef CIMAGEDATAWRITER_DEBUG    
        CDBDebug("Processing dataSource %d with result %d of %d results (%d) %f",d,step,dataSources[d]->getNumTimeSteps(),dataSources[d]->getDataObject(0)->hasNodataValue,dataSources[d]->getDataObject(0)->dfNodataValue);
        #endif
        

        CDataReader reader;
        //if(!headerIsAvailable)
        {
          if(openAll){
            //CDBDebug("OPEN ALL");
            #ifdef CIMAGEDATAWRITER_DEBUG    
              CDBDebug("OPEN ALL");
            #endif
            status = reader.open(dataSources[d],CNETCDFREADER_MODE_OPEN_ALL);
          }else{
            //CDBDebug("OPEN HEADER");
            #ifdef CIMAGEDATAWRITER_DEBUG    
              CDBDebug("OPEN Header %d",headerIsAvailable);
            #endif
            if(!headerIsAvailable||sameHeaderForAll==true){
              headerIsAvailable = true;
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
        ckey.print("%d:%d:%d:%d:%s:%f:%f:%f:%f",dX,dY,dataSource->dWidth,dataSource->dHeight,dataSource->nativeProj4.c_str(),dataSource->dfBBOX[0],dataSource->dfBBOX[1],dataSource->dfBBOX[2],dataSource->dfBBOX[3]);
        CImageDataWriter::ProjCacheInfo projCacheInfo = GetProjInfo(ckey,&drawImage,dataSource,&imageWarper,srvParam,dX,dY);
        //CDBDebug("key = %s",ckey.c_str());
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
        
       
        
        if(projCacheInfo.imx>=0&&projCacheInfo.imy>=0&&projCacheInfo.imx<projCacheInfo.dWidth&&projCacheInfo.imy<projCacheInfo.dHeight&&projCacheInfo.isOutsideBBOX==false){
         
          if(!openAll){
           
            #ifdef CIMAGEDATAWRITER_DEBUG 
            CDBDebug("Reading datasource %d for %d,%d",d,projCacheInfo.imx,projCacheInfo.imy);
            #endif
            
            status = reader.open(dataSources[d],CNETCDFREADER_MODE_OPEN_ALL,projCacheInfo.imx,projCacheInfo.imy);
            
            //dataSource->getDataObject(0)->cdfVariable->data = malloc(4);
            
            //((float*)dataSource->getDataObject(0)->cdfVariable->data)[0]=step;
  //           size_t ptr=0;
  //           if(openAll){
  //             ptr=projCacheInfo.imx+projCacheInfo.imy*projCacheInfo.dWidth;
  //           }
            //double pixel=convertValue(dataSource->getDataObject(d)->cdfVariable->getType(),dataSource->getDataObject(d)->cdfVariable->data,ptr);
            #ifdef CIMAGEDATAWRITER_DEBUG 
            CDBDebug("Done");
            #endif
            
          } 
          #ifdef CIMAGEDATAWRITER_DEBUG 
          else{
            CDBDebug("OpenALL = true");
          }
          #endif
            
          if(status!=0){
            CDBError("Could not open file: %s",dataSource->getFileName());
            return 1;
          }
        }/*else{
         
          GetFeatureInfoResult::Element * element = new GetFeatureInfoResult::Element();
          element->dataSource=dataSource;
          //Get variable name
          element->var_name.copy(&dataSources[d]->getDataObject(d)->variableName);
          //Get variable units
          CT::string units=dataSources[d]->getDataObject(d)->getUnits();
          element->units.copy(&units);
          element->value="nodata";
          getFeatureInfoResult->elements.push_back(element);
         
          //isOutsideBBOX = true;
          //everythingIsInBBOX = false;
        }*/
  //     }
  //     
  //     
  //     //SPLIT UP OF DATA RETRIEVAL AND DATA PROCESSING
  //      for(int step=0;step<dataSources[d]->getNumTimeSteps();step++){
  //       dataSources[d]->setTimeStep(step);
  //       dataSource = dataSources[d];
         
        if(everythingIsInBBOX == true){

        //TODO find raster projection units and find image projection units.
        
        //Determine if this is a GridRelative vector product
        //bool windVectorProduct=false;
        bool gridRelative=false;
       
//         if (dataSource->getNumDataObjects()>1){
//          
//           //windVectorProduct=true;
//           // Check standard_name/var_name for first vector component
//           // if x_wind/grid_east_wind of y_wind/grid_northward_wind then gridRelative=true
//           // if eastward_wind/northward_wind then gridRelative=false
//           // default is gridRelative=true
//           CT::string standard_name;
//           standard_name=dataSource->getDataObject(0)->variableName;
//           try {
//             dataSource->getDataObject(0)->cdfVariable->getAttribute("standard_name")->getDataAsString(&standard_name);
//           } catch (CDFError e) {}
//           if (standard_name.equals("x_wind")||standard_name.equals("grid_eastward_wind")||
//             standard_name.equals("y_wind")||standard_name.equals("grid_northward_wind")) {
//             gridRelative=true;
//           } else {
//             gridRelative=false;
//           }
//           #ifdef CIMAGEDATAWRITER_DEBUG 
//           CDBDebug("Grid propery gridRelative=%d", gridRelative);
//           #endif
//         }
            

      //Retrieve variable names
      for(size_t o=0;o<dataSource->getNumDataObjects();o++){
       
//        size_t j=d+o*dataSources.size();
        //CDBDebug("j = %d",j);
        //Create a new element and at it to the elements list.
        
        
        CT::string dimkey = "";
        for(size_t j=0;j<dataSources[d]->requiredDims.size();j++){
          dimkey.printconcat("[%s=%s]",cdfDims->getDimensionValue(j).c_str(),cdfDims->getDimensionName(j));
        }
        dimkey.printconcat("[dataobject=%d]",o);
         

        std::map<std::string,bool>::iterator dimensionKeyValueMapIterator = dimensionKeyValueMap.find(dimkey.c_str());
        if(dimensionKeyValueMapIterator!=dimensionKeyValueMap.end()){
          if(dimensionKeyValueMapIterator->second==false){
            if(getFeatureInfoResult->elements.size()>0){
              delete getFeatureInfoResult->elements[getFeatureInfoResult->elements.size()-1];
              getFeatureInfoResult->elements.pop_back();
            }
          }
        }else{
          dimensionKeyValueMap[dimkey.c_str()]=false;
        }

        GetFeatureInfoResult::Element * element = new GetFeatureInfoResult::Element();
        getFeatureInfoResult->elements.push_back(element);
        
        
        
          
          
        
        
           
        element->dataSource=dataSource;
        //Get variable name
        element->var_name.copy(&dataSources[d]->getDataObject(o)->variableName);
        //Get variable units
        CT::string units=dataSources[d]->getDataObject(o)->getUnits();
        element->units.copy(&units);
 
        //Get variable standard name
        CDF::Attribute * attr_standard_name=dataSources[d]->getDataObject(o)->cdfVariable->getAttributeNE("standard_name");
        if(attr_standard_name!=NULL){
          CT::string standardName;attr_standard_name->getDataAsString(&standardName);
          element->standard_name.copy(&standardName);
          // Make a more clean standard name.
          //standardName.replaceSelf("_"," ");standardName.replaceSelf(" status flag","");
          element->feature_name.print("%s_%d",standardName.c_str(),o);
        }
        if(element->standard_name.empty()){
          element->standard_name.copy(&element->var_name);
          element->feature_name.copy(&element->var_name);
          element->feature_name.print("%s_%d",element->var_name.c_str(),o);
        }

        
 
        // Get variable long name
        CDF::Attribute * attr_long_name=dataSources[d]->getDataObject(o)->cdfVariable->getAttributeNE("long_name");
        if(attr_long_name!=NULL){
          attr_long_name->getDataAsString(&element->long_name);
        }else element->long_name.copy(&element->var_name);
        
        // Assign CDF::Variable Pointer
        element->variable = dataSources[d]->getDataObject(o)->cdfVariable;
        element->value="nodata";
       
        //element->time=dataSources[d]->getDimensionValueForNameAndStep("time",dataSources[d]->getCurrentTimeStep());
        
        CCDFDims * cdfDims = dataSources[d]->getCDFDims();
        CT::string value,name;
        for(size_t j=0;j<dataSources[d]->requiredDims.size();j++){
           
          value=cdfDims->getDimensionValue(j);
            
          name=cdfDims->getDimensionName(j);
            
  //         if(name.indexOf("time")==0){
  //           value=element->cdfDims.getDimensionValue("time").c_str();
  //         }
//           CDBDebug("%d) %s == %s == %d",j,name.c_str(),value.c_str(),cdfDims->getDimensionIndex(j));
          element->cdfDims.addDimension(name.c_str(),value.c_str(),cdfDims->getDimensionIndex(j));
        }
  
         
        #ifdef CIMAGEDATAWRITER_DEBUG 
        CDBDebug("getFeatureInfoResult->elements has %d elements\n", getFeatureInfoResult->elements.size());
        #endif
        // Retrieve corresponding values.
  //       #ifdef CIMAGEDATAWRITER_DEBUG  
  //       CDBDebug("imx:%d imy:%d projCacheInfo.dWidth:%d projCacheInfo.dHeight:%d",projCacheInfo.imx,projCacheInfo.imy,projCacheInfo.dWidth,projCacheInfo.dHeight);
  //       #endif
        bool hasData = false;
        if(projCacheInfo.imx>=0&&projCacheInfo.imy>=0&&projCacheInfo.imx<projCacheInfo.dWidth&&projCacheInfo.imy<projCacheInfo.dHeight){

          
          //GetFeatureInfoResult::Element * element=getFeatureInfoResult->elements[j];

          size_t ptr=0;
          if(openAll){
            ptr=projCacheInfo.imx+projCacheInfo.imy*projCacheInfo.dWidth;
          }
          
          #ifdef CIMAGEDATAWRITER_DEBUG 
          CDBDebug("ptr = %d Dataobject = %d Timestep = %d",ptr,o,dataSource->getCurrentTimeStep());
          #endif
            dataSource->setTimeStep(step);
          double pixel=convertValue(dataSource->getDataObject(o)->cdfVariable->getType(),dataSource->getDataObject(o)->cdfVariable->data,ptr);

          #ifdef CIMAGEDATAWRITER_DEBUG 
          CDBDebug("pixel value = %f",pixel);
          #endif
          //Fill in the actual data value
          //Check whether this is a NoData value:
          
          
          if(
            (pixel!=dataSource->getDataObject(o)->dfNodataValue&&dataSource->getDataObject(o)->hasNodataValue==true&&pixel==pixel&&everythingIsInBBOX==true)||
            dataSource->getDataObject(o)->hasNodataValue==false){
            if(dataSource->getDataObject(o)->hasStatusFlag){
              //Add status flag
              
              CT::string flagMeaning;
              CDataSource::getFlagMeaningHumanReadable(&flagMeaning,&dataSource->getDataObject(o)->statusFlagList,pixel);
              element->value.print("%s (%d)",flagMeaning.c_str(),(int)pixel);
              element->units="";
            }else{
              //Add raster value
              //char szTemp[1024];
              //floatToString(szTemp,1023,pixel);
              
              element->value.print("%f",pixel);//=szTemp;
            }
            #ifdef CIMAGEDATAWRITER_DEBUG 
            CDBDebug("Element value == %s",element->value.c_str());
            #endif
            dimensionKeyValueMap[dimkey.c_str()]=true;
            hasData = true;
            
            if (dataSource->getDataObject(o)->features.empty() == false &&hasData==true){
              std::map<int,CFeature>::iterator featureIt = dataSource->getDataObject(o)->features.find(pixel);
              if(featureIt!=dataSource->getDataObject(o)->features.end()){
                CFeature *feature = &featureIt->second;
                if(feature->paramMap.empty() == false){
                  std::map<std::string,std::string>::iterator paramItemIt  ;
                  for (paramItemIt=feature->paramMap.begin(); paramItemIt!=feature->paramMap.end(); ++paramItemIt){
                    CDBDebug("Clicked %s %s",paramItemIt->first.c_str(),paramItemIt->second.c_str());
                    GetFeatureInfoResult::Element *featureParam=new GetFeatureInfoResult::Element();
                    featureParam->dataSource= dataSource;
                    for(size_t j=0;j<dataSources[d]->requiredDims.size();j++){
                      value=cdfDims->getDimensionValue(j);
                      name=cdfDims->getDimensionName(j);
                      featureParam->cdfDims.addDimension(name.c_str(),value.c_str(),cdfDims->getDimensionIndex(j));
                    }
                    getFeatureInfoResult->elements.push_back(featureParam);
                    featureParam->long_name=paramItemIt->first.c_str();
                    featureParam->var_name=paramItemIt->first.c_str();
                    featureParam->standard_name=paramItemIt->first.c_str();
                    featureParam->feature_name=paramItemIt->first.c_str();
                    featureParam->value=paramItemIt->second.c_str();
                    featureParam->units="";
                  }
                }
              }
            }
          
          }else {
          
            element->value="nodata";
            #ifdef CIMAGEDATAWRITER_DEBUG 
            CDBDebug("Element value == %s",element->value.c_str());
            #endif
            }
          }else{
            #ifdef CIMAGEDATAWRITER_DEBUG 
            CDBDebug("isOutsideBBOX");
            #endif

            if(hasData==false && dimensionKeyValueMap.find(dimkey.c_str())->second==true){
              getFeatureInfoResult->elements.pop_back();
              delete element;element=NULL;
            }
          
          
          }
        
          getFeatureInfoGetPointDataResults(dataSource, getFeatureInfoResult, o, element, 30);
   
          
        }
        //reader.close();
        #ifdef CIMAGEDATAWRITER_DEBUG 
        CDBDebug("dataSource->getNumDataObjects()==%d",dataSource->getNumDataObjects());
        #endif
        
        //For vectors, we will calculate angle and strength
        if((dataSource->getNumDataObjects()==2)&&(dataSource->getDataObject(0)->cdfVariable->getAttributeNE("ADAGUC_GEOJSONPOINT")==NULL)){
//          CDBDebug("VECTOR GFI!@!!!!!!!!");
          size_t ptr=0;
          if(openAll){
            ptr=projCacheInfo.imx+projCacheInfo.imy*projCacheInfo.dWidth;
          }

          double pi=3.141592;
          double pixel1=convertValue(dataSource->getDataObject(0)->cdfVariable->getType(),dataSource->getDataObject(0)->cdfVariable->data,ptr);
          double pixel2=convertValue(dataSource->getDataObject(1)->cdfVariable->getType(),dataSource->getDataObject(1)->cdfVariable->data,ptr);

          bool windDataValid = (((pixel1==pixel1&&everythingIsInBBOX==true&&dataSource->getDataObject(0)->hasNodataValue==true&&pixel1!=dataSource->getDataObject(0)->dfNodataValue)||
            dataSource->getDataObject(0)->hasNodataValue==false)&&
            ((pixel2==pixel2&&everythingIsInBBOX==true&&dataSource->getDataObject(1)->hasNodataValue==true&&pixel2!=dataSource->getDataObject(1)->dfNodataValue)||
            dataSource->getDataObject(1)->hasNodataValue==false)) ;
          
          
          /**
          * Derived wind vector calculation is now based on U and V, this gives incorrect results when using rotation and strength.
          * To prevent wrong answers in GFI this is disabled.
          */
          if(false){
            if (gridRelative&&windDataValid==true)  {
        //Add raster value

              status = imageWarper.initreproj(dataSource,drawImage.Geo,&srvParam->cfg->Projection);
    #ifdef ORIGINAL_JACOBIAN
              #ifdef CIMAGEDATAWRITER_DEBUG 
              CDBDebug("doJacoIntoLatLon(%f,%f,%f, %f, %f, %f)", pixel1,pixel2, projCacheInfo.lonX,projCacheInfo.lonY,0.01,0.01);
              #endif
              doJacoIntoLatLon(pixel1, pixel2, projCacheInfo.lonX, projCacheInfo.lonY, 0.01, 0.01, &imageWarper);
    #else
              #ifdef CIMAGEDATAWRITER_DEBUG 
              CDBDebug("Rot_UV_North(%f,%f,%f, %f, %f, %f)", pixel1,pixel2, projCacheInfo.lonX,projCacheInfo.lonY,0.01,0.01);
              #endif
              rotateUvNorth(pixel1, pixel2, projCacheInfo.nativeCoordX, projCacheInfo.nativeCoordY, 0.01, 0.01, &imageWarper);
    #endif
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
            //element2->time=dataSources[d]->getDimensionValueForNameAndStep("time",dataSources[d]->getCurrentTimeStep());
            for(size_t j=0;j<dataSources[d]->requiredDims.size();j++){
              value=cdfDims->getDimensionValue(j);
              name=cdfDims->getDimensionName(j);
  //             if(name.indexOf("time")==0){
  //               value=element2->cdfDims.getDimensionValue("time").c_str();
  //             }
              element2->cdfDims.addDimension(name.c_str(),value.c_str(),cdfDims->getDimensionIndex(j));
            }
            element2->dataSource= dataSource;
            getFeatureInfoResult->elements.push_back(element2);
            element2->long_name="wind direction";
            element2->var_name="wind direction";
            element2->standard_name="dir";
            element2->feature_name="wind direction";
            element2->units="degrees";

            if (windDataValid) {
              double angle=270-atan2(pixel2, pixel1)*180/pi;
              if (angle>360) angle-=360;
              if (angle<0) angle=angle+360;
              element2->value.print("%3.0f",angle);
            } else {
              element2->value.print("%s", "nodata");
            }
            #ifdef CIMAGEDATAWRITER_DEBUG 
            CDBDebug("pushed wind dir %s for step %d [%d]", element2->value.c_str(), step, getFeatureInfoResult->elements.size());
            #endif
            //element2->time=dataSources[d]->getDimensionValueForNameAndStep("time",dataSources[d]->getCurrentTimeStep());
            
            GetFeatureInfoResult::Element *windspeedOrigElement=new GetFeatureInfoResult::Element();
            windspeedOrigElement->dataSource= dataSource;
  //           windspeedOrigElement->time=dataSources[d]->getDimensionValueForNameAndStep("time",dataSources[d]->getCurrentTimeStep());
            for(size_t j=0;j<dataSources[d]->requiredDims.size();j++){
              value=cdfDims->getDimensionValue(j);
              name=cdfDims->getDimensionName(j);
  //             if(name.indexOf("time")==0){
  //               value=windspeedOrigElement->cdfDims.getDimensionValue("time").c_str();
  //             }
              windspeedOrigElement->cdfDims.addDimension(name.c_str(),value.c_str(),cdfDims->getDimensionIndex(j));
            }
            getFeatureInfoResult->elements.push_back(windspeedOrigElement);
            windspeedOrigElement->long_name="wind speed";
            windspeedOrigElement->var_name="wind speed";
            windspeedOrigElement->standard_name="speed1";
            windspeedOrigElement->feature_name="wind speed";
            windspeedOrigElement->units=dataSource->getDataObject(0)->getUnits();
  //           windspeedOrigElement->time=dataSources[d]->getDimensionValueForNameAndStep("time",dataSources[d]->getCurrentTimeStep());
            if (windDataValid) {
              double windspeed=hypot(pixel1, pixel2);
              windspeedOrigElement->value.print("%3.1f",windspeed);
            } else {
              windspeedOrigElement->value.print("%s","nodata");
            }
            #ifdef CIMAGEDATAWRITER_DEBUG 
            CDBDebug("pushed wind speed %s for step %d [%d]",windspeedOrigElement->value.c_str() , step, getFeatureInfoResult->elements.size());
            #endif

            //Skip KTS calculation if input data is not u and v vectors in m/s.
            bool skipKTSCalc = true;
            try{
              if(dataSource->getDataObject(0)->getUnits().indexOf("m/s")>=0){
                skipKTSCalc =false;
              }
            }catch(int e){}
          
            if(!skipKTSCalc){
              GetFeatureInfoResult::Element *element3=new GetFeatureInfoResult::Element();
              element3->dataSource= dataSource;
  //             element3->time=dataSources[d]->getDimensionValueForNameAndStep("time",dataSources[d]->getCurrentTimeStep());
              CCDFDims * cdfDims = dataSources[d]->getCDFDims();CT::string value,name;
              for(size_t j=0;j<dataSources[d]->requiredDims.size();j++){
                value=cdfDims->getDimensionValue(j);
                name=cdfDims->getDimensionName(j);
    /*           if(name.indexOf("time")==0){
                  value=windspeedOrigElement->cdfDims.getDimensionValue("time").c_str();
                }
    */
                element3->cdfDims.addDimension(name.c_str(),value.c_str(),cdfDims->getDimensionIndex(j));
              }
              getFeatureInfoResult->elements.push_back(element3);
              element3->long_name="wind speed";
              element3->var_name="wind speed";
              element3->standard_name="speed2";
              element3->feature_name="wind speed kts";
              element3->units="kts";
  //             element3->time=dataSources[d]->getDimensionValueForNameAndStep("time",dataSources[d]->getCurrentTimeStep());
              if (windDataValid) {
                double windspeedKTS=hypot(pixel1, pixel2)*(3600./1852.);
                element3->value.print("%3.1f",windspeedKTS);
              } else {
                element3->value.print("%s", "nodata");
              }
              #ifdef CIMAGEDATAWRITER_DEBUG 
              CDBDebug("pushed wind speed KTS %f for step %d [%d]\n", element3->value.c_str(), step, getFeatureInfoResult->elements.size());
              #endif
            }
          }
        }
        }
      }
    }
  }
  
  for(size_t j=0;j<getFeatureInfoResultList.size();j++){
    if(getFeatureInfoResultList[j]->elements.size()>0){
      getFeatureInfoResultList[j]->layerTitle = getFeatureInfoResultList[j]->elements[0]->long_name;
    }
  }
  #ifdef CIMAGEDATAWRITER_DEBUG 
  CDBDebug("[/getFeatureInfo %d]",getFeatureInfoResultList.size());
  #endif
  return 0;
}


int CImageDataWriter::createAnimation(){
  #ifdef CIMAGEDATAWRITER_DEBUG 
  CDBDebug("[createAnimation]");
  #endif
  if(drawImage.getRenderer() == CDRAWIMAGERENDERER_GD){
    printf("%s%c%c\n","Content-Type:image/gif",13,10);
  }
  drawImage.beginAnimation();
  animation = 1;
  #ifdef CIMAGEDATAWRITER_DEBUG 
  CDBDebug("[/createAnimation]");
  #endif
  return 0;
}

void CImageDataWriter::setDate(const char *szTemp){
  drawImage.setTextStroke(szTemp, strlen(szTemp),drawImage.Geo->dWidth-170,5,240,254,0);
}


CImageDataWriter::IndexRange::IndexRange() {
  min = 0;
  max = 0;
}

std::vector<CImageDataWriter::IndexRange> CImageDataWriter::getIndexRangesForRegex(CT::string match, CT::string *attributeValues, int n) {
  std::vector<CImageDataWriter::IndexRange> ranges;
  int ret;
  regex_t regex;
  ret=regcomp(&regex, match.c_str(), 0);

  if (!ret) {
    int first=-1;
    int last=-1;
    for (int i=0; i<n; i++) {
      int matched=1;
      if(attributeValues[i].length()>0){
        matched=regexec(&regex, attributeValues[i].c_str(), 0, NULL, 0);
      }
      //CDBDebug("Checking %s for %s",ids[i],match.c_str());
      
      if (matched==0) {
        //CDBDebug("match of %s [%d] with %s", attributeValues[i].c_str(), i, match.c_str());
        if (first==-1) {
          first=i;
          last=i+1;
        } else {
          if ((i-last)>0) {
            //new range
            ;
            ranges.push_back(CImageDataWriter::IndexRange(first, last));
            first=i;
            last=i+1;
          } else {
            last=i+1;
          }
        }
      }
    }
    ranges.push_back(CImageDataWriter::IndexRange(first,last));
  }
  regfree(&regex);
  return ranges;
}

pthread_mutex_t CImageDataWriter_addData_lock;
int CImageDataWriter::warpImage(CDataSource *dataSource,CDrawImage *drawImage){

  //Open the data of this dataSource
  int status = 0;
  #ifdef CIMAGEDATAWRITER_DEBUG  
  CDBDebug("Thread[%d]: Opening %s",dataSource->threadNr,dataSource->getFileName());
  #endif  
  

  CDataReader reader;
  pthread_mutex_lock(&CImageDataWriter_addData_lock);
  #ifdef MEASURETIME
  StopWatch_Stop("Thread[%d]: start Opening grid",dataSource->threadNr);
#endif
  
  bool usePixelExtent = false;
  if(usePixelExtent){

    status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_HEADER);
    CImageWarper warper;

      status = warper.initreproj(dataSource,srvParam->Geo,&srvParam->cfg->Projection);
      if(status != 0){
        CDBError("Unable to initialize projection");
        return 1;
      }
    
    
    
    CGeoParams sourceGeo;
        
    sourceGeo.dWidth = dataSource->dWidth;
    sourceGeo.dHeight = dataSource->dHeight;
    sourceGeo.dfBBOX[0] = dataSource->dfBBOX[0];
    sourceGeo.dfBBOX[1] = dataSource->dfBBOX[1];
    sourceGeo.dfBBOX[2] = dataSource->dfBBOX[2];
    sourceGeo.dfBBOX[3] = dataSource->dfBBOX[3];
    sourceGeo.dfCellSizeX = dataSource->dfCellSizeX;
    sourceGeo.dfCellSizeY = dataSource->dfCellSizeY;
    sourceGeo.CRS = dataSource->nativeProj4;
    int PXExtentBasedOnSource[4];
    GenericDataWarper::findPixelExtent(PXExtentBasedOnSource,&sourceGeo,srvParam->Geo,&warper);
    
    status = reader.openExtent(dataSource,CNETCDFREADER_MODE_OPEN_EXTENT,PXExtentBasedOnSource);
  }else{
   status = reader.open(dataSource,CNETCDFREADER_MODE_OPEN_ALL);
  }
  #ifdef MEASURETIME
  StopWatch_Stop("Thread[%d]: Opened grid",dataSource->threadNr);
#endif

  pthread_mutex_unlock(&CImageDataWriter_addData_lock);
 //return 0;
  #ifdef CIMAGEDATAWRITER_DEBUG  
  CDBDebug("Thread[%d]: Has opened %s",dataSource->threadNr,dataSource->getFileName());
  #endif    
  if(status!=0){
    CDBError("Could not open file: %s",dataSource->getFileName());
    return 1;
  }
  #ifdef CIMAGEDATAWRITER_DEBUG  
  CDBDebug("Thread[%d]: opened",dataSource->threadNr);
  #endif  
 

#ifdef MEASURETIME
  StopWatch_Stop("Thread[%d]: warp start",dataSource->threadNr);
#endif

/*if(renderMethod==nearest){CDBDebug("nearest");}
if(renderMethod==bilinear){CDBDebug("bilinear");}
if(renderMethod==bilinearcontour){CDBDebug("bilinearcontour");}
if(renderMethod==nearestcontour){CDBDebug("nearestcontour");}
if(renderMethod==contour){CDBDebug("contour");}*/

  CImageWarperRenderInterface *imageWarperRenderer;
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  CStyleConfiguration::RenderMethod renderMethod = styleConfiguration->renderMethod;

  
  /** Apply FeatureInterval config */
  if (styleConfiguration->featureIntervals!=NULL&&styleConfiguration->shadeIntervals!=NULL) {
    if(styleConfiguration->featureIntervals->size()>0){//&&styleConfiguration->shadeIntervals->size()==0){

      int numFeatures=0;
      try{
        numFeatures = dataSource->getDataObject(0)->cdfObject->getDimension("features")->getSize();
      }catch(int e){
        #ifdef CIMAGEDATAWRITER_DEBUG  
        CDBDebug("Note: While configuring featureInterval: Unable to find features variable");
        #endif
      }
      if (numFeatures > 0){
        CT::string attributeValues[numFeatures];
        /* Loop through all configured FeatureInterval elements */
        for(size_t j=0;j<styleConfiguration->featureIntervals->size();j++){
          CServerConfig::XMLE_FeatureInterval *featureInterval=((*styleConfiguration->featureIntervals)[j]);
          if(featureInterval->attr.match.empty()==false&&featureInterval->attr.matchid.empty()==false){
            /* Get the matchid attribute for the feature */
            CT::string attributeName = featureInterval->attr.matchid;
            for(int featureNr = 0;featureNr<numFeatures;featureNr++){
              attributeValues[featureNr] =  "";
              std::map<int,CFeature>::iterator feature = dataSource->getDataObject(0)->features.find(featureNr);
              if(feature!=dataSource->getDataObject(0)->features.end()){
                std::map<std::string,std::string>::iterator attributeValueItr =  feature->second.paramMap.find(attributeName.c_str());
                if(attributeValueItr!=feature->second.paramMap.end()){
                  attributeValues[featureNr] = attributeValueItr->second.c_str();
                }
              }
            }
            if(featureInterval->attr.fillcolor.empty()==false){
              std::vector<CImageDataWriter::IndexRange> ranges=getIndexRangesForRegex(featureInterval->attr.match, attributeValues, numFeatures);
              for (size_t i=0; i<ranges.size(); i++) {
                CServerConfig::XMLE_ShadeInterval *shadeInterval = new CServerConfig::XMLE_ShadeInterval ();
                styleConfiguration->shadeIntervals->push_back(shadeInterval);
                shadeInterval->attr.min.print("%d",ranges[i].min);
                shadeInterval->attr.max.print("%d",ranges[i].max);
                shadeInterval->attr.fillcolor=featureInterval->attr.fillcolor;
                shadeInterval->attr.bgcolor=featureInterval->attr.bgcolor;
                shadeInterval->attr.label=featureInterval->attr.label;
              }
            }
          }
        }
      }
    }
  }         
    //Initialize projection algorithm
  #ifdef CIMAGEDATAWRITER_DEBUG  
  CDBDebug("Thread[%d]: initreproj %s",dataSource->threadNr,dataSource->nativeProj4.c_str());
  #endif
    CImageWarper imageWarper;
  status = imageWarper.initreproj(dataSource,drawImage->Geo,&srvParam->cfg->Projection);
  if(status!=0){
    CDBError("initreproj failed");
    reader.close();
    return 1;
  }
  
  
  
  /**
  * Use fast nearest neighbourrenderer
  */
  if(renderMethod&RM_NEAREST||renderMethod&RM_AVG_RGBA||renderMethod&RM_POINT_LINEARINTERPOLATION){
    #ifdef CIMAGEDATAWRITER_DEBUG  
      CDBDebug("Using CImgWarpNearestNeighbour");
    #endif
    imageWarperRenderer = new CImgWarpNearestNeighbour();
    imageWarperRenderer->render(&imageWarper,dataSource,drawImage);
    delete imageWarperRenderer;
  }
 
  /**
  * Use RGBA renderer
  */
  if(renderMethod&RM_RGBA){
     #ifdef CIMAGEDATAWRITER_DEBUG  
       CDBDebug("Using CImgWarpNearestRGBA");
     #endif
     imageWarperRenderer = new CImgWarpNearestRGBA();
     imageWarperRenderer->render(&imageWarper,dataSource,drawImage);
     delete imageWarperRenderer;
  }
    
  /**
  * Use bilinear renderer
  */
  if(renderMethod&RM_CONTOUR||renderMethod&RM_BILINEAR||renderMethod&RM_SHADED||renderMethod&RM_VECTOR||renderMethod&RM_BARB||renderMethod&RM_THIN)  {
    #ifdef CIMAGEDATAWRITER_DEBUG  
        CDBDebug("Using CImgWarpBilinear");
    #endif
    if(dataSource->getDataObject(0)->points.size()==0){
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
      
      if(drawMap==true)bilinearSettings.printconcat("drawMap=true;");
      if(drawVector==true)bilinearSettings.printconcat("drawVector=true;");
      if(drawBarb==true)bilinearSettings.printconcat("drawBarb=true;");
      if(drawShaded==true)bilinearSettings.printconcat("drawShaded=true;");
      if(drawContour==true)bilinearSettings.printconcat("drawContour=true;");
      if (drawGridVectors)bilinearSettings.printconcat("drawGridVectors=true;");
      bilinearSettings.printconcat("smoothingFilter=%d;",styleConfiguration->smoothingFilter);
      if(drawShaded==true||drawContour==true){
        bilinearSettings.printconcat("shadeInterval=%0.12f;contourBigInterval=%0.12f;contourSmallInterval=%0.12f;",
                                    styleConfiguration->shadeInterval,styleConfiguration->contourIntervalH,styleConfiguration->contourIntervalL);
        
        if(styleConfiguration->shadeIntervals!=NULL){
          for(size_t j=0;j<styleConfiguration->shadeIntervals->size();j++){
            CServerConfig::XMLE_ShadeInterval *shadeInterval=((*styleConfiguration->shadeIntervals)[j]);
            if(shadeInterval->attr.min.empty()==false&&shadeInterval->attr.max.empty()==false){
              bilinearSettings.printconcat("shading=min(%s)$max(%s)$",shadeInterval->attr.min.c_str(),shadeInterval->attr.max.c_str());
              if(shadeInterval->attr.fillcolor.empty()==false){bilinearSettings.printconcat("$fillcolor(%s)$",shadeInterval->attr.fillcolor.c_str());}
               if(!shadeInterval->attr.bgcolor.empty()){
                bilinearSettings.printconcat("$bgcolor(%s)$",shadeInterval->attr.bgcolor.c_str());
              }
              bilinearSettings.printconcat(";");
            }
          }
        }

          
      }
      if(drawContour==true){
        
        if(styleConfiguration->contourLines!=NULL){
          for(size_t j=0;j<styleConfiguration->contourLines->size();j++){
            CServerConfig::XMLE_ContourLine * contourLine=((*styleConfiguration->contourLines)[j]);
            //Check if we have a interval contour line or a contourline with separate classes
            if(contourLine->attr.interval.empty()==false){
              //ContourLine interval
              bilinearSettings.printconcat("contourline=");
              if(contourLine->attr.width.empty()==false){bilinearSettings.printconcat("width(%s)$",contourLine->attr.width.c_str());}
              if(contourLine->attr.linecolor.empty()==false){bilinearSettings.printconcat("linecolor(%s)$",contourLine->attr.linecolor.c_str());}
              if(contourLine->attr.textcolor.empty()==false){bilinearSettings.printconcat("textcolor(%s)$",contourLine->attr.textcolor.c_str());}
              if(contourLine->attr.interval.empty()==false){bilinearSettings.printconcat("interval(%s)$",contourLine->attr.interval.c_str());}
              if(contourLine->attr.textformatting.empty()==false){bilinearSettings.printconcat("textformatting(%s)$",contourLine->attr.textformatting.c_str());}
              bilinearSettings.printconcat(";");
            }
            if(contourLine->attr.classes.empty()==false){
              //ContourLine classes
              bilinearSettings.printconcat("contourline=");
              if(contourLine->attr.width.empty()==false){bilinearSettings.printconcat("width(%s)$",contourLine->attr.width.c_str());}
              if(contourLine->attr.linecolor.empty()==false){bilinearSettings.printconcat("linecolor(%s)$",contourLine->attr.linecolor.c_str());}
              if(contourLine->attr.textcolor.empty()==false){bilinearSettings.printconcat("textcolor(%s)$",contourLine->attr.textcolor.c_str());}
              if(contourLine->attr.classes.empty()==false){bilinearSettings.printconcat("classes(%s)$",contourLine->attr.classes.c_str());}
              if(contourLine->attr.textformatting.empty()==false){bilinearSettings.printconcat("textformatting(%s)$",contourLine->attr.textformatting.c_str());}
              bilinearSettings.printconcat(";");
            }
            
          }
          //bilinearSettings.printconcat("%s","contourline=width(3.5)$color(#0000FF)$interval(6)$textformat(%f);");
          //bilinearSettings.printconcat("%s","contourline=width(1.0)$color(#00FF00)classes(5.5,7.7)$textformat(%2.1f);");
        }
        
        //bilinearSettings.printconcat("textScaleFactor=%f;textOffsetFactor=%f;",textScaleFactor,textOffsetFactor);
      }
      #ifdef CIMAGEDATAWRITER_DEBUG  
      CDBDebug("bilinearSettings.c_str() %s",bilinearSettings.c_str());
      #endif
      imageWarperRenderer->set(bilinearSettings.c_str());
      imageWarperRenderer->render(&imageWarper,dataSource,drawImage);
      delete imageWarperRenderer;
    }
  }
  
  /**
   * Use stippling renderer
   */
  if(renderMethod&RM_STIPPLING){
    #ifdef CIMAGEDATAWRITER_DEBUG  
        CDBDebug("Using CImgRenderStippling");
    #endif
    imageWarperRenderer = new CImgRenderStippling();
    imageWarperRenderer->render(&imageWarper,dataSource,drawImage);
    delete imageWarperRenderer;
  }
  
  /**
  * Use point renderer
  */
  if(renderMethod&RM_BARB||renderMethod&RM_VECTOR||renderMethod&RM_POINT){
    if(dataSource->getDataObject(0)->points.size()!=0){
      #ifdef CIMAGEDATAWRITER_DEBUG  
        CDBDebug("Using CImgRenderPoints");
      #endif
      imageWarperRenderer = new CImgRenderPoints();
      CT::string renderMethodAsString;
      CStyleConfiguration::getRenderMethodAsString(&renderMethodAsString,renderMethod);
      imageWarperRenderer->set(renderMethodAsString.c_str());
      imageWarperRenderer->render(&imageWarper,dataSource,drawImage);
      delete imageWarperRenderer;
    }
  }
  
  /**
   * Use polyline renderer
   */
  if (renderMethod&RM_POLYLINE) {
    if (dataSource->featureSet.length()!=0) {
      #ifdef CIMAGEDATAWRITER_DEBUG  
        CDBDebug("Using CImgRenderPolylines");
      #endif
      imageWarperRenderer = new CImgRenderPolylines();
      CT::string renderMethodAsString;
      CStyleConfiguration::getRenderMethodAsString(&renderMethodAsString,renderMethod);
      imageWarperRenderer->set(renderMethodAsString.c_str());
      imageWarperRenderer->render(&imageWarper,dataSource,drawImage);
      delete imageWarperRenderer;
      
    }
  }
#ifdef MEASURETIME
  StopWatch_Stop("Thread[%d]: warp finished",dataSource->threadNr);
#endif
  //imageWarper.closereproj();
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
  CImageWarper imageWarper;
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
            
            pixel[j] = convertValue(dsj->getDataObject(0)->cdfVariable->getType(),dsj->getDataObject(0)->cdfVariable->data,ptrj);
            
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
          setValue(dataSources[0]->getDataObject(0)->cdfVariable->getType(),dataSources[0]->getDataObject(0)->cdfVariable->data,ptr,pixel[0]);
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
        if(dataSource->cfgLayer->ImageText[0]->value.empty()==false){
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
  //if(dataSource->getCurrentTimeStep()>dataSource->getCurrentTimeStep()
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

    
    /* Cascaded WMS */        
    if(dataSource->dLayerType==CConfigReaderLayerTypeCascaded){
      //CDBDebug("Drawing cascaded WMS (grid/logo/external");
      if(dataSource->cfgLayer->WMSLayer.size()==1){
        status = drawCascadedWMS(dataSource,dataSource->cfgLayer->WMSLayer[0]->attr.service.c_str(),
                                 dataSource->cfgLayer->WMSLayer[0]->attr.layer.c_str(),
                                 dataSource->cfgLayer->WMSLayer[0]->attr.style.c_str(),
                                 dataSource->cfgLayer->WMSLayer[0]->attr.transparent,
                                 dataSource->cfgLayer->WMSLayer[0]->attr.bgcolor.c_str()
                                );
        if(status!=0){
          CDBError("drawCascadedWMS for layer %s failed",dataSource->layerName.c_str());
        }
      }
    }
    
    /* DataBase layers*/        
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


        CStyleConfiguration *styleConfiguration = dataSource->getStyle();
        if(styleConfiguration->legendIndex!=-1){
          status = drawImage.createGDPalette(srvParam->cfg->Legend[styleConfiguration->legendIndex]);
          if(status != 0){
            CDBError("Unknown palette type for %s",srvParam->cfg->Legend[styleConfiguration->legendIndex]->attr.name.c_str());
            return 1;
          }
        }
        
        
      }
      
#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("Start warping");
#endif
      
      status = warpImage(dataSource,&drawImage);
      
#ifdef CIMAGEDATAWRITER_DEBUG
      CDBDebug("Finished warping %s for step %d/%d",dataSource->layerName.c_str(),dataSource->getCurrentTimeStep(),dataSource->getNumTimeSteps());
#endif      
      if(status != 0){
        CDBError("warpImage for layer %s failed",dataSource->layerName.c_str());
        return status;
      }
    }
      //if(j==dataSources.size()-1)
      {
        if(status == 0){
          
          if(dataSource->cfgLayer->ImageText.size()>0){
          
            CT::string imageText = "";
            if(dataSource->cfgLayer->ImageText[0]->value.empty()==false){
              imageText.copy(dataSource->cfgLayer->ImageText[0]->value.c_str());
            }
          
            if(dataSource->getNumDataObjects()>0){
              //Determine ImageText based on configured netcdf attribute
              const char *attrToSearch=dataSource->cfgLayer->ImageText[0]->attr.attribute.c_str();
              if(attrToSearch!=NULL){
                //CDBDebug("Determining ImageText based on netcdf attribute %s",attrToSearch);
                try{
                  CDF::Attribute *attr=dataSource->getDataObject(0)->cdfObject->getAttribute(attrToSearch);
                  if(attr->length>0){
                    imageText.copy(attrToSearch);
                    imageText.concat(": ");
                    imageText.concat(attr->toString().c_str());
                  }
                }catch(int e){
                }
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
  
    
    //draw a grid in lat/lon coordinates.
    if(dataSource->cfgLayer->Grid.size()==1){
      double gridSize=10;
      double precision=0.25;
      double numTestSteps = 5;
      CColor textColor(0,0,0,128);
      float lineWidth=0.25;
      int lineColor= 247;
      
      if(dataSource->cfgLayer->Grid[0]->attr.resolution.empty()==false){
        gridSize = parseFloat(dataSource->cfgLayer->Grid[0]->attr.resolution.c_str());
      }
      precision=gridSize/10;
      if(dataSource->cfgLayer->Grid[0]->attr.precision.empty()==false){
        precision = parseFloat(dataSource->cfgLayer->Grid[0]->attr.precision.c_str());
      }
      
      bool useProjection = true;
      
      if(srvParam->Geo->CRS.equals("EPSG:4326")){
        //CDBDebug("Not using projection");
        useProjection = false;
      }
      CImageWarper imageWarper;
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


int CImageDataWriter::getTextForValue(CT::string *tv,float v,CStyleConfiguration *styleConfiguration){
  
  int textRounding=0;
  if(styleConfiguration==NULL){
    return 1;
  }
  float legendInterval=styleConfiguration->shadeInterval;
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
     
  
  
  if(writerStatus==uninitialized){CDBError("Not initialized");return 1;}
  if(writerStatus==finished){CDBError("Already finished");return 1;}
  writerStatus=finished;
  if(srvParam->requestType==REQUEST_WMS_GETFEATUREINFO){
    #ifdef CIMAGEDATAWRITER_DEBUG    
      CDBDebug("end, number of GF results: %d",getFeatureInfoResultList.size());
    #endif
    enum ResultFormats {textplain,texthtml,textxml, applicationvndogcgml,imagepng,imagegif,json,imagepng_eprofile};
    ResultFormats resultFormat=texthtml;
    
    if(srvParam->InfoFormat.equals("text/plain"))resultFormat=textplain;
    if(srvParam->InfoFormat.equals("text/xml"))resultFormat=textxml;
    if(srvParam->InfoFormat.equals("image/png"))resultFormat=imagepng;
    if(srvParam->InfoFormat.equals("image/gif"))resultFormat=imagegif;
    
    if(srvParam->InfoFormat.equals("application/vnd.ogc.gml"))resultFormat=applicationvndogcgml;
    
    if(isProfileData){
      resultFormat=imagepng_eprofile;

    
      printf("%s%c%c\n","Content-Type:image/png",13,10);
      drawImage.printImagePng8(true);
    
      return 0;
    }
    
    
    if(srvParam->InfoFormat.indexOf("application/json")!=-1){
      
      try{
        if(gfiStructure.get("root")!=NULL)
        {
          CDBDebug("Building JSON");
          CT::string data = gfiStructure.getList("root").toJSON(CXMLPARSER_JSONMODE_STANDARD);
          CT::string resultJSON;
          if (srvParam->JSONP.length()==0) {
            CDBDebug("CREATING JSON");
            printf("%s%c%c\n","Content-Type: application/json",13,10);
          } else {
            CDBDebug("CREATING JSONP %s",srvParam->JSONP.c_str() );
            printf("%s%c%c\n%s(","Content-Type: application/javascript",13,10,srvParam->JSONP.c_str());
          }
          
          puts(data.c_str());
          
          
          if (srvParam->JSONP.length()!=0) {
            printf(");");
          }
          resetErrors();
          writerStatus=finished;
          return 0;
        }
      }catch(int e){
      }
      resultFormat=json;
    }

    
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
            resultHTML.printconcat("<hr/><b>%s (%s)</b><br/>\n",g->layerTitle.c_str(),g->layerName.c_str());
          }
          else{
            resultHTML.printconcat("%s (%s)\n",g->layerTitle.c_str(),g->layerName.c_str());
          }
          
          CT::string currentTimeString = "";
          
          if(resultFormat==texthtml){
            resultHTML.printconcat("<table>");
          }
          for(size_t elNR=0;elNR<g->elements.size();elNR++){
            GetFeatureInfoResult::Element * e=g->elements[elNR];
            
            
            //TODO
//             if(currentTimeString.equals(e->cdfDims.getDimensionValue("time").c_str())==false){
//               if(resultFormat==texthtml){
//                 resultHTML.printconcat("<tr><td>%s</td></tr>",e->cdfDims.getDimensionValue("time").c_str());
//               }else{
//                 resultHTML.printconcat("%s\n",e->cdfDims.getDimensionValue("time").c_str());
//               }
//               currentTimeString = e->cdfDims.getDimensionValue("time").c_str();
//             }
            
            
            //CDBDebug(" %d elements.size() %d value '%s'",j,g->elements.size(),e->value.c_str());
            
            if(resultFormat==texthtml){
              resultHTML.printconcat("<tr>");
            }
            if(resultFormat==texthtml){
              resultHTML.printconcat("<td>&nbsp;</td>");
            }else{
              resultHTML.printconcat("  ");
            }
            if(g->elements.size()>1){
              if(resultFormat==texthtml){
                resultHTML.printconcat("<td>-</td>");
              }else{
                resultHTML.printconcat("- ");
              }
            }
            if(e->value.length()>0){
              if(resultFormat==texthtml){
                resultHTML.printconcat("<td>%s</td><td><b>%s</b></td>",e->long_name.c_str(),e->value.c_str());
              }else{
                resultHTML.printconcat("  %s %s",e->long_name.c_str(),e->value.c_str());
              }
              if(e->units.length()>0){
                if(!e->value.equals("nodata")&&!e->value.equals("")){
                  if(resultFormat==texthtml){
                    resultHTML.printconcat("<td> %s</td>",e->units.c_str());
                  }else{
                    resultHTML.printconcat(" %s",e->units.c_str());
                  }
                }
              }
            }
            if(resultFormat==texthtml)resultHTML.printconcat("</tr>\n");else resultHTML.printconcat("\n");
          }
          if(resultFormat==texthtml){
            resultHTML.printconcat("</table>");
          }
        }
        
      }
 
      
      if(resultFormat==texthtml)resultHTML.printconcat("</html>\n");else resultHTML.printconcat("\n");
      resetErrors();
      
      
      printf("%s",resultHTML.c_str());
    }/*End of text html */
 
    /* Text XML */
    if(resultFormat==applicationvndogcgml){
      CDBDebug("CREATING GML");
      CT::string resultXML;
      resultXML.print("%s%c%c\n","Content-Type:text/xml",13,10);
      resultXML.printconcat("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
      resultXML.printconcat("  <FeatureCollection\n");
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
          
          
         // resultXML.printconcat("  <%s_layer>\n",layerName.c_str());
          resultXML.printconcat("  <gml:featureMember>\n",layerName.c_str());
          CDBDebug("GFI[%d of %d] %d\n", j, getFeatureInfoResultList.size(), g->elements.size());
          for(size_t elNR=0;elNR<g->elements.size();elNR++){
            GetFeatureInfoResult::Element * e=g->elements[elNR];
            CT::string featureName=e->feature_name.c_str();featureName.replaceSelf(" ","_");
            resultXML.printconcat("    <%s_feature>\n",featureName.c_str());
//             resultXML.concat("  <gml:boundedBy>\n");
//             resultXML.concat("  <gml:Box srsName=\"http://www.opengis.net/gml/srs/epsg.xml#4326\">\n");
//             resultXML.concat("  <gml:coordinates decimal=\".\" cs=\",\" ts=\" \">-111.05265,40.994305 -104.051201,45.002853</gml:coordinates>\n");
//             resultXML.concat("  </gml:Box>\n");
//             resultXML.concat("  </gml:boundedBy>\n");

            resultXML.printconcat("          <gml:pos>%f,%f</gml:pos>\n",g->lon_coordinate,g->lat_coordinate);
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
            //resultXML.printconcat("      <Dimension name=\"time\">%s</Dimension>\n",e->cdfDims.getDimensionValue("time").c_str());
            for(size_t d=0;d<e->cdfDims.getNumDimensions();d++){
              //TODO MUST BECOME THE OGC DIMNAME
              //resultXML.printconcat("      <Dimension name=\"%s\" index=\"%d\">%s</Dimension>\n",e->cdfDims.dimensions[d]->name.c_str(),e->cdfDims.dimensions[d]->index,e->cdfDims.dimensions[d]->value.c_str());
              resultXML.printconcat("      <Dimension name=\"%s\" index=\"%d\">%s</Dimension>\n",e->dataSource->requiredDims[d]->name.c_str(),e->cdfDims.getDimensionIndex(d),e->cdfDims.getDimensionValue(d).c_str());
            }
            
            resultXML.printconcat("    </%s_feature>\n",featureName.c_str());
          }
          //resultXML.printconcat("  </%s_layer>\n",layerName.c_str());
          resultXML.printconcat("  </gml:featureMember>\n",layerName.c_str());
        }
      }
      resultXML.printconcat(" </FeatureCollection>\n");
      resetErrors();
      printf("%s",resultXML.c_str());
    }/* End of applicationvndogcgml */
    
   if(resultFormat==textxml){
      CDBDebug("CREATING XML");
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
          CDBDebug("GFI[%d of %d] %d\n", j, getFeatureInfoResultList.size(), g->elements.size());
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
            //resultXML.printconcat("      <Dimension name=\"time\">%s</Dimension>\n",e->cdfDims.getDimensionValue("time").c_str());
            for(size_t d=0;d<e->cdfDims.getNumDimensions();d++){
              //TODO MUST BECOME THE OGC DIMNAME
              //resultXML.printconcat("      <Dimension name=\"%s\" index=\"%d\">%s</Dimension>\n",e->cdfDims.dimensions[d]->name.c_str(),e->cdfDims.dimensions[d]->index,e->cdfDims.dimensions[d]->value.c_str());
              resultXML.printconcat("      <Dimension name=\"%s\" index=\"%d\">%s</Dimension>\n",e->dataSource->requiredDims[d]->name.c_str(),e->cdfDims.getDimensionIndex(d),e->cdfDims.getDimensionValue(d).c_str());
            }
            
            resultXML.printconcat("    </%s_feature>\n",featureName.c_str());
          }
          resultXML.printconcat("  </%s_layer>\n",layerName.c_str());
        }
      }
      resultXML.printconcat(" </GMLOutput>\n");
      resetErrors();
      printf("%s",resultXML.c_str());
      
    }/* End of text/xml */

    if (resultFormat==json) {
      CT::string resultJSON;
      if (srvParam->JSONP.length()==0) {
        CDBDebug("CREATING JSON");
        resultJSON.print("%s%c%c\n","Content-Type: application/json",13,10);
      } else {
        CDBDebug("CREATING JSONP %s",srvParam->JSONP.c_str() );
        resultJSON.print("%s%c%c\n","Content-Type: application/javascript",13,10);
      }
       
      CXMLParser::XMLElement rootElement;
 
      rootElement.setName("root");
                
      for (size_t j=0; j<getFeatureInfoResultList.size(); j++) {
       //CDBDebug("gfi len: %d of %d (%d el)\n", j, getFeatureInfoResultList.size(), getFeatureInfoResultList[j]->elements.size());
        GetFeatureInfoResult *g=getFeatureInfoResultList[j];

        // Find out number of different features in getFeatureInfoResultList[j]
        std::vector<CT::string> features;

        for(size_t elNr=0;elNr<g->elements.size(); elNr++){
          GetFeatureInfoResult::Element * element = g->elements[elNr];
          bool featureNameFound=false;
          for(size_t jj=0;jj<features.size();jj++){
            if(features[jj].equals(&element->feature_name)){featureNameFound=true;break;}
          }
          if(!featureNameFound){
            features.push_back(element->feature_name.c_str());
          }else{
            break;
          }
        }
        int nrFeatures=features.size();
        
        //Find available dimensions

        GetFeatureInfoResult::Element* e=g->elements[0];

        int nrDims=e->cdfDims.getNumDimensions();
        int dimLookup[nrDims]; //position of each dimension in cdfDims.dimensions
        //CDBDebug("nrDims = %d",nrDims);
        int timeDimIndex = -1;
        int endIndex = nrDims-1;
        for(int j=0;j<nrDims;j++){
          dimLookup[j]=j;
          if (timeDimIndex == -1 && e->cdfDims.isTimeDimension(j)) {
            timeDimIndex = j;
          }
        }
        
        if(timeDimIndex != -1){
          if(timeDimIndex != endIndex){
            int a = dimLookup[timeDimIndex];
            int b = dimLookup[endIndex];
            dimLookup[timeDimIndex] = b;
            dimLookup[endIndex] = a;
          }
        }
        
     

        for (int feat=0; feat<nrFeatures; feat++) {
             //CDBDebug("A %d/%d",feat,nrFeatures);
          GetFeatureInfoResult::Element *e=g->elements[feat];
          CT::string featureName=e->feature_name.c_str();
          featureName.replaceSelf(" ", "_");
          CXMLParser::XMLElement paramElement("param");
          paramElement.add(CXMLParser::XMLElement("name", e->var_name.c_str()));
          paramElement.add(CXMLParser::XMLElement("standard_name", e->standard_name.c_str()));
          paramElement.add(CXMLParser::XMLElement("feature_name", e->feature_name.c_str()));
          paramElement.add(CXMLParser::XMLElement("units", e->units.c_str()));
           
          CXMLParser::XMLElement point("point");
          point.add(CXMLParser::XMLElement("SRS", "EPSG:4326"));
          CT::string coord;
          coord.print("%f,%f", g->lon_coordinate, g->lat_coordinate);
          point.add(CXMLParser::XMLElement("coords", coord.c_str()));
          paramElement.add(point);
           //CDBDebug("e->cdfDims.getNumDimensions() = %d",e->cdfDims.getNumDimensions());
          for (size_t d=0; d<e->cdfDims.getNumDimensions()&&int(d)<nrDims;d++) {
//            paramElement.add(CXMLParser::XMLElement("dims", e->cdfDims.dimensions[dimLookup[d]]->name.c_str()));
           // CDBDebug("d= %d, dimLookup[d] = %d, e->dataSource->requiredDims.size() = %d",d,dimLookup[d],e->dataSource->requiredDims.size());
              paramElement.add(CXMLParser::XMLElement("dims", e->dataSource->requiredDims[dimLookup[d]]->name.c_str()));
          }      
           
          std::map<std::string, std::string>dataMap;
          for (size_t elNR=feat; elNR<g->elements.size(); elNR+=nrFeatures) {
            GetFeatureInfoResult::Element *e=g->elements[elNR];
            CT::string dimString="";
            for (size_t d=0; d<e->cdfDims.getNumDimensions(); d++) {
              dimString.printconcat("%s,", e->cdfDims.getDimensionValue(dimLookup[d]).c_str());
             // CDBDebug("Pushing %d %d [%s][%s]",d,dimLookup[d],dimString.c_str(),e->value.c_str());
            }
            
            dataMap[dimString.c_str()]=e->value.c_str();
          }
  
          std::map<std::string,std::string>::iterator dkit  ;
          CXMLParser::XMLElement data("data");

          CXMLParser::XMLElement *elP = &data;
          
          for (dkit=dataMap.begin(); dkit!=dataMap.end(); ++dkit){
            
            CT::string key = dkit->first.c_str();
            CT::string value= dkit->second.c_str();
            
            CT::string *dimValues = key.splitToArray(",");
            elP = &data;
            for(size_t i=0;i<dimValues->count;i++){
              try{
                elP=elP->get(dimValues[i].c_str());
              }catch(int e){
                elP=elP->add(dimValues[i].c_str());
              }
            }
            elP->setValue(value.c_str());
            delete[] dimValues;
          }
        
          paramElement.add(data);
//          layerElement.add(paramElement);
          rootElement.add(paramElement);
        }
           
//        rootElement.add(layerElement);
      }


      resetErrors();
      if (srvParam->JSONP.length()==0) {      
#ifdef use_PARAM
        resultJSON.concat(rootElement.getList("layer").toJSON(CXMLPARSER_JSONMODE_STANDARD).c_str());
#else
        resultJSON.concat(rootElement.getList("param").toJSON(CXMLPARSER_JSONMODE_STANDARD).c_str());
#endif
        printf("%s", resultJSON.c_str());
      } else {
        resultJSON.concat(srvParam->JSONP.c_str());
        resultJSON.concat("(");
#ifdef use_PARAM
        resultJSON.concat(rootElement.getList("layer").toJSON(CXMLPARSER_JSONMODE_STANDARD).c_str());
#else
        resultJSON.concat(rootElement.getList("param").toJSON(CXMLPARSER_JSONMODE_STANDARD).c_str());
#endif
        resultJSON.concat(");");
        printf("%s", resultJSON.c_str());
      }

    } /* End of json */
    
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
        if(srvParam->cfg->WMS[0]->ContourFont[0]->attr.location.empty()==false){
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
            numDims[layerNr].push_back(element->cdfDims.getNumDimensions());
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
              for(size_t j=1;j<element->cdfDims.getNumDimensions();j++){
                plotObject->name.concat(" @");
                plotObject->name.concat(element->cdfDims.getDimensionValue(j));
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
                  for(size_t d=1;d<element->cdfDims.getNumDimensions();d++){
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
      ctime->init("seconds since 1950",NULL);
      
      double startTimeValue=0;
      double stopTimeValue=0;
      bool firstDateDone = false;
      
      float overallMinValue = 0,overallMaxValue = 1;
      bool overallMinMaxValueDone = false;
      bool overallMinMaxValueWasEstimated = false;
        
      CDBDebug("Start minmax calc");
      
      for(size_t j=0;j<plotObjects.size();j++){
        PlotObject *plotObject = plotObjects[j];
        //CDBDebug("%d) %s in %s",j,plotObject->name.c_str(),plotObject->units.c_str());
        
        //Find min and max dates
        double minDate;
        double maxDate;
        try{
          
          minDate=ctime->ISOStringToDate(plotObject->elements[0]->cdfDims.getDimensionValue("time").c_str()).offset;
        }catch(int e){
          CDBError("Time startTimeValue error %s",plotObject->elements[0]->cdfDims.getDimensionValue("time").c_str());
        }
        
        try{
          maxDate=ctime->ISOStringToDate(plotObject->elements[plotObject->length-1]->cdfDims.getDimensionValue("time").c_str()).offset;
        }catch(int e){
          CDBError("Time stopTimeValue error %s",plotObject->elements[plotObject->length-1]->cdfDims.getDimensionValue("time").c_str());
        }
        
        if(!firstDateDone){
          startTimeValue = minDate;
          stopTimeValue  = maxDate;
          firstDateDone = true;
        }else{
          if(startTimeValue>minDate)startTimeValue=minDate;
          if(stopTimeValue<maxDate)stopTimeValue=maxDate;
        }
        
//         //if(j==0)
//         {
          bool firstDone = false;
          plotObject->minValue = 0;
          plotObject->maxValue = 0;
          
          //Find min and max values
          
          for(size_t i=0;i<plotObject->length;i++){
            GetFeatureInfoResult::Element * element = plotObject->elements[i];
            //if(i%2==0)
            {
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
          }
          
        
          
          //Minmax is fixed by layer settings:
          if(plotObject->elements[0]->dataSource != NULL){
            if(plotObject->elements[0]->dataSource->stretchMinMax == false){
              //Determine min max based on given datasource settings (scale/offset/log or min/max/log in config file)
              plotObject->minValue=getValueForColorIndex(plotObject->elements[0]->dataSource,0);
              plotObject->maxValue=getValueForColorIndex(plotObject->elements[0]->dataSource,239);
            }else{
              overallMinMaxValueWasEstimated = true;
            }
          }
          
          //Increase minmax if they are the same.
          if(fabs(plotObject->minValue-plotObject->maxValue)<0.1){
            plotObject->minValue=plotObject->minValue-1;
            plotObject->maxValue=plotObject->maxValue+1;
          }
            //plotObject->minValue=plotObject->minValue-2;
            //plotObject->maxValue=plotObject->maxValue+2;
//         }else{
//           //If not first plot:
//           plotObject->minValue = overallMinValue;
//           plotObject->maxValue = overallMaxValue;
//         }    
        
        if(overallMinMaxValueDone == false){
          overallMinMaxValueDone=true;
          overallMinValue = plotObject->minValue;
          overallMaxValue = plotObject->maxValue;
        }else{

         // if(overallMinValue>plotObject->minValue)overallMinValue = plotObject->minValue;
         // if(overallMaxValue<plotObject->maxValue)overallMaxValue = plotObject->maxValue;
        }
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

      CDataSource * dataSource=getFeatureInfoResultList[0]->elements[0]->dataSource;
      //CDBDebug("tickinterval = %f",styleConfiguration->legendTickInterval);
      CStyleConfiguration *styleConfiguration = dataSource->getStyle();
      if(styleConfiguration->legendTickInterval>0.0f){
        classes=(plotObjects[0]->minValue-plotObjects[0]->maxValue)/styleConfiguration->legendTickInterval;
      }
      
      
      if(styleConfiguration->legendTickRound>0){
        tickRound = int(round(log10(styleConfiguration->legendTickRound))+3);
      }
     
      
      float scale=styleConfiguration->legendScale;
      float offset=styleConfiguration->legendOffset;
      
 
      scale=240.0f/(overallMaxValue-overallMinValue);
      offset=-overallMinValue*scale;
      
      //Init title
      size_t nrOfPlotObjectsForTitle = plotObjects.size();
      if(nrOfPlotObjectsForTitle>9)nrOfPlotObjectsForTitle=9;
      int cols=1,rows=1;
      
      if(nrOfPlotObjectsForTitle>2)cols=2;
      if(nrOfPlotObjectsForTitle>5)cols=3;
      rows=int(float(nrOfPlotObjectsForTitle)/float(cols)+0.5);
      
      //Init canvas
         
      float plotOffsetX=(width*0.05);
      float plotOffsetY = rows*10+10;
      if(plotOffsetX<50)plotOffsetX=50;

      float plotHeight=((height-plotOffsetY-30));
      float plotWidth=((width-plotOffsetX)*0.96);
      
      CDrawImage plotCanvas;
      CDrawImage lineCanvas;
      if(resultFormat==imagepng){
        plotCanvas.setTrueColor(true);
        lineCanvas.setTrueColor(true);
        lineCanvas.enableTransparency(true);
        //plotCanvas.enableTransparency(true);
        //drawImage.enableTransparency(true);
        //plotCanvas.setBGColor(0,0,255);
        if(_setTransparencyAndBGColor(srvParam,&plotCanvas)!=0){
          CDBError("Unable to do setTransparencyAndBGColor");
          return -1;
        }
      }
      if(resultFormat==imagegif){
        plotCanvas.setTrueColor(false);
        plotCanvas.setBGColor(255,255,255);
        lineCanvas.setTrueColor(false);
        lineCanvas.enableTransparency(true);
      }
      plotCanvas.createImage(int(width),int(height));
      plotCanvas.create685Palette();
      lineCanvas.createImage(int(plotWidth),int(plotHeight));
      lineCanvas.create685Palette();
        
         
      //TODO
      //plotCanvas.line(int(plotOffsetX-1),int(plotOffsetY-1),int(plotWidth+plotOffsetX),int(plotHeight+plotOffsetY),0);
      plotCanvas.line(int(plotWidth+plotOffsetX),int(plotOffsetY-1),int(plotWidth+plotOffsetX),int(plotHeight+plotOffsetY),0.5,CColor(70,70,70,255));
      plotCanvas.line(int(plotOffsetX-1),int(plotOffsetY-1),int(plotWidth+plotOffsetX),int(plotOffsetY-1),0.5,CColor(70,70,70,255));
      plotCanvas.line(int(plotOffsetX-1),int(plotWidth+plotOffsetX),int(plotWidth+plotOffsetX),int(plotWidth+plotOffsetX),0.5,CColor(70,70,70,255));
      
      //Draw Title
      CT::string title;
    
      for(size_t j=0;j<nrOfPlotObjectsForTitle;j++){
        int tx=j%cols;
        int ty=j/cols;
        size_t tp=tx*rows+ty;
        if(tp<nrOfPlotObjectsForTitle){
          CT::string title=plotObjects[tp]->name.c_str();
          int x = int(tx*((width-80)/cols)+80);
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
        if(styleConfiguration->legendLog!=0){v=pow(styleConfiguration->legendLog,v);}
    
        //if(j!=0)
        lineCanvas.line(0,(int)c,plotWidth,(int)c,0.5,CColor(0,0,128,128));
        if(tickRound==0){
          floatToString(szTemp,255,overallMinValue,overallMaxValue,v);}else{
          floatToString(szTemp,255,tickRound,v);
        }
        plotCanvas.drawText(5,int(c+plotOffsetY+3),fontLocation,8,0,szTemp,CColor(0,0,0,255),CColor(255,255,255,0));
      }
  
      float lineWidth = 2.0;
      for(size_t plotNr=0;plotNr<plotObjects.size();plotNr++){
        PlotObject *plotObject = plotObjects[plotNr];
        CColor color=getColorForPlot(plotNr,plotObjects.size());
        

      if(plotObjects.size()>5)lineWidth=0.3;
        

      
      enum GraphTimeResType { monthly,daily,hourly,minutely};
      GraphTimeResType graphTimeResType = monthly;
      if(plotObject->length>1){
        try{
          CTime::Date timePosA=ctime->ISOStringToDate(plotObject->elements[0]->cdfDims.getDimensionValue("time").c_str());
          CTime::Date timePosB=ctime->ISOStringToDate(plotObject->elements[1]->cdfDims.getDimensionValue("time").c_str());
          float timeRes = timePosB.offset-timePosA.offset;
          if(timeRes<=3600*24){
            graphTimeResType = daily;
          }
          if(timeRes<=3600*6){
            graphTimeResType = hourly;
          }
          if(timeRes<1800){
            graphTimeResType = minutely;
          }
            
        }catch(int e){
          CDBError("Time conversion exception for %s or %s",plotObject->elements[0]->cdfDims.getDimensionValue("time").c_str(),plotObject->elements[1]->cdfDims.getDimensionValue("time").c_str());
          graphTimeResType = daily;
        }
        
        //CDBDebug("%f seconds %d ",timePosB.offset-timePosA.offset,graphTimeResType);
      }
      
      double timeWidth=(stopTimeValue-startTimeValue);
        for(size_t i=0;i<plotObject->length;i++){
          CTime::Date timePos1;
          try{
            timePos1=ctime->ISOStringToDate(plotObject->elements[i]->cdfDims.getDimensionValue("time").c_str());
          }catch(int e){
            CDBError("Time conversion exception for %s",plotObject->elements[i]->cdfDims.getDimensionValue("time").c_str());
          }
          
          
          double x1=((timePos1.offset-startTimeValue)/timeWidth)*plotWidth;
        
          
          
          
          if(plotNr==0){
            
            if(graphTimeResType == hourly){
            
              if(timePos1.hour==0&&timePos1.minute==0&&timePos1.second==0){
                lineCanvas.line(x1,0,x1,plotHeight,1.1,CColor(64,64,64,200));
                 CT::string text;
                if(timePos1.day!=1){
                  text.print("%d-%s",timePos1.day,months[timePos1.month-1].c_str());
                }else{
                  text.print("%s",months[timePos1.month-1].c_str());
                }
                
                
                plotCanvas.drawText(int(x1-text.length()*4+plotOffsetX+1),int(plotOffsetY+plotHeight+16),fontLocation,8,0,text.c_str(),CColor(0,0,0,255),CColor(255,255,255,0));
              }else if(timePos1.minute==0&&timePos1.second==0){
                lineCanvas.line(x1,0,x1,plotHeight,0.5,CColor(0,0,128,128));
                char szTemp[256];snprintf(szTemp,255,"%d",timePos1.hour);
                plotCanvas.drawText(int(x1-strlen(szTemp)*2+plotOffsetX+1),int(plotOffsetY+plotHeight+8),fontLocation,5,0,szTemp,CColor(0,0,0,192),CColor(255,255,255,0));
              }else{
                lineCanvas.line(x1,0,x1,plotHeight,0.5,CColor(128,128,128,128));
              }
            }
            
            if(graphTimeResType == daily)
            {
            
              if((timePos1.day==1&&timePos1.hour==0&&timePos1.minute==0&&timePos1.second==0)||i==0){
                lineCanvas.line(x1,0,x1,plotHeight,1.1,CColor(64,64,64,200));
                
                
                
                CT::string text;
                if(timePos1.day!=1){
                  text.print("%d-%s",timePos1.day,months[timePos1.month-1].c_str());
                }else{
                  text.print("%s",months[timePos1.month-1].c_str());
                }
                
                
                plotCanvas.drawText(int(x1-text.length()*4+plotOffsetX+1),int(plotOffsetY+plotHeight+12),fontLocation,8,0,text.c_str(),CColor(0,0,0,255),CColor(255,255,255,0));
              }else if(timePos1.hour==0&&timePos1.minute==0&&timePos1.second==0){
                //lineCanvas.line(x1,0,x1,plotHeight,0.5,CColor(0,0,128,128));
                //char szTemp[256];snprintf(szTemp,255,"%d",timePos1.hour);
                //plotCanvas.drawText(int(x1-strlen(szTemp)*2+plotOffsetX+1),int(plotOffsetY+plotHeight+8),fontLocation,5,0,szTemp,CColor(0,0,0,192),CColor(255,255,255,0));
              }else{
                //lineCanvas.line(x1,0,x1,plotHeight,0.5,CColor(128,128,128,128));
              }
            }
          }
        
          if(i<plotObject->length-1){
            CTime::Date timePos2=ctime->ISOStringToDate(plotObject->elements[i+1]->cdfDims.getDimensionValue("time").c_str());
          
            double x2=((timePos2.offset-startTimeValue)/timeWidth)*plotWidth;
        
            float v1=plotObject->values[i];
            float v2=plotObject->values[i+1];
            if(v1==v1&&v2==v2){
              //if(v1>minValue[elNr]&&v1<maxValue[elNr]&&v2>minValue[elNr]&&v2<maxValue[elNr]){
              //}

              float v1l=v1;
              float v2l=v2;
              bool noData=false;
              if(styleConfiguration->legendLog!=0){
                if ((v1>0)&&(v2>0)){
                  v1l=log10(v1l)/log10(styleConfiguration->legendLog);
                  v2l=log10(v2l)/log10(styleConfiguration->legendLog);
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
      title.print("%s till %s",startDateString.c_str(),stopDateString.c_str());
      plotCanvas.drawText(int(plotWidth/2-float(title.length())*2.5),int(height-5),fontLocation,8,0,title.c_str(),CColor(0,0,0,255),CColor(255,255,255,0));
        plotCanvas.draw(int(plotOffsetX), int(plotOffsetY),0,0,&lineCanvas);

      if(resultFormat==imagepng){
        printf("%s%c%c\n","Content-Type:image/png",13,10);
        plotCanvas.printImagePng8(true);
      }
      if(resultFormat==imagegif){
        printf("%s%c%c\n","Content-Type:image/gif",13,10);
        plotCanvas.printImageGif();
      }
      #ifdef MEASURETIME
      StopWatch_Stop("/Start creating image");
      #endif
    
      for(size_t j=0;j<plotObjects.size();j++){delete plotObjects[j];};plotObjects.clear();
      //CDBDebug("Done!");
    }/* End of imagepng */
    
    
    
    for(size_t j=0;j<getFeatureInfoResultList.size();j++){delete getFeatureInfoResultList[j];getFeatureInfoResultList[j]=NULL; } getFeatureInfoResultList.clear();
  }/* End of getfeatureInfo */
  if(srvParam->requestType!=REQUEST_WMS_GETMAP&&srvParam->requestType!=REQUEST_WMS_GETLEGENDGRAPHIC)return 0;
  
  
  //Output WMS getmap results
  if(errorsOccured()){
    CREPORT_ERROR_NODOC(CT::string("Error occured during image data writing"), CReportMessage::Categories::GENERAL);
    return 1;
  }
  
  //Animation image:
  if(animation==1){
    drawImage.addImage(100);
    //drawImage.endAnimation();
    //return 0;
  }

#ifdef MEASURETIME
StopWatch_Stop("Drawing finished, start printing image");
#endif

  //Static image
//CDBDebug("srvParam->imageFormat = %d",srvParam->imageFormat);
  int status = 1;
  if(srvParam->imageFormat==IMAGEFORMAT_IMAGEPNG8){
    CDBDebug("Creating 8 bit png with alpha");
    printf("%s%c%c\n","Content-Type:image/png",13,10);
    status=drawImage.printImagePng8(true);
  }else if(srvParam->imageFormat==IMAGEFORMAT_IMAGEPNG8_NOALPHA){
    CDBDebug("Creating 8 bit png without alpha");
    printf("%s%c%c\n","Content-Type:image/png",13,10);
    status=drawImage.printImagePng8(false);
  }else if(srvParam->imageFormat==IMAGEFORMAT_IMAGEPNG24){
    CDBDebug("Creating 24 bit png");
    printf("%s%c%c\n","Content-Type:image/png",13,10);
    status=drawImage.printImagePng24();
  }else if(srvParam->imageFormat==IMAGEFORMAT_IMAGEPNG32){
    CDBDebug("Creating 32 bit png");
    printf("%s%c%c\n","Content-Type:image/png",13,10);
    status=drawImage.printImagePng32();
  }else if(srvParam->imageFormat==IMAGEFORMAT_IMAGEWEBP){
    CDBDebug("Creating 32 bit webp");
    printf("%s%c%c\n","Content-Type:image/webp",13,10);
    status=drawImage.printImageWebP32();
  }else if(srvParam->imageFormat==IMAGEFORMAT_IMAGEGIF){
    //CDBDebug("LegendGraphic GIF");
    if(animation == 0){
      printf("%s%c%c\n","Content-Type:image/gif",13,10);
    }
    status=drawImage.printImageGif();
  }else {
    //CDBDebug("LegendGraphic PNG");
    printf("%s%c%c\n","Content-Type:image/png",13,10);
    status=drawImage.printImagePng8(true);
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
//   if(dataSource->stretchMinMax){
//     if(dataSource->stretchMinMaxDone == false){
//     if(dataSource->statistics==NULL){
//       dataSource->statistics = new CDataSource::Statistics();
//       dataSource->statistics->calculate(dataSource);
//     }
//     float minValue=(float)dataSource->statistics->getMinimum();
//     float maxValue=(float)dataSource->statistics->getMaximum();
//    // CDBDebug("%f %f",minValue,maxValue);
//     //maxValue+=10;
//     float ls=240/(maxValue-minValue);
//     float lo=-(minValue*ls);
//     styleConfiguration->legendScale=ls;
//     styleConfiguration->legendOffset=lo;
//     
//     //Check for infinities
    CStyleConfiguration *styleConfiguration = dataSource->getStyle();
    if(
      styleConfiguration->legendScale!=styleConfiguration->legendScale||
      styleConfiguration->legendScale==INFINITY||
      styleConfiguration->legendScale==NAN||
      styleConfiguration->legendScale==0.0||
      styleConfiguration->legendScale==-INFINITY||
      styleConfiguration->legendOffset!=styleConfiguration->legendOffset||
      styleConfiguration->legendOffset==INFINITY||
      styleConfiguration->legendOffset==NAN||
      styleConfiguration->legendOffset==-INFINITY){
      styleConfiguration->legendScale=240.0;
      styleConfiguration->legendOffset=0;
    }
//     //CDBDebug("max=%f; min=%f",maxValue,minValue);
//     //CDBDebug("scale=%f; offset=%f",ls,lo);
//     }
//   }  
  float v=index;
  v-=styleConfiguration->legendOffset;
  v/=styleConfiguration->legendScale;
  if(styleConfiguration->legendLog!=0){
    v=pow(styleConfiguration->legendLog,v);
  }
  return v;
}
int CImageDataWriter::getColorIndexForValue(CDataSource *dataSource,float value){
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();
  float val=value;
  if(styleConfiguration->legendLog!=0)val=log10(val+.000001)/log10(styleConfiguration->legendLog);
  val*=styleConfiguration->legendScale;
  val+=styleConfiguration->legendOffset;
  if(val>=239)val=239;else if(val<0)val=0;
  return int(val);
}

CColor CImageDataWriter::getPixelColorForValue(CDataSource*dataSource,float val){
  bool isNodata=false;
  
  CColor color;
  if(dataSource->getDataObject(0)->hasNodataValue){
    if(val==float(dataSource->getDataObject(0)->dfNodataValue))isNodata=true;
    if(!(val==val))isNodata=true;
  }
  if(!isNodata) {
    CStyleConfiguration *styleConfiguration = dataSource->getStyle();
    for(size_t j=0;j<styleConfiguration->shadeIntervals->size();j++){
      CServerConfig::XMLE_ShadeInterval *shadeInterval=((*styleConfiguration->shadeIntervals)[j]);
      if(shadeInterval->attr.min.empty()==false&&shadeInterval->attr.max.empty()==false){
        if ((val>=atof(shadeInterval->attr.min.c_str()))&&(val<atof(shadeInterval->attr.max.c_str()))){
          return CColor(shadeInterval->attr.fillcolor.c_str());
        }
      }
    }
    //     if(styleConfiguration->hasLegendValueRange==1)
    //       if(val<styleConfiguration->legendLowerRange||val>styleConfiguration->legendUpperRange)isNodata=true;
    //       if(!isNodata){
    //         if(styleConfiguration->legendLog!=0)val=log10(val+.000001)/log10(styleConfiguration->legendLog);
    //         val*=styleConfiguration->legendScale;
    //         val+=styleConfiguration->legendOffset;
    //         if(val>=239)val=239;else if(val<0)val=0;
    //         return int(val);
    //       }
  }
  return color; 
}

int CImageDataWriter::createLegend(CDataSource *dataSource,CDrawImage *legendImage){
  return CCreateLegend::createLegend(dataSource,legendImage);
}

int CImageDataWriter::createLegend(CDataSource *dataSource,CDrawImage *legendImage, bool rotate){
  return CCreateLegend::createLegend(dataSource,legendImage, rotate);
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
    
    void rotateUvNorth(double &u, double &v, double rlo, double rla, float deltaX, float deltaY, CImageWarper *warper) {
      fprintf(stderr, "rotateUvNorth(%f,%f,%f,%f,%f,%f)\n", u, v, rlo, rla, deltaX, deltaY);
              double lon_pnt0, lat_pnt0;
              double lon_pntEast, lat_pntEast;
              double lon_pntNorth, lat_pntNorth;
              double dLatEast, dLonEast;
              double dLatNorth, dLonNorth;
              double xpntEastSph,ypntEastSph, zpntEastSph;
              double xpntNorthSph, ypntNorthSph, zpntNorthSph;
              double xpntNorthSphRot, ypntNorthSphRot, zpntNorthSphRot;    
              double xpnt0Sph, ypnt0Sph, zpnt0Sph;
              double xnormSph, ynormSph, znormSph;
              double xncross,  yncross,  zncross;
              double vecAngle;
              double VJaa,VJab,VJba,VJbb;
              double magnitude, newMagnitude;
              double uu;
              double vv;
#define radians(aDeg) (deg2rad*aDeg)
#define NormVector(vec0,vec1,vec2) {\
    double vecLen = sqrt(vec0*vec0 + vec1*vec1 + vec2*vec2);\
    vec0 = vec0/vecLen; vec1 = vec1/vecLen; vec2 = vec2/vecLen; }
    
#define CrossProd(vecx0,vecx1,vecx2, vecy0,vecy1,vecy2, vecz0,vecz1,vecz2) {\
    vecz0 = vecx1*vecy2 - vecy1*vecx2;\
    vecz1 = vecx2*vecy0 - vecy2*vecx0;\
    vecz2 = vecx0*vecy1 - vecy0*vecx1; }
    
              lon_pnt0=rlo;
              lat_pnt0=rla;
              lon_pntEast=lon_pnt0+deltaX;
              lat_pntEast=lat_pnt0;
              lon_pntNorth=lon_pnt0;
              lat_pntNorth=lat_pnt0+deltaY;
              warper->reprojModelToLatLon(lon_pnt0, lat_pnt0);
              warper->reprojModelToLatLon(lon_pntNorth, lat_pntNorth);
              warper->reprojModelToLatLon(lon_pntEast, lat_pntEast);
              
              // (lon_pntNorth, lat_pntNorth)
              //     ^
              //     |       (lon_pntCenter, lat_pntCenter)   center of the cell-diagonal
              //     |
              // (lon_pnt0,lat_pnt0) ----> (lon_pntEast,lat_pntEast)
              
              //lon_pntCenter = 0.5*(lon_pntNorth + lon_pntEast);
              //lat_pntCenter = 0.5*(lat_pntNorth + lat_pntEast);
              //lon_pnt0 -= lon_pntCenter; lon_pntEast -= lon_pntCenter; lon_pntNorth -= lon_pntCenter; 
              //lat_pnt0 -= lat_pntCenter; lat_pntEast -= lat_pntCenter; lat_pntNorth -= lat_pntCenter; 
              
              // This is the local coordinate system of a grid cell where we have (u,v) at location (xpnt0,ypnt0).
              
              // The local coordinate system is now centered around (lon_pnt0,lat_pnt0)
              // The vector towards north pole at this location will be (0,1,0)
              // The tangent plane at this location is XY wil a normal (0, 0, 1)
              
              // Nummerical approach using projection onto a unit sphere
              dLonNorth = radians(lon_pntNorth); dLatNorth = radians(lat_pntNorth);
              xpntNorthSph = cos(dLatNorth) * cos(dLonNorth);
              ypntNorthSph = cos(dLatNorth) * sin(dLonNorth); // # Get [dLonNorth,dLatNorth] on the unit sphere.
              zpntNorthSph = sin(dLatNorth);                   //# Only XY plane is needed.
              dLonEast = radians(lon_pntEast); dLatEast = radians(lat_pntEast);
              xpntEastSph = cos(dLatEast) * cos(dLonEast);
              ypntEastSph = cos(dLatEast) * sin(dLonEast);  // # Get [dLonEast,dLatEast] on the unit sphere.
              zpntEastSph = sin(dLatEast);                  // # Only XY plane is needed.       
              lon_pnt0 = radians(lon_pnt0); lat_pnt0 = radians(lat_pnt0);
              xpnt0Sph = cos(lat_pnt0) * cos(lon_pnt0);
              ypnt0Sph = cos(lat_pnt0) * sin(lon_pnt0);   // # Get [lon_pnt0,lat_pnt0] on the unit sphere.
              zpnt0Sph = sin(lat_pnt0);                   // # Only XY plane is needed.
              
              xpntEastSph -= xpnt0Sph; ypntEastSph -= ypnt0Sph; zpntEastSph -= zpnt0Sph;  // make vectors from points
              xpntNorthSph-= xpnt0Sph, ypntNorthSph-= ypnt0Sph; zpntNorthSph-= zpnt0Sph;
            
              NormVector( xpntEastSph,  ypntEastSph,  zpntEastSph );  // vecx
              NormVector( xpntNorthSph, ypntNorthSph, zpntNorthSph );  // vecy
              
              //vecz = CrossProd(vecx,vecy)
              CrossProd( xpntEastSph,  ypntEastSph,  zpntEastSph,  xpntNorthSph, ypntNorthSph, zpntNorthSph, \
                        xnormSph, ynormSph, znormSph);  // vec z
              //# vecn = (0.0,0.0,1.0)                   // up-vector in a global coordinate system
              //# Project vecn onto plane XY, where plane-normal is vecz
              //# vecnProjXY = vecn - D*vecz;   D= a*x1+b*y1+c*z1;  vecz=(a,b,c); vecn=(x1,y1,z1)=(0,0,1)
              //#                               D= vecz[2]*1;
              //# vecyRot = NormVector( (0.0 - vecz[2]*vecz[0],0.0  - vecz[2]*vecz[1], 1.0  - vecz[2]*vecz[2]) )

              //double Dist =  xnormSph * 0.0 +  ynormSph * 0.0 + znormSph * 1.0; // Left out for optimization
              xpntNorthSphRot =     - znormSph*xnormSph;  // xpntNorthSphRot = 0.0 - Dist*xnormSph;
              ypntNorthSphRot =     - znormSph*ynormSph;  // ypntNorthSphRot = 0.0 - Dist*ynormSph;
              zpntNorthSphRot = 1.0 - znormSph*znormSph;  // zpntNorthSphRot = 1.0 - Dist*znormSph;
              NormVector(xpntNorthSphRot, ypntNorthSphRot, zpntNorthSphRot);
              
              // This would create in 3D the rotated Easting vector; but we don't need it in this routine.
              // Left out to optimize computation
              // CrossProd( xpntNorthSphRot, ypntNorthSphRot, zpntNorthSphRot, xnormSph, ynormSph, znormSph,
              //            xpntEastSph,  ypntEastSphRot,  zpntEastSphRot ); //vecxRot = CrossProd(vecy,vecz)
              
              vecAngle = acos( (xpntNorthSph*xpntNorthSphRot + ypntNorthSph*ypntNorthSphRot + zpntNorthSph*zpntNorthSphRot) ) ;
              // Determine the sign of the angle
              CrossProd( xpntNorthSphRot, ypntNorthSphRot, zpntNorthSphRot, xpntNorthSph, ypntNorthSph, zpntNorthSph,\
                        xncross,  yncross,  zncross); 
              if ( (xncross*xnormSph + yncross*ynormSph + zncross*znormSph) > 0.0)  // dotProduct
                  vecAngle *=-1.0;
              
              xpntNorthSph = sin(vecAngle);    // Rotate the point/vector (0,1) around Z-axis with vecAngle
              ypntNorthSph = cos(vecAngle);
              xpntEastSph  =   ypntNorthSph;   // Rotate the same point/vector around Z-axis with 90 degrees
              ypntEastSph  =  -xpntNorthSph;
              
              //zpntNorthSph = 0; zpntEastSph = 0;  // not needed in 2D
              
              // 1) Build the rotation matrix and put the axes-base vectors into the matrix
              VJaa = xpntEastSph ;
              VJab = xpntNorthSph;
              VJba = ypntEastSph ;
              VJbb = ypntNorthSph; 
              

              // 2) Transform the UV vector with jacobian matrix
//              u = 0.0;  v = 6.0; // test: 6 m/s along the easting direction of the grid
              magnitude=hypot(u, v);  // old vector magnitude in the model space
              //(uu) =   (VJaa VJab) * ( u )
              //(vv)     (VJba VJbb)   ( v )
              uu = VJaa*u+VJab*v;
              vv = VJba*u+VJbb*v;
              //(uu) =   (VJaa VJab VJac) * ( u )
              //(vv)     (VJba VJbb VJbc)   ( v )
              //(ww)     (VJba VJbb VJcc)   ( w )

              // 3) Apply scaling of the vector so that the vector keeps the original length (model space)
              newMagnitude = hypot(uu, vv);
              u = uu*magnitude/newMagnitude;
              v = vv*magnitude/newMagnitude;
              
    }
    
int CImageDataWriter::createScaleBar(CGeoParams *geoParams,CDrawImage *scaleBarImage){
  return CCreateScaleBar::createScaleBar(scaleBarImage,geoParams);
}



