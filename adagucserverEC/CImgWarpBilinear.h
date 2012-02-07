#ifndef CImgWarpBilinear_H
#define CImgWarpBilinear_H
#include <stdlib.h>
#include "CImageWarperRenderInterface.h"

class CImgWarpBilinear:public CImageWarperRenderInterface{
  private:
    bool drawMap,enableContour,enableVector,enableBarb,enableShade;
    float shadeInterval,contourSmallInterval,contourBigInterval;
    int smoothingFilter;
    class Point{
      public:
        int x,y;
    };
    std::vector<Point*> minimaPoints;
    std::vector<Point*> maximaPoints;
    DEF_ERRORFUNCTION();
  public:
    CImgWarpBilinear(){
      drawMap=false;
      enableContour=false;
      enableVector=false;
      enableBarb=false;
      enableShade=false;
      smoothingFilter=1;
    }
    ~CImgWarpBilinear(){
      for(size_t j=0;j<minimaPoints.size();j++)delete minimaPoints[j];
      for(size_t j=0;j<maximaPoints.size();j++)delete maximaPoints[j];
    }
    void render(CImageWarper *warper,CDataSource *sourceImage,CDrawImage *drawImage);
    int set(const char *pszSettings){
      //"drawMap=false;drawContour=true;contourSmallInterval=1.0;contourBigInterval=10.0;"
      if(pszSettings==NULL)return 0;
      if(strlen(pszSettings)==0)return 0;
      CT::string settings(pszSettings);
      CT::string *nodes= settings.split(";");
      for(size_t j=0;j<nodes->count;j++){
        CT::string *values=nodes[j].split("=");
        if(values->count==2){
          if(values[0].equals("drawMap")){
            if(values[1].equals("true"))drawMap=true;
            if(values[1].equals("false"))drawMap=false;
          }
          if(values[0].equals("drawContour")){
            if(values[1].equals("true"))enableContour=true;
            if(values[1].equals("false"))enableContour=false;
          }
          if(values[0].equals("drawShaded")){
            if(values[1].equals("true"))enableShade=true;
            if(values[1].equals("false"))enableShade=false;
          }
          if(values[0].equals("drawVector")){
            if(values[1].equals("true"))enableVector=true;
            if(values[1].equals("false"))enableVector=false;
          }
          if(values[0].equals("drawBarb")){
            if(values[1].equals("true"))enableBarb=true;
            if(values[1].equals("false"))enableBarb=false;
          }
          if(values[0].equals("contourSmallInterval")){
            contourSmallInterval=values[1].toFloat();
            if(contourSmallInterval==0.0f){CDBWarning("invalid value given for contourSmallInterval: %s",pszSettings);}
          }
          if(values[0].equals("shadeInterval")){
            shadeInterval=values[1].toFloat();
            if(shadeInterval==0.0f){CDBWarning("invalid value given for shadeInterval %s",pszSettings);}
          }
          if(values[0].equals("smoothingFilter")){
            smoothingFilter=values[1].toInt();
            if(smoothingFilter<0||smoothingFilter>20){CDBWarning("invalid value given for smoothingFilter %s",pszSettings);}
          }
          
          if(values[0].equals("contourBigInterval")){
            contourBigInterval=values[1].toFloat();
            if(contourBigInterval==0.0f){CDBWarning("invalid value given for contourBigInterval %s",pszSettings);}
          }
        }
        delete[] values;
      }
      delete[] nodes;
      return 0;
    }
    
    void setValuePixel(CDataSource*sourceImage,CDrawImage*drawImage,int destX,int destY,double val){
/*  if(destX<0)return;
      if(destY<0)return;
      if(destX>drawImage->Geo->dWidth-1)return;
      if(destY>drawImage->Geo->dHeight-1)return;
      if(!sourceImage->dataObject[0]->hasNodataValue)if(val==-9999z)return;*/
      bool isNodata=false;
      
      if(sourceImage->dataObject[0]->hasNodataValue){
        if(val==sourceImage->dataObject[0]->dfNodataValue)isNodata=true;
        if(!(val==val))isNodata=true;
      }
      if(!isNodata)
        if(sourceImage->legendValueRange==1)
          if(val<sourceImage->legendLowerRange||val>sourceImage->legendUpperRange)isNodata=true;
      if(!isNodata){
        if(sourceImage->legendLog!=0)val=log10(val+.000001)/log10(sourceImage->legendLog);
        val*=sourceImage->legendScale;
        val+=sourceImage->legendOffset;
        if(val>=239)val=239;else if(val<0)val=0;
        drawImage->setPixelIndexed(destX,destY,(unsigned char)val);
      }//else drawImage->setPixelIndexed(destX,destY,248);
    }

    void drawContour(float *valueData,float fNodataValue,float interval,float smallContInterval,float bigContInterval,CDataSource *dataSource,CDrawImage *drawImage,bool drawLine, bool drawShade, bool drawText);

    void fillTriangle(float  *data, float  *values, int W,int H, int *xP,int *yP){
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
  
  /*for(int j=0;j<3;j++){
      for(int y=-1;y<2;y++){
      for(int x=-1;x<2;x++){
      data[(int)xP[j]+x+((int)yP[j]+y)*W]=values[j];
    }
    }
    }*/
  
    }
    void smoothData(float *valueData,float fNodataValue,int smoothWindow, int W,int H);




   
};

#endif
