#ifndef GenericDataWarper_H
#define GenericDataWarper_H
#define ACCEPT_USE_OF_DEPRECATED_PROJ_API_H 1
#include <math.h>
#include <stdlib.h>
#include <proj_api.h>
#include <math.h>
#include <cfloat>
#include "CGeoParams.h"
#include "CImageWarper.h"
#include "CDebugger.h"

//#define GenericDataWarper_DEBUG

class GenericDataWarper{
 private:
  DEF_ERRORFUNCTION();
  template <class T>
  static int drawTriangle(int *xP,int *yP, T value,int destWidth,int destHeight,void *settings, void (*drawFunction)(int ,int,T,void *settings)){
    
    int W = destWidth;
    int H = destHeight;
    if(xP[0]<0&&xP[1]<0&&xP[2]<0)return 0;
    if(xP[0]>=W&&xP[1]>=W&&xP[2]>=W)return 0;
    if(yP[0]<0&&yP[1]<0&&yP[2]<0)return 0;
    if(yP[0]>=H&&yP[1]>=H&&yP[2]>=H)return 0;  
    
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
    
    /*
    //  1
    //   \
    //    2
    //   /
    //  3
    */
    //If top point is equal to bottom point of triangle, skip
    //If middle is equal to top and middle is equal to bottom, skip
    if((Y1 == Y3)||(Y2==Y1&&Y3==Y2)){
      int minx=X1;if(minx>X2)minx=X2;if(minx>X3)minx=X3;
      int maxx=X1;if(maxx<X2)maxx=X2;if(maxx<X3)maxx=X3;
      for(int x=minx;x<maxx+1;x++){
        drawFunction(x,yP[2],value,settings);
      }
      return 1;
    }
 
    

    float rcl = float(X3-X1)/float(Y3-Y1);
    if(Y2!=Y1&&Y1<H&&Y2>0){
      float rca = float(X2-X1)/float(Y2-Y1);
      int sy = (Y1<0)?0:Y1;
      int ey = (Y2>H)?H:Y2;
      for(int y=sy;y<=ey-1;y++){
        int xL = floor(rcl*float(y-Y1)+X1);
        int xA = floor(rca*float(y-Y1)+X1);
        int x1,x2;
        if(xL<xA){x1=xL;x2=xA;}else{x2=xL;x1=xA;}
        if(x1<W&&x2>0){
          int sx = (x1<0)?0:x1;
          int ex = (x2>W)?W:x2;
          for(int x=sx;x<=ex-1;x++){
            drawFunction(x,y,value,settings);
          }
        }
      }
    }
    
    if(Y3 != Y2&&Y2<H&&Y3>0){
      float rcb = float(X3-X2)/float(Y3-Y2);
      int sy = (Y2<0)?0:Y2;
      int ey = (Y3>H)?H:Y3;
      for(int y=sy;y<=ey-1;y++){
        int xL = floor(rcl*float(y-Y1)+X1);
        int xB = floor(rcb*float(y-Y2)+X2);
        int x1,x2;
        if(xL<=xB){x1=xL;x2=xB;}else{x2=xL;x1=xB;}
        if(x1<W&&x2>0){
          int sx = (x1<0)?0:x1;
          int ex = (x2>W)?W:x2;
          for(int x=sx;x<=ex-1;x++){
            drawFunction(x,y,value,settings);
          }
        } 
      }
    }
    return 0;
  }
   
   
  
   
  public:
    static int findPixelExtent(int *PXExtentBasedOnSource,CGeoParams*sourceGeoParams,CGeoParams*destGeoParams,CImageWarper*warper);
    
  template <class T>
  static int render(CImageWarper *warper,void *sourceData,CGeoParams*sourceGeoParams,CGeoParams*destGeoParams,void *drawFunctionSettings,void (*drawFunction)(int ,int,T,void *drawFunctionSettings)){
        
#ifdef GenericDataWarper_DEBUG
    CDBDebug("render");
#endif
    

    int imageHeight = destGeoParams->dHeight;
    int imageWidth = destGeoParams->dWidth;
  
    
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
    double dfSourceExtH=(sourceGeoParams->dfBBOX[3]-sourceGeoParams->dfBBOX[1]);
    double dfSourceW = double(sourceGeoParams->dWidth);
    double dfSourceH = double(sourceGeoParams->dHeight);
    
    double dfDestW = double(destGeoParams->dWidth);
    double dfDestH = double(destGeoParams->dHeight);

    double dfSourcedExtW=dfSourceExtW/(dfSourceW);
    double dfSourcedExtH=dfSourceExtH/(dfSourceH);
    double dfSourceOrigX=sourceGeoParams->dfBBOX[0];
    double dfSourceOrigY=sourceGeoParams->dfBBOX[1];
    
    double dfDestExtW = destGeoParams->dfBBOX[2]-destGeoParams->dfBBOX[0];
    double dfDestExtH = destGeoParams->dfBBOX[1]-destGeoParams->dfBBOX[3];
        double multiDestX = double(imageWidth)/dfDestExtW;
    
    double multiDestY = double(imageHeight)/dfDestExtH;
    
    double dfDestOrigX = destGeoParams->dfBBOX[0];//-0.5/multiDestX;;
    double dfDestOrigY = destGeoParams->dfBBOX[3];//+0.5/multiDestY;;;

    

  /*      
    CT::string destinationCRS;
    warper->decodeCRS(&destinationCRS,&destGeoParams->CRS);
    if(destinationCRS.indexOf("longlat")>=0){
      destNeedsDegreeRadianConversion = true;
    }
    if(sourceGeoParams->CRS.indexOf("longlat")>=0){
      sourceNeedsDegreeRadianConversion = true;
    }*/
    
    
    //Determine source BBOX of based on destination grid
#ifdef GenericDataWarper_DEBUG    
    CDBDebug("Creating px extent");
#endif    
    
    int sourceDataWidth = sourceGeoParams->dWidth;
    int sourceDataHeight = sourceGeoParams->dHeight;
    
    int PXExtentBasedOnSource[4];
    
    PXExtentBasedOnSource[0]=0;
    PXExtentBasedOnSource[1]=0;
    PXExtentBasedOnSource[2]=sourceDataWidth;
    PXExtentBasedOnSource[3]=sourceDataHeight;
    
    bool tryToFindExtend = false;
    
    
    if(tryToFindExtend){
      findPixelExtent(PXExtentBasedOnSource,sourceGeoParams,destGeoParams,warper);

    }
     
    if(PXExtentBasedOnSource[2]-PXExtentBasedOnSource[0]<=0)return 0;
    if(PXExtentBasedOnSource[3]-PXExtentBasedOnSource[1]<=0)return 0;

    

    
    
//     dfDestOrigX-=fabs(0.5/multiDestX);
//     dfDestOrigY-=fabs(0.5/multiDestY);
//     
//     double dfCellSizeX = dataSource->dfCellSizeX;
//     double dfCellSizeY = dataSource->dfCellSizeY;
    
    int dataWidth = PXExtentBasedOnSource[2]-PXExtentBasedOnSource[0];
    int dataHeight = PXExtentBasedOnSource[3]-PXExtentBasedOnSource[1];
    

        /* When geographical map projections are equal, just do a simple linear transformation */
    if(warper->isProjectionRequired()==false)
    {
#ifdef GenericDataWarper_DEBUG
      CDBDebug("warper->isProjectionRequired() = %d: Applying simple linear transformation",warper->isProjectionRequired());
#endif
      for(int y=PXExtentBasedOnSource[1];y<PXExtentBasedOnSource[3];y++){
        for(int x=PXExtentBasedOnSource[0];x<PXExtentBasedOnSource[2];x++){
          
          double dfx=x;
          double dfy=y;
          int sx1=floor((((((dfx)/(dfSourceW))*dfSourceExtW+dfSourceOrigX)-dfDestOrigX)/dfDestExtW)*dfDestW+0.5);
          int sy1=floor((((((dfy)/(dfSourceH))*dfSourceExtH+dfSourceOrigY)-dfDestOrigY)/dfDestExtH)*dfDestH+0.5);
          int sx2=floor((((((dfx+1)/(dfSourceW))*dfSourceExtW+dfSourceOrigX)-dfDestOrigX)/dfDestExtW)*dfDestW+0.5);
          int sy2=floor((((((dfy+1)/(dfSourceH))*dfSourceExtH+dfSourceOrigY)-dfDestOrigY)/dfDestExtH)*dfDestH+0.5);
          bool skip = false;
          int sxw=floor(fabs(sx2-sx1))+1;
          int syh=floor(fabs(sy2-sy1))+1;
          if(sx1<-sxw&&sx2<-sxw)skip=true;
          if(sy1<-syh&&sy2<-syh)skip=true;
          if(sx1>=destGeoParams->dWidth+sxw&&sx2>=destGeoParams->dWidth+sxw)skip=true;
          if(sy1>=destGeoParams->dHeight+syh&&sy2>=destGeoParams->dHeight+syh)skip=true;
//           
          
          
          if(!skip){
            T value = ((T*)sourceData)[x+(sourceGeoParams->dHeight-1-y)*sourceGeoParams->dWidth];
            int lx1,lx2,ly1,ly2;
            if(sx1>sx2){lx2=sx1;lx1=sx2;}else{lx2=sx2;lx1=sx1;}
            if(sy1>sy2){ly2=sy1;ly1=sy2;}else{ly2=sy2;ly1=sy1;}
            if(ly2==ly1)ly2++;
            if(lx2==lx1)lx2++;
            for(int sjy=ly1;sjy<ly2;sjy++){
              for(int sjx=lx1;sjx<lx2;sjx++){
                drawFunction(sjx,sjy,value,drawFunctionSettings);
              }
            }
          }
        }
      }
     // CDBDebug("warper->isProjectionRequired() = %d: Finished simple linear transformation",warper->isProjectionRequired());
      return 0;
    }
#ifdef GenericDataWarper_DEBUG
    CDBDebug("warp is required");
#endif

    
   

    size_t dataSize = (dataWidth+1) * (dataHeight+1);

    

    double *px = new double[dataSize];
    double *py = new double[dataSize];
    char *skip = new char[dataSize];
    
      
    for(int y=0;y<dataHeight+1;y++){
      for(int x=0;x<dataWidth+1;x++){
        size_t p = x+y*(dataWidth+1);
        px[p] =dfSourcedExtW*double(double(x)+PXExtentBasedOnSource[0])+dfSourceOrigX;//+dfSourcedExtW/2.0;
        py[p] =dfSourcedExtH*double(double(y)+PXExtentBasedOnSource[1])+dfSourceOrigY;//+dfSourcedExtH/2.0;
        skip[p] = false;
      }
    }
    

   
//     for(size_t j=0;j<dataSize;j++){
//       if(px[j]>0&&px[j]<1)skip[j]=true;        
//     }
    
    if(warper->isProjectionRequired()){
      if(warper->sourceNeedsDegreeRadianConversion){
        for(size_t j=0;j<dataSize;j++){
          px[j]*=DEG_TO_RAD;
          py[j]*=DEG_TO_RAD;
        }
      }
      
      if(pj_transform(warper->sourcepj,warper->destpj, dataSize,0,px,py,NULL)){
        CDBDebug("Unable to do pj_transform");
      }
      if(warper->destNeedsDegreeRadianConversion){
        for(size_t j=0;j<dataSize;j++){
          px[j]/=DEG_TO_RAD;
          py[j]/=DEG_TO_RAD;
        }
      }
    }
    
#ifdef GenericDataWarper_DEBUG
    CDBDebug("Reprojection done");
#endif

    for(size_t j=0;j<dataSize;j++){
      if(!(px[j]>-DBL_MAX&&px[j]<DBL_MAX))skip[j]=true;        
      
    }
    
    
    double avgDX = 0;
    double avgDY = 0;
/*    
    T blue = T(double(255.+0*256.+0*256.*256.+255.*256.*256.*256.));
    T yellow  = T(double(0.+255.*256.+255.*256.*256.+255.*256.*256.*256.));
    */
    
    for(int y=0;y<dataHeight;y=y+1){
      for(int x=0;x<dataWidth;x=x+1){
        size_t p=x+y*(dataWidth+1);
        if(skip[p]==false&&skip[p+1]==false&&skip[p+dataWidth+1]==false&&skip[p+dataWidth+2]==false)
        {
          bool doDraw = true;
          double px1 = px[p];
          double px2 = px[p+1];
          double px3 = px[p+dataWidth+2];
          double px4 = px[p+dataWidth+1];
            
          //CDBDebug("destGeoParams = %s",destGeoParams->CRS.c_str());
          if( CGeoParams::isLonLatProjection(&destGeoParams->CRS)==true||CGeoParams::isMercatorProjection(&destGeoParams->CRS)==true)
          {
            double lons[4];
            lons[0] = px1;
            lons[1] = px2;
            lons[2] = px3;
            lons[3] = px4;
            
            float lonMin,lonMax,lonMiddle=0;
            for(int j=0;j<4;j++){
              float lon = lons[j];
              if(j==0){lonMin=lon;lonMax=lon;}else{
                if(lon<lonMin)lonMin=lon;
                if(lon>lonMax)lonMax=lon;
              }
              lonMiddle+=lon;
              if(lon==INFINITY||lon==-INFINITY||!(lon==lon)){doDraw=false;break;}
            }
            lonMiddle/=4;
            double sphereWidth=360;
            if(CGeoParams::isMercatorProjection(&destGeoParams->CRS)){
              sphereWidth=40000000;
            }
            
            if(lonMax-lonMin>=sphereWidth*0.9)
            {
              if(lonMiddle>0){
                  for(int j=0;j<4;j++)if(lons[j]<lonMiddle)lons[j]+=sphereWidth;
              }else{
                for(int j=0;j<4;j++)if(lons[j]>lonMiddle)lons[j]-=sphereWidth;
              }
            }
            px1=lons[0];
            px2=lons[1]; 
            px3=lons[2]; 
            px4=lons[3]; 
          }
          
          px1-=dfDestOrigX;px1*=multiDestX;//px1+=0.5;
          px2-=dfDestOrigX;px2*=multiDestX;//px2+=0.5;
          px3-=dfDestOrigX;px3*=multiDestX;//px3+=0.5;
          px4-=dfDestOrigX;px4*=multiDestX;//px4+=0.5;
 
          

          double py1 = py[p];
          double py2 = py[p+1];
          double py3 = py[p+dataWidth+2];
          double py4 = py[p+dataWidth+1];
          
          py1-=dfDestOrigY;py1*=multiDestY;//py1+=0.5;
          py2-=dfDestOrigY;py2*=multiDestY;//py2+=0.5;
          py3-=dfDestOrigY;py3*=multiDestY;//py3+=0.5;
          py4-=dfDestOrigY;py4*=multiDestY;//py4+=0.5;

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
            int sourceGridX = x+PXExtentBasedOnSource[0];
            int sourceGridY = y+PXExtentBasedOnSource[1];
            T value = ((T*)sourceData)[sourceGridX+(sourceDataHeight-1-sourceGridY)*sourceDataWidth];

            
//             if(sourceGridX ==0||sourceGridX==sourceDataWidth-1||sourceGridY ==0||sourceGridY==sourceDataHeight-1){value=blue;}
//             if((sourceGridX ==10||sourceGridX==sourceDataWidth-10)&& sourceGridY >10 &&sourceGridY<sourceDataHeight-10){value=yellow;}
//             if((sourceGridY ==10||sourceGridY==sourceDataHeight-10)&& sourceGridX >10 &&sourceGridX<sourceDataWidth-10){value=yellow;}
//                      

            
            double mX = (px1+px2+px3+px4)/4;
            double mY = (py1+py2+py3+py4)/4;
/*            
            if(px2>px1)px2--;else if (px2<px1)px2++;
            if(px3>px1)px3--;else if (px3<px1)px3++;
            if(py3>py1)py3--;else if (py3<py1)py3++;
            if(py4>py1)py4--;else if (py4<py1)py4++;*/
            int dmX=floor(mX+0.5);
            int dmY=floor(mY+0.5);
            int dpx1=floor(px1+0.5);
            int dpy1=floor(py1+0.5);
            int dpx2=floor(px2+0.5);
            int dpy2=floor(py2+0.5);
            int dpx3=floor(px3+0.5);
            int dpy3=floor(py3+0.5);
            int dpx4=floor(px4+0.5);
            int dpy4=floor(py4+0.5);

           
            int xP[3];
            int yP[3];
            xP[0] = dpx1;
            yP[0] = dpy1;
            
            xP[1] = dpx2;
            yP[1] = dpy2;

            xP[2] = dmX;
            yP[2] = dmY;
            drawTriangle<T>( xP,yP,value,imageWidth,imageHeight,drawFunctionSettings,drawFunction); //bottom

            xP[0] = dpx3;
            yP[0] = dpy3;
            
            xP[1] = dpx2;
            yP[1] = dpy2;

            xP[2] = dmX;
            yP[2] = dmY;
            drawTriangle<T>( xP,yP,value,imageWidth,imageHeight,drawFunctionSettings,drawFunction);//right

            xP[0] = dpx3;
            yP[0] = dpy3;
            
            xP[1] = dpx4;
            yP[1] = dpy4;

            xP[2] = dmX;
            yP[2] = dmY;
            drawTriangle<T>( xP,yP,value,imageWidth,imageHeight,drawFunctionSettings,drawFunction);//top

            xP[0] = dpx1;
            yP[0] = dpy1;
            
            xP[1] = dpx4;
            yP[1] = dpy4;

            xP[2] = dmX;
            yP[2] = dmY;
            drawTriangle<T>( xP,yP,value,imageWidth,imageHeight,drawFunctionSettings,drawFunction);//left
//             for(int vy=-1;vy<2;vy++)for(int vx=-1;vx<2;vx++)
//             drawFunction(dmX+vx,dmY+vy,1,drawFunctionSettings);

          }
        }
      }
    }
//     for(int y=0;y<dataHeight;y=y+1){
//       for(int x=0;x<dataWidth;x=x+1){
//         size_t p=x+y*(dataWidth+1);
//         drawFunction(px[p],py[p],12,drawFunctionSettings);
//       }
//     }
//  
    delete[] px;
    delete[] py;
    delete[] skip;
#ifdef GenericDataWarper_DEBUG
    CDBDebug("render done");
#endif
    return 0;
  }
};
#endif
