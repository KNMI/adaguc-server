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

#include "CFillTriangle.h"

void fillTriangleGouraud(float  *data, float  *values, int W,int H, int *xP,int *yP){
  
  if(xP[0]<0&&xP[1]<0&&xP[2]<0)return;
  if(xP[0]>=W&&xP[1]>=W&&xP[2]>=W)return;
  if(yP[0]<0&&yP[1]<0&&yP[2]<0)return;
  if(yP[0]>=H&&yP[1]>=H&&yP[2]>=H)return;
  float yDiff[3],xDiff[3];
  int yDist[3],yDir[3];
  int startX,stopX;
  //Calculate differences
  yDiff[0] = yP[0]-yP[1];yDiff[1] = yP[1]-yP[2];yDiff[2] = yP[2]-yP[0];
  xDiff[0] = xP[0]-xP[1];xDiff[1] = xP[1]-xP[2];xDiff[2] = xP[2]-xP[0];
  //Calculate distances and directions
  yDist[0]=int(fabs(yDiff[0]));yDir[0] = yDiff[0]< 0 ? 1 : -1;
  yDist[1]=int(fabs(yDiff[1]));yDir[1] = yDiff[1]< 0 ? 1 : -1;
  yDist[2]=int(fabs(yDiff[2]));yDir[2] = yDiff[2]< 0 ? 1 : -1;
  //Determine longest side of triangle
  int longSide=0;
  if(yDist[longSide] < yDist[1]) longSide=1;
  if(yDist[longSide] < yDist[2]) longSide=2;

  if(yDist[longSide]>0){
    //Determine the other two sides of the triangle
    int shortSide1=(longSide+1)%3;
    int shortSide2=(longSide+2)%3;
    float rc2=xDiff[longSide]/yDiff[longSide];
    float scaleSSP,scaleSSN,scaleSSIncr,scaleLS;
    //calculate the color at y=max for the longside;
    scaleLS=float(yDist[shortSide2])/float(yDist[longSide]);
    float valLS = (1-scaleLS)*values[longSide]+scaleLS*values[shortSide1];
    float cx1,cx2;

    //Fill shortSide 1
    if(yDist[shortSide1]>0){
      scaleSSIncr = 1/float(yDist[shortSide1]);
      scaleSSP = 1;scaleSSN = 0;
      for(int ty=0;ty<yDist[shortSide1]+1;ty++){
        float y=ty*yDir[shortSide1];
        int y1=yP[shortSide1]+int(y);
        if(y1>=0&&y1<H){
          float rc1=xDiff[shortSide1]/yDiff[shortSide1];
          float x1=xP[shortSide1]+(rc1*y);
          float x2=xP[shortSide1]+(rc2*y);
          //printf("%f\n",scaleSSP);
          if(x1>x2){
            cx1= values[shortSide1]*scaleSSP+valLS*scaleSSN;
            cx2= values[shortSide1]*scaleSSP+values[shortSide2]*scaleSSN;
            startX=int(x2);stopX=int(x1);
          }else{
            cx2= values[shortSide1]*scaleSSP+valLS*scaleSSN;
            cx1= values[shortSide1]*scaleSSP+values[shortSide2]*scaleSSN;
            startX=int(x1);stopX=int(x2);
          }
          float colorXRC = stopX-startX;
          if(colorXRC>0){
            colorXRC=1/colorXRC;
            float scalex=0;
            for(int x=startX;x<stopX+1;x++){
              if(x>=0&&x<W){
                data[x+y1*W]=cx1*(1-scalex)+cx2*scalex;
              }
              scalex+=colorXRC;
            }
          }
        }
        scaleSSP -= scaleSSIncr;
        scaleSSN += scaleSSIncr;
      }
    }
    //Fill shortSide 2
    if(yDist[shortSide2]>0){
      scaleSSIncr = 1/float(yDist[shortSide2]);
      scaleSSP = 1;scaleSSN = 0;
      for(int ty=0;ty<yDist[shortSide2]+1;ty++){
        float y=ty*yDir[shortSide2];
        int y1=yP[shortSide2]+int(y);
        if(y1>=0&&y1<H){
          float rc1=xDiff[shortSide2]/yDiff[shortSide2];
          float x1=xP[shortSide2]+(rc1*y);
          float x2=xP[longSide]+(rc2*(y)+rc2*yDiff[shortSide2]);
          if(x1>x2){
            cx2= values[longSide]*(scaleSSN)+values[shortSide2]*scaleSSP;
            cx1= values[longSide]*(scaleSSN)+valLS*scaleSSP;
            startX=int(x2);stopX=int(x1);
          }else{
            cx1= values[longSide]*(scaleSSN)+values[shortSide2]*scaleSSP;
            cx2= values[longSide]*(scaleSSN)+valLS*scaleSSP;
            startX=int(x1);stopX=int(x2);
          }
          float colorXRC = stopX-startX;
          if(colorXRC>0){
            colorXRC=1/colorXRC;
            float scalex=0;
            for(int x=startX;x<stopX+1;x++){
              if(x>=0&&x<W){
                data[x+y1*W]=cx1*(1-scalex)+cx2*scalex;
              }
              scalex+=colorXRC;
            }
          }
        } 
        scaleSSP -= scaleSSIncr;
        scaleSSN += scaleSSIncr;
      }
    }
  }
}
    
#include <limits.h>
void fillQuadGouraud(float  *data, float  *values, int W,int H, int *xP,int *yP){
    

  

  if(xP[0]<0&&xP[1]<0&&xP[2]<0&&xP[3]<0)return;
  if(xP[0]>=W&&xP[1]>=W&&xP[2]>=W&&xP[3]>=W)return;
  if(yP[0]<0&&yP[1]<0&&yP[2]<0&&yP[3]<0)return;
  if(yP[0]>=H&&yP[1]>=H&&yP[2]>=H&&yP[3]>=H)return;
  
  
  
  int minX=xP[0];
  if(minX>xP[1])minX=xP[1];
  if(minX>xP[2])minX=xP[2];
  if(minX>xP[3])minX=xP[3];
  
  int minY=yP[0];
  if(minY>yP[1])minY=yP[1];
  if(minY>yP[2])minY=yP[2];
  if(minY>yP[3])minY=yP[3];
  
  int maxX=xP[0];
  if(maxX<xP[1])maxX=xP[1];
  if(maxX<xP[2])maxX=xP[2];
  if(maxX<xP[3])maxX=xP[3];
  
  int maxY=yP[0];
  if(maxY<yP[1])maxY=yP[1];
  if(maxY<yP[2])maxY=yP[2];
  if(maxY<yP[3])maxY=yP[3];
  
  
  if(minX==maxX||minY==maxY){
    return;
  }
  if(minX == INT_MIN)return;
  if(minY == INT_MIN)return;
  if(maxX == INT_MAX)return;
  if(maxY == INT_MAX)return;

 /*        
  int diag1X,diag2X,diag1Y,diag2Y;

  
  diag1X=(xP[2]-xP[0]);
  diag2X=(xP[3]-xP[1]);
  diag1Y=(yP[2]-yP[0]);
  diag2Y=(yP[3]-yP[1]);

  if((diag1X == 0 && diag1Y == 0) || (diag2X == 0 && diag2Y == 0)){
    //data[xP[0]+yP[0]*W]=values[0];
   // return;
  }
  {
    float diag1=sqrt(diag1X*diag1X+diag1Y*diag1Y);
    float diag2=sqrt(diag2X*diag2X+diag2Y*diag2Y);
    float area = (diag1*diag2)/2.0f;
    if(area <= 0){
      data[xP[0]+yP[0]*W]=values[0];
      return;
    }
  
  }*/
  
  //Does the quad cover the complete field?
  if(minX<0&&minY<0&&maxX>=W&&maxY>H){
    size_t l=W*H;
    float a= values[0];
    for(size_t j=0;j<l;j++)data[j]=a;
    return;
  }
 
 
//   if(minX<0)return;
//   if(minY<0)return;
//   if(maxX>W)return;
//   if(maxY>W)return;

  
  
  
  float cornerV[3];
  int cornerX[3];
  int cornerY[3];
  int cx=(xP[0]+xP[1]+xP[2]+xP[3])/4;
  int cy=(yP[0]+yP[1]+yP[2]+yP[3])/4;
  float cv=(values[0]+values[1]+values[2]+values[3])/4.0;
  
  cornerX[0]=(int)xP[0];cornerY[0]=(int)yP[0];cornerV[0]=values[0];
  cornerX[1]=(int)xP[1];cornerY[1]=(int)yP[1];cornerV[1]=values[1];
  cornerX[2]=(int)  cx   ;cornerY[2]=(int) cy    ;cornerV[2]=cv;
  fillTriangleGouraud(data, cornerV, W,H, cornerX,cornerY);
  
  cornerX[0]=(int)xP[1];cornerY[0]=(int)yP[1];cornerV[0]=values[1];
  cornerX[1]=(int)xP[3];cornerY[1]=(int)yP[3];cornerV[1]=values[3];
  cornerX[2]=(int)  cx   ;cornerY[2]=(int) cy    ;cornerV[2]=cv;
  fillTriangleGouraud(data, cornerV, W,H, cornerX,cornerY);
  
  cornerX[0]=(int)xP[3];cornerY[0]=(int)yP[3];cornerV[0]=values[3];
  cornerX[1]=(int)xP[2];cornerY[1]=(int)yP[2];cornerV[1]=values[2];
  cornerX[2]=(int)  cx   ;cornerY[2]=(int) cy    ;cornerV[2]=cv;
  fillTriangleGouraud(data, cornerV, W,H, cornerX,cornerY);
  
  cornerX[0]=(int)xP[2];cornerY[0]=(int)yP[2];cornerV[0]=values[2];
  cornerX[1]=(int)xP[0];cornerY[1]=(int)yP[0];cornerV[1]=values[0];
  cornerX[2]=(int)  cx   ;cornerY[2]=(int) cy    ;cornerV[2]=cv;
  fillTriangleGouraud(data, cornerV, W,H, cornerX,cornerY);
  
  
  
}

void drawCircle(float *data,float value,int W,int H,int orgx,int orgy,int radius){
  for(int y=-radius; y<=radius; y++){
    for(int x=-radius; x<=radius; x++){
      if(x*x+y*y < radius*radius){
        int nx=orgx+x;
        int ny=orgy+y;
        if(nx>=0&&ny>=0&&nx<W&&ny<H){
          data[nx+ny*W]=value;
        }
      }
    }
  }
}
