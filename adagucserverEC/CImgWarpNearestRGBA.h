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

#ifndef CIMGWARPNEARESTRGBA_H
#define CIMGWARPNEARESTRGBA_H
#include <float.h>
#include <pthread.h>
#include "CImageWarperRenderInterface.h"


/**
 *  This tile just runs over the datasource field, and calculates the destination pixel color over and over again when it is requested twice.
 *  This class is very fast for large datasets, with low zoom levels (zoomed out completely)
 */
class CDrawTileObjBGRA{
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
    
    dfNodataValue    = dataSource->dataObject[0]->dfNodataValue ;
    legendValueRange = dataSource->styleConfiguration->hasLegendValueRange;
    legendLowerRange = dataSource->styleConfiguration->legendLowerRange;
    legendUpperRange = dataSource->styleConfiguration->legendUpperRange;
    hasNodataValue   = dataSource->dataObject[0]->hasNodataValue;
    width = dataSource->dWidth;
    height = dataSource->dHeight;
    legendLog = dataSource->styleConfiguration->legendLog;
    if(legendLog>0){
      legendLogAsLog = log10(legendLog);
    }else{
      legendLogAsLog = 0;
    }
    legendScale = dataSource->styleConfiguration->legendScale;
    legendOffset = dataSource->styleConfiguration->legendOffset;
  }
  int drawTile(double *x_corners,double *y_corners,int &dDestX,int &dDestY);
  
};

/**
 *  This is the main class of this file. It renders the sourcedata on the destination image using nearest neighbour interpolation.
 *  It uses tile blocks to render the data. Only the corners of these tiles are projected. Not each source pixel is projected,
 *  because this is uneccessary time consuming for a visualization.
 */
class CImgWarpNearestRGBA:public CImageWarperRenderInterface{
private:
  DEF_ERRORFUNCTION();
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
    CDrawTileObjBGRA *drawTile;
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
      #ifdef CIMGWARPNEARESTRGBA_DEBUG
      CDBDebug("Drawing tile %d",j);
      #endif 
      if(ct->id>=0){
        #ifdef CIMGWARPNEARESTRGBA_DEBUG
        CDBDebug("Drawing tile id %d",ct->id);
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
      ;else return 1;
      psx[0]=dfTiledBBOX[2];
      psx[1]=dfTiledBBOX[2];
      psx[2]=dfTiledBBOX[0];
      psx[3]=dfTiledBBOX[0];
      psy[0]=dfTiledBBOX[1];
      psy[1]=dfTiledBBOX[3];
      psy[2]=dfTiledBBOX[3];
      psy[3]=dfTiledBBOX[1];
      if(warper->isProjectionRequired()){
        CT::string destinationCRS;
        warper->decodeCRS(&destinationCRS,&GeoDest->CRS);
        if(destinationCRS.indexOf("longlat")>=0){
          for(int j=0;j<4;j++){
            psx[j]*=DEG_TO_RAD;
            psy[j]*=DEG_TO_RAD;
          }
        }
        pj_transform(warper->destpj,warper->sourcepj, 4,0,psx,psy,NULL);
        if(dataSource->nativeProj4.indexOf("longlat")>=0){
          for(int j=0;j<4;j++){
            psx[j]/=DEG_TO_RAD;
            psy[j]/=DEG_TO_RAD;
          }
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
  
  //Setup projection and all other settings for the tiles to draw
  void render(CImageWarper *warper,CDataSource *dataSource,CDrawImage *drawImage){
    /*for(int y=0;y<drawImage->Geo->dHeight;y++){
      for(int x=0;x<drawImage->Geo->dWidth;x++){
        drawImage->setPixelIndexed(x,y,10);
      }
    }*/
    
    //CDBDebug("Render");
    //This enables if tiles are divided allong threads.
    int numThreads=4;
    //Threading is not needed when only one thread is specified.
    bool useThreading=true;
    if(numThreads==1)useThreading=false;
         
         warper->findExtent(dataSource,dfMaskBBOX);
    int x_div=drawImage->Geo->dWidth/16;
    int y_div=drawImage->Geo->dHeight/16;
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
        CDrawTileObjBGRA *drawTileClass= NULL;
        if(dataSource->dataObject[0]->cdfVariable->getType()==CDF_CHAR||
          dataSource->dataObject[0]->cdfVariable->getType()==CDF_BYTE||
          dataSource->dataObject[0]->cdfVariable->getType()==CDF_UBYTE
        ){
          drawTileClass = new CDrawTileObjBGRA();           //Do not keep the calculated results for CDF_CHAR
          
        }else{
          //drawTileClass = new CDrawTileObjBGRAByteCache();  //keep the calculated results
          drawTileClass = new CDrawTileObjBGRA();           //Do not keep the calculated results for CDF_CHAR
        }
        //drawTileClass = new CDrawTileObjBGRAByteCache();           //Do not keep the calculated results for CDF_CHAR
        //drawTileClass = new CDrawTileObjBGRA();  //keep the calculated results
        drawTileClass->init(dataSource,drawImage,(int)tile_width,(int)tile_height);
        
        #ifdef CIMGWARPNEARESTRGBA_DEBUG
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
