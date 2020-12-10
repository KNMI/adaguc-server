/******************************************************************************
 * 
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2020-12-09
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

#ifndef CImgWarpHillShaded_H
#define CImgWarpHillShaded_H
#include <stdlib.h>
#include "CImageWarperRenderInterface.h"
#include "CGenericDataWarper.h"

class Vector {
  public:
  Vector (float x, float y, float z){
    this->x=x;
    this->y=y;
    this->z=z;
  }

  Vector operator -(const Vector &v) {
    return Vector(x-v.x, y-v.y, z-v.z);
  }
  float square(){
    return x*x+y*y+z*z;
  }
  float magnitude(){
    return sqrt(square());
  }
  Vector normalize () {
    float f = magnitude();
    if (f==0)return Vector(0,0,0);
    return Vector(x/f, y/f, z/f);
  }
  float x,y, z;
} ;

static Vector CrossProduct(const Vector &v1, const Vector &v2){
    return Vector(v1.y*v2.z - v1.z*v2.y,
                  v1.z*v2.x - v1.x*v2.z,
                  v1.x*v2.y - v1.y*v2.x);
}

static float DotProduct(const Vector &a, const Vector &b){
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

class CImgWarpHillShaded:public CImageWarperRenderInterface{
  private:
  DEF_ERRORFUNCTION();
  class Settings{
  public:
    double dfNodataValue;
    double legendValueRange;
    double legendLowerRange;
    double legendUpperRange;
    bool hasNodataValue;
    float *dataField;
    size_t width,height;
  };

  template <class T>
  static void drawFunction(int x,int y,T val, void *_settings, void *g){
    Settings *drawSettings = (Settings*)_settings;
    GenericDataWarper *genericDataWarper = (GenericDataWarper*)g;
    bool isNodata=false;
    if(drawSettings->hasNodataValue){if((val)==(T)drawSettings->dfNodataValue)isNodata=true;}if(!(val==val))isNodata=true;
    if(!isNodata)if(drawSettings->legendValueRange)if(val<drawSettings->legendLowerRange||val>drawSettings->legendUpperRange)isNodata=true;
    if(!isNodata){
      T *sourceData = (T*)genericDataWarper->sourceData;
      size_t sourceDataPX = genericDataWarper->sourceDataPX;
      size_t sourceDataPY = genericDataWarper->sourceDataPY;
      size_t sourceDataWidth = genericDataWarper->sourceDataWidth;
      size_t sourceDataHeight = genericDataWarper->sourceDataHeight;
      
      if (sourceDataPY < 3 || sourceDataPY > sourceDataHeight-10) return;
      if (sourceDataPX <3 || sourceDataPX > sourceDataWidth-10) return;

      float values[3]={0,0,0};
      for(int wy=-2;wy<2;wy++){
        for(int wx=-2;wx<2;wx++){
          values[0]+=(float)((T*)sourceData)[sourceDataPX + wx+(sourceDataPY+wy)*sourceDataWidth];
          values[1]+=(float)((T*)sourceData)[sourceDataPX +2+wx + (sourceDataPY+wy)*sourceDataWidth];
          values[2]+=(float)((T*)sourceData)[sourceDataPX + wx+(sourceDataPY+2+wy)*sourceDataWidth];

        }
      }

       Vector v1 = Vector((float)sourceDataPX, (float)sourceDataPY,  values[0]);
       Vector v2 = Vector((float)(sourceDataPX+1), (float)sourceDataPY,  values[1]);
       Vector v3 = Vector((float)sourceDataPX, (float)(sourceDataPY+1),  values[2]);
       
      if(x>=0&&y>=0&&x<(int)drawSettings->width&&y<(int)drawSettings->height){
        Vector normal = CrossProduct(v2-v1, v3-v1).normalize();
        Vector lightSource = (Vector(-1,-1,-1)).normalize();;
        float c = DotProduct(lightSource, normal);
        drawSettings->dataField[x+y*drawSettings->width]= (c+1)/1.816486;
      }
    }
  };
  public:
    CImgWarpHillShaded(){
    }
    ~CImgWarpHillShaded(){
    }
    void render(CImageWarper *warper,CDataSource *dataSource,CDrawImage *drawImage) override;
    int set(const char *pszSettings) override;
};

#endif

