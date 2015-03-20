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
  static void drawTriangle(int *xP,int *yP, void *sourceData,int sourceWidth,int sourceHeight,size_t *indexX,size_t *indexY,int destWidth,int destHeight,void *settings, void (*drawFunction)(int ,int,T,void *settings)){
    

   //Sort the vertices in Y direction
    int W = destWidth;
    int H = destHeight;
    if(xP[0]<0&&xP[1]<0&&xP[2]<0)return;
    if(xP[0]>=W&&xP[1]>=W&&xP[2]>=W)return;
    if(yP[0]<0&&yP[1]<0&&yP[2]<0)return;
    if(yP[0]>=H&&yP[1]>=H&&yP[2]>=H)return;  

    
    T *data=(T*)sourceData;

    
    unsigned int lower;
    unsigned int middle;
    unsigned int upper;
    
    if(yP[0]<yP[1]){
      if(yP[0]<yP[2]){
        lower=0;
        if(yP[1]<yP[2]){
          middle=1;
          upper=2;
        }else{
          middle=2;
          upper=1;
        }
      }else{
        middle=0;
        lower=2;
        upper=1;
      }
    }else{
      if(yP[1]<yP[2]){
        lower=1;
        if(yP[0]<yP[2]){
          middle=0;
          upper=2;
        }else{
          middle=2;
          upper=0;
        }
      }else{
        middle=1;
        lower=2;
        upper=0;
      }
    }
    
    int X1 = xP[lower];
    int X2 = xP[middle];
    int X3 = xP[upper];
    int Y1 = yP[lower];
    int Y2 = yP[middle];
    int Y3 = yP[upper];
    
    if(Y1 == Y3)return;
    
    if(Y2==Y1&&Y3==Y2)return;
    
    

    
    
   
    
    float VX1 = indexX[lower];
    float VX2 = indexX[middle];
    float VX3 = indexX[upper];
    
    float VY1 = indexY[lower];
    float VY2 = indexY[middle];
    float VY3 = indexY[upper];

    float rcl = float(X3-X1)/float(Y3-Y1);
    float rcvxl = (VX3-VX1)/float(Y3-Y1);
    float rcvyl = (VY3-VY1)/float(Y3-Y1);
    
    
    if(Y2!=Y1&&Y1<H&&Y2>0){
      float rca = float(X2-X1)/float(Y2-Y1);
      float rcvxa = (VX2-VX1)/float(Y2-Y1);
      float rcvya = (VY2-VY1)/float(Y2-Y1);

    
      int sy = (Y1<0)?0:Y1;
      int ey = (Y2>H)?H:Y2;
      
      for(int y=sy;y<ey;y++){
        int xL = (int)(rcl*float(y-Y1)+X1);
        int xA = (int)(rca*float(y-Y1)+X1);
        float vxL = rcvxl*float(y-Y1)+VX1;
        float vxA = rcvxa*float(y-Y1)+VX1;
        float vyL = rcvyl*float(y-Y1)+VY1;
        float vyA = rcvya*float(y-Y1)+VY1;
        int x1,x2;
        float vx1,vx2,vy1,vy2;
        if(xL<xA){x1=xL;x2=xA;vx1=vxL;vx2=vxA;vy1=vyL;vy2=vyA;}else{x2=xL;x1=xA;vx1=vxA;vx2=vxL;vy1=vyA;vy2=vyL;}
        if(x1<W&&x2>0){
          int sx = (x1<0)?0:x1;
          int ex = (x2>W)?W:x2;
          float rcxvx = float(vx2-vx1)/float(x2-x1);
          float rcxvy = float(vy2-vy1)/float(x2-x1);
          for(int x=sx;x<ex;x++){
            int vx = int(rcxvx*float(x-x1)+vx1);
            int vy = int(rcxvy*float(x-x1)+vy1);
            if(vx>=0&&vy>=0&&vx<sourceWidth+1&&vy<sourceHeight){
              drawFunction(x,y,data[vx+vy*sourceWidth],settings);
            }
          }
        }
      }
    }
    
    if(Y3 != Y2&&Y2<H&&Y3>0){
      float rcb = float(X3-X2)/float(Y3-Y2);
      float rcvxb = (VX3-VX2)/float(Y3-Y2);
      float rcvyb = (VY3-VY2)/float(Y3-Y2);
   
      int sy = (Y2<0)?0:Y2;
      int ey = (Y3>H)?H:Y3;
      
      for(int y=sy;y<ey;y++){
        
     
        int xL = (int)(rcl*float(y-Y1)+X1);
        int xB = (int)(rcb*float(y-Y2)+X2);
        float vxL = rcvxl*float(y-Y1)+VX1;
        float vxB = rcvxb*float(y-Y2)+VX2;
        float vyL = rcvyl*float(y-Y1)+VY1;
        float vyB = rcvyb*float(y-Y2)+VY2;
        int x1,x2;
        float vx1,vx2,vy1,vy2;
        if(xL<=xB){x1=xL;x2=xB;vx1=vxL;vx2=vxB;vy1=vyL;vy2=vyB;}else{x2=xL;x1=xB;vx1=vxB;vx2=vxL;vy1=vyB;vy2=vyL;}
        if(x1<W&&x2>0){
          int sx = (x1<0)?0:x1;
          int ex = (x2>W)?W:x2;
          float rcxvx = float(vx2-vx1)/float(x2-x1);
          float rcxvy = float(vy2-vy1)/float(x2-x1);
          
          for(int x=sx;x<ex;x++){
            int vx = int(rcxvx*float(x-x1)+vx1);
            int vy = int(rcxvy*float(x-x1)+vy1);
            if(vx>=0&&vy>=0&&vx<sourceWidth+1&&vy<sourceHeight){
              drawFunction(x,y,data[vx+vy*sourceWidth],settings);
            }
          }
        } 
      }
    }
  }
  public:
  template <class T>
  static int render(CImageWarper *warper,void *sourceData,CGeoParams*sourceGeoParams,CGeoParams*destGeoParams,void *drawFunctionSettings,void (*drawFunction)(int ,int,T,void *drawFunctionSettings)){
    
    size_t orgDataSize = sourceGeoParams->dWidth*sourceGeoParams->dHeight;
    bool drawBil = false;
    double stride = 1;
    
    if(!drawBil){  
      while(orgDataSize/(stride*stride)>512*1024){
        stride*=2;
      }
    }
    
    //stride=100;
    //stride=32;
 //  stride = 8;
    //int stridei = stride;
//    CDBDebug("STRIDE = %d",stridei);
    int dataWidth = (float(sourceGeoParams->dWidth)/stride+0.0);
    int dataHeight = (float(sourceGeoParams->dHeight)/stride+0.0);
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
            
            //checkBBOX
            //CDBDebug("New BBOX:  %f %f %f %f",sourceGeoParams->dfBBOX[0],sourceGeoParams->dfBBOX[1],sourceGeoParams->dfBBOX[2],sourceGeoParams->dfBBOX[3]);
          }
      }
    }
//     
    double dfSourceExtW=(sourceGeoParams->dfBBOX[2]-sourceGeoParams->dfBBOX[0]);
    double dfSourceExtH=(sourceGeoParams->dfBBOX[1]-sourceGeoParams->dfBBOX[3]);
    double dfCellSizeX = sourceGeoParams->dfCellSizeX;
    double dfCellSizeY = sourceGeoParams->dfCellSizeY;
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

    
    double multiDestX = double(imageWidth+1)/dfDestExtW;
    double multiDestY = double(imageHeight+1)/dfDestExtH;
    
    size_t dataSize = (dataWidth+1) * (dataHeight+1);

    
    CT::string destinationCRS;
    warper->decodeCRS(&destinationCRS,&destGeoParams->CRS);
    if(destinationCRS.indexOf("longlat")>=0){
      destNeedsDegreeRadianConversion = true;
    }
    if(sourceGeoParams->CRS.indexOf("longlat")>=0){
      sourceNeedsDegreeRadianConversion = true;
    }
    
    
//     CDBDebug("SRC: %d %s",sourceNeedsDegreeRadianConversion,sourceGeoParams->CRS.c_str());
//     CDBDebug("DST: %d %s",destNeedsDegreeRadianConversion,destinationCRS.c_str());
    
    double *px = new double[dataSize];
    double *py = new double[dataSize];
    char *skip = new char[dataSize];
    
//     CDBDebug("(%f,%f)  %f %f",dfSourceOrigX,dfSourceOrigY,dfSourcedExtW,dfSourcedExtH);
    
    
    if(drawBil){
      for(int y=0;y<dataHeight+1;y++){
        for(int x=0;x<dataWidth+1;x++){
          size_t p = x+y*(dataWidth+1);
          px[p] =dfSourcedExtW*double(x)+dfSourceOrigX+dfCellSizeX/2.0;
          py[p] =dfSourcedExtH*double(y)+dfSourceOrigY+dfCellSizeY/2.0;
          skip[p] = false;
        }
      }
    }else{
      for(int y=0;y<dataHeight+1;y++){
        for(int x=0;x<dataWidth+1;x++){
          size_t p = x+y*(dataWidth+1);
          px[p] =dfSourcedExtW*double(x*stride)+dfSourceOrigX;
          py[p] =dfSourcedExtH*double(y*stride)+dfSourceOrigY;
          skip[p] = false;
        }
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
          //if(j<10){CDBDebug("%f %f",px[j],py[j]);}
          px[j]/=DEG_TO_RAD;
          py[j]/=DEG_TO_RAD;
        }
      }
    }

//     CDBDebug("--");
    for(size_t j=0;j<dataSize;j++){
      if(px[j]>-DBL_MAX&&px[j]<DBL_MAX){
        px[j]-=dfDestOrigX;
        py[j]-=dfDestOrigY;
        px[j]*=multiDestX;
        py[j]*=multiDestY;
        py[j]--;
      }else{
        skip[j]=true;        
      }
    }
   
    double avgDX = 0;
    double avgDY = 0;
    
    

    if(drawBil){

//       float *destField = new float[imageWidth*imageHeight];
//       T *data=(T*)dataSource->getDataObject(0)->cdfVariable->data;
//    
//       for(int y=0;y<dataHeight;y++){
//         for(int x=0;x<dataWidth;x++){
//       
//           
//           bool isNodata=false;
//           
//           
//           T val[4];
//           val[0] = data[x+(y)*(dataWidth)];
//           val[1] = data[x+1+(y)*(dataWidth)];
//           val[2] = data[x+(y+1)*(dataWidth)];
//           val[3] = data[x+1+(y+1)*(dataWidth)];
//           for(int j=0;j<4&&!isNodata;j++){
//             if(settings.hasNodataValue){if(val[j]==settings.nodataValue)isNodata=true;else if(!(val[j]==val[j]))isNodata=true;}
//             if(!isNodata)if(settings.legendValueRange)if(val[j]<settings.legendLowerRange||val[j]>settings.legendUpperRange)isNodata=true;
//           }
//           
//           
//           if(!isNodata){
//             int pcolorind[4];
//             for(int j=0;j<4;j++){
//               if(settings.legendLog!=0){
//                 if(val[j]>0){
//                   val[j]=(T)(log10(val[j])/settings.legendLogAsLog);
//                 }else val[j]=(T)(-settings.legendOffset);
//               }
//               pcolorind[j]=(int)(val[j]*settings.legendScale+settings.legendOffset);
//               
//               if(pcolorind[j]>=239)pcolorind[j]=239;else if(pcolorind[j]<=0)pcolorind[j]=0;
//             }
//             size_t p=x+y*(dataWidth+1);
//             if(skip[p]==false&&skip[p+1]==false&&skip[p+dataWidth+1]==false&&skip[p+dataWidth+2]==false){
//               double px1 = px[p];
//               double px2 = px[p+1];
//               double px3 = px[p+dataWidth+2];
//               double px4 = px[p+dataWidth+1];
// 
//               double py1 = py[p];
//               double py2 = py[p+1];
//               double py3 = py[p+dataWidth+2];
//               double py4 = py[p+dataWidth+1];
//               bool doDraw = true;
//               
//               if(x==0)avgDX = px1;
//               if(y==0)avgDY = py1;
//               
//               if(x==dataWidth-1){
//                 if(fabs(avgDX-px2)>fabs(dfCellSizeX)){
//                   doDraw = false;
//                 }
//               }
//               if(y==dataHeight-1){
//                 if(fabs(avgDY-py4)>fabs(dfCellSizeY)){
//                   doDraw = false;
//                 }
//               }
//               
//               if(doDraw){
//                 float values [4];
//           
//                 double mX = (px1+px2+px3+px4)/4;
//                 double mY = (py1+py2+py3+py4)/4;
//                 
//                 float middleValue = (pcolorind[0]+pcolorind[1]+pcolorind[2]+pcolorind[3])/4;
//                 
//                 int xP[3];
//                 int yP[3];
//                 xP[0] = px1;
//                 xP[1] = px2;
//                 xP[2] = mX;
// 
//                 yP[0] = py1;
//                 yP[1] = py2;
//                 yP[2] = mY;
//                 
//                 values[0] = pcolorind[0];
//                 values[1] = pcolorind[1];
//                 values[2] = middleValue;
//               
//             
//                 //drawTriangleBil(CDrawImage *drawImage, int *xP,int *yP, float *values);
//                 drawTriangleBil(drawImage,destField,xP,yP, values);
// 
//                 xP[0] = px3;
//                 yP[0] = py3;
//                 values[0]=pcolorind[3];
//                 drawTriangleBil(drawImage,destField, xP,yP, values);
// 
//                 xP[1]=px4;
//                 yP[1]=py4;
//                 values[1]=pcolorind[2];
//                 drawTriangleBil(drawImage,destField, xP,yP, values);
// 
//                 xP[0] = px1;
//                 yP[0] = py1;
//                 values[0]=pcolorind[0];
//                 drawTriangleBil(drawImage,destField, xP,yP, values);
//                
//               }
//             }
//           }
//         }
//       }
    }else{
   
    /*  
      for(int y=0;y<dataHeight*stride/2;y++){
        for(int x=0;x<dataWidth*stride;x++){
            bool onx = false;
            bool ony = false;
            if(y%2==0)ony = true;
            if((x+ony)%2==0)onx = true;
            size_t p=x+y*((dataWidth)*stride);
            T *data=(T*)dataSource->getDataObject(0)->cdfVariable->data;
            data[p]=0;
            if(onx)data[p]=2550;
            
        }
      }*/
    
      
      int sourceWidth = dataWidth;//sourceGeoParams->dWidth;
      int sourceHeight = dataHeight;//sourceGeoParams->dHeight;
      for(int y=0;y<dataHeight;y++){
        for(int x=0;x<dataWidth;x++){
                  
              
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
//               if(fabs(avgDY-py4)<fabs(py1-py4)/2){
//                 doDraw = false;
//               }
            }
            
            if(doDraw)
            {

//             if(
//               (px1>=0&&px1<imageWidth)||
//               (px2>=0&&px2<imageWidth)||
//               (px3>=0&&px3<imageWidth)||
//               (px4>=0&&px4<imageWidth)||
//               (py1>=0&&py1<imageHeight)||
//               (py2>=0&&py2<imageHeight)||
//               (py3>=0&&py3<imageHeight)||
//               (py4>=0&&py4<imageHeight))
            {
              
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
              


                size_t xs[3],ys[3];
                xs[0]=x*stride;
                xs[1]=x*stride+stride;
                xs[2]=x*stride+stride/2;
                ys[0]=y*stride;
                ys[1]=y*stride;
                ys[2]=y*stride+stride/2;
                drawTriangle<T>( xP,yP,sourceData,sourceWidth,sourceHeight,xs,ys,imageWidth,imageHeight,drawFunctionSettings,drawFunction);
                
                xP[0] = px3;
                yP[0] = py3;
                xs[0]=x*stride+stride;
                ys[0]=y*stride+stride;
                drawTriangle<T>( xP,yP,sourceData,sourceWidth,sourceHeight,xs,ys,imageWidth,imageHeight,drawFunctionSettings,drawFunction);

                xP[1]=px4;
                yP[1]=py4;
                xs[1]=x*stride;
                ys[1]=y*stride+stride;

                drawTriangle<T>( xP,yP,sourceData,sourceWidth,sourceHeight,xs,ys,imageWidth,imageHeight,drawFunctionSettings,drawFunction);

                xP[0] = px1;
                yP[0] = py1;
                xs[0]=x*stride;
                ys[0]=y*stride;
                drawTriangle<T>( xP,yP,sourceData,sourceWidth,sourceHeight,xs,ys,imageWidth,imageHeight,drawFunctionSettings,drawFunction);
              }
            }
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
