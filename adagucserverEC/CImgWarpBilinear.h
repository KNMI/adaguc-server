#ifndef CImgWarpBilinear_H
#define CImgWarpBilinear_H
#include <stdlib.h>
#include "CFillTriangle.h"
#include "CImageWarperRenderInterface.h"


class CalculatedWindVector {
public: 
  CalculatedWindVector() {}
  CalculatedWindVector(int x, int y, double dir, double strength, bool convertToKnots, bool flip) {
    this->x=x;
    this->y=y;
    this->dir=dir;
    this->strength=strength;
    this->convertToKnots=convertToKnots;
    this->flip=flip;
  }
  int x, y;
  double dir, strength;
  bool convertToKnots, flip;
};

class CImgWarpBilinear:public CImageWarperRenderInterface{
  private:
    bool drawMap,enableContour,enableVector,enableBarb,enableShade,drawGridVectors;
    float shadeInterval,contourSmallInterval,contourBigInterval;
    int smoothingFilter;
    
    std::vector<PointD*> minimaPoints;
    std::vector<PointD*> maximaPoints;
    DEF_ERRORFUNCTION();
  public:
    CImgWarpBilinear(){
      drawMap=false;
      enableContour=false;
      enableVector=false;
      enableBarb=false;
      enableShade=false;
      smoothingFilter=1;
      drawGridVectors=false;
    }
    ~CImgWarpBilinear(){
      for(size_t j=0;j<minimaPoints.size();j++)delete minimaPoints[j];
      for(size_t j=0;j<maximaPoints.size();j++)delete maximaPoints[j];
    }
    void render(CImageWarper *warper,CDataSource *sourceImage,CDrawImage *drawImage);
    int set(const char *pszSettings){
      //fprintf(stderr, "CImgWarpBilinear.set(%s)\n", pszSettings);
      //"drawMap=false;drawContour=true;contourSmallInterval=1.0;contourBigInterval=10.0;"
      if(pszSettings==NULL)return 0;
      if(strlen(pszSettings)==0)return 0;
      CT::string settings(pszSettings);
      CT::string *nodes= settings.splitToArray(";");
      for(size_t j=0;j<nodes->count;j++){
        CT::string *values=nodes[j].splitToArray("=");
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
          
          if (values[0].equals("drawGridVectors")) {
            drawGridVectors=values[1].equals("true");
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


    void smoothData(float *valueData,float fNodataValue,int smoothWindow, int W,int H);




   
};

#endif

