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

#include "CImgRenderPoints.h"
const char *CImgRenderPoints::className="CImgRenderPoints";

void CImgRenderPoints::render(CImageWarper*warper, CDataSource*dataSource, CDrawImage*drawImage){
  bool drawVector = false;
  bool drawPoints = true;
  bool drawBarb = false;
  bool drawDiscs = false;
  bool drawText = true;
  bool drawVolume = false;
  bool doThinning=false;
  int thinningRadius = 25;
  int discRadius = 28;
  float fontSize = 15;
  const char *fontFile = dataSource->srvParams->cfg->WMS[0]->ContourFont[0]->attr.location.c_str();
  const char *drawPointFontFile = dataSource->srvParams->cfg->WMS[0]->ContourFont[0]->attr.location.c_str();
  int drawPointRadius = 8;
  int drawPointTextRadius=drawPointRadius+2;
  bool drawPointDot=true;
  float drawTextAngleStart=-90;
  float drawTextAngleStep=180;
  int drawPointFontSize=15;
  CT::string drawTextFormat="";
  CColor textColor(255,255,255,255);
  CColor fillColor(0,0,0,128);
  CColor lineColor(0,0,0,255);
  CColor drawPointTextColor(0,0,0,255);
  CColor drawPointFillColor(0,0,0,128);
  CColor drawPointLineColor(0,0,0,255);
  
  CDBDebug("style settings: %s", settings.c_str());
  if(settings.indexOf("vector")!=-1){
    drawVector = true;
  }
  if(settings.indexOf("barb")!=-1){
    drawBarb = true;
  }
  if(settings.indexOf("volume")!=-1){
    drawPoints = false;
    drawVolume = true;
  }
  if(settings.indexOf("disc")!=-1){
    drawPoints = false;
    drawDiscs = true;
    CStyleConfiguration *styleConfiguration = dataSource->getStyle();
    if(styleConfiguration!=NULL){
      if(styleConfiguration->styleConfig!=NULL){
        CServerConfig::XMLE_Style* s = styleConfiguration->styleConfig;
        if(s -> Disc.size() == 1){
          if(s -> Disc[0]->attr.fillcolor.empty()==false){
            fillColor.parse(s -> Disc[0]->attr.fillcolor.c_str());
          }
          if(s -> Disc[0]->attr.linecolor.empty()==false){
            lineColor.parse(s -> Disc[0]->attr.linecolor.c_str());
          }
          if(s -> Disc[0]->attr.textcolor.empty()==false){
            textColor.parse(s -> Disc[0]->attr.textcolor.c_str());
          }
          if(s -> Disc[0]->attr.fontfile.empty()==false){
            fontFile = s -> Disc[0]->attr.fontfile.c_str();
          }
          if(s -> Disc[0]->attr.fontsize.empty()==false){
            fontSize = s -> Disc[0]->attr.fontsize.toFloat();
          }
          if(s -> Disc[0]->attr.discradius.empty()==false){
            discRadius = s -> Disc[0]->attr.discradius.toFloat();
          }
          
        }
        
      }
    }
  }
  if(settings.indexOf("point")!=-1){
      drawPoints=true;
  }
  CDBDebug("drawPoints=%d drawText=%d drawBarb=%d drawVector=%d", drawPoints, drawText, drawBarb, drawVector);
  CStyleConfiguration *styleConfiguration = dataSource->getStyle();

  if (drawPoints) {
    CDBDebug("point style before: %d %d %f %f t:#%02x%02x%02x%02x f:#%02x%02x%02x%02x l:#%02x%02x%02x%02x",drawPointRadius, drawPointDot, drawTextAngleStart, drawTextAngleStep, 
             drawPointTextColor.r,drawPointTextColor.g,drawPointTextColor.b,drawPointTextColor.a,
             drawPointFillColor.r,drawPointFillColor.g,drawPointFillColor.b,drawPointFillColor.a,
             drawPointLineColor.r,drawPointLineColor.g,drawPointLineColor.b,drawPointLineColor.a
            );
    if(styleConfiguration!=NULL){
      if(styleConfiguration->styleConfig!=NULL){
        CServerConfig::XMLE_Style* s = styleConfiguration->styleConfig;
        if(s -> Point.size() == 1){
          if(s -> Point[0]->attr.fillcolor.empty()==false){
            drawPointFillColor.parse(s -> Point[0]->attr.fillcolor.c_str());
          }
          if(s -> Point[0]->attr.linecolor.empty()==false){
            drawPointLineColor.parse(s -> Point[0]->attr.linecolor.c_str());
          }
          if(s -> Point[0]->attr.textcolor.empty()==false){
            drawPointTextColor.parse(s -> Point[0]->attr.textcolor.c_str());
          }
          if(s -> Point[0]->attr.fontfile.empty()==false){
            drawPointFontFile = s -> Point[0]->attr.fontfile.c_str();
          }
          if(s -> Point[0]->attr.fontsize.empty()==false){
            drawPointFontSize = s -> Point[0]->attr.fontsize.toFloat();
          }
          if(s -> Point[0]->attr.discradius.empty()==false){
            drawPointRadius = s -> Point[0]->attr.discradius.toInt();
          }      
          if(s -> Point[0]->attr.textradius.empty()==false){
            drawPointTextRadius = s -> Point[0]->attr.textradius.toInt();
          } else {
            drawPointTextRadius = drawPointRadius+2;
          }
          if(s -> Point[0]->attr.dot.empty()==false){
            drawPointDot = s -> Point[0]->attr.dot.equals("true");
          }      
          if(s -> Point[0]->attr.anglestart.empty()==false){
            drawTextAngleStart = s -> Point[0]->attr.anglestart.toFloat();
          }      
          if(s -> Point[0]->attr.anglestep.empty()==false){
            drawTextAngleStep = s -> Point[0]->attr.anglestep.toFloat();
          }      
          if(s -> Point[0]->attr.textformat.empty()==false){
            drawTextFormat = s -> Point[0]->attr.textformat.c_str();
          }      
        }
      }
    }
    CDBDebug("point style after: %d %d %f %f t:#%02x%02x%02x%02x f:#%02x%02x%02x%02x l:#%02x%02x%02x%02x",drawPointRadius, drawPointDot, drawTextAngleStart, drawTextAngleStep, 
             drawPointTextColor.r,drawPointTextColor.g,drawPointTextColor.b,drawPointTextColor.a,
             drawPointFillColor.r,drawPointFillColor.g,drawPointFillColor.b,drawPointFillColor.a,
             drawPointLineColor.r,drawPointLineColor.g,drawPointLineColor.b,drawPointLineColor.a
            );
  }
  
  if(settings.indexOf("thin")!=-1){
    doThinning = true;
    if(styleConfiguration!=NULL){
      if(styleConfiguration->styleConfig!=NULL){
        CServerConfig::XMLE_Style* s = styleConfiguration->styleConfig;
        if(s -> Thinning.size() == 1){
          if(s -> Thinning[0]->attr.radius.empty()==false){
            thinningRadius = s -> Thinning[0]->attr.radius.toInt();
            //CDBDebug("Thinning radius = %d",s -> Thinning[0]->attr.radius.toInt());
          }
        }
      }
    }
  }
  
  int alphaPoint[15*15];
  if (drawVolume) {
    int p = 0;
    for(int y1=-7;y1<8;y1++){
      for(int x1=-7;x1<8;x1++){
        float d= sqrt(x1*x1+y1*y1);
        if(d<0)d=0;
        d=10-d;
        d=d*1.2;
        alphaPoint[p++] = d;
      }
    }
  }
  
//   if(dataSource->getNumDataObjects()==1&&false){
//     CT::string t;
//     for(size_t pointNo=0;pointNo<nrThinnedPoints;pointNo++){
//       size_t j=pointNo;
//       if (doThinning) j=thinnedPointsIndex[pointNo];
//       
//       if(drawVolume){
//         int x=(*p1)[j].x;
//         int y=dataSource->dHeight-(*p1)[j].y;
//         drawImage->setPixelTrueColor(x,y, 0,0,0,255);
//         int p=0;
//         for(int y1=-7;y1<8;y1++){
//           for(int x1=-7;x1<8;x1++){
//             drawImage->setPixelTrueColor(x+x1,y+y1, 0,0,255,alphaPoint[p++]);
//           }
//         }
//       }
//       
//       if(drawPoints){
//         int x=(*p1)[j].x;
//         int y=dataSource->dHeight-(*p1)[j].y;
//         drawImage->circle(x,y, 8, 240,0.65);
//         drawImage->circle(x,y, 1, 240,0.65);
//         if(drawText){
//           //drawImage->setPixelIndexed(x,y, 240);
//           float v=(*p1)[j].v;
//           if(v==v){
//             t.print("%0.1f",v);
//             drawImage->setText(t.c_str(), t.length(),x-t.length()*3,y+8, 240,0);
//           }else{
//             if((*p1)[j].paramList.size()>0){
//               CT::string value = (*p1)[j].paramList[0].value;
//               drawImage->setText(value.c_str(), value.length(),x-value.length()*3,y-20, 240,0);
//             }
//           }
//         }
//       }
//       if (drawDiscs) {
//         int x=(*p1)[j].x;
//         int y=dataSource->dHeight-(*p1)[j].y;
//         //drawImage->circle(x,y, discRadius, 240,0.65);
//         float v=(*p1)[j].v;
//         if(v==v){
//           t.print("%0.1f",v);
//           drawImage->setTextDisc( x, y,discRadius, t.c_str(),fontFile, fontSize,textColor,fillColor, lineColor);
//          
//             
//           //drawImage->setTextDisc(t.c_str(), t.length(), x, y, 28, 240, 0);
//         }
//       }
//     }
//   }
  
  if(dataSource->getNumDataObjects()!=2){ // Not for vector (u/v or speed/dir pairs) TODO
    for (size_t dataObject=0; dataObject<dataSource->getNumDataObjects(); dataObject++) {
      std::vector<PointDVWithLatLon> *pts=&dataSource->getDataObject(dataObject)->points;
      float useangle=drawTextAngleStart+drawTextAngleStep*dataObject;
      int kwadrant=0;
      if (useangle<0) {
        kwadrant=3-int(useangle/90);
      } else {
        kwadrant=int(useangle/90);
      }
      float usedx=drawPointTextRadius*sin(useangle*M_PI/180);
      float usedy=drawPointTextRadius*cos(useangle*M_PI/180);
//      CDBDebug("angles[%d] %f %d %f %f", dataObject, useangle, kwadrant, usedx, usedy);
    
      //THINNING
      std::vector<PointDVWithLatLon> *p1=&dataSource->getDataObject(dataObject)->points;
      size_t l=p1->size();
      size_t nrThinnedPoints=l;
      std::vector<size_t> thinnedPointsIndex;
      
      if (doThinning) {
        thinnedPointsIndex.push_back(0);
        for(size_t j=1;j<l;j++){
          size_t nrThinnedPoints=thinnedPointsIndex.size();
          size_t i;
          for(i=0;i<nrThinnedPoints;i++){ 
            if(hypot((*p1)[thinnedPointsIndex[i]].x-(*p1)[j].x,(*p1)[thinnedPointsIndex[i]].y-(*p1)[j].y)<thinningRadius)break;
          }
          if(i==nrThinnedPoints)thinnedPointsIndex.push_back(j);      
        }
          
        nrThinnedPoints=thinnedPointsIndex.size();
      }
      CT::string t;
      for(size_t pointNo=0;pointNo<nrThinnedPoints;pointNo++){
        size_t j=pointNo;
        if (doThinning) j=thinnedPointsIndex[pointNo];

        if(drawVolume){
          int x=(*pts)[j].x;
          int y=dataSource->dHeight-(*pts)[j].y;
          drawImage->setPixelTrueColor(x,y, 0,0,0,255);
          int p=0;
          for(int y1=-7;y1<8;y1++){
            for(int x1=-7;x1<8;x1++){
              drawImage->setPixelTrueColor(x+x1,y+y1, 0,0,255,alphaPoint[p++]);
            }
          }
        }
        
        if(drawPoints){
          int x=(*pts)[j].x;
          int y=dataSource->dHeight-(*pts)[j].y;
          if (drawPointDot) drawImage->circle(x,y, 1, 240,0.65);
          if(drawText){
            //drawImage->setPixelIndexed(x,y, 240);
            float v=(*pts)[j].v;
            if(v==v){
              t.print("%0.1f",v);
//              CDBDebug("[%d] v=%f: drawPointRadius=%d n=%d index=%d", j, v, drawPointRadius, dataSource->getNumDataObjects(), getPixelIndexForValue(dataSource, v));
              if (drawPointRadius==0) {
//                CDBDebug("radius==0 => centeredtext only %f", v);
                drawImage->drawCenteredText(x,y, drawPointFontFile, drawPointFontSize, 0, t.c_str(), drawPointTextColor);
              } else {
                if (dataSource->getNumDataObjects()==1) {
                  int pointColorIndex=getPixelIndexForValue(dataSource, v);
 //                 CDBDebug("1 layer, so circle colored by val %f => index %d", v, pointColorIndex);
                  drawImage->setText(t.c_str(), t.length(),x-t.length()*3,y+drawPointTextRadius, 240,0);
                  drawImage->setDisc(x, y, drawPointRadius, pointColorIndex, pointColorIndex);
                } else {
//                  CDBDebug("more layers, so circle with fixed color ");
                  drawImage->setDisc(x, y, drawPointRadius, drawPointFillColor, drawPointLineColor);
                  drawImage->drawAnchoredText(x+usedx,y-usedy, drawPointFontFile, drawPointFontSize, 0, t.c_str(), drawPointTextColor, kwadrant);
                }
              }
              if ((*pts)[j].paramList.size()) {
                CDBDebug("drawText=%d paramList %d el.: %s at [%d,%d] for %d,%d", drawText, (*pts)[j].paramList.size(), (*pts)[j].paramList[0].value.c_str(), x, y, dataObject, j);
              } else {
                CDBDebug("drawText=%d paramList %d el.", drawText, (*pts)[j].paramList.size());
              }
              if((*pts)[j].paramList.size()>0){
                CT::string value =(*pts)[j].paramList[0].value;
                drawImage->setText(value.c_str(), value.length(),x-value.length()*3,y-20, 240,0);
              }
            }else{
              CDBDebug("Value not available");
              if((*pts)[j].paramList.size()>0){
                CT::string value = (*pts)[j].paramList[0].value;
                CDBDebug("Extra value: %s fixed color with radius %d", value.c_str(), drawPointRadius);
                if (drawPointRadius==0) {
                  drawImage->drawCenteredText(x,y, drawPointFontFile, drawPointFontSize, 0, value.c_str(), drawPointTextColor);                  
                } else {
                  drawImage->drawAnchoredText(x+usedx,y-usedy, drawPointFontFile, drawPointFontSize, 0, value.c_str(), drawPointTextColor, kwadrant);
                }
              }

            }
          }
        }
        if (drawDiscs) {
          int x=(*pts)[j].x;
          int y=dataSource->dHeight-(*pts)[j].y;
          //drawImage->circle(x,y, discRadius, 240,0.65);
          float v=(*pts)[j].v;
          if(v==v){
            t.print("%0.1f",v);
            drawImage->setTextDisc( x, y,discRadius, t.c_str(),fontFile, fontSize,textColor,fillColor, lineColor);
          }
        }
      }
    }
  }
  
  if(dataSource->getNumDataObjects()==2){
    CT::string varName1=dataSource->getDataObject(0)->cdfVariable->name.c_str();
    CT::string varName2=dataSource->getDataObject(1)->cdfVariable->name.c_str();
    std::vector<PointDVWithLatLon> *p1=&dataSource->getDataObject(0)->points;
    std::vector<PointDVWithLatLon> *p2=&dataSource->getDataObject(1)->points;
    
    CT::string t;
      //THINNING
    size_t l=p1->size();
    size_t nrThinnedPoints=l;
    std::vector<size_t> thinnedPointsIndex;
    
    if (doThinning) {
      thinnedPointsIndex.push_back(0);
      for(size_t j=1;j<l;j++){
        size_t nrThinnedPoints=thinnedPointsIndex.size();
        size_t i;
        for(i=0;i<nrThinnedPoints;i++){ 
          if(hypot((*p1)[thinnedPointsIndex[i]].x-(*p1)[j].x,(*p1)[thinnedPointsIndex[i]].y-(*p1)[j].y)<thinningRadius)break;
        }
        if(i==nrThinnedPoints)thinnedPointsIndex.push_back(j);      
      }
        
      nrThinnedPoints=thinnedPointsIndex.size();
    }   
    for(size_t pointNo=0;pointNo<nrThinnedPoints;pointNo++){
      size_t j=pointNo;
      if (doThinning) j=thinnedPointsIndex[pointNo];
      int x=(*p1)[j].x;
      double lat=(*p1)[j].lat;
      double lon=(*p1)[j].lon;
      double rotation=(*p1)[j].rotation;
      
      if(rotation == 0){
        double latForRot=lat;
        double lonForRot=lon;
        double latOffSetForRot=lat-0.01;
        double lonOffSetForRot=lon;
        bool projectionRequired = true;
  
          
        if(projectionRequired){
          warper->reprojfromLatLon(lonForRot,latForRot);
          warper->reprojfromLatLon(lonOffSetForRot,latOffSetForRot);
        }
       
        if(projectionRequired){
          double dy=latForRot- latOffSetForRot;
          double dx=lonForRot - lonOffSetForRot;
          rotation= -(atan2(dy,dx)/(3.141592654))*180+90;
        
        }
        
      }
      
      int y=dataSource->dHeight-(*p1)[j].y;
      
      float strength = (*p1)[j].v;
      float direction = (*p2)[j].v+rotation;//direction=rotation;
     // direction=360-45;
      //drawImage->drawVector(x, y, ((90-direction)/360)*3.141592654*2, strength*2, 240);
      if(drawBarb){
        if(lat>0){
          drawImage->drawBarb(x, y, ((270-direction)/360)*3.141592654*2, strength, 240,false,false);
        }else{
          drawImage->drawBarb(x, y, ((270-direction)/360)*3.141592654*2, strength, 240,false,true);
        }
      }
      if(drawVector){
        drawImage->drawVector(x, y, ((270-direction)/360)*3.141592654*2, strength*2, 240);
      }
       //void drawBarb(int x,int y,double direction, double strength,int color,bool toKnots,bool flip);
      if(drawPoints && drawText){
         if((*p1)[j].paramList.size()>0){
            CT::string value = (*p1)[j].paramList[0].value;
            drawImage->setText(value.c_str(), value.length(),x-value.length()*3,y-20, 240,0);
          }
      }
      if (drawDiscs) {
        // Draw a disc with the speed value in text and the dir. value as an arrow
        int x=(*p1)[j].x;
        int y=dataSource->dHeight-(*p1)[j].y;
        float v=(*p1)[j].v;
        if(v==v){
          t.print("%0.1f",v);
          drawImage->setTextDisc( x, y,discRadius, t.c_str(),fontFile, fontSize,textColor,fillColor, lineColor);
        }
        float dir = (*p2)[j].v;
        if (dir==dir) {
          drawImage->drawVector(x, y, ((270-direction)/360)*3.141592654*2, 80, 240);
        }
      }
    }
  }
}
int CImgRenderPoints::set(const char*values){

  settings.copy(values);
  return 0;
}

int CImgRenderPoints::getPixelIndexForValue(CDataSource*dataSource,float val){
      bool isNodata=false;
      
      if(dataSource->getDataObject(0)->hasNodataValue){
        if(val==float(dataSource->getDataObject(0)->dfNodataValue))isNodata=true;
        if(!(val==val))isNodata=true;
      }
      if(!isNodata) {
        CStyleConfiguration *styleConfiguration = dataSource->getStyle();
        if(styleConfiguration->hasLegendValueRange==1)
          if(val<styleConfiguration->legendLowerRange||val>styleConfiguration->legendUpperRange)isNodata=true;
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
