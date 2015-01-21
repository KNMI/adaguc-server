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


//#define CIMGWARPNEARESTNEIGHBOUR_DEBUG

/**
 *  This interface represents the tile rendering classes.
 */
class CDrawTileObjInterface{
public:
  virtual void init(CDataSource *dataSource,CDrawImage *drawImage,int tileWidth,int tileHeight) = 0;
  virtual int drawTile(double *x_corners,double *y_corners,int &dDestX,int &dDestY) = 0;
  virtual ~CDrawTileObjInterface(){
  }
};


/**
 *  This tile just runs over the datasource field, and calculates the destination pixel color over and over again when it is requested twice.
 *  This class is very fast for large datasets, with low zoom levels (zoomed out completely)
 */
class CDrawTileObj:public CDrawTileObjInterface{
private:
  DEF_ERRORFUNCTION();
public:
  double x_div,y_div;
  double dfSourceBBOX[4];
  double dfImageBBOX[4];
  double dfNodataValue;
  double legendLowerRange;
  double legendUpperRange;
  double legendValueRange;
  double hasNodataValue;
  int width,height;
  float legendLog,legendScale,legendOffset;
  float legendLogAsLog;
  CDataSource * dataSource;
  CDrawImage *drawImage;
  //size_t prev_imgpointer;
  void init(CDataSource *dataSource,CDrawImage *drawImage,int tileWidth,int tileHeight){
    this->dataSource = dataSource;
    this->drawImage = drawImage;
    x_div=tileWidth+1;y_div=tileHeight+1;
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
//    CDBDebug("myDrawRawTile %f, %f, %f, %f, %d, %d %f %f",dfSourceBBOX[0],dfSourceBBOX[1],dfSourceBBOX[2],dfSourceBBOX[3],width,height,x_div,y_div);
    #endif 
    double sample_sy,sample_sx;
    double line_dx1,line_dy1,line_dx2,line_dy2;
    double rcx_1,rcy_1,rcx_2,rcy_2,rcx_3,rcy_3;
    int x,y;
    int srcpixel_x,srcpixel_y;
    int dstpixel_x,dstpixel_y;
    int k;
    rcx_1= (x_corners[0] - x_corners[3])/x_div;
    rcy_1= (y_corners[0] - y_corners[3])/x_div;
    rcx_2= (x_corners[1] - x_corners[2])/x_div;
    rcy_2= (y_corners[1] - y_corners[2])/x_div;
    for(k=0;k<4;k++){
      if(fabs(x_corners[k]-x_corners[0])>=fabs(dfSourceBBOX[2]-dfSourceBBOX[0]))break;
    }
    if(k==4){
      for(k=0;k<4;k++)
        if(x_corners[k]>dfSourceBBOX[0]&&x_corners[k]<dfSourceBBOX[2])break;
        if(k==4){
           #ifdef CIMGWARPNEARESTNEIGHBOUR_DEBUG
          CDBDebug("Return @ %d because %f<%f && %f>%f",__LINE__,x_corners[0],dfSourceBBOX[0],x_corners[0],dfSourceBBOX[2]);
          #endif
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
          #ifdef CIMGWARPNEARESTNEIGHBOUR_DEBUG
          //CDBDebug("Return @ %d",__LINE__);
          #endif
          return __LINE__;
        }
    }
    
  
        /*
         * [D: CImgWarpNearestNeighbour.h, 490 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z x_div, y_div:  1 1
         * [D: CImgWarpNearestNeighbour.h, 491 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z datasource:  6.668803 54.172408 100.846018 -15.367730
         * [D: CImgWarpNearestNeighbour.h, 492 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z destination: 6.668803 -15.367730 100.846018 54.172408
         * [D: CImgWarpNearestNeighbour.h, 371 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z Drawing tile 0
         * [D: CImgWarpNearestNeighbour.h, 375 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z Drawing tile id 0
         * [D: CImgWarpNearestNeighbour.h, 257 in CDrawTileObj]               2012-07-25T09:52:02Z myDrawRawTile 6.668803, -15.367729, 100.846016, 54.172409
         * 
         * 
         * [D: CImgWarpNearestNeighbour.h, 490 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z x_div, y_div:  1 1
         * [D: CImgWarpNearestNeighbour.h, 491 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z datasource:  14.914277 46.821022 109.091492 -22.719116
         * [D: CImgWarpNearestNeighbour.h, 492 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z destination: 14.914277 -22.719116 109.091492 46.821022
         * [D: CImgWarpNearestNeighbour.h, 371 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z Drawing tile 0
         * [D: CImgWarpNearestNeighbour.h, 375 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z Drawing tile id 0
         * [D: CImgWarpNearestNeighbour.h, 257 in CDrawTileObj]               2012-07-25T09:53:06Z myDrawRawTile 14.914276, -22.719116, 109.091492, 46.821022
         * 
         */
      #ifdef CIMGWARPNEARESTNEIGHBOUR_DEBUG
      //CDBDebug("myDrawRawTile %f, %f, %f, %f, %d, %d %f %f",dfSourceBBOX[0],dfSourceBBOX[1],dfSourceBBOX[2],dfSourceBBOX[3],width,height,x_div,y_div);
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
      for(x=0;x<=x_div;x++){
        line_dx1+=rcx_1;line_dx2+=rcx_2;line_dy1+=rcy_1;line_dy2+=rcy_2;
        rcx_3= (line_dx2 -line_dx1)/y_div;
        rcy_3= (line_dy2 -line_dy1)/y_div;
        dstpixel_x=int(x)+dDestX;
        for(y=0;y<=y_div;y=y+1){
          
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
                  isNodata=false;
                  if(hasNodataValue){if(val==nodataValue)isNodata=true;else if(!(val==val))isNodata=true;}
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
  
  //This class represents settings/properties for each separate tile
  class DrawTileSettings{
  public:
    int id;
    double y_corners[4],x_corners[4];
    int tile_offset_x,tile_offset_y;
    CDrawTileObjInterface *drawTile;
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
    for(int y=0;y<drawImage->Geo->dHeight;y++){
      for(int x=0;x<drawImage->Geo->dWidth;x++){
        T val= data[x+y*drawImage->Geo->dWidth];

          bool isNodata=false;
          if(hasNodataValue){if(val==nodataValue)isNodata=true;else if(!(val==val))isNodata=true;}
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

            drawImage->setPixelIndexed(x,drawImage->Geo->dHeight-y,pcolorind);
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
  };

  template <class T>
  void drawTriangle(CDrawImage *drawImage, int *xP,int *yP, void *sourceData,int sourceWidth,int sourceHeight,size_t *indexX,size_t *indexY,Settings *settings){
    

   //Sort the vertices in Y direction
    int W = drawImage->Geo->dWidth;
    int H = drawImage->Geo->dHeight;
    if(xP[0]<0&&xP[1]<0&&xP[2]<0)return;
    if(xP[0]>=W&&xP[1]>=W&&xP[2]>=W)return;
    if(yP[0]<0&&yP[1]<0&&yP[2]<0)return;
    if(yP[0]>=H&&yP[1]>=H&&yP[2]>=H)return;  

    
    T *data=(T*)sourceData;

    
    unsigned int lower;
    unsigned int middle;
    unsigned int upper;
    
    if(yP[0]<yP[1]){
      if(yP[0]<yP[2]){
        lower=0;
        if(yP[1]<yP[2]){
          middle=1;
          upper=2;
        }else{
          middle=2;
          upper=1;
        }
      }else{
        middle=0;
        lower=2;
        upper=1;
      }
    }else{
      if(yP[1]<yP[2]){
        lower=1;
        if(yP[0]<yP[2]){
          middle=0;
          upper=2;
        }else{
          middle=2;
          upper=0;
        }
      }else{
        middle=1;
        lower=2;
        upper=0;
      }
    }
    
    int X1 = xP[lower];
    int X2 = xP[middle];
    int X3 = xP[upper];
    int Y1 = yP[lower];
    int Y2 = yP[middle];
    int Y3 = yP[upper];
    
    if(Y1 == Y3)return;
    
    if(Y2==Y1&&Y3==Y2)return;
    
    

    
    
   
    
    float VX1 = indexX[lower];
    float VX2 = indexX[middle];
    float VX3 = indexX[upper];
    
    float VY1 = indexY[lower];
    float VY2 = indexY[middle];
    float VY3 = indexY[upper];

    float rcl = float(X3-X1)/float(Y3-Y1);
    float rcvxl = (VX3-VX1)/float(Y3-Y1);
    float rcvyl = (VY3-VY1)/float(Y3-Y1);
    
    
    if(Y2!=Y1&&Y1<H&&Y2>0){
      float rca = float(X2-X1)/float(Y2-Y1);
      float rcvxa = (VX2-VX1)/float(Y2-Y1);
      float rcvya = (VY2-VY1)/float(Y2-Y1);

    
      int sy = (Y1<0)?0:Y1;
      int ey = (Y2>H)?H:Y2;
      
      for(int y=sy;y<ey;y++){
        int xL = (int)(rcl*float(y-Y1)+X1);
        int xA = (int)(rca*float(y-Y1)+X1);
        float vxL = rcvxl*float(y-Y1)+VX1;
        float vxA = rcvxa*float(y-Y1)+VX1;
        float vyL = rcvyl*float(y-Y1)+VY1;
        float vyA = rcvya*float(y-Y1)+VY1;
        int x1,x2;
        float vx1,vx2,vy1,vy2;
        if(xL<xA){x1=xL;x2=xA;vx1=vxL;vx2=vxA;vy1=vyL;vy2=vyA;}else{x2=xL;x1=xA;vx1=vxA;vx2=vxL;vy1=vyA;vy2=vyL;}
        if(x1<W&&x2>0){
          int sx = (x1<0)?0:x1;
          int ex = (x2>W)?W:x2;
          float rcxvx = float(vx2-vx1)/float(x2-x1);
          float rcxvy = float(vy2-vy1)/float(x2-x1);
          for(int x=sx;x<ex;x++){
            
            //data[x+y*W]=rcxv*float(x-x1)+v1;
            int vx = int(rcxvx*float(x-x1)+vx1);
            int vy = int(rcxvy*float(x-x1)+vy1);
  //           int v = vy;
  //           if(v<0)v=0;if(v>239)v=239;
  //           drawImage->setPixelIndexed(x,y,v);
            if(vx>=0&&vy>=0&&vx<sourceWidth+1&&vy<sourceHeight){
              T val= data[vx+vy*sourceWidth];
              bool isNodata=false;
              if(settings->hasNodataValue){if(val==settings->nodataValue)isNodata=true;else if(!(val==val))isNodata=true;}
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
                drawImage->setPixelIndexed(x,y,pcolorind);
              }
            }
          }
        }
      }
    }
    
    if(Y3 != Y2&&Y2<H&&Y3>0){
      float rcb = float(X3-X2)/float(Y3-Y2);
      float rcvxb = (VX3-VX2)/float(Y3-Y2);
      float rcvyb = (VY3-VY2)/float(Y3-Y2);
   
      int sy = (Y2<0)?0:Y2;
      int ey = (Y3>H)?H:Y3;
      
      for(int y=sy;y<ey;y++){
        
     
        int xL = (int)(rcl*float(y-Y1)+X1);
        int xB = (int)(rcb*float(y-Y2)+X2);
        float vxL = rcvxl*float(y-Y1)+VX1;
        float vxB = rcvxb*float(y-Y2)+VX2;
        float vyL = rcvyl*float(y-Y1)+VY1;
        float vyB = rcvyb*float(y-Y2)+VY2;
        int x1,x2;
        float vx1,vx2,vy1,vy2;
        if(xL<=xB){x1=xL;x2=xB;vx1=vxL;vx2=vxB;vy1=vyL;vy2=vyB;}else{x2=xL;x1=xB;vx1=vxB;vx2=vxL;vy1=vyB;vy2=vyL;}
        if(x1<W&&x2>0){
          int sx = (x1<0)?0:x1;
          int ex = (x2>W)?W:x2;
          float rcxvx = float(vx2-vx1)/float(x2-x1);
          float rcxvy = float(vy2-vy1)/float(x2-x1);
          
          for(int x=sx;x<ex;x++){
            int vx = int(rcxvx*float(x-x1)+vx1);
            int vy = int(rcxvy*float(x-x1)+vy1);
  //           int v=vy;
  //           if(v<0)v=0;if(v>239)v=239;
  //           drawImage->setPixelIndexed(x,y,v);
            if(vx>=0&&vy>=0&&vx<sourceWidth+1&&vy<sourceHeight){
              T val= data[vx+vy*sourceWidth];
              bool isNodata=false;
              if(settings->hasNodataValue){if(val==settings->nodataValue)isNodata=true;else if(!(val==val))isNodata=true;}
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
                drawImage->setPixelIndexed(x,y,pcolorind);
              }
            }
          }
        } 
      }
    }
    
//     drawImage->line(X1,Y1,X2,Y2,240);
//     drawImage->line(X1,Y1,X3,Y3,240);
//     drawImage->line(X2,Y2,X3,Y3,240);
    
  }

  
  template <class T>
  void _render(CImageWarper *warper,CDataSource *dataSource,CDrawImage *drawImage){
    
    size_t orgDataSize = dataSource->dWidth*dataSource->dHeight;
    bool drawBil = false;
    double stride = 1;
    
    if(!drawBil){  
      while(orgDataSize/(stride*stride)>512*1024){
        stride*=2;
      }
    }
    
    //stride=100;
    //stride=32;
 //  stride = 8;
    //int stridei = stride;
//    CDBDebug("STRIDE = %d",stridei);
    int dataWidth = (float(dataSource->dWidth)/stride+0.0);
    int dataHeight = (float(dataSource->dHeight)/stride+0.0);
    int imageWidth = drawImage->Geo->dWidth;
    int imageHeight = drawImage->Geo->dHeight;
    bool destNeedsDegreeRadianConversion = false;
    bool sourceNeedsDegreeRadianConversion = false;
    
      //Reproj back and forth datasource boundingbox
    double y1=dataSource->dfBBOX[1];
    double y2=dataSource->dfBBOX[3];
    double x1=dataSource->dfBBOX[0];
    double x2=dataSource->dfBBOX[2];
    
    

    
    if(y2<y1){
        if(y1>-360&&y2<360&&x1>-720&&x2<720){
          if(dataSource->srvParams->isLonLatProjection(&dataSource->nativeProj4)==false){
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
//     
    double dfSourceExtW=(dataSource->dfBBOX[2]-dataSource->dfBBOX[0]);
    double dfSourceExtH=(dataSource->dfBBOX[1]-dataSource->dfBBOX[3]);
    double dfCellSizeX = dataSource->dfCellSizeX;
    double dfCellSizeY = dataSource->dfCellSizeY;
    double dfSourceW = double(dataSource->dWidth);
    double dfSourceH = double(dataSource->dHeight);
    double dfSourcedExtW=dfSourceExtW/dfSourceW;
    double dfSourcedExtH=dfSourceExtH/dfSourceH;
    double dfSourceOrigX=dataSource->dfBBOX[0];
    double dfSourceOrigY=dataSource->dfBBOX[3];
    
    double dfDestExtW = drawImage->Geo->dfBBOX[2]-drawImage->Geo->dfBBOX[0];
    double dfDestExtH = drawImage->Geo->dfBBOX[1]-drawImage->Geo->dfBBOX[3];
    double dfDestOrigX = drawImage->Geo->dfBBOX[0];
    double dfDestOrigY = drawImage->Geo->dfBBOX[3];

    
    double multiDestX = double(imageWidth+1)/dfDestExtW;
    double multiDestY = double(imageHeight+1)/dfDestExtH;
    
    size_t dataSize = (dataWidth+1) * (dataHeight+1);

    
    CT::string destinationCRS;
    warper->decodeCRS(&destinationCRS,&drawImage->Geo->CRS);
    if(destinationCRS.indexOf("longlat")>=0){
      destNeedsDegreeRadianConversion = true;
    }
    if(dataSource->nativeProj4.indexOf("longlat")>=0){
      sourceNeedsDegreeRadianConversion = true;
    }
    
    
//     CDBDebug("SRC: %d %s",sourceNeedsDegreeRadianConversion,dataSource->nativeProj4.c_str());
//     CDBDebug("DST: %d %s",destNeedsDegreeRadianConversion,destinationCRS.c_str());
    
    double *px = new double[dataSize];
    double *py = new double[dataSize];
    char *skip = new char[dataSize];
    
//     CDBDebug("(%f,%f)  %f %f",dfSourceOrigX,dfSourceOrigY,dfSourcedExtW,dfSourcedExtH);
    
    
    if(drawBil){
      for(int y=0;y<dataHeight+1;y++){
        for(int x=0;x<dataWidth+1;x++){
          size_t p = x+y*(dataWidth+1);
          px[p] =dfSourcedExtW*double(x)+dfSourceOrigX+dfCellSizeX/2.0;
          py[p] =dfSourcedExtH*double(y)+dfSourceOrigY+dfCellSizeY/2.0;
          skip[p] = false;
        }
      }
    }else{
      for(int y=0;y<dataHeight+1;y++){
        for(int x=0;x<dataWidth+1;x++){
          size_t p = x+y*(dataWidth+1);
          px[p] =dfSourcedExtW*double(x*stride)+dfSourceOrigX;
          py[p] =dfSourcedExtH*double(y*stride)+dfSourceOrigY;
          skip[p] = false;
        }
      }
    }
    
    if(warper->isProjectionRequired()){
      
      if(sourceNeedsDegreeRadianConversion){
        for(size_t j=0;j<dataSize;j++){
          px[j]*=DEG_TO_RAD;
          py[j]*=DEG_TO_RAD;
        }
      }
      if(pj_transform(warper->sourcepj,warper->destpj, dataSize,0,px,py,NULL)){
        CDBDebug("Unable to do pj_transform");
      }
      if(destNeedsDegreeRadianConversion){
        for(size_t j=0;j<dataSize;j++){
          //if(j<10){CDBDebug("%f %f",px[j],py[j]);}
          px[j]/=DEG_TO_RAD;
          py[j]/=DEG_TO_RAD;
        }
      }
    }

//     CDBDebug("--");
    for(size_t j=0;j<dataSize;j++){
      if(px[j]>-DBL_MAX&&px[j]<DBL_MAX){
        px[j]-=dfDestOrigX;
        py[j]-=dfDestOrigY;
        px[j]*=multiDestX;
        py[j]*=multiDestY;
        py[j]--;
      }else{
        skip[j]=true;        
      }
    }
   
    double avgDX = 0;
    double avgDY = 0;
    
    
    Settings settings;
    CStyleConfiguration *styleConfiguration = dataSource->getStyle();    
    settings.dfNodataValue    = dataSource->getDataObject(0)->dfNodataValue ;
    settings.legendValueRange = styleConfiguration->hasLegendValueRange;
    settings.legendLowerRange = styleConfiguration->legendLowerRange;
    settings.legendUpperRange = styleConfiguration->legendUpperRange;
    settings.hasNodataValue   = dataSource->getDataObject(0)->hasNodataValue;
    settings.nodataValue = (float)settings.dfNodataValue;
    settings.legendLog = styleConfiguration->legendLog;
    if(settings.legendLog>0){
      settings.legendLogAsLog = log10(settings.legendLog);
    }else{
      settings.legendLogAsLog = 0;
    }
    settings.legendScale = styleConfiguration->legendScale;
    settings.legendOffset = styleConfiguration->legendOffset;
    
    if(drawBil){

//       float *destField = new float[imageWidth*imageHeight];
//       T *data=(T*)dataSource->getDataObject(0)->cdfVariable->data;
//    
//       for(int y=0;y<dataHeight;y++){
//         for(int x=0;x<dataWidth;x++){
//       
//           
//           bool isNodata=false;
//           
//           
//           T val[4];
//           val[0] = data[x+(y)*(dataWidth)];
//           val[1] = data[x+1+(y)*(dataWidth)];
//           val[2] = data[x+(y+1)*(dataWidth)];
//           val[3] = data[x+1+(y+1)*(dataWidth)];
//           for(int j=0;j<4&&!isNodata;j++){
//             if(settings.hasNodataValue){if(val[j]==settings.nodataValue)isNodata=true;else if(!(val[j]==val[j]))isNodata=true;}
//             if(!isNodata)if(settings.legendValueRange)if(val[j]<settings.legendLowerRange||val[j]>settings.legendUpperRange)isNodata=true;
//           }
//           
//           
//           if(!isNodata){
//             int pcolorind[4];
//             for(int j=0;j<4;j++){
//               if(settings.legendLog!=0){
//                 if(val[j]>0){
//                   val[j]=(T)(log10(val[j])/settings.legendLogAsLog);
//                 }else val[j]=(T)(-settings.legendOffset);
//               }
//               pcolorind[j]=(int)(val[j]*settings.legendScale+settings.legendOffset);
//               
//               if(pcolorind[j]>=239)pcolorind[j]=239;else if(pcolorind[j]<=0)pcolorind[j]=0;
//             }
//             size_t p=x+y*(dataWidth+1);
//             if(skip[p]==false&&skip[p+1]==false&&skip[p+dataWidth+1]==false&&skip[p+dataWidth+2]==false){
//               double px1 = px[p];
//               double px2 = px[p+1];
//               double px3 = px[p+dataWidth+2];
//               double px4 = px[p+dataWidth+1];
// 
//               double py1 = py[p];
//               double py2 = py[p+1];
//               double py3 = py[p+dataWidth+2];
//               double py4 = py[p+dataWidth+1];
//               bool doDraw = true;
//               
//               if(x==0)avgDX = px1;
//               if(y==0)avgDY = py1;
//               
//               if(x==dataWidth-1){
//                 if(fabs(avgDX-px2)>fabs(dfCellSizeX)){
//                   doDraw = false;
//                 }
//               }
//               if(y==dataHeight-1){
//                 if(fabs(avgDY-py4)>fabs(dfCellSizeY)){
//                   doDraw = false;
//                 }
//               }
//               
//               if(doDraw){
//                 float values [4];
//           
//                 double mX = (px1+px2+px3+px4)/4;
//                 double mY = (py1+py2+py3+py4)/4;
//                 
//                 float middleValue = (pcolorind[0]+pcolorind[1]+pcolorind[2]+pcolorind[3])/4;
//                 
//                 int xP[3];
//                 int yP[3];
//                 xP[0] = px1;
//                 xP[1] = px2;
//                 xP[2] = mX;
// 
//                 yP[0] = py1;
//                 yP[1] = py2;
//                 yP[2] = mY;
//                 
//                 values[0] = pcolorind[0];
//                 values[1] = pcolorind[1];
//                 values[2] = middleValue;
//               
//             
//                 //drawTriangleBil(CDrawImage *drawImage, int *xP,int *yP, float *values);
//                 drawTriangleBil(drawImage,destField,xP,yP, values);
// 
//                 xP[0] = px3;
//                 yP[0] = py3;
//                 values[0]=pcolorind[3];
//                 drawTriangleBil(drawImage,destField, xP,yP, values);
// 
//                 xP[1]=px4;
//                 yP[1]=py4;
//                 values[1]=pcolorind[2];
//                 drawTriangleBil(drawImage,destField, xP,yP, values);
// 
//                 xP[0] = px1;
//                 yP[0] = py1;
//                 values[0]=pcolorind[0];
//                 drawTriangleBil(drawImage,destField, xP,yP, values);
//                
//               }
//             }
//           }
//         }
//       }
    }else{
   
    /*  
      for(int y=0;y<dataHeight*stride/2;y++){
        for(int x=0;x<dataWidth*stride;x++){
            bool onx = false;
            bool ony = false;
            if(y%2==0)ony = true;
            if((x+ony)%2==0)onx = true;
            size_t p=x+y*((dataWidth)*stride);
            T *data=(T*)dataSource->getDataObject(0)->cdfVariable->data;
            data[p]=0;
            if(onx)data[p]=2550;
            
        }
      }*/
    
      void *sourceData = dataSource->getDataObject(0)->cdfVariable->data;
      int sourceWidth = dataWidth;//dataSource->dWidth;
      int sourceHeight = dataHeight;//dataSource->dHeight;
      for(int y=0;y<dataHeight;y++){
        for(int x=0;x<dataWidth;x++){
                  
              
          size_t p=x+y*(dataWidth+1);
          if(skip[p]==false&&skip[p+1]==false&&skip[p+dataWidth+1]==false&&skip[p+dataWidth+2]==false)
          {
            double px1 = px[p];
            double px2 = px[p+1];
            double px3 = px[p+dataWidth+2];
            double px4 = px[p+dataWidth+1];

            double py1 = py[p];
            double py2 = py[p+1];
            double py3 = py[p+dataWidth+2];
            double py4 = py[p+dataWidth+1];
            bool doDraw = true;
            
            if(x==0)avgDX = px2;
            if(y==0)avgDY = py4;
            
            if(x==dataWidth-1){
              if(fabs(avgDX-px1)<fabs(px1-px2)/2){
                doDraw = false;
              }
              if(fabs(avgDX-px2)<fabs(px1-px2)/2){
                doDraw = false;
              }
            }
            if(y==dataHeight-1){
              if(fabs(avgDY-py1)<fabs(py1-py4)/2){
                doDraw = false;
              }
//               if(fabs(avgDY-py4)<fabs(py1-py4)/2){
//                 doDraw = false;
//               }
            }
            
            if(doDraw)
            {

//             if(
//               (px1>=0&&px1<imageWidth)||
//               (px2>=0&&px2<imageWidth)||
//               (px3>=0&&px3<imageWidth)||
//               (px4>=0&&px4<imageWidth)||
//               (py1>=0&&py1<imageHeight)||
//               (py2>=0&&py2<imageHeight)||
//               (py3>=0&&py3<imageHeight)||
//               (py4>=0&&py4<imageHeight))
            {
              
              double mX = (px1+px2+px3+px4)/4;
              double mY = (py1+py2+py3+py4)/4;
              int xP[3];
              int yP[3];
              xP[0] = px1;
              xP[1] = px2;
              xP[2] = mX;

              yP[0] = py1;
              yP[1] = py2;
              yP[2] = mY;
              


                size_t xs[3],ys[3];
                xs[0]=x*stride;
                xs[1]=x*stride+stride;
                xs[2]=x*stride+stride/2;
                ys[0]=y*stride;
                ys[1]=y*stride;
                ys[2]=y*stride+stride/2;
                drawTriangle<T>(drawImage, xP,yP,sourceData,sourceWidth,sourceHeight,xs,ys,&settings);
                
                xP[0] = px3;
                yP[0] = py3;
                xs[0]=x*stride+stride;
                ys[0]=y*stride+stride;
                drawTriangle<T>(drawImage, xP,yP,sourceData,sourceWidth,sourceHeight,xs,ys,&settings);

                xP[1]=px4;
                yP[1]=py4;
                xs[1]=x*stride;
                ys[1]=y*stride+stride;

                drawTriangle<T>(drawImage, xP,yP,sourceData,sourceWidth,sourceHeight,xs,ys,&settings);

                xP[0] = px1;
                yP[0] = py1;
                xs[0]=x*stride;
                ys[0]=y*stride;
                drawTriangle<T>(drawImage, xP,yP,sourceData,sourceWidth,sourceHeight,xs,ys,&settings);
              }
            }
          }
        }
      }
    }
  
  
    
    delete[] px;
    delete[] py;
    delete[] skip;
  }
  //Setup projection and all other settings for the tiles to draw
  void render(CImageWarper *warper,CDataSource *dataSource,CDrawImage *drawImage){
    
    
    bool fieldsAreIdentical = true;
    if((float)dataSource->dfBBOX[0] != (float)drawImage->Geo->dfBBOX[0]){fieldsAreIdentical = false;}
    if((float)dataSource->dfBBOX[1] != (float)drawImage->Geo->dfBBOX[3]){fieldsAreIdentical = false;}
    if((float)dataSource->dfBBOX[2] != (float)drawImage->Geo->dfBBOX[2]){fieldsAreIdentical = false;}
    if((float)dataSource->dfBBOX[3] != (float)drawImage->Geo->dfBBOX[1]){fieldsAreIdentical = false;}
    if((int)dataSource->dWidth != (int)drawImage->Geo->dWidth){fieldsAreIdentical = false;}
    
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
    
    if(dataSource->dWidth*dataSource->dHeight<512*512||1==2){
      CDBDebug("field is small enough for precise renderer: using _render");
      CDFType dataType=dataSource->getDataObject(0)->cdfVariable->getType();
        switch(dataType){
        case CDF_CHAR  : return _render<char>(warper,dataSource,drawImage);break;
        case CDF_BYTE  : return _render<char>(warper,dataSource,drawImage);break;
        case CDF_UBYTE : return _render<unsigned char>(warper,dataSource,drawImage);break;
        case CDF_SHORT : return _render<short>(warper,dataSource,drawImage);break;
        case CDF_USHORT: return _render<ushort>(warper,dataSource,drawImage);break;
        case CDF_INT   : return _render<int>(warper,dataSource,drawImage);break;
        case CDF_UINT  : return _render<uint>(warper,dataSource,drawImage);break;
        case CDF_FLOAT : return _render<float>(warper,dataSource,drawImage);break;
        case CDF_DOUBLE: return _render<double>(warper,dataSource,drawImage);break;
      }
      return;
    }
    
    //CDBDebug("Render");
    //This enables if tiles are divided allong threads.
    int numThreads=4;
    //Threading is not needed when only one thread is specified.
    bool useThreading=true;
    if(numThreads==1)useThreading=false;
         
         warper->findExtent(dataSource,dfMaskBBOX);
    int x_div=drawImage->Geo->dWidth/16;
    int y_div=drawImage->Geo->dHeight/16;
//     int x_div=drawImage->Geo->dWidth;
//     int y_div=drawImage->Geo->dHeight;
     //useThreading=false;
    if(warper->isProjectionRequired()==false){
      //CDBDebug("No reprojection required");
      x_div=1;
      y_div=1;
      //When we are drawing just one tile, threading is not needed
      useThreading=false;
    }
    double tile_width=(double(drawImage->Geo->dWidth)/double(x_div));
    double tile_height=(double(drawImage->Geo->dHeight)/double(y_div));
    
    //Setup the renderer to draw the tiles with.We do not keep the calculated results for CDF_CHAR (faster)
        CDrawTileObjInterface *drawTileClass= NULL;
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
              if(dataSource->srvParams->isLonLatProjection(&dataSource->nativeProj4)==false){
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
        
        
     
        drawTileClass->init(dataSource,drawImage,(int)tile_width,(int)tile_height);
        
        #ifdef CIMGWARPNEARESTNEIGHBOUR_DEBUG
        CDBDebug("x_div, y_div:  %d %d",x_div,y_div);
        CDBDebug("datasource:  %f %f %f %f",dataSource->dfBBOX[0],dataSource->dfBBOX[1],dataSource->dfBBOX[2],dataSource->dfBBOX[3]);
        CDBDebug("destination: %f %f %f %f",drawImage->Geo->dfBBOX[0],drawImage->Geo->dfBBOX[1],drawImage->Geo->dfBBOX[2],drawImage->Geo->dfBBOX[3]);
        #endif 
        
       
        int numberOfTiles=x_div*y_div;
        DrawTileSettings *drawTileSettings = new DrawTileSettings[numberOfTiles];
        DrawTileSettings *curTileSettings;
        for(int x=0;x<x_div;x++){
          for(int y=0;y<y_div;y++){
            status = reproj(warper,dataSource,drawImage->Geo,x,(y_div-1)-y,x_div,y_div);
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
              curTileSettings->x_corners[0]=drawImage->Geo->dfBBOX[2];
              curTileSettings->x_corners[1]=drawImage->Geo->dfBBOX[2];
              curTileSettings->x_corners[2]=drawImage->Geo->dfBBOX[0];
              curTileSettings->x_corners[3]=drawImage->Geo->dfBBOX[0];
              curTileSettings->y_corners[0]=drawImage->Geo->dfBBOX[3];
              curTileSettings->y_corners[1]=drawImage->Geo->dfBBOX[1];
              curTileSettings->y_corners[2]=drawImage->Geo->dfBBOX[1];
              curTileSettings->y_corners[3]=drawImage->Geo->dfBBOX[3];
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
