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

class ShadeDefinition{
public:
  float min,max;
  bool foundColor;
  bool hasBGColor;
  CColor fillColor;
  CColor bgColor;
  ShadeDefinition(float min,float max,CColor fillColor,bool foundColor,CColor bgColor,bool hasBGColor){
    this->min=min;
    this->max=max;
    this->fillColor=fillColor;
    this->foundColor=foundColor;
    this->bgColor=bgColor;
    this->hasBGColor=hasBGColor;
  }
};


  
class ContourDefinition{
public:
  ContourDefinition(){
    lineWidth=1;
    linecolor.r=0;
    linecolor.g=0;
    linecolor.b=0;
    linecolor.a=255;
    textcolor.r=0;
    textcolor.g=0;
    textcolor.b=0;
    textcolor.a=0;
    continuousInterval=0;
    textFormat="%f";
  }
  
  std::vector<float> definedIntervals;
  float continuousInterval;
  float lineWidth;
  CT::string textFormat;
  CColor linecolor;
  CColor textcolor;
  
  ContourDefinition(float lineWidth,CColor linecolor,CColor textcolor,const char *_definedIntervals,const char *_textFormat){
    this->lineWidth=lineWidth;
    this->linecolor=linecolor;
    this->textcolor=textcolor;
    this->continuousInterval=continuousInterval;
    
    if(_definedIntervals!=NULL){
      CT::string defIntervalString=_definedIntervals;
      CT::string *defIntervalList = defIntervalString.splitToArray(",");
      for(size_t j=0;j<defIntervalList->count;j++){
        definedIntervals.push_back( defIntervalList[j].toFloat());
      }
      delete[] defIntervalList;
    }
    
    
    if(_textFormat!=NULL){
      if(strlen(_textFormat)>1){
        this->textFormat=_textFormat;
        return;
      }
    }
    
  }
  
  ContourDefinition(float lineWidth,CColor linecolor,CColor textcolor, float continuousInterval,const char *_textFormat){
    
    this->lineWidth=lineWidth;
    this->linecolor=linecolor;
    this->textcolor=textcolor;
    this->continuousInterval=continuousInterval;
    
    if(_textFormat!=NULL){
      if(strlen(_textFormat)>1){
        this->textFormat=_textFormat;
        return;
      }
    }
    
    float fracPart=continuousInterval-int(continuousInterval);
    int textRounding=-int(log10(fracPart)-0.9999999f);
    if(textRounding<=0)textFormat="%2.0f";
    if(textRounding==1)textFormat="%2.1f";
    if(textRounding==2)textFormat="%2.2f";
    if(textRounding==3)textFormat="%2.3f";
    if(textRounding==4)textFormat="%2.4f";
    if(textRounding==5)textFormat="%2.5f";
    if(textRounding>=6)textFormat="%f";
  }
};


class Point{
public:
  Point(int x,int y){
   this->x=x;
   this->y=y;
  }
  int x,y;
};

#define CONTOURDEFINITIONLOOKUPLENGTH 32
#define DISTANCEFIELDTYPE unsigned int
class CImgWarpBilinear:public CImageWarperRenderInterface{
  private:
    bool drawMap,enableContour,enableVector,enableBarb,enableShade,drawGridVectors;
    float shadeInterval;
    int smoothingFilter;
    
    unsigned short checkIfContourRequired(float *val);
    
    std::vector<ContourDefinition> contourDefinitions;
    std::vector<ShadeDefinition> shadeDefinitions;

    
    std::vector<PointD*> minimaPoints;
    std::vector<PointD*> maximaPoints;
    DEF_ERRORFUNCTION();
    void drawTextForContourLines(CDrawImage * drawImage,ContourDefinition *contourDefinition, int lineX, int lineY,int endX, int endY, std::vector<Point> *textLocations,float value,CColor textColor);
    void traverseLine(CDrawImage * drawImage,DISTANCEFIELDTYPE *distance,float *valueField,int lineX,int lineY,int dImageWidth,int dImageHeight,float lineWidth,CColor lineColor,CColor textColor,ContourDefinition *contourDefinition,DISTANCEFIELDTYPE lineMask,bool drawText,std::vector<Point> *textLocations);
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
    void render(CImageWarper *warper,CDataSource *dataSource,CDrawImage *drawImage);
    int set(const char *pszSettings);
    
    int getPixelIndexForValue(CDataSource*dataSource,float val){
      bool isNodata=false;
      
      if(dataSource->getDataObject(0)->hasNodataValue){
        if(val==float(dataSource->getDataObject(0)->dfNodataValue))isNodata=true;
        if(!(val==val))isNodata=true;
      }
      if(!isNodata){
        CStyleConfiguration *styleConfiguration = dataSource->getStyle();
        if(styleConfiguration->hasLegendValueRange==1){
          if(val<styleConfiguration->legendLowerRange||val>styleConfiguration->legendUpperRange){
            isNodata=true;
          }
        }
        if(!isNodata){
          if(styleConfiguration->legendLog!=0)val=log10(val+.000001)/log10(styleConfiguration->legendLog);
          val*=styleConfiguration->legendScale;
          val+=styleConfiguration->legendOffset;
          if(val>=239)val=239;else if(val<0)val=0;
          return int(val);
        }
      }
      return 0;
    }
    
    static void setValuePixel(CDataSource*dataSource,CDrawImage*drawImage,int destX,int destY,float val){
      bool isNodata=false;
      
      if(dataSource->getDataObject(0)->hasNodataValue){
        if(val==float(dataSource->getDataObject(0)->dfNodataValue)){isNodata=true;  return;}
        if(!(val==val)){isNodata=true;  return;}
      }
      CStyleConfiguration *styleConfiguration = dataSource->getStyle();
      if(!isNodata){
        if(styleConfiguration->hasLegendValueRange==1)
          if(val<styleConfiguration->legendLowerRange||val>styleConfiguration->legendUpperRange)isNodata=true;
      }
      if(!isNodata){
        if(styleConfiguration->legendLog!=0)val=log10(val+.000001)/log10(styleConfiguration->legendLog);
        val*=styleConfiguration->legendScale;
        val+=styleConfiguration->legendOffset;
        if(val>=239)val=239;else if(val<0)val=0;
        drawImage->setPixelIndexed(destX,destY,(unsigned char)val);
      }
    }

    void drawContour(float *valueData,float fNodataValue,float interval,CDataSource *dataSource,CDrawImage *drawImage,bool drawLine, bool drawShade, bool drawText);
    void smoothData(float *valueData,float fNodataValue,int smoothWindow, int W,int H);

};

#endif

