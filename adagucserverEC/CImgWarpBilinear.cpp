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

#include "CImgWarpBilinear.h"
#include "CImageDataWriter.h"
#include <gd.h>
#include <set>
#ifndef M_PI
#define M_PI            3.14159265358979323846  // pi 
#endif

#ifndef  rad2deg
#define  rad2deg  (180./M_PI)   // conversion for rad to deg 
#endif

#ifndef  deg2rad
#define  deg2rad  (M_PI/180.)   // conversion for deg to rad 
#endif



#include "CImgRenderFieldVectors.h"

const char *CImgWarpBilinear::className="CImgWarpBilinear";
void CImgWarpBilinear::render(CImageWarper *warper,CDataSource *sourceImage,CDrawImage *drawImage){
  #ifdef CImgWarpBilinear_DEBUG
  CDBDebug("Render");
  #endif
  int dImageWidth=drawImage->Geo->dWidth+1;
  int dImageHeight=drawImage->Geo->dHeight+1;
  
  double dfSourceExtW=(sourceImage->dfBBOX[2]-sourceImage->dfBBOX[0]);
  double dfSourceExtH=(sourceImage->dfBBOX[1]-sourceImage->dfBBOX[3]);
  double dfSourceW = double(sourceImage->dWidth);
  double dfSourceH = double(sourceImage->dHeight);
  double dfSourcedExtW=dfSourceExtW/dfSourceW;
  double dfSourcedExtH=dfSourceExtH/dfSourceH;
  double dfSourceOrigX=sourceImage->dfBBOX[0];
  double dfSourceOrigY=sourceImage->dfBBOX[3];
  
  double dfDestExtW = drawImage->Geo->dfBBOX[2]-drawImage->Geo->dfBBOX[0];
  double dfDestExtH = drawImage->Geo->dfBBOX[1]-drawImage->Geo->dfBBOX[3];
  double dfDestOrigX = drawImage->Geo->dfBBOX[0];
  double dfDestOrigY = drawImage->Geo->dfBBOX[3];
  double dfDestW = double(dImageWidth);
  double dfDestH = double(dImageHeight);
  double hCellSizeX = (dfSourceExtW/dfSourceW)/2.0f;
  double hCellSizeY = (dfSourceExtH/dfSourceH)/2.0f;
  double dfPixelExtent[4];
  int dPixelExtent[4];
  bool tryToOptimizeExtent=false;
  
  //  CDBDebug("enableBarb=%d enableVectors=%d drawGridVectors=%d", enableBarb, enableVector, drawGridVectors);
  if(tryToOptimizeExtent){
    //Reproject the boundingbox from the destination bbox: 
    for(int j=0;j<4;j++)dfPixelExtent[j]=drawImage->Geo->dfBBOX[j];  
    #ifdef CImgWarpBilinear_DEBUG
    for(int j=0;j<4;j++){
      CDBDebug("dfPixelExtent: %d %f",j,dfPixelExtent[j]);
    }
    #endif   
    //warper->findExtent(sourceImage,dfPixelExtent);
    warper->reprojBBOX(dfPixelExtent);
    
    //Convert the bbox to source image pixel extent
    dfPixelExtent[0]=((dfPixelExtent[0]-dfSourceOrigX)/dfSourceExtW)*dfSourceW;
    dfPixelExtent[1]=((dfPixelExtent[1]-dfSourceOrigY)/dfSourceExtH)*dfSourceH;
    dfPixelExtent[2]=((dfPixelExtent[2]-dfSourceOrigX)/dfSourceExtW)*dfSourceW+2;
    dfPixelExtent[3]=((dfPixelExtent[3]-dfSourceOrigY)/dfSourceExtH)*dfSourceH+3;
    
    //Make sure the images are not swapped in Y dir.
    if(dfPixelExtent[1]>dfPixelExtent[3]){
      float t1=dfPixelExtent[1];dfPixelExtent[1]=dfPixelExtent[3];dfPixelExtent[3]=t1;
    }
    
    //Make sure the images are not swapped in X dir.
    if(dfPixelExtent[0]>dfPixelExtent[2]){
      float t1=dfPixelExtent[0];dfPixelExtent[0]=dfPixelExtent[2];dfPixelExtent[2]=t1;
    }
    
    //Convert the pixel extent to integer values
    //Also stretch the BBOX a bit, to hide edge effects
    
    dPixelExtent[0]=int(dfPixelExtent[0]);
    dPixelExtent[1]=int(dfPixelExtent[1]);
    dPixelExtent[2]=int(dfPixelExtent[2]);
    dPixelExtent[3]=int(dfPixelExtent[3]);
    //Return if we are zoomed in to an infinite area
    if(dPixelExtent[0]==dPixelExtent[2]){
      CDBDebug("Infinite area, dPixelExtent[0]==dPixelExtent[2]: %d==%d, stopping.... ",dPixelExtent[0],dPixelExtent[2]);
      return;
    }
    if(dPixelExtent[1]==dPixelExtent[3]){
      CDBDebug("Infinite area, dPixelExtent[1]==dPixelExtent[3]: %d==%d, stopping.... ",dPixelExtent[1],dPixelExtent[3]);
      return;
    }
    
    dPixelExtent[0]-=2;
    dPixelExtent[1]-=4;
    dPixelExtent[2]+=2;
    dPixelExtent[3]+=2;
    //Extent should lie within the source image size
    if(dPixelExtent[0]<0)dPixelExtent[0]=0;
    if(dPixelExtent[1]<0)dPixelExtent[1]=0;
    if(dPixelExtent[2]>sourceImage->dWidth)dPixelExtent[2]=sourceImage->dWidth;
    if(dPixelExtent[3]>sourceImage->dHeight)dPixelExtent[3]=sourceImage->dHeight; 
  }else{
    dPixelExtent[0]=0;
    dPixelExtent[1]=0;
    dPixelExtent[2]=sourceImage->dWidth;
    dPixelExtent[3]=sourceImage->dHeight;
  }
  
  
  //Get width and height of the pixel extent
  int dPixelDestW=dPixelExtent[2]-dPixelExtent[0];
  int dPixelDestH=dPixelExtent[3]-dPixelExtent[1];
  size_t numDestPixels = (dPixelDestW+1)*(dPixelDestH+1);
  
  //TODO increase field resolution in order to create better contour plots.
  
  //Allocate memory
  #ifdef CImgWarpBilinear_DEBUG
  CDBDebug("Allocate, numDestPixels %d x %d",dPixelDestW,dPixelDestH);
  #endif
  int *dpDestX = new int[numDestPixels];//refactor to numGridPoints
  int *dpDestY = new int[numDestPixels];
  
  class ValueClass{
  public:
    ValueClass(){
      fpValues=NULL;
      valueData=NULL;
    }
    ~ValueClass(){
      if(fpValues!=NULL){delete[] fpValues;fpValues=NULL;}
      if(valueData!=NULL){delete[] valueData;valueData=NULL;}
    }
    float *fpValues;
    float *valueData;
  };
  ValueClass *valObj = new ValueClass[sourceImage->getNumDataObjects()];
  for(size_t dNr=0;dNr<sourceImage->getNumDataObjects();dNr++){
    #ifdef CImgWarpBilinear_DEBUG
    CDBDebug("Allocating valObj[%d].fpValues: numDestPixels %d x %d",dNr,dPixelDestW,dPixelDestH);
    CDBDebug("Allocating valObj[%d].valueData: imageSize %d x %d",dNr,dImageWidth,dImageHeight);
    #endif
    valObj[dNr].fpValues = new float [numDestPixels];
    valObj[dNr].valueData = new float[dImageWidth*dImageHeight];
  }
  
  if(!sourceImage->getDataObject(0)->hasNodataValue){
    /* When the datasource has no nodata value, assign -9999.0f */
    #ifdef CImgWarpBilinear_DEBUG
    CDBDebug("Source image has no NoDataValue, assigning -9999.0f");
    #endif
    sourceImage->getDataObject(0)->dfNodataValue=-9999.0f;
    sourceImage->getDataObject(0)->hasNodataValue=true;
  }else{
    /* Create a real nodata value instead of a nanf. */
    
    if(!(sourceImage->getDataObject(0)->dfNodataValue==sourceImage->getDataObject(0)->dfNodataValue)){
      #ifdef CImgWarpBilinear_DEBUG
      CDBDebug("Source image has no nodata value NaNf, changing this to -9999.0f");
      #endif
      sourceImage->getDataObject(0)->dfNodataValue=-9999.0f;
    }
  }
  //Get the nodatavalue
  float fNodataValue = sourceImage->getDataObject(0)->dfNodataValue ;
  
  //Reproject all the points 
  #ifdef CImgWarpBilinear_DEBUG
  CDBDebug("Nodata value = %f",fNodataValue);
  
  StopWatch_Stop("Start Reprojecting all the points");
  char temp[32];
  CDF::getCDFDataTypeName(temp,31,sourceImage->getDataObject(0)->cdfVariable->getType());
  CDBDebug("datatype: %s",temp);
  for(int j=0;j<4;j++){
    CDBDebug("dPixelExtent[%d]=%d",j,dPixelExtent[j]);
  } 
  #endif
  
  #ifdef CImgWarpBilinear_DEBUG
  StopWatch_Stop("Setting data objects");
  #endif

  for(int y=dPixelExtent[1];y<dPixelExtent[3]+1;y++){
    for(int x=dPixelExtent[0];x<dPixelExtent[2]+1;x++){
      size_t p = size_t((x-(dPixelExtent[0]))+((y-(dPixelExtent[1]))*(dPixelDestW+1)));
      double destX,destY;
      destX=dfSourcedExtW*double(x)+dfSourceOrigX;
      destY=dfSourcedExtH*double(y)+dfSourceOrigY;
      destX+=hCellSizeX;
      destY+=hCellSizeY;
      int status = warper->reprojpoint_inv(destX,destY);
      destX-=dfDestOrigX;
      destY-=dfDestOrigY;
      destX/=dfDestExtW;
      destY/=dfDestExtH;
      destX*=dfDestW;
      destY*=dfDestH;
      
      
      
      dpDestX[p]=(int)destX;//2-200;
      dpDestY[p]=(int)destY;//2+200;
      
      
      //CDBDebug("%f - %f s:%d x:%d  y:%d  p:%d",destX,destY,status,x,y,p);
      //  drawImage->setPixelIndexed(dpDestX[p],dpDestY[p],240);
      for(size_t varNr=0;varNr<sourceImage->getNumDataObjects();varNr++){
        void *data=sourceImage->getDataObject(varNr)->cdfVariable->data;
        float *fpValues=valObj[varNr].fpValues;
        int x1=x;
        int y1=y;
        if(x1>=sourceImage->dWidth){
          x1-=sourceImage->dWidth;
        }
        if(y1>=sourceImage->dHeight){
          y1-=sourceImage->dHeight;
        }
        //if(x1>=0&&x1<sourceImage->dWidth&&y>=0&&y<sourceImage->dHeight){
        size_t sp = x1+y1*sourceImage->dWidth;
        

        switch(sourceImage->getDataObject(varNr)->cdfVariable->getType()){
          case CDF_CHAR:
            fpValues[p]= ((signed char*)data)[sp];
            break;
          case CDF_BYTE:
            fpValues[p]= ((signed char*)data)[sp];
            break;
          case CDF_UBYTE:
            fpValues[p]= ((unsigned char*)data)[sp];
            break;
          case CDF_SHORT:
            fpValues[p]= ((signed short*)data)[sp];
            break;
          case CDF_USHORT:
            fpValues[p]= ((unsigned short*)data)[sp];
            break;
          case CDF_INT:
            fpValues[p]= ((signed int*)data)[sp];
            break;
          case CDF_UINT:
            fpValues[p]= ((unsigned int*)data)[sp];
            break;
          case CDF_FLOAT:
            fpValues[p]= ((float*)data)[sp];
            break;
          case CDF_DOUBLE:
            fpValues[p]= ((double*)data)[sp];
            break;
        }
        //}
        if(!(fpValues[p]==fpValues[p]))fpValues[p]=fNodataValue;
        if(status == 1)fpValues[p]=fNodataValue;

      }
    }
  }
  #ifdef CImgWarpBilinear_DEBUG
  StopWatch_Stop("reprojection finished");
  #endif

    float *uValues=valObj[0].fpValues;
    float *vValues=valObj[1].fpValues;
applyUVConversion(warper, sourceImage, enableVector, enableBarb,dPixelExtent, uValues, vValues);

for(size_t varNr=0;varNr<sourceImage->getNumDataObjects();varNr++){
  float *fpValues=valObj[varNr].fpValues;
  float *valueData=valObj[varNr].valueData;
  
  //Smooth the data (better for contour lines)
  #ifdef CImgWarpBilinear_DEBUG
  CDBDebug("start smoothing data with filter %d",smoothingFilter);
  #endif
  smoothData(fpValues,fNodataValue,smoothingFilter, dPixelDestW+1,dPixelDestH+1);
  
  //Draw the obtained raster by using triangle tesselation (eg gouraud shading)
  int xP[4],yP[4];
  float vP[4];
  
  size_t drawImageSize = dImageWidth*dImageHeight;
  //Set default nodata values
  for(size_t j=0;j<drawImageSize;j++)valueData[j]=fNodataValue;
  //start drawing triangles
  #if defined(CImgWarpBilinear_DEBUG) || defined(CImgWarpBilinear_TIME)
  StopWatch_Stop("Start triangle generation");
  #endif
  
  /*
   * 
   * float cubicInterpolate (float p[4], float x) {
   *  return p[1] + 0.5 * x*(p[2] - p[0] + x*(2.0*p[0] - 5.0*p[1] + 4.0*p[2] - p[3] + x*(3.0*(p[1] - p[2]) + p[3] - p[0])));
   }
   
   float bicubicInterpolate (float p[4][4], float x, float y) {
     float arr[4];
     arr[0] = cubicInterpolate(p[0], y);
     arr[1] = cubicInterpolate(p[1], y);
     arr[2] = cubicInterpolate(p[2], y);
     arr[3] = cubicInterpolate(p[3], y);
     return cubicInterpolate(arr, x);
   }
   */
  
    int avgDX = 0;
//     int avgDY = 0;

  for(int y=dPixelExtent[1];y<dPixelExtent[3]-1;y++){
    for(int x=dPixelExtent[0];x<dPixelExtent[2];x++){
      size_t p = size_t((x-(dPixelExtent[0]))+((y-(dPixelExtent[1]))*(dPixelDestW+1)));
      size_t p00=p;
      size_t p10=p+1;
      size_t p01=p+dPixelDestW+1;
      size_t p11=p+1+dPixelDestW+1;
     

      if(fpValues[p00]!=fNodataValue&&fpValues[p10]!=fNodataValue&&
        fpValues[p01]!=fNodataValue&&fpValues[p11]!=fNodataValue)
      {
        
        yP[0]=dpDestY[p00]; yP[1]=dpDestY[p01]; yP[2]=dpDestY[p10]; yP[3]=dpDestY[p11];
        xP[0]=dpDestX[p00]; xP[1]=dpDestX[p01]; xP[2]=dpDestX[p10]; xP[3]=dpDestX[p11];
        

        vP[0]=fpValues[p00]; vP[1]=fpValues[p01]; vP[2]=fpValues[p10]; vP[3]=fpValues[p11]; 
        
        
        
        bool doDraw = true;
       
                  
        if(x==dPixelExtent[0])avgDX = xP[2];
//         if(y==dPixelExtent[1])avgDY = yP[3];
        
        if(x==dPixelExtent[2]-1){
          if(abs(avgDX-xP[0])>dImageWidth/4){
             doDraw= false;
          }
          if(abs(avgDX-xP[2])>0){
            if(abs(avgDX-xP[2])<abs(xP[2]-xP[0])/4){
              doDraw = false;
              //CDBDebug("%d %d (%d %d %d %d) ",avgDX-xP[2],xP[2]-xP[0],avgDX,xP[0],xP[1],xP[2]);
            }
          }
        }
//         if(y==dPixelExtent[3]-1){
//           if(abs(avgDY-yP[0])>0){
//             if(abs(avgDY-yP[0])<abs(yP[3]-yP[0])/2){
//               doDraw = false;
//             }
//           }
//         }
        
        
        

        
        if(doDraw){
        
          fillQuadGouraud(valueData,vP,  dImageWidth,dImageHeight, xP,yP);
        
        }
//         /*for(int iy=-15;iy<16;iy++){
//           for(int ix=-15;ix<16;ix++){
//             if(iy+dpDestY[p00]>0&&ix+dpDestX[p00]>0&&iy+dpDestY[p00]<dImageHeight&&ix+dpDestX[p00]<dImageWidth){
//               valueData[(iy+dpDestY[p00])*dImageWidth+ix+dpDestX[p00]]=fpValues[p00];
//             }
//           }
//         }*/
      }
    }
  }
  
  
  /*(for(int y=dPixelExtent[1];y<dPixelExtent[3]-3;y=y+1){
    for(int x=dPixelExtent[0];x<dPixelExtent[2]-3;x=x+1){
      size_t p = size_t((x-(dPixelExtent[0]))+((y-(dPixelExtent[1]))*dPixelDestW));
      size_t p00=p;
      size_t p10=p+1;
      size_t p01=p+dPixelDestW;
      size_t p11=p+1+dPixelDestW;
      
      size_t pc00=p;
      size_t pc01=p+1;
      size_t pc02=p+2;
      size_t pc03=p+3;
      
      
      float a = -0.5*fpValues[pc00]+
      
    }
  }*/
}

//Copy pointerdatabitmap to graphics
#ifdef CImgWarpBilinear_DEBUG
CDBDebug("Start converting float bitmap to graphics");
#endif

float *valueData=valObj[0].valueData;
//Draw bilinear, simple variable
if(drawMap==true&&enableShade==false&&enableVector==false&&enableBarb==false){
  for(int y=0;y<dImageHeight;y++){
    for(int x=0;x<dImageWidth;x++){
      setValuePixel(sourceImage,drawImage,x,y,valueData[x+y*dImageWidth]);
    }
  }
}

float *uValueData=valObj[0].valueData;
float *vValueData=valObj[1].valueData;
std::vector<CalculatedWindVector> windVectors = renderBarbsAndVectors(warper, sourceImage, drawImage, enableShade, enableContour, enableBarb, drawMap, enableVector, drawGridVectors, dPixelExtent, uValueData, vValueData, dpDestX, dpDestY);



//Make Contour if desired
//drawContour(valueData,fNodataValue,500,5000,drawImage);
if(enableContour||enableShade){
  drawContour(valueData,fNodataValue,shadeInterval,sourceImage,drawImage,enableContour,enableShade,enableContour);
}

/*
 *    for(int y=dPixelExtent[1];y<dPixelExtent[3];y++){
 *    for(int x=dPixelExtent[0];x<dPixelExtent[2];x++){
 *    size_t p = size_t((x-(dPixelExtent[0]))+((y-(dPixelExtent[1]))*dPixelDestW));
 *    for(int y1=-2;y1<4;y1++){
 *    for(int x1=-2;x1<4;x1++){
 *    drawImage->setPixel(dpDestX[p]+x1,dpDestY[p]+y1,248);
 * }
 * }
 * 
 * for(int y1=-1;y1<3;y1++){
 *  for(int x1=-1;x1<3;x1++){
 *  setValuePixel(sourceImage,drawImage,dpDestX[p]+x1,dpDestY[p]+y1,fpValues[p]);
 }
 }
 }
 }*/
/*if(drawMap==true)
 *    {
 *      for(int y=0;y<dImageHeight;y++){
 *        for(int x=0;x<dImageWidth;x++){
 *          setValuePixel(sourceImage,drawImage,x,y,valObj[0].valueData[x+y*dImageWidth]);
 * }
 * }
 * }*/

//                  if (enableVector) drawImage->drawVector(x,y,direction,strength,240);
//                  if (enableBarb) drawImage->drawBarb(x,y,direction,strength,240,convertToKnots,flip);
//                    
// if (enableVector) {
//   CalculatedWindVector wv;
//   for (size_t sz=0; sz<windVectors.size();sz++) {
//     wv=windVectors[sz];
//     drawImage->drawVector(wv.x, wv.y, wv.dir, wv.strength, 240);
//   }
// }                 
// if (enableBarb) {
//   CalculatedWindVector wv;
//   for (size_t sz=0; sz<windVectors.size();sz++) {
//     wv=windVectors[sz];
//     drawImage->drawBarb(wv.x, wv.y, wv.dir, wv.strength, 240,wv.convertToKnots,wv.flip);
//   }
// }    

if (enableVector) {
    CalculatedWindVector wv;
    for (size_t sz=0; sz<windVectors.size();sz++) {
      wv=windVectors[sz];
      drawImage->drawVector(wv.x, wv.y, wv.dir, wv.strength, 240);
    }
  }                 
  if (enableBarb) {
    CalculatedWindVector wv;
    for (size_t sz=0; sz<windVectors.size();sz++) {
      wv=windVectors[sz];
      drawImage->drawBarb(wv.x, wv.y, wv.dir, wv.strength, 240,wv.convertToKnots,wv.flip);
    }
  }                 

//Clean up
delete[] dpDestX;
delete[] dpDestY;

delete[] valObj;
 }
 
 
 
 
 /**
  * Checks at regular intervals wheter a contour line should be drawn or not.
  * @param val The four pixels to check
  * @param contourinterval The regular number to check against
  * @return True on contour with this interval needed, false on do nothing.
  */
 int checkContourRegularInterval(float *val,float &contourinterval){
   float allowedDifference=contourinterval/100000;
   float a,b;
   a = (val[0]<val[1]?val[0]:val[1]);b = (val[2]<val[3]?val[2]:val[3]);
   float min=a<b?a:b;
   a = (val[0]>val[1]?val[0]:val[1]);b = (val[2]>val[3]?val[2]:val[3]);
   float max=a>b?a:b;
   float iMin=int(min/contourinterval); if(min<0)iMin-=1;iMin*=contourinterval;
   float iMax=int(max/contourinterval); if(max<0)iMax-=1;iMax*=contourinterval+contourinterval;
   float difference=iMax-iMin;
   if((iMax-iMin)/contourinterval<3&&(iMax-iMin)/contourinterval>1&&difference>allowedDifference){
     for(double c=iMin;c<iMax;c=c+contourinterval){
       if((val[0]>=c&&val[1]<c)||(val[0]>c&&val[1]<=c)||(val[0]<c&&val[1]>=c)||(val[0]<=c&&val[1]>c)||
         (val[0]>c&&val[2]<=c)||(val[0]>=c&&val[2]<c)||(val[0]<=c&&val[2]>c)||(val[0]<c&&val[2]>=c))
       {
         return 1;
       }
     }
   }
   return 0;
 }
 
  
  /**
    * Checks at defined intervals wheter a contour line should be drawn or not.
    * @param val The four pixels to check
    * @param contourinterval The numbers to check against
    * @return True on contour with this interval needed, false on do nothing.
    */
  int checkContourDefinedInterval(float *val,std::vector<float> *intervals){
    for(size_t j=0;j<intervals->size();j++){
      float c= (*intervals)[j];
      if((val[0]>=c&&val[1]<c)||(val[0]>c&&val[1]<=c)||(val[0]<c&&val[1]>=c)||(val[0]<=c&&val[1]>c)||
        (val[0]>c&&val[2]<=c)||(val[0]>=c&&val[2]<c)||(val[0]<=c&&val[2]>c)||(val[0]<c&&val[2]>=c))
      {
        return 1;
      }
    }
    return 0;
  }
  

  

  
  /**
   * Check if this requires contours
   * @returns 16 bits unsigned short, 
   * where first 8 bytes represent which contourDefinition to use, incremented with one, 
   * and last 8 bytes represent which defined interval to use (in case when classes are defined)
   * When Zero is returned, no contours should be used.
   * e.g:
   * contourDefinitionIndex = ((returnValue-1)%CONTOURDEFINITIONLOOKUPLENGTH)
   * definedIntervalIndex within contourDefinition= (returnValue/CONTOURDEFINITIONLOOKUPLENGTH)-1
   */
  
  unsigned short CImgWarpBilinear::checkIfContourRequired(float *val){
    for(size_t j=0;j<contourDefinitions.size();j++){
      
      //Check for defined intervals
      if(contourDefinitions[j].definedIntervals.size()>0){
        for(size_t i=0;i<contourDefinitions[j].definedIntervals.size();i++){
          float c= contourDefinitions[j].definedIntervals[i];
          //(val[0]>=c&&val[1]<c)||(val[0]>c&&val[1]<=c)||(val[0]<c&&val[1]>=c)||(val[0]<=c&&val[1]>c)||
          //(val[0]>c&&val[2]<=c)||(val[0]>=c&&val[2]<c)||(val[0]<=c&&val[2]>c)||(val[0]<c&&val[2]>=c)
          if(
            (val[0]>=c&&val[1]<c)||(val[0]>c&&val[1]<=c)||(val[0]<c&&val[1]>=c)||(val[0]<=c&&val[1]>c)||
          (val[0]>c&&val[2]<=c)||(val[0]>=c&&val[2]<c)||(val[0]<=c&&val[2]>c)||(val[0]<c&&val[2]>=c)
          
          ){
            return j+(i+1)*CONTOURDEFINITIONLOOKUPLENGTH+1;
          }
        }
      }else{
        //Check for continuous intervals
        if(contourDefinitions[j].continuousInterval!=0.0){
          if(checkContourRegularInterval(val,contourDefinitions[j].continuousInterval)){
            return j+1;
          }
        }
      }
    
    }
    
    return 0;
  }
  
 
     
     
void CImgWarpBilinear::smoothData(float *valueData,float fNodataValue,int smoothWindow, int W,int H){
       
       //SmootH!
       #ifdef CImgWarpBilinear_TIME
       StopWatch_Stop("[SmoothData]");
       #endif
       if(smoothWindow==0)return;//No smoothing.
  size_t drawImageSize = W*H;
  float *valueData2 = new float[W*H];
  int smw=smoothWindow;
  //Create distance window;
  float distanceWindow[(smw+1)*2*(smw+1)*2];
  float distanceAmmount= 0;
  int dWinP=0;
  for(int y1=-smw;y1<smw+1;y1++){
    for(int x1=-smw;x1<smw+1;x1++){
      float d=sqrt(x1*x1+y1*y1);
      //d=d*8;
      
      d=1/(d+1);
      // d=1;
      distanceWindow[dWinP++]=d;
      distanceAmmount+=d;
    }
  }
  
  float d;
  for(int y=0;y<H;y++){
    for(int x=0;x<W;x++){
      size_t p = size_t(x+y*W);
      if(valueData[p]!=fNodataValue){
        dWinP=0;
        distanceAmmount=0;
        valueData2[p]=0;
        for(int y1=-smw;y1<smw+1;y1++){
          size_t yp=y1*W;
          for(int x1=-smw;x1<smw+1;x1++){
            if(x1+x<W&&y1+y<H&&x1+x>=0&&y1+y>=0){
              float val=valueData[p+x1+yp];
              if(val!=fNodataValue){
                d=distanceWindow[dWinP];
                distanceAmmount+=d;
                valueData2[p] += val*d;
              }
            }
            dWinP++;
          }
        }
        if(distanceAmmount>0)valueData2[p]/=distanceAmmount;
      }else valueData2[p]=fNodataValue;
    }
  }
  for(size_t p=0;p<drawImageSize;p++){valueData[p]=valueData2[p];}
  delete[] valueData2;
  #ifdef CImgWarpBilinear_TIME
  StopWatch_Stop("[/SmoothData]");
  #endif
  
  
}



int CImgWarpBilinear::set(const char *pszSettings){
  //fprintf(stderr, "CImgWarpBilinear.set(%s)\n", pszSettings);
  //"drawMap=false;drawContour=true;contourSmallInterval=1.0;contourBigInterval=10.0;"
  if(pszSettings==NULL)return 0;
  if(strlen(pszSettings)==0)return 0;
  contourDefinitions.clear();
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
     
      if(values[0].equals("shadeInterval")){
        shadeInterval=values[1].toFloat();
       // if(shadeInterval==0.0f){CDBWarning("invalid value given for shadeInterval %s",pszSettings);}
      }
      if(values[0].equals("smoothingFilter")){
        smoothingFilter=values[1].toInt();
        if(smoothingFilter<0||smoothingFilter>20){CDBWarning("invalid value given for smoothingFilter %s",pszSettings);}
      }
      

      if(values[0].equals("contourBigInterval")){
        float f = values[1].toFloat();
        if(f>0){
          contourDefinitions.push_back(ContourDefinition(1.4,CColor(0,0,0,255),CColor(0,0,0,255), f,NULL));
        }
      }
      
      if(values[0].equals("contourSmallInterval")){
        float f = values[1].toFloat();
        if(f>0){
          contourDefinitions.push_back(ContourDefinition(0.35,CColor(0,0,0,255),CColor(0,0,0,255), f,NULL));
        }
      }
      
      if(values[0].equals("shading")){
        CColor fillcolor=CColor(0,0,0,0);
        CColor bgColor=CColor(0,0,0,0);
        float max,min;
        bool foundColor=false;
        bool hasBGColor = false;
        
        CT::string *shadeSettings=values[1].splitToArray("$");
        for(size_t l=0;l<shadeSettings->count;l++){
          CT::string *kvp=shadeSettings[l].splitToArray("(");
          if(kvp[0].equals("min"))min=kvp[1].toFloat();
          if(kvp[0].equals("max"))max=kvp[1].toFloat();
          //if(kvp[0].equals("fillcolor")){kvp[1].setSize(7);fillcolor=CColor(kvp[1].c_str());foundColor=true;}
          if(kvp[0].equals("fillcolor")){
            kvp[1].setSize(kvp[1].length()-1);//Remove trailing bracket (')')
            fillcolor=CColor(kvp[1].c_str());foundColor=true;
          }
          if(kvp[0].equals("bgcolor")){
            kvp[1].setSize(kvp[1].length()-1);//Remove trailing bracket (')')
            CDBDebug("Found bgcolor");
            bgColor=CColor(kvp[1].c_str());hasBGColor=true;
          }
          delete[] kvp;
        }

        shadeDefinitions.push_back(ShadeDefinition(min,max,fillcolor,foundColor,bgColor,hasBGColor));
        delete[] shadeSettings;
      }
      
      
      if(values[0].equals("contourline")){
        float lineWidth=1;
        CColor linecolor=CColor(0,0,0,255);
        CColor textcolor=CColor(0,0,0,255);
        float interval=0;
        CT::string textformat="";
        CT::string classes="";
        CT::string *lineSettings=values[1].splitToArray("$");
        for(size_t l=0;l<lineSettings->count;l++){
          CT::string *kvp=lineSettings[l].splitToArray("(");
          if(kvp->count==2){
            int endOfKVP = kvp[1].lastIndexOf(")");
            if(endOfKVP!=-1){
              kvp[1].setSize(endOfKVP);
            }
            
            if(kvp[0].equals("width"))lineWidth=kvp[1].toFloat();
            if(kvp[0].equals("interval")){
              interval=kvp[1].toFloat();
            }
            if(kvp[0].equals("classes")){
              classes.copy(kvp[1].c_str());
            }
            if(kvp[0].equals("linecolor")){kvp[1].setSize(7);linecolor=CColor(kvp[1].c_str());}
            if(kvp[0].equals("textcolor")){kvp[1].setSize(7);textcolor=CColor(kvp[1].c_str());}
            if(kvp[0].equals("textformatting")){textformat.copy(kvp[1].c_str(),kvp[1].length());}
          }
          delete[] kvp;
        }
   
        if(classes.length()>0){
          contourDefinitions.push_back(ContourDefinition(lineWidth,linecolor, textcolor,classes.c_str(),textformat.c_str()));
        }else{
          if(interval>0){
            contourDefinitions.push_back(ContourDefinition(lineWidth,linecolor, textcolor,interval,textformat.c_str()));
          }
        }
      
        delete[] lineSettings;
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

bool IsTextTooClose(std::vector<Point> *textLocations,int x,int y){
  
  for(size_t j=0;j<textLocations->size();j++){
    int dx=x-(*textLocations)[j].x;
    int dy=y-(*textLocations)[j].y;
    if((dx*dx+dy*dy)<10*10){
      return true;
    }
  }
  return false;
}

void CImgWarpBilinear::drawTextForContourLines(CDrawImage * drawImage,ContourDefinition *contourDefinition, int lineX, int lineY,int endX, int endY, std::vector<Point> *textLocations,float value,CColor textColor) {

  /* Draw text */
  CT::string text;
  text.print(contourDefinition->textFormat.c_str(),value);

  double angle;
  if(lineY-endY!=0){
    angle=atan((-double(lineY-endY))/(double(lineX-endX)));
  }else {
    angle=0;
  }
  float cx=(lineX+endX)/2;
  float cy=(endY+lineY)/2;
  float tx=cx;
  float ty=cy;
  float ca=cos(angle);
  float sa=sin(angle);
  float offX=((ca*text.length()*2.3)+(sa*-2.3));
  float offY=(-(sa*text.length()*3.0)+(ca*-2.3));
  tx-=offX;
  ty-=offY;
  int x = int(tx)+1;
  int y=  int(ty)+1;
  drawImage->drawText( x,y, angle,text.c_str(),textColor);
  
}

/* 
Search window for xdir and ydir:
      -1  0  1  (x)
  -1   6  5  4 
   0   7  X  3 
   1   0  1  2 
  (y)
/*           0  1  2  3  4  5  6  7*/
int xdir[]={-1, 0, 1, 1, 1, 0,-1,-1};
int ydir[]={ 1, 1, 1, 0,-1,-1,-1, 0};
/*                0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15 */
int xdirOuter[]={-2,-1, 0, 1, 2, 2, 2, 2, 2, 1, 0,-1,-2,-2,-2,-2};
int ydirOuter[]={ 2, 2, 2, 2, 2, 1, 0,-1,-2,-2,-2,-2,-2,-1, 0, 1};


#define MAX_LINE_SEGMENTS 1000

void CImgWarpBilinear::traverseLine(CDrawImage * drawImage,DISTANCEFIELDTYPE *distance,float *valueField,int lineX,int lineY,int dImageWidth,int dImageHeight,float lineWidth,CColor lineColor,CColor textColor,ContourDefinition *contourDefinition,DISTANCEFIELDTYPE lineMask,bool drawText,std::vector<Point> *textLocations){
  size_t p = lineX+lineY*dImageWidth; /* Starting pointer */
  bool foundLine = true;   /* This function starts at the beginning of a line segment */
  int maxLineDistance = 5; /* Maximum length of each line segment */
  int currentLineDistance = maxLineDistance;
  int lineSegmentsX[MAX_LINE_SEGMENTS + 1];
  int lineSegmentsY[MAX_LINE_SEGMENTS + 1];
  
  int lineSegmentCounter = 0;

  /* Push the beginning of this line*/
  lineSegmentsX[lineSegmentCounter]=lineX;
  lineSegmentsY[lineSegmentCounter]=lineY;
  float lineSegmentsValue = valueField[p];
  float binnedLineSegmentsValue;
     
  if(contourDefinition->definedIntervals.size()>0){
    float closestValue;
    int definedIntervalIndex = 0;
    for(size_t j=0;j<contourDefinition->definedIntervals.size();j++){
      float c =contourDefinition->definedIntervals[j];
      float d= fabs(lineSegmentsValue-c);
      if(j==0)closestValue = d;else{
        if(d<closestValue){
          closestValue=d;
          definedIntervalIndex = j;
        }
      }
    }
    binnedLineSegmentsValue = contourDefinition->definedIntervals[definedIntervalIndex];
  }else{
    binnedLineSegmentsValue = convertValueToClass(lineSegmentsValue+contourDefinition->continuousInterval/2,contourDefinition->continuousInterval);
  }

  lineSegmentCounter++;
 
  /* Use the distance field and walk the line */
  while(foundLine){
    distance[p]&=~lineMask; /* Indicate found, set to false */ 
    /* Search around using the small search window and find the continuation of this line */
    foundLine = false;
    int nextLineX = lineX;
    int nextLineY = lineY;
    for(int j=0;j<8;j++){
      int tx = lineX+xdir[j];
      int ty = lineY+ydir[j];
      if(tx>=0&&tx<dImageWidth&&ty>=0&&ty<dImageHeight){
        p = tx+ty*dImageWidth;
        if(distance[p]&lineMask && !foundLine){
          nextLineX=tx;
          nextLineY=ty;
          foundLine = true;
        }
        distance[p]&=~lineMask;/* Indicate found, set to false */ 
      }  
    }

    /* Search line with outer window. */
    if (!foundLine) {
      // Try to find the line with an outer window...
      for(int j=0;j<16;j++){
        int tx = lineX+xdirOuter[j];
        int ty = lineY+ydirOuter[j];
        if(tx>=0&&tx<dImageWidth&&ty>=0&&ty<dImageHeight){
          p = tx+ty*dImageWidth;
          if(distance[p]&lineMask && !foundLine){
            nextLineX=tx;
            nextLineY=ty;
            foundLine = true;
            break;
          }
        }  
      }
    }
    // if (!foundLine){
    //   drawImage->rectangle(lineX-5, lineY-5, lineX+5,lineY+5, 240);
    // }
    lineX = nextLineX;
    lineY = nextLineY;
    /* Decrease the max currentLineDist counter, 
       when zero, the max line distance is reached 
       and we should add a line segment */
    currentLineDistance--;
    if (currentLineDistance<=0 || foundLine == false){
      currentLineDistance = maxLineDistance;
      if (lineSegmentCounter<MAX_LINE_SEGMENTS){
        lineSegmentsX[lineSegmentCounter]=lineX;
        lineSegmentsY[lineSegmentCounter]=lineY;
        lineSegmentCounter++;
        /* If we have reached max line segments, stop it */
        if (lineSegmentCounter >= MAX_LINE_SEGMENTS) {
          foundLine = false;
        }
      }
    }
  }

  // textLocations->clear();
  /* Now draw this line */  
  drawImage->moveTo(lineSegmentsX[0], lineSegmentsY[0]);
  bool textSkip = false;
  bool textOn = false;
  for(int j=0; j < lineSegmentCounter; j++){
    if (j%50 == 0 && j+3 <lineSegmentCounter) {
      textOn = false;
      if(IsTextTooClose(textLocations,lineSegmentsX[j],lineSegmentsY[j])==false){
        textSkip = false;
        textLocations->push_back(Point(lineSegmentsX[j],lineSegmentsY[j]));
        this->drawTextForContourLines(drawImage, contourDefinition, lineSegmentsX[j],lineSegmentsY[j],lineSegmentsX[j+3],lineSegmentsY[j+3], textLocations, binnedLineSegmentsValue,textColor);
        textOn = true;
      } else {
        textSkip = true;
      }
    }
    if (j%50 == 0 && textOn && !textSkip){
      drawImage->endLine();
    }
    if (j%50 > 3 || textSkip){
      drawImage->lineTo(lineSegmentsX[j],lineSegmentsY[j], lineWidth,lineColor);
    }
  }
  drawImage->endLine();
}


void CImgWarpBilinear::drawContour(float *valueData,float fNodataValue,float interval,CDataSource *dataSource,CDrawImage *drawImage,bool drawLine, bool drawShade, bool drawText){
   CStyleConfiguration *styleConfiguration = dataSource->getStyle();// TODO SLOW
   //When using min/max stretching, the shadeclasses need to be extended according to its shade interval
   if(dataSource->stretchMinMax==true){
     if(dataSource->statistics!=NULL){
       float legendInterval=interval;
       float minValue=(float)dataSource->statistics->getMinimum();
       float maxValue=(float)dataSource->statistics->getMaximum();
       float iMin=convertValueToClass(minValue,legendInterval);
       float iMax=convertValueToClass(maxValue,legendInterval)+legendInterval;
       //Calculate new scale and offset for this:
       float ls=240/((iMax-iMin));
       float lo=-(iMin*ls);
       styleConfiguration->legendScale=ls;
       styleConfiguration->legendOffset=lo;
     }
   }
 
   
   
  
   //float ival = interval;
   //   float ivalLine = interval;
   //float idval=int(ival+0.5);
   //  if(idval<1)idval=1;
   //TODO
   
   //char szTemp[8192];
   //szTemp[0]='\0';
   //float currentTextValue = 0;
   //int contourDefinitionIndex = -1;
   
   int dImageWidth=drawImage->Geo->dWidth+1;
   int dImageHeight=drawImage->Geo->dHeight+1;
   
   size_t imageSize=(dImageHeight+0)*(dImageWidth+1);
   #ifdef CImgWarpBilinear_DEBUG
   CDBDebug("imagesize = %d",(int)imageSize);
   #endif
   
   //Create a distance field, this is where the line information will be put in.
   DISTANCEFIELDTYPE *distance = new DISTANCEFIELDTYPE[imageSize];
  
  
/*   

   
   //TODO "pleister" om contourlijnen goed te krijgen.
   if(1==2){
     #ifdef CImgWarpBilinear_TIME
     StopWatch_Stop("substracting ival/100");
     #endif

     float substractVal=ival/100;
     for(int y=0;y<dImageHeight;y++){
       for(int x=0;x<dImageWidth;x++){
         valueData[x+y*dImageWidth]-=substractVal;
       }
     }
     fNodataValue-=substractVal;
     #ifdef CImgWarpBilinear_TIME
     StopWatch_Stop("finished substracting ival/100");
     #endif
   }*/
   
   #ifdef CImgWarpBilinear_DEBUG
   CDBDebug("start shade/contour with nodatavalue %f",fNodataValue);
   #endif
  
   float val[4];
   
   
   //Expand the ShadeDefinitions
   std::vector<ShadeDefinition> shadeDefinitionsExpanded;
   std::set<double>intervals;
   std::set<double>::iterator intervalsit;
   for(size_t j=0;j<shadeDefinitions.size();j++){
     intervals.insert(shadeDefinitions[j].min);
     intervals.insert(shadeDefinitions[j].max);
   }
   double previ;
   int nr = 0;
   for (intervalsit=intervals.begin(); intervalsit!=intervals.end(); ++intervalsit){
     double i = *intervalsit;
     if(nr>0){
      int foundOne = -1;
      for(int j=int (shadeDefinitions.size())-1;j>=0;j--){
        if(shadeDefinitions[j].min<=previ&&shadeDefinitions[j].max>=i){
          foundOne = j;
          break;
        }
      }
      if(foundOne!=-1){
        // CDBDebug("SHADEDEF %d uses def %d\t(%f\t%f)",shadeDefinitionsExpanded.size(),foundOne,previ,i);
        shadeDefinitionsExpanded.push_back(ShadeDefinition(previ,i,shadeDefinitions[foundOne].fillColor,shadeDefinitions[foundOne].foundColor,shadeDefinitions[foundOne].bgColor,shadeDefinitions[foundOne].hasBGColor));
      }
     }
     previ=i;
     nr++;
   }
   
   //Fill out the bgcolor
    
    if(shadeDefinitionsExpanded.size()>0){
      if(shadeDefinitionsExpanded[0].hasBGColor){
        for(int y=0;y<dImageHeight;y++){
          for(int x=0;x<dImageWidth;x++){
            drawImage->setPixelTrueColor(x,y,shadeDefinitionsExpanded[0].bgColor.r,
                                         shadeDefinitionsExpanded[0].bgColor.g,
                                         shadeDefinitionsExpanded[0].bgColor.b,
                                         shadeDefinitionsExpanded[0].bgColor.a);
          }
        }
      }
    }
    
    int snr=0;
    int numShadeDefs=(int)shadeDefinitionsExpanded.size();
    float shadeDefMin[numShadeDefs];
    float shadeDefMax[numShadeDefs];
    unsigned char shadeColorR[numShadeDefs];
    unsigned char shadeColorG[numShadeDefs];
    unsigned char shadeColorB[numShadeDefs];
    unsigned char shadeColorA[numShadeDefs];
    for(snr=0;snr<numShadeDefs;snr++){
      shadeDefMin[snr]=shadeDefinitionsExpanded[snr].min;
      shadeDefMax[snr]=shadeDefinitionsExpanded[snr].max;
      
      if(shadeDefinitionsExpanded[snr].foundColor){
      shadeColorR[snr]=shadeDefinitionsExpanded[snr].fillColor.r;
      shadeColorG[snr]=shadeDefinitionsExpanded[snr].fillColor.g;
      shadeColorB[snr]=shadeDefinitionsExpanded[snr].fillColor.b;
      shadeColorA[snr]=shadeDefinitionsExpanded[snr].fillColor.a;
      }else{
        CColor color=drawImage->getColorForIndex(getPixelIndexForValue(dataSource,shadeDefMin[snr]));
        shadeColorR[snr]=color.r;
        shadeColorG[snr]=color.g;
        shadeColorB[snr]=color.b;
        shadeColorA[snr]=color.a;
      }
    }
    int lastShadeDef=0;
    
   //float minValue = CImageDataWriter::getValueForColorIndex(dataSource,0);;
   //float maxValue = CImageDataWriter::getValueForColorIndex(dataSource,240);;
//     for(int y=0;y<dImageHeight-1;y++){
//      for(int x=0;x<dImageWidth-1;x++){
//        
//        float v = valueData[x+y*dImageWidth];
//        if(v>maxValue)valueData[x+y*dImageWidth]=maxValue;
//        if(v<minValue)valueData[x+y*dImageWidth]=minValue;
//      }
//     }
   //Shade
   for(int y=0;y<dImageHeight-1;y++){
     for(int x=0;x<dImageWidth-1;x++){
       size_t p1 = size_t(x+y*dImageWidth);
       val[0] = valueData[p1];
       val[1] = valueData[p1+1];
       val[2] = valueData[p1+dImageWidth];
       val[3] = valueData[p1+dImageWidth+1];
       
      
       
       
       //Check if all pixels have values...
       if(val[0]!=fNodataValue&&val[1]!=fNodataValue&&val[2]!=fNodataValue&&val[3]!=fNodataValue&&
         val[0]==val[0]&&val[1]==val[1]&&val[2]==val[2]&&val[3]==val[3]
       ){
//         for(int i=0;i<4;i++){
//           if(val[i]<minValue)val[i]=minValue;else if(val[i]>maxValue)val[i]=maxValue;
//         }
//          

         //Draw shading
         if(drawShade){
           if(interval!=0){
              setValuePixel(dataSource,drawImage,x,y,convertValueToClass(val[0],interval));
           }else{
             int done=numShadeDefs;
             if(val[0]>=shadeDefMin[lastShadeDef]&&val[0]<shadeDefMax[lastShadeDef]){
               done=-1;
            }else{
              do{
                lastShadeDef++;if(lastShadeDef>numShadeDefs-1)lastShadeDef=0;
                done--;
                if(val[0]>=shadeDefMin[lastShadeDef]&&val[0]<shadeDefMax[lastShadeDef]){
                  done=-1;
                }
              }while(done>0);
             }
             if(done==-1){
               if(shadeColorA[lastShadeDef] == 0){ //When a fully transparent color is deliberately set, force this color in the image
                 drawImage->setPixelTrueColorOverWrite(x,y,shadeColorR[lastShadeDef],shadeColorG[lastShadeDef],shadeColorB[lastShadeDef],shadeColorA[lastShadeDef]);
               }else{
                drawImage->setPixelTrueColor(x,y,shadeColorR[lastShadeDef],shadeColorG[lastShadeDef],shadeColorB[lastShadeDef],shadeColorA[lastShadeDef]);
               }
             }
           }
         }
       }
     }
   }

    float lineWidth= 4;
    CColor lineColor = CColor(0,0,0,255);
    CColor textColor = CColor(0,0,0,255);
    //Determine contour lines
    memset (distance,0,imageSize*sizeof(DISTANCEFIELDTYPE));
  
    for(int y=0;y<dImageHeight-1;y++){
      for(int x=0;x<dImageWidth-1;x++){
        size_t p1 = size_t(x+y*dImageWidth);
        val[0] = valueData[p1];
        val[1] = valueData[p1+1];
        val[2] = valueData[p1+dImageWidth];
        val[3] = valueData[p1+dImageWidth+1];
        
        
        //Check if all pixels have values...
        if(val[0]!=fNodataValue&&val[1]!=fNodataValue&&val[2]!=fNodataValue&&val[3]!=fNodataValue&&
          val[0]==val[0]&&val[1]==val[1]&&val[2]==val[2]&&val[3]==val[3]
        ){
//           for(int i=0;i<4;i++){
//             if(val[i]<minValue)val[i]=minValue;else if(val[i]>maxValue)val[i]=maxValue;
//           }
          //Draw contourlines
          if(drawLine||drawText){
            int mask=1;
            for(size_t j=0;j<contourDefinitions.size();j++){
              if(contourDefinitions[j].definedIntervals.size()>0){
                //Check for intervals
                for(size_t i=0;i<contourDefinitions[j].definedIntervals.size();i++){
                  float c= contourDefinitions[j].definedIntervals[i];
                  if(
                    (val[0]>=c&&val[1]<c)||(val[0]>c&&val[1]<=c)||(val[0]<c&&val[1]>=c)||(val[0]<=c&&val[1]>c)||
                    (val[0]>c&&val[2]<=c)||(val[0]>=c&&val[2]<c)||(val[0]<=c&&val[2]>c)||(val[0]<c&&val[2]>=c)
                
                  ){
                    distance[p1]|=mask;
                    break;
                  }
                }
              }else{
                //Check for continuous lines
                if(contourDefinitions[j].continuousInterval!=0.0){
                  float contourinterval = contourDefinitions[j].continuousInterval;
                  float allowedDifference=contourinterval/100000;
                  /*float a,b;
                  a = (val[0]<val[1]?val[0]:val[1]);b = (val[2]<val[3]?val[2]:val[3]);
                  //float min=a<b?a:b;
                  a = (val[0]>val[1]?val[0]:val[1]);b = (val[2]>val[3]?val[2]:val[3]);
                  //float max=a>b?a:b;*/
                  float min,max;
                  min=val[0];max=val[0];
                  for(int j=1;j<4;j++){
                    if(val[j]<min)min=val[j];
                    if(val[j]>max)max=val[j];
                  }
                  float iMin=int(min/contourinterval); if(min<0)iMin-=1;iMin*=contourinterval;
                  float iMax=int(max/contourinterval); if(max<0)iMax-=1;iMax*=contourinterval;
                  iMax+=contourinterval;
                  float difference=iMax-iMin;

                  float classStart = round(val[0]/contourinterval)*contourinterval;
                  {
                    {
                      if(
                         (val[0]>classStart&&val[1]<=classStart)|| // TL => TR
                         (val[0]>classStart&&val[2]<=classStart)|| // TL => BL
                         (val[1]>classStart&&val[0]<=classStart)|| // TR => TL
                         (val[2]>classStart&&val[0]<=classStart)   //  BL => TL
                        )
                      {
                        distance[p1]|=mask;
                        break;
                      }
                    }
                  }
              }
            }
            mask=mask+mask;
          }
        }
      }
    }
  }
      
  std::vector<Point> textLocations;
  
  DISTANCEFIELDTYPE lineMask = 1;
  for(size_t j=0;j<contourDefinitions.size();j++){
    lineColor=contourDefinitions[j].linecolor;
    textColor=contourDefinitions[j].textcolor;
    lineWidth=contourDefinitions[j].lineWidth;
    
    /* Everywhere */
    for(int y=0;y<dImageHeight;y++){
      for(int x=0;x<dImageWidth;x++){     
        size_t p = x+y*dImageWidth;
        if(distance[p]&lineMask){
          traverseLine(drawImage,distance,valueData,x,y,dImageWidth,dImageHeight,lineWidth,lineColor,textColor,&contourDefinitions[j],lineMask,drawText,&textLocations);
        }
      }
    }
    lineMask=lineMask+lineMask;    
  }

  #ifdef CImgWarpBilinear_DEBUG
  CDBDebug("Deleting distance[]");
  #endif
  
  delete[] distance;
  
  #ifdef CImgWarpBilinear_DEBUG
  CDBDebug("Finished drawing lines and text");
  #endif
       
}
