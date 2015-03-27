#include <math.h>
#include <stdlib.h>
#include <proj_api.h>
#include <math.h>
#include <cfloat>
#include "CGeoParams.h"
#include "CImageWarper.h"
#include "CDebugger.h"

class GenericDataWarper{
 private:
  DEF_ERRORFUNCTION();
  template <class T>
  static void drawTriangle(int *xP,int *yP, T value,int destWidth,int destHeight,void *settings, void (*drawFunction)(int ,int,T,void *settings)){
    
    int W = destWidth;
    int H = destHeight;
    if(xP[0]<0&&xP[1]<0&&xP[2]<0)return;
    if(xP[0]>=W&&xP[1]>=W&&xP[2]>=W)return;
    if(yP[0]<0&&yP[1]<0&&yP[2]<0)return;
    if(yP[0]>=H&&yP[1]>=H&&yP[2]>=H)return;  
    
    unsigned int lower;
    unsigned int middle;
    unsigned int upper;
    
    /*Sort the vertices in Y direction*/
    if(yP[0]<yP[1]){
      if(yP[0]<yP[2]){lower=0;if(yP[1]<yP[2]){middle=1;upper=2;}else{middle=2;upper=1;}}else{middle=0;lower=2;upper=1;}
    }else{
      if(yP[1]<yP[2]){lower=1;if(yP[0]<yP[2]){middle=0;upper=2;}else{middle=2;upper=0;}}else{middle=1;lower=2;upper=0;}
    }
    
    int X1 = xP[lower];
    int X2 = xP[middle];
    int X3 = xP[upper];
    int Y1 = yP[lower];
    int Y2 = yP[middle];
    int Y3 = yP[upper];
    
    if((Y1 == Y3)||(Y2==Y1&&Y3==Y2))return;

    float rcl = float(X3-X1)/float(Y3-Y1);
    if(Y2!=Y1&&Y1<H&&Y2>0){
      float rca = float(X2-X1)/float(Y2-Y1);
      int sy = (Y1<0)?0:Y1;
      int ey = (Y2>H)?H:Y2;
      for(int y=sy;y<=ey;y++){
        int xL = int(rcl*float(y-Y1)+X1);
        int xA = int(rca*float(y-Y1)+X1);
        int x1,x2;
        if(xL<xA){x1=xL;x2=xA;}else{x2=xL;x1=xA;}
        if(x1<W&&x2>0){
          int sx = (x1<0)?0:x1;
          int ex = (x2>W)?W:x2;
          for(int x=sx;x<=ex;x++){
            drawFunction(x,y,value,settings);
          }
        }
      }
    }
    
    if(Y3 != Y2&&Y2<H&&Y3>0){
      float rcb = float(X3-X2)/float(Y3-Y2);
      int sy = (Y2<0)?0:Y2;
      int ey = (Y3>H)?H:Y3;
      for(int y=sy;y<=ey;y++){
        int xL = floor(rcl*float(y-Y1)+X1);
        int xB = floor(rcb*float(y-Y2)+X2);
        int x1,x2;
        if(xL<=xB){x1=xL;x2=xB;}else{x2=xL;x1=xB;}
        if(x1<W&&x2>0){
          int sx = (x1<0)?0:x1;
          int ex = (x2>W)?W:x2;
          for(int x=sx;x<=ex;x++){
            drawFunction(x,y,value,settings);
          }
        } 
      }
    }
  }
   
  public:
  template <class T>
  static int render(CImageWarper *warper,void *sourceData,CGeoParams*sourceGeoParams,CGeoParams*destGeoParams,void *drawFunctionSettings,void (*drawFunction)(int ,int,T,void *drawFunctionSettings)){
    
    int dataWidth = sourceGeoParams->dWidth;
    int dataHeight = sourceGeoParams->dHeight;
    int imageWidth = destGeoParams->dWidth;
    int imageHeight = destGeoParams->dHeight;
    bool destNeedsDegreeRadianConversion = false;
    bool sourceNeedsDegreeRadianConversion = false;
    
    //Reproj back and forth sourceGeoParams boundingbox
    double y1=sourceGeoParams->dfBBOX[1];
    double y2=sourceGeoParams->dfBBOX[3];
    double x1=sourceGeoParams->dfBBOX[0];
    double x2=sourceGeoParams->dfBBOX[2];
    
    if(y2<y1){
      if(y1>-360&&y2<360&&x1>-720&&x2<720){
        if( CGeoParams::isLonLatProjection(&sourceGeoParams->CRS)==false){
          double checkBBOX[4];
          for(int j=0;j<4;j++)checkBBOX[j]=sourceGeoParams->dfBBOX[j];
          
          //CDBDebug("Current BBOX:  %f %f %f %f",sourceGeoParams->dfBBOX[0],sourceGeoParams->dfBBOX[1],sourceGeoParams->dfBBOX[2],sourceGeoParams->dfBBOX[3]);
          bool hasError = false;
          if(warper->reprojpoint_inv(checkBBOX[0],checkBBOX[1])!=0)hasError=true;  
          if(warper->reprojpoint(checkBBOX[0],checkBBOX[1])!=0)hasError=true;  
          
          if(warper->reprojpoint_inv(checkBBOX[2],checkBBOX[3])!=0)hasError=true;  
          if(warper->reprojpoint(checkBBOX[2],checkBBOX[3])!=0)hasError=true;  
          
          if(hasError == false){
            for(int j=0;j<4;j++)sourceGeoParams->dfBBOX[j] = checkBBOX[j];
          }
        }
      }
    }

    double dfSourceExtW=(sourceGeoParams->dfBBOX[2]-sourceGeoParams->dfBBOX[0]);
    double dfSourceExtH=(sourceGeoParams->dfBBOX[1]-sourceGeoParams->dfBBOX[3]);
    double dfSourceW = double(sourceGeoParams->dWidth);
    double dfSourceH = double(sourceGeoParams->dHeight);
    double dfSourcedExtW=dfSourceExtW/dfSourceW;
    double dfSourcedExtH=dfSourceExtH/dfSourceH;
    double dfSourceOrigX=sourceGeoParams->dfBBOX[0];
    double dfSourceOrigY=sourceGeoParams->dfBBOX[3];
    
    double dfDestExtW = destGeoParams->dfBBOX[2]-destGeoParams->dfBBOX[0];
    double dfDestExtH = destGeoParams->dfBBOX[1]-destGeoParams->dfBBOX[3];
    double dfDestOrigX = destGeoParams->dfBBOX[0];
    double dfDestOrigY = destGeoParams->dfBBOX[3];

    
    double multiDestX = double(imageWidth)/dfDestExtW;
    double multiDestY = double(imageHeight)/dfDestExtH;

    size_t dataSize = (dataWidth+1) * (dataHeight+1);

    
    CT::string destinationCRS;
    warper->decodeCRS(&destinationCRS,&destGeoParams->CRS);
    if(destinationCRS.indexOf("longlat")>=0){
      destNeedsDegreeRadianConversion = true;
    }
    if(sourceGeoParams->CRS.indexOf("longlat")>=0){
      sourceNeedsDegreeRadianConversion = true;
    }
    
    double *px = new double[dataSize];
    double *py = new double[dataSize];
    char *skip = new char[dataSize];
      
    for(int y=0;y<dataHeight+1;y++){
      for(int x=0;x<dataWidth+1;x++){
        size_t p = x+y*(dataWidth+1);
        px[p] =dfSourcedExtW*double(x)+dfSourceOrigX;//+dfCellSizeX/2.0;
        py[p] =dfSourcedExtH*double(y)+dfSourceOrigY;//+dfCellSizeY/2.0;
        skip[p] = false;
      }
    }

    if(warper->isProjectionRequired()){
      if(sourceNeedsDegreeRadianConversion){
        for(size_t j=0;j<dataSize;j++){
          px[j]*=DEG_TO_RAD;
          py[j]*=DEG_TO_RAD;
        }
      }
      if(pj_transform(warper->sourcepj,warper->destpj, dataSize,0,px,py,NULL)){
        CDBDebug("Unable to do pj_transform");
      }
      if(destNeedsDegreeRadianConversion){
        for(size_t j=0;j<dataSize;j++){
          px[j]/=DEG_TO_RAD;
          py[j]/=DEG_TO_RAD;
        }
      }
    }

    for(size_t j=0;j<dataSize;j++){
      if(px[j]>-DBL_MAX&&px[j]<DBL_MAX){
        px[j]-=dfDestOrigX;
        py[j]-=dfDestOrigY;
        px[j]*=multiDestX;
        py[j]*=multiDestY;
        px[j]+=0.5;
        py[j]+=0.5;
      }else{
        skip[j]=true;        
      }
    }
    
    double avgDX = 0;
    double avgDY = 0;

    for(int y=0;y<dataHeight;y=y+1){
      for(int x=0;x<dataWidth;x=x+1){
        size_t p=x+y*(dataWidth+1);
        if(skip[p]==false&&skip[p+1]==false&&skip[p+dataWidth+1]==false&&skip[p+dataWidth+2]==false)
        {
          double px1 = px[p];
          double px2 = px[p+1];
          double px3 = px[p+dataWidth+2];
          double px4 = px[p+dataWidth+1];

          double py1 = py[p];
          double py2 = py[p+1];
          double py3 = py[p+dataWidth+2];
          double py4 = py[p+dataWidth+1];
          bool doDraw = true;
          
          if(x==0)avgDX = px2;
          if(y==0)avgDY = py4;
          
          if(x==dataWidth-1){
            if(fabs(avgDX-px1)<fabs(px1-px2)/2){
              doDraw = false;
            }
            if(fabs(avgDX-px2)<fabs(px1-px2)/2){
              doDraw = false;
            }
          }
          if(y==dataHeight-1){
            if(fabs(avgDY-py1)<fabs(py1-py4)/2){
              doDraw = false;
            }
          }

          if(doDraw){
            T value = ((T*)sourceData)[x+y*dataWidth];
            double mX = (px1+px2+px3+px4)/4;
            double mY = (py1+py2+py3+py4)/4;
            int xP[3];
            int yP[3];
            xP[0] = px1;
            xP[1] = px2;
            xP[2] = mX;

            yP[0] = py1;
            yP[1] = py2;
            yP[2] = mY;
            drawTriangle<T>( xP,yP,value,imageWidth,imageHeight,drawFunctionSettings,drawFunction);
            
            xP[0] = px3;
            yP[0] = py3;
            drawTriangle<T>( xP,yP,value,imageWidth,imageHeight,drawFunctionSettings,drawFunction);

            xP[1]=px4;
            yP[1]=py4;
            drawTriangle<T>( xP,yP,value,imageWidth,imageHeight,drawFunctionSettings,drawFunction);

            xP[0] = px1;
            yP[0] = py1;
            drawTriangle<T>( xP,yP,value,imageWidth,imageHeight,drawFunctionSettings,drawFunction);
          }
        }
      }
    }

    delete[] px;
    delete[] py;
    delete[] skip;
    return 0;
  }
};
