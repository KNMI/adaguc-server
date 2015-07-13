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

//#define CIMGWARPNEARESTRGBA_DEBUG
#include "CImgWarpNearestRGBA.h"
const char *CImgWarpNearestRGBA::className="CImgWarpNearestRGBA";
const char *CDrawTileObjBGRA::className="CDrawTileObjBGRA";
int CDrawTileObjBGRA::drawTile(double *x_corners,double *y_corners,int &dDestX,int &dDestY){
    uint *data=(uint*)dataSource->getDataObject(0)->cdfVariable->data;
    uint *imageData = (uint*)drawImage->getCanvasMemory();
    int imageWidth=drawImage->Geo->dWidth;
    int imageHeight=drawImage->Geo->dHeight;
    #ifdef CIMGWARPNEARESTRGBA_DEBUG
    CDBDebug("myDrawRawTile %f, %f, %f, %f, %f, %f %f %f",dfSourceBBOX[0],dfSourceBBOX[1],dfSourceBBOX[2],dfSourceBBOX[3],width,height,x_div,y_div);
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
        if(k==4)return __LINE__;
    }
    for(k=0;k<4;k++){
      if(fabs(y_corners[k]-y_corners[0])>=fabs(dfSourceBBOX[3]-dfSourceBBOX[1]))break;
    }
    if(k==4){
      for(k=0;k<4;k++)
        if(y_corners[k]>dfSourceBBOX[1]&&y_corners[k]<dfSourceBBOX[3])break;
        if(k==4)return __LINE__;
    }
  
        /*
         * [D: CImgWarpNearestNeighbour.h, 490 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z x_div, y_div:  1 1
         * [D: CImgWarpNearestNeighbour.h, 491 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z datasource:  6.668803 54.172408 100.846018 -15.367730
         * [D: CImgWarpNearestNeighbour.h, 492 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z destination: 6.668803 -15.367730 100.846018 54.172408
         * [D: CImgWarpNearestNeighbour.h, 371 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z Drawing tile 0
         * [D: CImgWarpNearestNeighbour.h, 375 in CImgWarpNearestNeighbour]   2012-07-25T09:52:02Z Drawing tile id 0
         * [D: CImgWarpNearestNeighbour.h, 257 in CDrawTileObjBGRA]               2012-07-25T09:52:02Z myDrawRawTile 6.668803, -15.367729, 100.846016, 54.172409
         * 
         * 
         * [D: CImgWarpNearestNeighbour.h, 490 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z x_div, y_div:  1 1
         * [D: CImgWarpNearestNeighbour.h, 491 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z datasource:  14.914277 46.821022 109.091492 -22.719116
         * [D: CImgWarpNearestNeighbour.h, 492 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z destination: 14.914277 -22.719116 109.091492 46.821022
         * [D: CImgWarpNearestNeighbour.h, 371 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z Drawing tile 0
         * [D: CImgWarpNearestNeighbour.h, 375 in CImgWarpNearestNeighbour]   2012-07-25T09:53:06Z Drawing tile id 0
         * [D: CImgWarpNearestNeighbour.h, 257 in CDrawTileObjBGRA]               2012-07-25T09:53:06Z myDrawRawTile 14.914276, -22.719116, 109.091492, 46.821022
         * 
         */
      #ifdef CIMGWARPNEARESTRGBA_DEBUG
      CDBDebug("myDrawRawTile %f, %f, %f, %f, %f, %f %f %f",dfSourceBBOX[0],dfSourceBBOX[1],dfSourceBBOX[2],dfSourceBBOX[3],width,height,x_div,y_div);
      #endif
      line_dx1= x_corners[3];
      line_dx2= x_corners[2];
      line_dy1= y_corners[3];
      line_dy2= y_corners[2];
      
      size_t imgpointer;
      for(x=0;x<=x_div-1;x++){
        line_dx1+=rcx_1;line_dx2+=rcx_2;line_dy1+=rcy_1;line_dy2+=rcy_2;
        rcx_3= (line_dx2 -line_dx1)/y_div;
        rcy_3= (line_dy2 -line_dy1)/y_div;
        dstpixel_x=int(x)+dDestX;
        for(y=0;y<=y_div-1;y=y+1){
          dstpixel_y=y+dDestY-1;
          sample_sx=line_dx1+rcx_3*double(y);
          if(sample_sx>=dfSourceBBOX[0]&&sample_sx<dfSourceBBOX[2])
          {
            sample_sy=line_dy1+rcy_3*y;
            if(sample_sy>=dfSourceBBOX[1]&&sample_sy<dfSourceBBOX[3])
            {
              srcpixel_x=int(((sample_sx-dfImageBBOX[0])/(dfImageBBOX[2]-dfImageBBOX[0]))*(width));
              if(srcpixel_x>=0&&srcpixel_x<width){
                srcpixel_y=int(((sample_sy-dfImageBBOX[1])/(dfImageBBOX[3]-dfImageBBOX[1]))*height);
                if(srcpixel_y>=0&&srcpixel_y<height)
                {
                  if(dstpixel_x>=0&&dstpixel_y>=0&&dstpixel_x<imageWidth&&dstpixel_y<imageHeight){
                    imgpointer=srcpixel_x+(height-1-srcpixel_y)*width;
                    uint v=data[imgpointer];
                   // if(v!=2147483649){;//uint(-2147483647)){
                    if(v!=4294967295){
                    //v=v*10;
                    unsigned char r=((unsigned char)v);
                    unsigned char g=((unsigned char)(v>>8));
                    unsigned char b=((unsigned char)(v>>16));
                    unsigned char a=((unsigned char)(v>>24));;
                    if(a!=255){
                      //For cairo, Alpha is precomputed into components. We need to do this here as well.
                      unsigned char r1= float(r)*(float(a)/255.);
                      unsigned char g1 =float(g)*(float(a)/255.);
                      unsigned char b1=float(b)*(float(a)/255.);
                      imageData[dstpixel_x+dstpixel_y*imageWidth]=b1+g1*256+r1*256*256+a*256*256*256;
                    }else{
                      imageData[dstpixel_x+dstpixel_y*imageWidth]=b+g*256+r*256*256+4278190080;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    return 0;
  }
