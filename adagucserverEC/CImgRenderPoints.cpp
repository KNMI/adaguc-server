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
  bool drawText = true;
  bool drawVolume = false;
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
  int alphaPoint[15*15];
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
  
  
  /*if(settings.indexOf("nearest")!=-1){
    drawText = false;
  }*/
  if(dataSource->getNumDataObjects()==1){
    
    std::vector<PointDVWithLatLon> *p1=&dataSource->getDataObject(0)->points;
    size_t l=p1->size();
    size_t s=1;
    /*while(l/s>(80*32)){
      s=s+s;
    };*/
    l=p1->size();
    CT::string t;
    for(size_t j=0;j<l;j=j+s){
      
      

      if(drawVolume){
        int x=(*p1)[j].x;
        int y=dataSource->dHeight-(*p1)[j].y;
        drawImage->setPixelTrueColor(x,y, 0,0,0,255);
        int p=0;
        for(int y1=-7;y1<8;y1++){
          for(int x1=-7;x1<8;x1++){
            drawImage->setPixelTrueColor(x+x1,y+y1, 0,0,255,alphaPoint[p++]);
          }
        }
      }
      
      if(drawPoints){
        int x=(*p1)[j].x;
        int y=dataSource->dHeight-(*p1)[j].y;
        drawImage->circle(x,y, 8, 240,0.65);
        drawImage->circle(x,y, 1, 240,0.65);
        if(drawText){
          //drawImage->setPixelIndexed(x,y, 240);
          float v=(*p1)[j].v;
          if(v==v){
            t.print("%0.1f",v);
            drawImage->setText(t.c_str(), t.length(),x-t.length()*3,y+8, 240,0);
          }else{
            if((*p1)[j].paramList.size()>0){
              CT::string value = (*p1)[j].paramList[0].value;
              drawImage->setText(value.c_str(), value.length(),x-value.length()*3,y-20, 240,0);
            }
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
    
    size_t l=p1->size();
    
    
    //THINNING
    
    std::vector<size_t> thinnedPointsIndex;
    thinnedPointsIndex.push_back(0);
    for(size_t j=1;j<l;j++){
      size_t nrThinnedPoints=thinnedPointsIndex.size();
      size_t i;
      for(i=0;i<nrThinnedPoints;i++){ 
        if(hypot((*p1)[thinnedPointsIndex[i]].x-(*p1)[j].x,(*p1)[thinnedPointsIndex[i]].y-(*p1)[j].y)<25)break;
      }
      if(i==nrThinnedPoints)thinnedPointsIndex.push_back(j);      
    }
    
    
     size_t nrThinnedPoints=thinnedPointsIndex.size();
    for(size_t ti=0;ti<nrThinnedPoints;ti++){
      size_t j=thinnedPointsIndex[ti];
      int x=(*p1)[j].x;
      double lat=(*p1)[j].lat;
      double rotation=(*p1)[j].rotation;
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
      if(drawText){
         if((*p1)[j].paramList.size()>0){
            CT::string value = (*p1)[j].paramList[0].value;
            drawImage->setText(value.c_str(), value.length(),x-value.length()*3,y-20, 240,0);
          }
      }
    }
  }
}
int CImgRenderPoints::set(const char*values){

  settings.copy(values);
  return 0;
}
