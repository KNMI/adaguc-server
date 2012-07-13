#include "CImgWarpBilinear.h"
#include <gd.h>

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
  size_t numDestPixels = dPixelDestW*dPixelDestH;
  
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
  ValueClass *valObj = new ValueClass[sourceImage->dataObject.size()];
  for(size_t dNr=0;dNr<sourceImage->dataObject.size();dNr++){
    #ifdef CImgWarpBilinear_DEBUG
    CDBDebug("Allocating valObj[%d].fpValues: numDestPixels %d x %d",dNr,dPixelDestW,dPixelDestH);
    CDBDebug("Allocating valObj[%d].valueData: imageSize %d x %d",dNr,dImageWidth,dImageHeight);
    #endif
    valObj[dNr].fpValues = new float [numDestPixels];
    valObj[dNr].valueData = new float[dImageWidth*dImageHeight];
  }
  
  if(!sourceImage->dataObject[0]->hasNodataValue){
    /* When the datasource has no nodata value, assign -9999.0f */
    #ifdef CImgWarpBilinear_DEBUG
    CDBDebug("Source image has no NoDataValue, assigning -9999.0f");
    #endif
    sourceImage->dataObject[0]->dfNodataValue=-9999.0f;
    sourceImage->dataObject[0]->hasNodataValue=true;
  }else{
    /* Create a real nodata value instead of a nanf. */
    
    if(!(sourceImage->dataObject[0]->dfNodataValue==sourceImage->dataObject[0]->dfNodataValue)){
      #ifdef CImgWarpBilinear_DEBUG
      CDBDebug("Source image has no nodata value NaNf, changing this to -9999.0f");
      #endif
      sourceImage->dataObject[0]->dfNodataValue=-9999.0f;
    }
  }
  //Get the nodatavalue
  float fNodataValue = sourceImage->dataObject[0]->dfNodataValue ;
  
  //Reproject all the points 
  #ifdef CImgWarpBilinear_DEBUG
  CDBDebug("Nodata value = %f",fNodataValue);
  
  StopWatch_Stop("Start Reprojecting all the points");
  char temp[32];
  CDF::getCDFDataTypeName(temp,31,sourceImage->dataObject[0]->cdfVariable->type);
  CDBDebug("datatype: %s",temp);
  for(int j=0;j<4;j++){
    CDBDebug("dPixelExtent[%d]=%d",j,dPixelExtent[j]);
  }
  #endif
  
  for(int y=dPixelExtent[1];y<dPixelExtent[3];y++){
    for(int x=dPixelExtent[0];x<dPixelExtent[2];x++){
      size_t p = size_t((x-(dPixelExtent[0]))+((y-(dPixelExtent[1]))*dPixelDestW));
      double destX,destY;
      destX=dfSourcedExtW*double(x)+dfSourceOrigX;
      destY=dfSourcedExtH*double(y)+dfSourceOrigY;
      destX+=hCellSizeX;
      destY+=hCellSizeY;
      //CDBDebug("%f - %f",destX,destY);
      //TODO:
      //int status = 
      warper->reprojpoint_inv(destX,destY);
      
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
      for(size_t varNr=0;varNr<sourceImage->dataObject.size();varNr++){
        void *data=sourceImage->dataObject[varNr]->cdfVariable->data;
        float *fpValues=valObj[varNr].fpValues;
        
        switch(sourceImage->dataObject[varNr]->cdfVariable->type){
          case CDF_CHAR:
            fpValues[p]= ((signed char*)data)[x+y*sourceImage->dWidth];
            break;
          case CDF_BYTE:
            fpValues[p]= ((signed char*)data)[x+y*sourceImage->dWidth];
            break;
          case CDF_UBYTE:
            fpValues[p]= ((unsigned char*)data)[x+y*sourceImage->dWidth];
            break;
          case CDF_SHORT:
            fpValues[p]= ((signed short*)data)[x+y*sourceImage->dWidth];
            break;
          case CDF_USHORT:
            fpValues[p]= ((unsigned short*)data)[x+y*sourceImage->dWidth];
            break;
          case CDF_INT:
            fpValues[p]= ((signed int*)data)[x+y*sourceImage->dWidth];
            break;
          case CDF_UINT:
            fpValues[p]= ((unsigned int*)data)[x+y*sourceImage->dWidth];
            break;
          case CDF_FLOAT:
            fpValues[p]= ((float*)data)[x+y*sourceImage->dWidth];
            break;
          case CDF_DOUBLE:
            fpValues[p]= ((double*)data)[x+y*sourceImage->dWidth];
            break;
        }
        if(!(fpValues[p]==fpValues[p]))fpValues[p]=fNodataValue;
        //if(valObj[0].fpValues[p]==fNodataValue)valObj[0].fpValues[p]=0;
        //valObj[0].fpValues[p]+=10.0f;
      }
    }
  }
  #ifdef CImgWarpBilinear_DEBUG
  StopWatch_Stop("reprojection finished");
  #endif
  
  //return;
  //If there are 2 components, we have wind u and v.
  //Use Jacobians for rotating u and v
  //After calculations 
  bool gridRelative=true;
  
  if (sourceImage->dataObject.size()>1) {
    // Check standard_name/var_name for first vector component
    // if x_wind/grid_east_wind of y_wind/grid_northward_wind then gridRelative=true
    // if eastward_wind/northward_wind then gridRelative=false
    // default is gridRelative=true
    CT::string standard_name;
    standard_name=sourceImage->dataObject[0]->variableName;
    try {
      sourceImage->dataObject[0]->cdfVariable->getAttribute("standard_name")->getDataAsString(&standard_name);
    } catch (CDFError e) {}
    if (standard_name.equals("x_wind")||standard_name.equals("grid_eastward_wind")||
      standard_name.equals("y_wind")||standard_name.equals("grid_northward_wind")) {
      gridRelative=true;
    } else {
        gridRelative=false;
    }
    CDBDebug("Grid propery gridRelative=%d", gridRelative);
    
#ifdef CImgWarpBilinear_DEBUG
    StopWatch_Stop("u/v rotation started");
#endif
      double delta=0.01;
      double deltaLon;
      double deltaLat;
      float *uValues=valObj[0].fpValues;
      float *vValues=valObj[1].fpValues;
      double X=dfSourceOrigX;
      double XLon=X+dfSourcedExtW;
      double XLat=X;
      double Y=dfSourceOrigY;
      double YLon=Y;
      double YLat=Y+dfSourcedExtH;
      CDBDebug("Data raster: %f,%f with %f,%f lon: (%f,%f) lat: (%f,%f)\n", X, Y,dfSourcedExtW, dfSourcedExtH, XLon, YLon, XLat, YLat);
      warper->reprojModelToLatLon(X,Y);
      warper->reprojModelToLatLon(XLon, YLon);
      warper->reprojModelToLatLon(XLat, YLat);
      if (dfSourcedExtH<0){
        deltaLat=-delta;
      } else {
        deltaLat=+delta;
      }
      if (dfSourcedExtW>0) {
        deltaLon=delta;
      } else {
        deltaLon=-delta;
      }
      double signLon=(dfSourceExtW<0)?-1:1; // sign for adaptation of Jacobian to grid organisation
      double signLat=(dfSourceExtH<0)?-1:1; // sign for adaptation of Jacobian to grid organisation
      CDBDebug("deltaLon %f deltaLat %f signLon %f signLat %f", deltaLon, deltaLat, signLon, signLat);

      for(int y=dPixelExtent[1];y<dPixelExtent[3];y=y+1){
        for(int x=dPixelExtent[0];x<dPixelExtent[2];x=x+1){
          size_t p = size_t((x-(dPixelExtent[0]))+((y-(dPixelExtent[1]))*dPixelDestW));
          if ((uValues[p]!=fNodataValue)&&(vValues[p]!=fNodataValue)) {
            double modelX, modelY;
            if (x==dPixelExtent[2]-1) {
              modelX=dfSourcedExtW*double(x-1)+dfSourceOrigX;
            } else {
              modelX=dfSourcedExtW*double(x)+dfSourceOrigX;
            }  
            if (y==dPixelExtent[3]-1) {
              modelY=dfSourcedExtH*double(y-1)+dfSourceOrigY;
            } else {
              modelY=dfSourcedExtH*double(y)+dfSourceOrigY;
            }
            double modelXLat, modelYLat;
            double modelXLon, modelYLon;
            double VJaa,VJab,VJba,VJbb;
            if (gridRelative) {
              modelXLon=modelX+dfSourcedExtW;
              modelYLon=modelY;
              modelXLat=modelX;
              modelYLat=modelY+dfSourcedExtH;
//              if (y==0) { CDBDebug("modelXY[%d,%d] {%d} (%f, %f) (%f,%f) (%f,%f) =>", x, y, gridRelative, modelX, modelY, modelXLon, modelYLon, modelXLat, modelYLat);}
              warper->reprojpoint_inv(modelX,modelY); // model to vis proj.
              warper->reprojpoint_inv(modelXLon, modelYLon);
              warper->reprojpoint_inv(modelXLat, modelYLat);
//              if (y==0) { CDBDebug("modelXY[%d,%d] {%d} (%f, %f) (%f,%f) (%f,%f) =>", x, y, gridRelative, modelX, modelY, modelXLon, modelYLon, modelXLat, modelYLat);}
              double distLon=hypot(modelXLon-modelX, modelYLon-modelY);
              double distLat=hypot(modelXLat-modelX, modelYLat-modelY);
              //          if (y==0) { CDBDebug("(%f, %f) (%f,%f) (%f,%f): %f, %f {%d}\n", modelX,modelY, modelXLon, modelYLon, modelXLat, modelYLat, distLon, distLat, gridRelative); }
              
              VJaa=signLon*(modelXLon-modelX)/distLon;
              VJab=signLon*(modelXLat-modelX)/distLat;
              VJba=signLat*(modelYLon-modelY)/distLon;
              VJbb=signLat*(modelYLat-modelY)/distLat;
              double magnitude=hypot(uValues[p], vValues[p]);
              double uu;
              double vv;
              uu = VJaa*uValues[p]+VJab*vValues[p];
              vv = VJba*uValues[p]+VJbb*vValues[p];
              
              //           if (y==0) {CDBDebug("(%f, %f) ==> (%f,%f)", uValues[p], vValues[p], uu, vv);}
              double newMagnitude = hypot(uu, vv);
              uValues[p]=uu*magnitude/newMagnitude;
              vValues[p]=vv*magnitude/newMagnitude;
              //           if (y==0) {CDBDebug("==> (%f,%f)",uValues[p], vValues[p]);}  
              
            } else {
              warper->reprojModelToLatLon(modelX, modelY); // model to latlon proj.
              modelXLon=modelX+deltaLon; // latlons
              modelYLon=modelY;
              modelXLat=modelX;
              modelYLat=modelY+deltaLat;
              //            if (y==0) { CDBDebug("modelXY[%d,%d] {%d} {%f,%f} (%f,%f) (%f,%f) =>", x, y, gridRelative, modelX, modelY, modelXLon, modelYLon, modelXLat, modelYLat);}
              
              //            warper->reprojModelFromLatLon(modelX, modelY);
              /*            warper->reprojModelFromLatLon(modelXLat, modelYLat);
               *            warper->reprojModelFromLatLon(modelXLon, modelYLon);            
               *            warper->reprojpoint_inv(modelX, modelY);
               *            warper->reprojpoint_inv(modelXLon, modelYLon);
               *            warper->reprojpoint_inv(modelXLat, modelYLat);*/
              if (y==0) { CDBDebug("modelXY[%d,%d] {%d} {%f,%f} (%f,%f) (%f,%f) =>", x, y, gridRelative, modelX, modelY, modelXLon, modelYLon, modelXLat, modelYLat);}
              warper->reprojfromLatLon(modelX, modelY); // latlon to vis proj.
              warper->reprojfromLatLon(modelXLon, modelYLon);
              warper->reprojfromLatLon(modelXLat, modelYLat);

              if (y==0) { CDBDebug("modelXY[%d,%d] {%d} {%f,%f} (%f,%f) (%f,%f) =>", x, y, gridRelative, modelX, modelY, modelXLon, modelYLon, modelXLat, modelYLat);}
              
              double distLon=hypot(modelXLon-modelX, modelYLon-modelY);
              double distLat=hypot(modelXLat-modelX, modelYLat-modelY);
            
              VJaa=(modelXLon-modelX)/distLon;
              VJab=(modelXLat-modelX)/distLat;
              VJba=(modelYLon-modelY)/distLon;
              VJbb=(modelYLat-modelY)/distLat;

              if (y==0) { CDBDebug("jaco: %f,%f,%f,%f\n", VJaa, VJab, VJba, VJbb);}
              
              double magnitude=hypot(uValues[p], vValues[p]);
              double uu;
              double vv;
/*              uu = VJaa*uValues[p]+VJab*vValues[p];
              vv = VJba*uValues[p]+VJbb*vValues[p];*/
              vv = VJaa*uValues[p]+VJab*vValues[p];
              uu =-( VJba*uValues[p]+VJbb*vValues[p]);
              
              //           if (y==0) {CDBDebug("(%f, %f) ==> (%f,%f)", uValues[p], vValues[p], uu, vv);}
              double newMagnitude = hypot(uu, vv);
              uValues[p]=uu*magnitude/newMagnitude;
              vValues[p]=vv*magnitude/newMagnitude;
              //           if (y==0) {CDBDebug("==> (%f,%f)",uValues[p], vValues[p]);}    
//              uValues[p]=6;
//              vValues[p]=0;
    
            }
            //          drawImage->circle(modelX, modelY,3, 240);
            //          if (y==0) { CDBDebug("(%f, %f, %f,%f)\n", VJaa, VJab, VJba, VJbb);}
            
//                      uValues[p]=0;
//                    vValues[p]=6;
//             double u = uValues[p];
//             uValues[p]=vValues[p];
//             vValues[p]=uValues[p];

  }
}
}
#ifdef CImgWarpBilinear_DEBUG
StopWatch_Stop("u/v rotation finished");
#endif
}

for(size_t varNr=0;varNr<sourceImage->dataObject.size();varNr++){
  float *fpValues=valObj[varNr].fpValues;
  float *valueData=valObj[varNr].valueData;
  
  //Smooth the data (better for contour lines)
  #ifdef CImgWarpBilinear_DEBUG
  CDBDebug("start smoothing data with filter %d",smoothingFilter);
  #endif
  smoothData(fpValues,fNodataValue,smoothingFilter, dPixelDestW,dPixelDestH);
  
  //Draw the obtained raster by using triangle tesselation (eg gouraud shading)
  int xP[3],yP[3];
  size_t drawImageSize = dImageWidth*dImageHeight;
  //Set default nodata values
  for(size_t j=0;j<drawImageSize;j++)valueData[j]=fNodataValue;
               //start drawing triangles
  #if defined(CImgWarpBilinear_DEBUG) || defined(CImgWarpBilinear_TIME)
  StopWatch_Stop("Start triangle generation");
  #endif
  
  for(int y=dPixelExtent[1];y<dPixelExtent[3]-1;y=y+1){
    for(int x=dPixelExtent[0];x<dPixelExtent[2]-1;x=x+1){
      size_t p = size_t((x-(dPixelExtent[0]))+((y-(dPixelExtent[1]))*dPixelDestW));
      size_t p00=p;
      size_t p10=p+1;
      size_t p01=p+dPixelDestW;
      size_t p11=p+1+dPixelDestW;
      float cx=(dpDestX[p00]+dpDestX[p01]+dpDestX[p10]+dpDestX[p11])/4.0f;
      float cy=(dpDestY[p00]+dpDestY[p01]+dpDestY[p10]+dpDestY[p11])/4.0f;
      if(cx>-3000&&cx<6000&&
        cy>-3000&&cy<6000){
        float cv = (fpValues[p00]+fpValues[p01]+fpValues[p10]+fpValues[p11])/4.0f;
      //Fill in the poly
        //By Using triangles
        float vP[3];
        //if(1==1){
          if(fpValues[p00]!=fNodataValue&&fpValues[p10]!=fNodataValue&&
            fpValues[p01]!=fNodataValue&&fpValues[p11]!=fNodataValue)
          {
            //Fill upper side
            yP[0]=dpDestY[p00]; yP[1]=dpDestY[p10]; yP[2]=int(cy);
            xP[0]=dpDestX[p00]; xP[1]=dpDestX[p10]; xP[2]=int(cx);
            vP[0]=fpValues[p00];vP[1]=fpValues[p10];vP[2]=cv;
            fillTriangle(valueData, vP, dImageWidth,dImageHeight, xP,yP);
            //Fill right side
            yP[0]=dpDestY[p11];xP[0]=dpDestX[p11];vP[0]=fpValues[p11];
            fillTriangle(valueData, vP, dImageWidth,dImageHeight, xP,yP);
            //Fill lower side
            yP[1]=dpDestY[p01];xP[1]=dpDestX[p01];vP[1]=fpValues[p01];
            fillTriangle(valueData, vP, dImageWidth,dImageHeight, xP,yP);
            //fill left side
            yP[0]=dpDestY[p00];xP[0]=dpDestX[p00];vP[0]=fpValues[p00];
            fillTriangle(valueData, vP, dImageWidth,dImageHeight, xP,yP);
          }
        }
        /*}else{
         *        if(fpValues[p00]!=fNodataValue&&fpValues[p10]!=fNodataValue&&
         *        fpValues[p01]!=fNodataValue&&fpValues[p11]!=fNodataValue)
         *        {
         *                //Fill upper right side
         *        yP[0]=dpDestY[p00]; yP[1]=dpDestY[p10]; yP[2]=dpDestY[p11];
         *        xP[0]=dpDestX[p00]; xP[1]=dpDestX[p10]; xP[2]=dpDestX[p11];
         *        vP[0]=fpValues[p00];vP[1]=fpValues[p10];vP[2]=fpValues[p11];
         *        fillTriangle(valueData, vP, dImageWidth,dImageHeight, xP,yP);
         *              //Fill lower left side
         *        yP[1]=dpDestY[p01];xP[1]=dpDestX[p01];vP[1]=fpValues[p01];
         *        fillTriangle(valueData, vP, dImageWidth,dImageHeight, xP,yP);
        }
        }*/
        }
  }
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
//Wind VECTOR
std::vector<CalculatedWindVector> windVectors; //holds windVectors after calculation to draw them on top
bool convertToKnots=false; //default is false
if((enableVector||enableBarb)){
  if(sourceImage->dataObject.size()==2){
    int firstXPos=0;
    int firstYPos=0;
    
    double tx=((-drawImage->Geo->dfBBOX[0])/(drawImage->Geo->dfBBOX[2]-drawImage->Geo->dfBBOX[0]))*double(dImageWidth);
    double ty=dImageHeight-((-drawImage->Geo->dfBBOX[1])/(drawImage->Geo->dfBBOX[3]-drawImage->Geo->dfBBOX[1]))*double(dImageHeight);
    
    //Are u/v values in m/s? (Should we convert for wind barb drawing?)
    //Depends on value units
    //Derive convertToKnots from units
    CT::string units="m/s";
    units=sourceImage->dataObject[0]->units;
    CDBDebug("units = %s", units.c_str());
    if (!(units.equals("kts")||units.equals("knots"))) convertToKnots=true;
         
         //Number of pixels between the vectors:
    int vectorDensityPy=50;//22;
    int vectorDensityPx=50;//22;
    firstXPos=int(tx)%vectorDensityPy;
    firstYPos=int(ty)%vectorDensityPx;
    double u,v;
    
    double direction;
    double strength;
    //double pi=3.141592654;
    int stepx=vectorDensityPx; //Raster stride at barb distances
    int stepy=vectorDensityPy;
    // If contouring, drawMap or shading is wanted, step through all destination raster points
    if(enableContour||enableShade||drawMap){
      stepy=1;
      stepx=1;
    }
    //Loops through complete destination image in screen coord.
    for(int y=firstYPos-vectorDensityPx;y<dImageHeight;y=y+stepy){
      for(int x=firstXPos-vectorDensityPy;x<dImageWidth;x=x+stepx){
        //        CDBDebug("pos: %d,%d", x, y);
        if(x>=0&&y>=0){
          size_t p = size_t(x+y*dImageWidth); //pointer in dest. image
          //          CDBDebug("pos: %d,%d ==> p", x, y, p);
          
          //valObj[0].fpValues;
          //drawImage->setPixel(dpDestX[p],dpDestY[p],240);
          u=valObj[0].valueData[p];
          v=valObj[1].valueData[p];
          
          if((u!=fNodataValue)&&(v!=fNodataValue)){
            direction=atan2(v,u);
            strength=sqrt(u*u+v*v);
            valObj[0].valueData[p]=strength;
            
            if(drawMap==true){       
              setValuePixel(sourceImage,drawImage,x,y,strength);
            }
            if (!drawGridVectors) {
              if((int(x-firstXPos)%vectorDensityPy==0&&(y-firstYPos)%vectorDensityPx==0)||(enableContour==false&&enableShade==false)){
                strength=(strength)*1.0;
                
                //Calculate coordinates from requested coordinate system
                double projectedCoordX=((double(x)/double(dImageWidth))*(drawImage->Geo->dfBBOX[2]-drawImage->Geo->dfBBOX[0]))+drawImage->Geo->dfBBOX[0];;
                double projectedCoordY=((double(dImageHeight-y)/double(dImageHeight))*(drawImage->Geo->dfBBOX[3]-drawImage->Geo->dfBBOX[1]))+drawImage->Geo->dfBBOX[1];;
                
                //CDBDebug("W= d H=%d",dImageWidth,dImageHeight);
                //CDBDebug("BBOX= %f,%f,%f,%f",drawImage->Geo->dfBBOX[0],drawImage->Geo->dfBBOX[1],drawImage->Geo->dfBBOX[2],drawImage->Geo->dfBBOX[3]);
                
                
                warper->reprojToLatLon(projectedCoordX,projectedCoordY);
                
                bool flip=projectedCoordY<0; //Remember if we have to flip barb dir for southern hemisphere
                flip=false;
                CalculatedWindVector wv(x, y, direction, strength,convertToKnots,flip);
                //                  drawImage->circle(x,y,2, 240);
                windVectors.push_back(wv);
                //                  if (enableVector) drawImage->drawVector(x,y,direction,strength,240);
                //                  if (enableBarb) drawImage->drawBarb(x,y,direction,strength,240,convertToKnots,flip);
              }
            }
          }else valObj[0].valueData[p]=sourceImage->dataObject[0]->dfNodataValue; //Set speeed to nodata if u OR v == no data
        }
      }
    }
  }
  
} 

if(((enableVector||enableBarb)&&drawGridVectors)){
  int wantedSpacing=30;
  int distPoint=int(hypot(dpDestX[1]-dpDestX[0], dpDestY[1]-dpDestY[0]));
  
  int stepx=int(float(wantedSpacing)/distPoint+0.5);
  if (stepx<1) stepx=1;
         int stepy=stepx;
  if(sourceImage->dataObject.size()==2){
    bool doprint=true;
    for(int y=dPixelExtent[1];y<dPixelExtent[3];y=y+stepy){ //TODO Refactor to GridExtent
         for(int x=dPixelExtent[0];x<dPixelExtent[2];x=x+stepx){
           size_t p = size_t((x-(dPixelExtent[0]))+((y-(dPixelExtent[1]))*dPixelDestW));
           //double destX,destY;
           // size_t pSrcData = size_t(x+y*dfSourceW);//TODO Refactor to GridWidth
           
           //Skip rest if x,y outside drawArea
           if ((dpDestX[p]>=0)&&(dpDestX[p]<dImageWidth)&&(dpDestY[p]>=0)&&(dpDestY[p]<dImageHeight)){
             double direction;
             //double pi=3.141592654;
             double strength;
             double u=valObj[0].fpValues[p];
             double v=valObj[1].fpValues[p];
             //             drawImage->circle(dpDestX[p], dpDestY[p]pi, 2, 246);
             
             if((u!=fNodataValue)&&(v!=fNodataValue)) {
               direction=atan2(v,u);
               
               strength=sqrt(u*u+v*v);
               //               valObj[0].valueData[p]=strength;
               
               //Calculate coordinates from requested coordinate system to determine barb flip
               double modelX=dfSourcedExtW*double(x)+dfSourceOrigX;
               double modelY=dfSourcedExtH*double(y)+dfSourceOrigY;
               //double XX=modelX; double YY=modelY;
               warper->reprojToLatLon(modelX,modelY);
               
               bool flip=modelY<0; //Remember if we have to flip barb dir for southern hemisphere
               flip=false;
               //                if ((fabs(projectedCoordX)<0.8)&&(fabs(projectedCoordY-52)<1)) {
//  CDBDebug("[%d,%d] p=%d {w=%d} (%f,%f) MLL:{%f,%f} LL:{%f,%f} [%d,%d]", x, y, p, dPixelDestW, u, v, modelX, modelY, XX, YY, dpDestX[p], dpDestY[p]);
  //                }  
  //Project x,y source grid indexes topi vis. space
//  double modelX=dfSourcedExtW*double(x)+dfSourceOrigX;
//  double modelY=dfSourcedExtH*double(y)+dfSourceOrigY;
//  warper->reprojpoint_inv(modelX,modelY); // model to vis proj.
  
  //               if (y==0) {CDBDebug("PLOT %d,%d at %d,%d {%f} (%f,%f)", x, y, dpDestX[p], dpDestY[p], p, direction, strength);}
  CalculatedWindVector wv(dpDestX[p], dpDestY[p], direction, strength,convertToKnots,flip);
  windVectors.push_back(wv);
  }
             }
           }
           doprint=false;
         }
    }
  }
  //Make Contour if desired
  //drawContour(valueData,fNodataValue,500,5000,drawImage);
  if(enableContour||enableShade){
    drawContour(valueData,fNodataValue,shadeInterval,contourSmallInterval,contourBigInterval,
                sourceImage,drawImage,enableContour,enableShade,enableContour);
  }
  
  /*
   *    for(int y=dPixelExtent[1];y<dPixelExtent[3];y++){
   *    for(int x=dPixelExtent[0];x<dPixelExtent[2];x++){
   *    size_t p = size_t((x-(dPixelExtent[0]))+((y-(dPixelExtent[1]))*dPixelDestW));
   *    for(int y1=-2;y1<4;y1++){
   *    for(int x1=-2;x1<4;x1++){
   *    drawImage->setPixel(dpDestX[p]+x1,dpDestY[p]+y1,248);
}
}

for(int y1=-1;y1<3;y1++){
  for(int x1=-1;x1<3;x1++){
  setValuePixel(sourceImage,drawImage,dpDestX[p]+x1,dpDestY[p]+y1,fpValues[p]);
}
}
}
}*/
  /*if(drawMap==true)
   *    {
   *      for(int y=0;y<dImageHeight;y++){
   *        for(int x=0;x<dImageWidth;x++){
   *          setValuePixel(sourceImage,drawImage,x,y,valObj[0].valueData[x+y*dImageWidth]);
}
}
}*/
  
  //                  if (enableVector) drawImage->drawVector(x,y,direction,strength,240);
  //                  if (enableBarb) drawImage->drawBarb(x,y,direction,strength,240,convertToKnots,flip);
  //                    
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



void CImgWarpBilinear::drawContour(float *valueData,float fNodataValue,float interval,float smallContInterval,float bigContInterval,CDataSource *dataSource,CDrawImage *drawImage,bool drawLine, bool drawShade, bool drawText){
  float val[4];
  //drawText=false;
  
  
  //When using min/max stretching, the classes need to be extended according to its shade itnerval
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
      dataSource->legendScale=ls;
      dataSource->legendOffset=lo;
    }
  }
  #ifdef CImgWarpBilinear_DEBUG 
  CDBDebug("scale=%f offset=%f",dataSource->legendScale,dataSource->legendOffset);
  #endif  
  
  int col1=244;
  int col2=243;
  int drawnTextY=0;
  int drawnTextX=0;
  float ival = interval;
  //   float ivalLine = interval;
  //float idval=int(ival+0.5);
  //  if(idval<1)idval=1;
  //TODO
  
  char szTemp[8192];
  int dImageWidth=drawImage->Geo->dWidth+1;
  int dImageHeight=drawImage->Geo->dHeight+1;
  
  size_t imageSize=(dImageHeight+0)*(dImageWidth+1);
  #ifdef CImgWarpBilinear_DEBUG
  CDBDebug("imagesize = %d",(int)imageSize);
  #endif
  unsigned int *distance = NULL;
  
  distance = new unsigned int[imageSize];
  memset (distance,0,imageSize*sizeof(unsigned int));
  
  #ifdef CImgWarpBilinear_DEBUG
  CDBDebug("distance field cleared");
  #endif
  
  
  /* for(int y=0;y<dImageHeight;y++){
   *    for(int x=0;x<dImageWidth+1;x++){
   *      size_t p1 = size_t(x+y*dImageWidth);
   *      //valueData[p1]=100.0f;
}
}*/
  /*FILE * pFile;
   *  pFile = fopen ( "/nobackup/users/plieger/eclipseworkspace/cpp/adagucserver-2011-02-04/adagucserverEC/src/test.bin" , "wb" );
   *  CT::string ascii;
   *  ascii.printconcat("nrcols %d\n",dImageWidth);
   *  ascii.printconcat("nrrows %d\n",dImageHeight);
   *  ascii.printconcat("xllcorner 0\n");
   *  ascii.printconcat("yllcorner 0\n");
   *  ascii.printconcat("cellsize 1\n");
   *  
   *  for(int y=0;y<dImageHeight;y++){
   *    for(int x=0;x<dImageWidth+1;x++){
   *      ascii.printconcat("%0.2f ",valueData[x+y*dImageWidth]);
}
ascii.concat("\n");
}
fwrite (ascii.c_str() , ascii.length() , sizeof(char) , pFile );
  fclose (pFile);*/
  /*float allowedDifference=(1.0f/(1000/ival));
   *  for(int y=0;y<dImageHeight;y++){
   *    for(int x=0;x<dImageWidth+1;x++){
   *      float a=int(valueData[x+y*dImageWidth]/allowedDifference);
   *      a*=allowedDifference;
   *      valueData[x+y*dImageWidth]=a;
}
}*/
  
  
  float allowedDifference=ival/100000;
  if(1==1){
    #ifdef CImgWarpBilinear_TIME
    StopWatch_Stop("substracting ival/100");
    #endif
    //TODO "pleister" om contourlijnen goed te krijgen.
    float substractVal=ival/100;
    CDBDebug("*** substractVal=%f ival=%f",substractVal,ival);
    //substractVal=0.01;
    for(int y=0;y<dImageHeight;y++){
      for(int x=0;x<dImageWidth;x++){
        //float a=int(valueData[x+y*dImageWidth]/allowedDifference);
        //a*=allowedDifference;
        valueData[x+y*dImageWidth]-=substractVal;
        //valueData[x+y*dImageWidth]-=0.1;
      }
    }
    fNodataValue-=substractVal;
    #ifdef CImgWarpBilinear_TIME
    StopWatch_Stop("finished substracting ival/100");
    #endif
  }
  #ifdef CImgWarpBilinear_DEBUG
  CDBDebug("start shade/contour with nodatavalue %f",fNodataValue);
  //size_t p1 = size_t(0+0*dImageWidth);
  //CDBDebug("Field value at (0,0) = %f",valueData[p1]);
  #endif
  
  
  for(int y=0;y<dImageHeight-1;y++){
    for(int x=0;x<dImageWidth-1;x++){
      size_t p1 = size_t(x+y*dImageWidth);
      val[0] = valueData[p1];
      val[1] = valueData[p1+1];
      val[2] = valueData[p1+dImageWidth];
      val[3] = valueData[p1+dImageWidth+1];
      if(val[0]!=fNodataValue&&val[1]!=fNodataValue&&val[2]!=fNodataValue&&val[3]!=fNodataValue&&
        val[0]==val[0]&&val[1]==val[1]&&val[2]==val[2]&&val[3]==val[3]
      ){
        float min,max;
        min=val[0];max=val[0];
        for(int j=1;j<3;j++){
          if(val[j]<min)min=val[j];
                   if(val[j]>max)max=val[j];
        }
        float difference=max-min;
        
        min=convertValueToClass(min,ival);
        max=convertValueToClass(max,ival)+ival;
        
        if(drawShade){
          //setValuePixel(dataSource,drawImage,x,y,(min+max)/2);
          
          setValuePixel(dataSource,drawImage,x,y,min);
          
          //setValuePixel(dataSource,drawImage,x,y,val[0]);
        }
        if((max-min)/ival<3&&(max-min)/ival>1&&difference>allowedDifference){
          for(double c=min;c<max;c=c+ival){
            if((val[0]>=c&&val[1]<c)||(val[0]>c&&val[1]<=c)||(val[0]<c&&val[1]>=c)||(val[0]<=c&&val[1]>c)||
              (val[0]>c&&val[2]<=c)||(val[0]>=c&&val[2]<c)||(val[0]<=c&&val[2]>c)||(val[0]<c&&val[2]>=c))
            {
              float c = valueData[x+y*dImageWidth];
              int bigC;int smallC;int modulo;
              modulo=int((bigContInterval/ival)+0.5f);
              if(modulo>0){bigC=(int((fabs(c)/ival)+0.5f))%modulo;}else bigC=1;
                   modulo=int((smallContInterval/ival)+0.5f);
                   if(modulo>0){smallC=(int((fabs(c)/ival)+0.5f))%modulo;}else smallC=1;
                   if(bigC==0||smallC==0){
                     distance[x+y*dImageWidth]=1;
                   }
            }
          }
        }
      }
      if(drawnTextX>0)drawnTextX--;
    }
    if(drawnTextY>0)drawnTextY--;
  }
  #ifdef CImgWarpBilinear_DEBUG
  CDBDebug("finished shade/contour");
  #endif
  
  #ifdef CImgWarpBilinear_DEBUG
  CDBDebug("Starting to draw lines and text");
  #endif
  
  //Start tr
  //  int xdir[]={0,0,-1,-1,-1, 0, 1, 1, 1,0,-1,-2,-2,-2,-2,-2,-1, 0, 1, 2, 2, 2, 2, 2, 1, 0};
  //  int ydir[]={0,1, 1, 0,-1,-1,-1, 0, 1,2, 2, 2, 1, 0,-1,-2,-2,-2,-2,-2,-1, 0, 1, 2, 2, 0};
  
  int xdir[]={0,-1, 0, 1,-1,-1, 1, 1,0,-1,-2,-2,-2,-2,-2,-1, 0, 1, 2, 2, 2, 2, 2, 1, 0};
  int ydir[]={1,0 ,-1, 0, 1,-1,-1, 1,2, 2, 2, 1, 0,-1,-2,-2,-2,-2,-2,-1, 0, 1, 2, 2, 0};
  
  //  int xdir[]={0,0,-1,-1,-1, 0, 1, 1, 1};
  //int ydir[]={0,1, 1, 0,-1,-1,-1, 0, 1};
  ;
  
  //A line can be traversed in two directions:
  int secondDirX,secondDirY;
  bool secondDirAvailable=false;
  
  
  int linePointDistance=6;
  
  for(int y=0;y<dImageHeight;y++){
    for(int x=0;x<dImageWidth;x++){        
      if(secondDirAvailable){
        int j;
        secondDirAvailable=false;
        x=secondDirX;
        y=secondDirY;
        for(j=0;j<25;j++){
          if(x+xdir[j]>=1&&x+xdir[j]<dImageWidth-1&&
            y+ydir[j]>=1&&y+ydir[j]<dImageHeight-1){
            if(distance[x+xdir[j]+(y+ydir[j])*dImageWidth]==1){
              x=x+xdir[j];
              y=y+ydir[j];
              //{CDBDebug("FOUND SECOND DIR");};
              //drawImage->line(x,y,x+4,y+4,2,240);
              break;
              }
            }
            }
        }
        
        int maxneighbordist=(int)distance[x+y*dImageWidth];
        if(maxneighbordist==1){
          distance[x+y*dImageWidth]=2;
          maxneighbordist++;
          int j,tx=x,ty=y;
          int startX=x,startY=y;
          
          if(secondDirAvailable==false){
            secondDirAvailable=true;
            secondDirX=x;secondDirY=y;
          }
          
          //drawImage->line(x,y,x+4,y+4,2,244);
          int col=col2;
          float w;
          
          int lastXdir=-10,lastYdir;
          int distanceFromStart=0;
          int drawnText=0;
          int busyDrawingText=0;
          int drawTextStartX=0;
          int drawTextStartY=0;
          int needToDrawText=0;
          int textAreaLeft=25;//int(float(dImageWidth)*(1.0f/10.0f));
          int textAreaRight=dImageWidth-25;//int(float(dImageWidth)*(9.0f/10.0f));
          do{
            for(j=0;j<24;j++){
              if(x+xdir[j]>=1&&x+xdir[j]<dImageWidth-1&&
                y+ydir[j]>=1&&y+ydir[j]<dImageHeight-1){
                int d=distance[x+xdir[j]+(y+ydir[j])*dImageWidth];
              //(abs(startY-y)<2)
                
                if(drawText){
                  if(d%500==499){
                    drawnText=0;needToDrawText=0;
                  }
                  if(needToDrawText==0){
                    if((d%100==29)&&x>textAreaLeft&&x<textAreaRight&&y>24&&y<dImageHeight-25&&drawnText==0){
                      needToDrawText=1;
                    }
                  }else{
                    if(drawnText==0){
                      if(lastXdir!=-10&&lastYdir==0){
                        if(abs(startY-y)==0&&d>25){
                          //drawImage->line(x,y,x+4,y+4,2,240);
                          drawTextStartX=x;
                          drawTextStartY=y;
                          drawnText=1;
                          needToDrawText=0;
                          float c = valueData[x+y*dImageWidth];
                          //Calculate which value we need to print for the contourBigInterval
                          //Add the half of the interval in order to make sure that we are always in the upper bin.
                          float m=convertValueToClass(c+ival/2,ival);
                          
                          int textRounding=0;
                          if(ival!=0){
                            float fracPart=ival-int(ival);
                            textRounding=-int(log10(fracPart)-0.9999999f);
                          }
                          float valToPrint=m;//(m*textScaleFactor+textOffsetFactor);
                          if(textRounding<=0)sprintf(szTemp,"%2.0f",valToPrint);
                                                if(textRounding==1)sprintf(szTemp,"%2.1f",valToPrint);
                                                   if(textRounding==2)sprintf(szTemp,"%2.2f",valToPrint);
                                                      if(textRounding==3)sprintf(szTemp,"%2.3f",valToPrint);
                                                         if(textRounding==4)sprintf(szTemp,"%2.4f",valToPrint);
                                                         if(textRounding==5)sprintf(szTemp,"%2.5f",valToPrint);
                                                         if(textRounding>6)sprintf(szTemp,"%f",valToPrint);
                                                         /*if(float(int(interval))==interval&&int(m)==m){
                                                          *                          snprintf(szTemp,10,"%d",int(m*textScaleFactor+textOffsetFactor));
                                                         }else{
                                                           //floatToString(szTemp,255,m*textScaleFactor+textOffsetFactor);
                                                           snprintf(szTemp,10,"%6.1f",(m*textScaleFactor+textOffsetFactor));
                                                         }*/
                                                         busyDrawingText=int(float(strlen(szTemp))*1.3)+2;
                        }
                      }
                    }
                  }
                }
                if(d==1){
                  lastXdir=xdir[j];
                  lastYdir=ydir[j];
                  x=x+lastXdir;
                  y=y+lastYdir;
                  //drawImage->setPixelIndexed(x,y,maxneighbordist%255);
                  maxneighbordist++;
                  distance[x+y*dImageWidth]=maxneighbordist;
                  
                  distanceFromStart = maxneighbordist-3;
                  
                  if(distanceFromStart%linePointDistance==0){
                    
                    //Contour type calculations
                    float c = valueData[x+y*dImageWidth];
                    int bigC;int smallC;int modulo;
                    modulo=int((bigContInterval/ival)+0.5f);
                    if(modulo>0){bigC=(int((fabs(c)/ival)+0.5f))%modulo;}else bigC=1;
           modulo=int((smallContInterval/ival)+0.5f);
           if(modulo>0){smallC=(int((fabs(c)/ival)+0.5f))%modulo;}else smallC=1;
           
           
           col1=241;
           col2=241;
           col=col2;
           
           w=0.4;
           if(smallC==0){w=0.7;col=col2;}
           if(bigC==0){w=1.4;col=col1;}
           //w=0.8;
           
           if(drawLine){//&&(distanceFromStart%(linePointDistance*3)>linePointDistance*1)){
             
             if(busyDrawingText>0){
               busyDrawingText--;
               //busyDrawingText=0;
               if(busyDrawingText==0){
                 
                 //snprintf(szTemp,5,"%f",m);
                 float l=strlen(szTemp);
                 double angle;
                 if(y-drawTextStartY!=0){
                   angle=atan((double(y-drawTextStartY))/(double(drawTextStartX-x)));
                 }else angle=0;
                            float cx=(drawTextStartX+x)/2;
                 float cy=(drawTextStartY+y)/2;
                 float tx=cx;
                 float ty=cy;
                 //drawImage->rectangle(tx-1,ty-1,tx+1,ty+1, 241,248);
                 float ca=cos(angle);
                 float sa=sin(angle);
                 
                 float offX=((ca*l*2.3)+(sa*-2.3));
                 float offY=(-(sa*l*3.0)+(ca*-2.3));
                 tx-=offX;
                 ty-=offY;
                 //drawImage->drawText( int(tx), int(ty), angle,8,szTemp,agg::rgba8(255,255,255,240));
                 drawImage->drawText( int(tx)+1, int(ty)+1, angle,szTemp,240);
                 //Draw a line under the text
                 /*if(l<3&&1==2){
                  *                          float a=3.1415f/4;
                  *                          float o=10;
                  *                          drawImage->line(int(cx+cos(angle-a)*o),int(cy-sin(angle-a)*o),
                  *                                      int(cx+cos(angle-(a+(3.1415f/2)))*o),int(cy-sin(angle-(a+(3.1415f/2)))*o),0.5,240);
               }*/
                 
               }
             }else{
               drawImage->line(startX,startY+1,x,y+1,w,col);
               //drawImage->moveTo(startX,startY);//,x+1,y+1,w,col);
               //drawImage->lineTo(x,y);//,x+1,y+1,w,col);
               //drawImage->endLine(col1);//,x+1,y+1,w,col);
               //drawImage->setPixelIndexed(startX,startY,241);
             }
           }else lastXdir=-10;
                            startX=x;startY=y;
                  }
                  j=0;
           //break;
                }
                }
            }
          }while(j<24);
          if(drawLine){
            if(lastXdir!=-10&&distanceFromStart > 1){
              drawImage->line(x+lastXdir*2,y+lastYdir+1,startX-lastXdir*2,startY-lastYdir+1,w,col);
              
            }
            
          }
          x=tx;y=ty;
          //return;
        }
      }
      
    }
    
    #ifdef CImgWarpBilinear_DEBUG
    CDBDebug("Deleting distance[]");
    #endif
    
    delete[] distance;
    #ifdef CImgWarpBilinear_DEBUG
    CDBDebug("Finisched drawing lines and text");
    #endif
    
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
