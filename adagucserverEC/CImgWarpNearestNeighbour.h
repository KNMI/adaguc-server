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

#ifndef CIMGWARPNEARESTNEIGHBOUR_H
#define CIMGWARPNEARESTNEIGHBOUR_H
#include <float.h>
#include <pthread.h>
#include "CImageWarperRenderInterface.h"
#include "CGenericDataWarper.h"

//#define CIMGWARPNEARESTNEIGHBOUR_DEBUG




/**
 *  This tile just runs over the datasource field, and calculates the destination pixel color over and over again when it is requested twice.
 *  This class is very fast for large datasets, with low zoom levels (zoomed out completely)
 */
class CDrawTileObj{
private:
  DEF_ERRORFUNCTION();
public:
  double dfTileWidth,dfTileHeight;
  double dfSourceBBOX[4];
  double dfImageBBOX[4];
  double dfNodataValue;
  double legendLowerRange;
  double legendUpperRange;
  double legendValueRange;
  double hasNodataValue;
  int width,height;
  int internalWidth,internalHeight;
  float legendLog,legendScale,legendOffset;
  float legendLogAsLog;
  CDataSource * dataSource;
  CDrawImage *drawImage;
  //size_t prev_imgpointer;
  void init(CDataSource *dataSource,CDrawImage *drawImage,int tileWidth,int tileHeight){
    this->dataSource = dataSource;
    this->drawImage = drawImage;
    dfTileWidth=tileWidth;dfTileHeight=tileHeight;
    for(int k=0;k<4;k++){
      dfSourceBBOX[k]=dataSource->dfBBOX[k];
      dfImageBBOX[k]=dataSource->dfBBOX[k];
    }
    
    //Look whether BBOX was swapped in y dir
    if(dataSource->dfBBOX[3]<dataSource->dfBBOX[1]){
      dfSourceBBOX[1]=dataSource->dfBBOX[3];
      dfSourceBBOX[3]=dataSource->dfBBOX[1];
    }
    //Look whether BBOX was swapped in x dir
    if(dataSource->dfBBOX[2]<dataSource->dfBBOX[0]){
      dfSourceBBOX[0]=dataSource->dfBBOX[2];
      dfSourceBBOX[2]=dataSource->dfBBOX[0];
    }
    
    CStyleConfiguration *styleConfiguration = dataSource->getStyle();
    
    
    dfNodataValue    = dataSource->getDataObject(0)->dfNodataValue ;
    legendValueRange = styleConfiguration->hasLegendValueRange;
    legendLowerRange = styleConfiguration->legendLowerRange;
    legendUpperRange = styleConfiguration->legendUpperRange;
    hasNodataValue   = dataSource->getDataObject(0)->hasNodataValue;
    width = dataSource->dWidth;
    height = dataSource->dHeight;
    legendLog = styleConfiguration->legendLog;
    if(legendLog>0){
      legendLogAsLog = log10(legendLog);
    }else{
      legendLogAsLog = 0;
    }
    legendScale = styleConfiguration->legendScale;
    legendOffset = styleConfiguration->legendOffset;
    
    internalWidth = width;
    internalHeight =height;
  }
  int drawTile(double *x_corners,double *y_corners,int &dDestX,int &dDestY){
    CDFType dataType=dataSource->getDataObject(0)->cdfVariable->getType();
    void *data=dataSource->getDataObject(0)->cdfVariable->data;
    switch(dataType){
      case CDF_CHAR  : return myDrawRawTile((char*)data,x_corners,y_corners,dDestX,dDestY);break;
      case CDF_BYTE  : return myDrawRawTile((char*)data,x_corners,y_corners,dDestX,dDestY);break;
      case CDF_UBYTE : return myDrawRawTile((unsigned char*)data,x_corners,y_corners,dDestX,dDestY);break;
      case CDF_SHORT : return myDrawRawTile((short*)data,x_corners,y_corners,dDestX,dDestY);break;
      case CDF_USHORT: return myDrawRawTile((ushort*)data,x_corners,y_corners,dDestX,dDestY);break;
      case CDF_INT   : return myDrawRawTile((int*)data,x_corners,y_corners,dDestX,dDestY);break;
      case CDF_UINT  : return myDrawRawTile((uint*)data,x_corners,y_corners,dDestX,dDestY);break;
      case CDF_FLOAT : return myDrawRawTile((float*)data,x_corners,y_corners,dDestX,dDestY);break;
      case CDF_DOUBLE: return myDrawRawTile((double*)data,x_corners,y_corners,dDestX,dDestY);break;
    }
    return 1;
  }
  template <class T>
  int myDrawRawTile(T*data,double *x_corners,double *y_corners,int &dDestX,int &dDestY){
    #ifdef CIMGWARPNEARESTNEIGHBOUR_DEBUG
//    CDBDebug("myDrawRawTile %f, %f, %f, %f, %d, %d %f %f",dfSourceBBOX[0],dfSourceBBOX[1],dfSourceBBOX[2],dfSourceBBOX[3],width,height,dfTileWidth,dfTileHeight);
    #endif 
    double sample_sy,sample_sx;
    double line_dx1,line_dy1,line_dx2,line_dy2;
    double rcx_1,rcy_1,rcx_2,rcy_2,rcx_3,rcy_3;
    int x,y;
    int srcpixel_x,srcpixel_y;
    int dstpixel_x,dstpixel_y;
    int k;
    rcx_1= (x_corners[0] - x_corners[3])/dfTileWidth;
    rcy_1= (y_corners[0] - y_corners[3])/dfTileWidth;
    rcx_2= (x_corners[1] - x_corners[2])/dfTileWidth;
    rcy_2= (y_corners[1] - y_corners[2])/dfTileWidth;
    for(k=0;k<4;k++){
      if(fabs(x_corners[k]-x_corners[0])>=fabs(dfSourceBBOX[2]-dfSourceBBOX[0]))break;
    }
    if(k==4){
      for(k=0;k<4;k++)
        if(x_corners[k]>dfSourceBBOX[0]&&x_corners[k]<dfSourceBBOX[2])break;
        if(k==4){
//            #ifdef CIMGWARPNEARESTNEIGHBOUR_DEBUG
//           CDBDebug("Return @ %d because %f<%f && %f>%f",__LINE__,x_corners[0],dfSourceBBOX[0],x_corners[0],dfSourceBBOX[2]);
//           #endif
          return __LINE__;
        }
    }
    for(k=0;k<4;k++){
      if(fabs(y_corners[k]-y_corners[0])>=fabs(dfSourceBBOX[3]-dfSourceBBOX[1]))break;
    }
    if(k==4){
      for(k=0;k<4;k++)
        if(y_corners[k]>dfSourceBBOX[1]&&y_corners[k]<dfSourceBBOX[3])break;
        if(k==4){
//           #ifdef CIMGWARPNEARESTNEIGHBOUR_DEBUG
//           //CDBDebug("Return @ %d",__LINE__);
//           #endif
          return __LINE__;
        }
    }
    
  
        /*
         * [D: CImgWarpNearestNeighbour.h, 490 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z dfTileWidth, dfTileHeight:  1 1
         * [D: CImgWarpNearestNeighbour.h, 491 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z datasource:  6.668803 54.172408 100.846018 -15.367730
         * [D: CImgWarpNearestNeighbour.h, 492 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z destination: 6.668803 -15.367730 100.846018 54.172408
         * [D: CImgWarpNearestNeighbour.h, 371 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z Drawing tile 0
         * [D: CImgWarpNearestNeighbour.h, 375 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z Drawing tile id 0
         * [D: CImgWarpNearestNeighbour.h, 257 in CDrawTileObj]               2012-07-25T09:52:02Z myDrawRawTile 6.668803, -15.367729, 100.846016, 54.172409
         * 
         * 
         * [D: CImgWarpNearestNeighbour.h, 490 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z dfTileWidth, dfTileHeight:  1 1
         * [D: CImgWarpNearestNeighbour.h, 491 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z datasource:  14.914277 46.821022 109.091492 -22.719116
         * [D: CImgWarpNearestNeighbour.h, 492 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z destination: 14.914277 -22.719116 109.091492 46.821022
         * [D: CImgWarpNearestNeighbour.h, 371 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z Drawing tile 0
         * [D: CImgWarpNearestNeighbour.h, 375 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z Drawing tile id 0
         * [D: CImgWarpNearestNeighbour.h, 257 in CDrawTileObj]               2012-07-25T09:53:06Z myDrawRawTile 14.914276, -22.719116, 109.091492, 46.821022
         * 
         */
      #ifdef CIMGWARPNEARESTNEIGHBOUR_DEBUG
      //CDBDebug("myDrawRawTile %f, %f, %f, %f, %d, %d %f %f",dfSourceBBOX[0],dfSourceBBOX[1],dfSourceBBOX[2],dfSourceBBOX[3],width,height,dfTileWidth,dfTileHeight);
      #endif
      line_dx1= x_corners[3];
      line_dx2= x_corners[2];
      line_dy1= y_corners[3];
      line_dy2= y_corners[2];
      
//      CDBDebug("lines: %f %f %f %f",line_dx1,line_dx2,line_dy1,line_dy2);
   //   CDBDebug("rcs: %f %f %f %f",rcx_1,rcy_1,rcx_2,rcy_2);
      
      bool isNodata=false;
      T val;
      T nodataValue=(T)dfNodataValue;
      
      size_t imgpointer;
      for(x=0;x<=dfTileWidth-1;x++){
        line_dx1+=rcx_1;line_dx2+=rcx_2;line_dy1+=rcy_1;line_dy2+=rcy_2;
        rcx_3= (line_dx2 -line_dx1)/dfTileHeight;
        rcy_3= (line_dy2 -line_dy1)/dfTileHeight;
        dstpixel_x=int(x)+dDestX;
        for(y=0;y<=dfTileHeight-1;y=y+1){
          
          dstpixel_y=y+dDestY-1;
          sample_sx=line_dx1+rcx_3*double(y);
          //CDBDebug("Drawing1 sx:%f l:%f rc:%f y:%d between %f-%f",sample_sx,line_dx1,rcx_3,y,dfSourceBBOX[0],dfSourceBBOX[2]);
          if(sample_sx>=dfSourceBBOX[0]&&sample_sx<dfSourceBBOX[2])
          {
           //  CDBDebug("Drawing2");
            sample_sy=line_dy1+rcy_3*y;
            if(sample_sy>=dfSourceBBOX[1]&&sample_sy<dfSourceBBOX[3])
            {
               //CDBDebug("Drawing3");
              srcpixel_x=int(((sample_sx-dfImageBBOX[0])/(dfImageBBOX[2]-dfImageBBOX[0]))*(width));
              if(srcpixel_x>=0&&srcpixel_x<width){
                //  CDBDebug("Drawing4");
                srcpixel_y=int(((sample_sy-dfImageBBOX[1])/(dfImageBBOX[3]-dfImageBBOX[1]))*height);
                if(srcpixel_y>=0&&srcpixel_y<height){
                  //  CDBDebug("Drawing4");
                  imgpointer=srcpixel_x+(height-1-srcpixel_y)*width;
                  //imgpointer=srcpixel_x+(dHeight-1-srcpixel_y)*dWidth;
                  val=data[imgpointer];

#ifdef ADAGUC_TILESTITCHER_DEBUG
                  
                      if(srcpixel_x ==0||srcpixel_x==width-1||srcpixel_y ==0||srcpixel_y==height-1)val=12;
                      
                      
                      if((srcpixel_x ==10||srcpixel_x==width-10)&& srcpixel_y >10 &&srcpixel_y<height-10){
                        val = 5;
                      }
                      if((srcpixel_y ==10||srcpixel_y==width-10)&& srcpixel_x >10 &&srcpixel_x<width-10){
                        val = 5;
                      }
#endif                    
                                    
                  
                  isNodata=false;
                  if(hasNodataValue){if(val==nodataValue)isNodata=true;}if(!(val==val))isNodata=true;
                  if(!isNodata)if(legendValueRange)if(val<legendLowerRange||val>legendUpperRange)isNodata=true;
                  if(!isNodata){
                    if(legendLog!=0){
                      if(val>0){
                        val=(T)(log10(val)/legendLogAsLog);
                      }else val=(T)(-legendOffset);
                    }
                    int pcolorind=(int)(val*legendScale+legendOffset);
                    //val+=legendOffset;
                    if(pcolorind>=239)pcolorind=239;else if(pcolorind<=0)pcolorind=0;
                    
                    //drawImage->setPixelIndexed(dstpixel_x,dstpixel_y,drawImage->colors[(unsigned char)val]);
                    drawImage->setPixelIndexed(dstpixel_x,dstpixel_y,pcolorind);
                  }
                }
              }
            }
          }
        }
      }
      return 0;
  }
};

/**
 *  This is the main class of this file. It renders the sourcedata on the destination image using nearest neighbour interpolation.
 *  It uses tile blocks to render the data. Only the corners of these tiles are projected. Not each source pixel is projected,
 *  because this is uneccessary time consuming for a visualization.
 */
class CImgWarpNearestNeighbour:public CImageWarperRenderInterface{
private:
  DEF_ERRORFUNCTION();
  
  
  static void drawTriangle(CDrawImage *drawImage, int *xP,int *yP, int &value);
  static void drawTriangleBil(CDrawImage *drawImage, float * destField,int *xP,int *yP, float *values);
  
  
  int set(const char *settings){
    return 0;
  }
  int status;
  
  int internalWidth;
  int internalHeight;
  
  //This class represents settings/properties for each separate tile
  class DrawTileSettings{
  public:
    int id;
    double y_corners[4],x_corners[4];
    int tile_offset_x,tile_offset_y;
    CDrawTileObj *drawTile;
  };
  //This class represents which bunch of tiles needs to be drawn by which thread.
  class DrawMultipleTileSettings{
  public:
    DrawTileSettings *ct;
    int numberOfTiles,startTile,endTile;
  };
  //This method draws th bunch of selected tiles with the proper renderer.
  //This method can be called by a thread.
  static void *drawTiles(void *arg){
    DrawMultipleTileSettings *dmf = (DrawMultipleTileSettings *)arg;
    for(int j=dmf->startTile;j<dmf->endTile&&j<dmf->numberOfTiles;j++){
      DrawTileSettings *ct = &dmf->ct[j];
      #ifdef CIMGWARPNEARESTNEIGHBOUR_DEBUG
      //CDBDebug("Drawing tile %d",j);
      #endif 
      if(ct->id>=0){
        #ifdef CIMGWARPNEARESTNEIGHBOUR_DEBUG
        //CDBDebug("Drawing tile id %d",ct->id);
        #endif 
        //int status = 
        ct->drawTile->drawTile(ct->x_corners,ct->y_corners,ct->tile_offset_x,ct->tile_offset_y);
        /*if(status!=0){
          CDBError("Unable to draw tile at line %d",status);
        }*/
      }
    }
    return arg;
  }
  //Reproject the corners of the tiles
  double y_corners[4],x_corners[4];
  double dfMaskBBOX[4];
  int reproj(CImageWarper *warper,CDataSource *dataSource,CGeoParams *GeoDest,double dfx,double dfy,double x_div,double y_div){
    double psx[4];
    double psy[4];
    double dfTiledBBOX[4];
    double dfTileW=(GeoDest->dfBBOX[2]-GeoDest->dfBBOX[0])/double(x_div);
    double dfTileH=(GeoDest->dfBBOX[3]-GeoDest->dfBBOX[1])/double(y_div);
    
    dfTiledBBOX[0]=GeoDest->dfBBOX[0]+dfTileW*dfx;
    dfTiledBBOX[1]=GeoDest->dfBBOX[1]+dfTileH*dfy;
    dfTiledBBOX[2]=dfTiledBBOX[0]+(dfTileW);
    dfTiledBBOX[3]=dfTiledBBOX[1]+(dfTileH);
    double dfSourceBBOX[4];
    for(int k=0;k<4;k++)dfSourceBBOX[k]=dfMaskBBOX[k];
    if(dfMaskBBOX[3]<dfMaskBBOX[1]){
      dfSourceBBOX[1]=dfMaskBBOX[3];
      dfSourceBBOX[3]=dfMaskBBOX[1];
    }
    if(
      (dfTiledBBOX[0]>dfSourceBBOX[0]-dfTileW&&dfTiledBBOX[0]<dfSourceBBOX[2]+dfTileW)&&
      (dfTiledBBOX[2]>dfSourceBBOX[0]-dfTileW&&dfTiledBBOX[2]<dfSourceBBOX[2]+dfTileW)&&
      (dfTiledBBOX[1]>dfSourceBBOX[1]-dfTileH&&dfTiledBBOX[1]<dfSourceBBOX[3]+dfTileH)&&
      (dfTiledBBOX[3]>dfSourceBBOX[1]-dfTileH&&dfTiledBBOX[3]<dfSourceBBOX[3]+dfTileH)
    )
    {}else{ return 1;}
    psx[0]=dfTiledBBOX[2];
    psx[1]=dfTiledBBOX[2];
    psx[2]=dfTiledBBOX[0];
    psx[3]=dfTiledBBOX[0];
    psy[0]=dfTiledBBOX[1];
    psy[1]=dfTiledBBOX[3];
    psy[2]=dfTiledBBOX[3];
    psy[3]=dfTiledBBOX[1];
    if(warper->isProjectionRequired()){
    //return 0;
    
    CT::string destinationCRS;
    warper->decodeCRS(&destinationCRS,&GeoDest->CRS);
    if(destinationCRS.indexOf("longlat")>=0){
      for(int j=0;j<4;j++){
        psx[j]*=DEG_TO_RAD;
        psy[j]*=DEG_TO_RAD;
      }
    }

    
    if(pj_transform(warper->destpj,warper->sourcepj, 4,0,psx,psy,NULL)){
      CDBDebug("Unable to do pj_transform");
    }

    
    if(dataSource->nativeProj4.indexOf("longlat")>=0)
      for(int j=0;j<4;j++){
        psx[j]/=DEG_TO_RAD;
        psy[j]/=DEG_TO_RAD;
      }
    }
    x_corners[0]=psx[1];
    y_corners[0]=psy[1];
    
    x_corners[1]=psx[0];
    y_corners[1]=psy[0];
    
    x_corners[2]=psx[3];
    y_corners[2]=psy[3];
    
    x_corners[3]=psx[2];
    y_corners[3]=psy[2];
    

    return 0;
  }
  
  template <class T>
  void _plot(CImageWarper *warper,CDataSource *dataSource,CDrawImage *drawImage){
    CStyleConfiguration *styleConfiguration = dataSource->getStyle();
    double dfNodataValue    = dataSource->getDataObject(0)->dfNodataValue ;
    double legendValueRange = styleConfiguration->hasLegendValueRange;
    double legendLowerRange = styleConfiguration->legendLowerRange;
    double legendUpperRange = styleConfiguration->legendUpperRange;
    bool hasNodataValue   = dataSource->getDataObject(0)->hasNodataValue;
    float nodataValue = (float)dfNodataValue;
    float legendLog = styleConfiguration->legendLog;
    float legendLogAsLog;
    if(legendLog>0){
    legendLogAsLog = log10(legendLog);
    }else{
    legendLogAsLog = 0;
    }
    float legendScale = styleConfiguration->legendScale;
    float legendOffset = styleConfiguration->legendOffset;
        
    T *data=(T*)dataSource->getDataObject(0)->cdfVariable->data;
    
    bool shade = false;
    if(styleConfiguration!=NULL){
      if(styleConfiguration->shadeIntervals!=NULL){
        if(styleConfiguration->shadeIntervals->size()>0){
          shade = true;
        }
      }
    }
    
    if(shade){
      int numShadeDefs=(int)styleConfiguration->shadeIntervals->size();
      T shadeDefMin[numShadeDefs];
      T shadeDefMax[numShadeDefs];
      CColor fillColors[numShadeDefs];
      CColor bgColor;
      bool hasBgColor = false;
      for(int j=0;j<numShadeDefs;j++){
        CServerConfig::XMLE_ShadeInterval *featureInterval=((*styleConfiguration->shadeIntervals)[j]);
        shadeDefMin[j] = (T)featureInterval->attr.min.toDouble();
        shadeDefMax[j] = (T)featureInterval->attr.max.toDouble();
        fillColors[j]=CColor(featureInterval->attr.fillcolor.c_str());
        if(j==0){
          if(featureInterval->attr.bgcolor.empty()==false){
            hasBgColor = true;
            bgColor = CColor(featureInterval->attr.bgcolor.c_str());
          }
        }
      }
              
      for(int y=0;y<drawImage->Geo->dHeight;y++){
        for(int x=0;x<drawImage->Geo->dWidth;x++){
          T val= data[x+y*drawImage->Geo->dWidth];
          bool drawnPixel = false;
          bool isNodata=false;
          if(hasNodataValue){if(val==nodataValue)isNodata=true;}if(!(val==val))isNodata=true;
          if(!isNodata)if(legendValueRange)if(val<legendLowerRange||val>legendUpperRange)isNodata=true;
          if(!isNodata){
            for(int snr=numShadeDefs;snr>=0;snr--){
              if(val>=shadeDefMin[snr]&&val<shadeDefMax[snr]){
                if(fillColors[snr].a == 0){ //When a fully transparent color is deliberately set, force this color in the image
                  drawImage->setPixelTrueColorOverWrite(x,(drawImage->Geo->dHeight-1)-y,fillColors[snr].r,fillColors[snr].g,fillColors[snr].b,fillColors[snr].a);  
                }else{
                  drawImage->setPixelTrueColor(x,(drawImage->Geo->dHeight-1)-y,fillColors[snr].r,fillColors[snr].g,fillColors[snr].b,fillColors[snr].a);  
                }
                drawnPixel = true;
                break;
              }
            }
          }
          if(hasBgColor&&!drawnPixel){
            drawImage->setPixelTrueColor(x,(drawImage->Geo->dHeight-1)-y,bgColor.r,bgColor.g,bgColor.b,bgColor.a);  
          }
        }
      }
    }
    
    if(shade == false){
      for(int y=0;y<drawImage->Geo->dHeight;y++){
        for(int x=0;x<drawImage->Geo->dWidth;x++){
          T val= data[x+y*drawImage->Geo->dWidth];

            bool isNodata=false;
            if(hasNodataValue){if(val==nodataValue)isNodata=true;}if(!(val==val))isNodata=true;
            if(!isNodata)if(legendValueRange)if(val<legendLowerRange||val>legendUpperRange)isNodata=true;
            if(!isNodata){
              if(legendLog!=0){
                if(val>0){
                  val=(T)(log10(val)/legendLogAsLog);
                }else val=(T)(-legendOffset);
              }
              int pcolorind=(int)(val*legendScale+legendOffset);
              //val+=legendOffset;
              if(pcolorind>=239)pcolorind=239;else if(pcolorind<=0)pcolorind=0;

              drawImage->setPixelIndexed(x,(drawImage->Geo->dHeight-1)-y,pcolorind);
            }
          
        }
      }
    }
  }
  
  class Settings{
  public:
    double dfNodataValue;
    double legendValueRange;
    double legendLowerRange;
    double legendUpperRange;
    bool hasNodataValue;
    float nodataValue;
    float legendLog;
    float legendLogAsLog;
    float legendScale;
    float legendOffset;
    CDrawImage *drawImage;
    float *rField = NULL, *gField=NULL, *bField = NULL;
    int *numField = NULL;
    bool trueColorRGBA;
  };

  
  template <class T>
  static void drawFunction(int x,int y,T val, void *_settings){
    Settings *settings = (Settings*)_settings;
    if(settings->trueColorRGBA == false){

      bool isNodata=false;
      if(settings->hasNodataValue){if(val==settings->nodataValue)isNodata=true;}if(!(val==val))isNodata=true;
      if(!isNodata)if(settings->legendValueRange)if(val<settings->legendLowerRange||val>settings->legendUpperRange)isNodata=true;
      if(!isNodata){
        if(settings->legendLog!=0){
          if(val>0){
            val=(T)(log10(val)/settings->legendLogAsLog);
          }else val=(T)(-settings->legendOffset);
        }
        int pcolorind=(int)(val*settings->legendScale+settings->legendOffset);
        //val+=legendOffset;
        if(pcolorind>=239)pcolorind=239;else if(pcolorind<=0)pcolorind=0;
      
        settings->drawImage->setPixelIndexed(x,y,pcolorind);
      }
    }else{
    

      size_t size=settings->drawImage->Geo->dWidth*settings->drawImage->Geo->dHeight;
      if(settings->rField == NULL){
        
        settings->rField = new float[size];
        settings->gField = new float[size];
        settings->bField = new float[size];
        settings->numField = new int[size];
        for(size_t j=0;j<size;j++){
          settings->rField[j] = 0;
          settings->gField[j] = 0;
          settings->bField[j] = 0;
          settings->numField[j] = 0;
        }
      }
      
      size_t p = x+y*settings->drawImage->Geo->dWidth;
      if(p<size){
            
        uint v = val;
        unsigned char r=((unsigned char)v);
        unsigned char g=((unsigned char)(v>>8));
        unsigned char b=((unsigned char)(v>>16));
        unsigned char a=((unsigned char)(v>>24));
        
          
        settings->numField[p]++;
        settings->rField[p]+=r;
        settings->gField[p]+=g;
        settings->bField[p]+=b;
        
        r= (unsigned char)(settings->rField[p]/settings->numField[p]);
        g= (unsigned char)(settings->gField[p]/settings->numField[p]);
        b= (unsigned char)(settings->bField[p]/settings->numField[p]);
      
        settings->drawImage->setPixelTrueColorOverWrite(x,y,r,g,b,a);  
      
      }
    }
  };
  
  
  //Setup projection and all other settings for the tiles to draw
  void render(CImageWarper *warper,CDataSource *dataSource,CDrawImage *drawImage){
    
    
    bool fieldsAreIdentical = true;
    if((float)dataSource->dfBBOX[0] != (float)drawImage->Geo->dfBBOX[0]){fieldsAreIdentical = false;}
    if((float)dataSource->dfBBOX[1] != (float)drawImage->Geo->dfBBOX[3]){fieldsAreIdentical = false;}
    if((float)dataSource->dfBBOX[2] != (float)drawImage->Geo->dfBBOX[2]){fieldsAreIdentical = false;}
    if((float)dataSource->dfBBOX[3] != (float)drawImage->Geo->dfBBOX[1]){fieldsAreIdentical = false;}
    if((int)dataSource->dWidth != (int)drawImage->Geo->dWidth){fieldsAreIdentical = false;}
    if((int)dataSource->dHeight != (int)drawImage->Geo->dHeight){fieldsAreIdentical = false;}
    
    if(fieldsAreIdentical){
      CDBDebug("fieldsAreIdentical: using _plot");
      CDFType dataType=dataSource->getDataObject(0)->cdfVariable->getType();
        switch(dataType){
        case CDF_CHAR  : return _plot<char>(warper,dataSource,drawImage);break;
        case CDF_BYTE  : return _plot<char>(warper,dataSource,drawImage);break;
        case CDF_UBYTE : return _plot<unsigned char>(warper,dataSource,drawImage);break;
        case CDF_SHORT : return _plot<short>(warper,dataSource,drawImage);break;
        case CDF_USHORT: return _plot<ushort>(warper,dataSource,drawImage);break;
        case CDF_INT   : return _plot<int>(warper,dataSource,drawImage);break;
        case CDF_UINT  : return _plot<uint>(warper,dataSource,drawImage);break;
        case CDF_FLOAT : return _plot<float>(warper,dataSource,drawImage);break;
        case CDF_DOUBLE: return _plot<double>(warper,dataSource,drawImage);break;
      }
      return;
    }
    
    if(dataSource->dWidth*dataSource->dHeight<720*720||1==2){
      CDBDebug("field is small enough for precise renderer: using _render");
      Settings settings;
      CStyleConfiguration *styleConfiguration = dataSource->getStyle();    
      settings.dfNodataValue    = dataSource->getDataObject(0)->dfNodataValue ;
      settings.legendValueRange = styleConfiguration->hasLegendValueRange;
      settings.legendLowerRange = styleConfiguration->legendLowerRange;
      settings.legendUpperRange = styleConfiguration->legendUpperRange;
      settings.hasNodataValue   = dataSource->getDataObject(0)->hasNodataValue;
      settings.nodataValue = (float)settings.dfNodataValue;
      settings.trueColorRGBA = false;
      settings.rField = NULL;
      settings.gField=NULL;
      settings.bField = NULL;
      settings.numField = NULL;
      settings.legendLog = styleConfiguration->legendLog;
      if(settings.legendLog>0){
        settings.legendLogAsLog = log10(settings.legendLog);
      }else{
        settings.legendLogAsLog = 0;
      }
      settings.legendScale = styleConfiguration->legendScale;
      settings.legendOffset = styleConfiguration->legendOffset;
      settings.drawImage = drawImage;
      
      CDFType dataType=dataSource->getDataObject(0)->cdfVariable->getType();
      void *sourceData = dataSource->getDataObject(0)->cdfVariable->data;
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
      
      switch(dataType){
        case CDF_CHAR  :  GenericDataWarper::render<char>  (warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
        case CDF_BYTE  :  GenericDataWarper::render<char>  (warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
        case CDF_UBYTE :  GenericDataWarper::render<unsigned char> (warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
        case CDF_SHORT :  GenericDataWarper::render<short> (warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
        case CDF_USHORT:  GenericDataWarper::render<ushort>(warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
        case CDF_INT   :  GenericDataWarper::render<int>   (warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
        case CDF_UINT  :  GenericDataWarper::render<uint>  (warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
        case CDF_FLOAT :  GenericDataWarper::render<float> (warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
        case CDF_DOUBLE:  GenericDataWarper::render<double>(warper,sourceData,&sourceGeo,drawImage->Geo,&settings,&drawFunction);break;
      }
      return;
    }
    
    CDBDebug("Render");
    //This enables if tiles are divided allong threads.
    int numThreads=4;
    //Threading is not needed when only one thread is specified.
    bool useThreading=true;
    if(numThreads==1)useThreading=false;
         
      warper->findExtent(dataSource,dfMaskBBOX);

    
    int tile_width = 16;
    int tile_height = 16;
    int x_div = 1;
    int y_div = 1;
    if(warper->isProjectionRequired()==false){
      //CDBDebug("No reprojection required");
      tile_height = drawImage->Geo->dHeight;
      tile_width = drawImage->Geo->dWidth;
      //When we are drawing just one tile, threading is not needed
      useThreading=false;
    }else{
      x_div=int((float(drawImage->Geo->dWidth)/tile_width))+1;
      y_div=int((float(drawImage->Geo->dHeight)/tile_height))+1;

    }
    internalWidth = tile_width*x_div;
    internalHeight = tile_height*y_div;
    
    //New geo location needs to be extended based on new width and height
    CGeoParams internalGeo;
    internalGeo.copy(drawImage->Geo);
    
    internalGeo.dfBBOX[2]=((drawImage->Geo->dfBBOX[2]-drawImage->Geo->dfBBOX[0])/double(drawImage->Geo->dWidth))*double(internalWidth)+drawImage->Geo->dfBBOX[0];
    internalGeo.dfBBOX[1]=((drawImage->Geo->dfBBOX[1]-drawImage->Geo->dfBBOX[3])/double(drawImage->Geo->dHeight))*double(internalHeight)+drawImage->Geo->dfBBOX[3];
    //internalGeo->dfBBOX[3]
    
    //int tile_width=int((double(drawImage->Geo->dWidth)/double(x_div))+0.5);
    //int tile_height=int((double(drawImage->Geo->dHeight)/double(y_div))+0.5);
    

    
    //Setup the renderer to draw the tiles with.We do not keep the calculated results for CDF_CHAR (faster)
        CDrawTileObj *drawTileClass= NULL;
        if(dataSource->getDataObject(0)->cdfVariable->getType()==CDF_CHAR||
          dataSource->getDataObject(0)->cdfVariable->getType()==CDF_BYTE||
          dataSource->getDataObject(0)->cdfVariable->getType()==CDF_UBYTE
        ){
          drawTileClass = new CDrawTileObj();           //Do not keep the calculated results for CDF_CHAR
          
        }else{
          //drawTileClass = new CDrawTileObjByteCache();  //keep the calculated results
          drawTileClass = new CDrawTileObj();           //Do not keep the calculated results for CDF_CHAR
        }
        //drawTileClass = new CDrawTileObjByteCache();           //Do not keep the calculated results for CDF_CHAR
        //drawTileClass = new CDrawTileObj();  //keep the calculated results
        
        //Reproj back and forth datasource boundingbox
        double y1=dataSource->dfBBOX[1];
        double y2=dataSource->dfBBOX[3];
        double x1=dataSource->dfBBOX[0];
        double x2=dataSource->dfBBOX[2];
        
        if(y2<y1){
            if(y1>-360&&y2<360&&x1>-720&&x2<720){
              if( CGeoParams::isLonLatProjection(&dataSource->nativeProj4)==false){
                double checkBBOX[4];
                for(int j=0;j<4;j++)checkBBOX[j]=dataSource->dfBBOX[j];
                
                //CDBDebug("Current BBOX:  %f %f %f %f",dataSource->dfBBOX[0],dataSource->dfBBOX[1],dataSource->dfBBOX[2],dataSource->dfBBOX[3]);
                bool hasError = false;
                if(warper->reprojpoint_inv(checkBBOX[0],checkBBOX[1])!=0)hasError=true;  
                if(warper->reprojpoint(checkBBOX[0],checkBBOX[1])!=0)hasError=true;  
                
                if(warper->reprojpoint_inv(checkBBOX[2],checkBBOX[3])!=0)hasError=true;  
                if(warper->reprojpoint(checkBBOX[2],checkBBOX[3])!=0)hasError=true;  
                
                if(hasError == false){
                  for(int j=0;j<4;j++)dataSource->dfBBOX[j] = checkBBOX[j];
                }
                
                //checkBBOX
                //CDBDebug("New BBOX:  %f %f %f %f",dataSource->dfBBOX[0],dataSource->dfBBOX[1],dataSource->dfBBOX[2],dataSource->dfBBOX[3]);
              }
          }
        }
        
        
     
        drawTileClass->init(dataSource,drawImage,tile_width,tile_height);
        
        #ifdef CIMGWARPNEARESTNEIGHBOUR_DEBUG
        CDBDebug("x_div, y_div:  %d %d",x_div,y_div);
        CDBDebug("tile_width, tile_height:  %d %d",tile_width,tile_height);
        CDBDebug("internalWidth, internalHeight:  %d %d",internalWidth,internalHeight);
        CDBDebug("datasource:  %f %f %f %f",dataSource->dfBBOX[0],dataSource->dfBBOX[1],dataSource->dfBBOX[2],dataSource->dfBBOX[3]);
        CDBDebug("destination: %f %f %f %f",internalGeo.dfBBOX[0],internalGeo.dfBBOX[1],internalGeo.dfBBOX[2],internalGeo.dfBBOX[3]);
        #endif 
        
       
        int numberOfTiles=x_div*y_div;
        DrawTileSettings *drawTileSettings = new DrawTileSettings[numberOfTiles];
        DrawTileSettings *curTileSettings;
        for(int x=0;x<x_div;x++){
          for(int y=0;y<y_div;y++){
            status = reproj(warper,dataSource,&internalGeo,x,(y_div-1)-y,x_div,y_div);
            int tileId=x+y*x_div;
            curTileSettings=&drawTileSettings[tileId];
            curTileSettings->id=tileId;
            if(status != 0)curTileSettings->id=(-tileId)-1;//This one does not need to be done.
            for(int j=0;j<4;j++){
              curTileSettings->x_corners[j]=x_corners[j];
              curTileSettings->y_corners[j]=y_corners[j];
            }
            //Some safety checks when odd files come out of the projection algorithm
            if((x_corners[0]>=DBL_MAX||x_corners[0]<=-DBL_MAX)&&x_div==1&&x_div==1){
              curTileSettings->x_corners[0]=internalGeo.dfBBOX[2];
              curTileSettings->x_corners[1]=internalGeo.dfBBOX[2];
              curTileSettings->x_corners[2]=internalGeo.dfBBOX[0];
              curTileSettings->x_corners[3]=internalGeo.dfBBOX[0];
              curTileSettings->y_corners[0]=internalGeo.dfBBOX[3];
              curTileSettings->y_corners[1]=internalGeo.dfBBOX[1];
              curTileSettings->y_corners[2]=internalGeo.dfBBOX[1];
              curTileSettings->y_corners[3]=internalGeo.dfBBOX[3];
            }
            curTileSettings->tile_offset_x=int(x*tile_width);
            curTileSettings->tile_offset_y=int(y*tile_height);
            curTileSettings->drawTile=drawTileClass;
          }
        }
        
        if(useThreading==false){
          DrawMultipleTileSettings dmf;
          dmf.ct=drawTileSettings;
          dmf.numberOfTiles=numberOfTiles;
          dmf.startTile=0;
          dmf.endTile=numberOfTiles;
          drawTiles(&dmf);
        }
        
        if(useThreading==true){
          int errcode;
          pthread_t threads[numThreads];  
          DrawMultipleTileSettings dmf[numThreads];
          int tileBlockSize=numberOfTiles/numThreads;
          //Divide the tiles over the threads, and start the thread.
          for(int j=0;j<numThreads;j++){
            dmf[j].ct=drawTileSettings;
            dmf[j].numberOfTiles=numberOfTiles;
            dmf[j].startTile=tileBlockSize*j;;
            dmf[j].endTile=tileBlockSize*(j+1);
            
            //Make sure that all blocks are processed
            if(j==numThreads-1)dmf[j].endTile=numberOfTiles;
                                                
                                                //CDBDebug("%d - start %d stop %d",j,dmf[j].startTile,dmf[j].endTile);
            DrawMultipleTileSettings *t_dmf=&dmf[j];
            errcode=pthread_create(&threads[j],NULL,drawTiles,t_dmf);
            if(errcode){CDBError("pthread_create");return ;}
          }
          // reap the threads as they exit 
          for (int worker=0; worker<numThreads; worker++) {
            int *status;//TODO fix somehow the return status of the threads.
            // wait for thread to terminate 
            errcode=pthread_join(threads[worker],(void **) &status);
            if(errcode) { CDBError("pthread_join");return;}
          }
        }
        delete[] drawTileSettings;
        delete drawTileClass;
  }
};
#endif
