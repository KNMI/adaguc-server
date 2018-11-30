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

#include "CDrawImage.h"
#include "CXMLParser.h"
const char *CDrawImage::className="CDrawImage";

float convertValueToClass(float val,float interval){
  float f=int(val/interval);
  if(val<0)f-=1;
  return f*interval;
}

CDrawImage::CDrawImage(){
  //CDBDebug("[CONS] CDrawImage");
  dImageCreated=0;
  dPaletteCreated=0;
  currentLegend = NULL;
  _bEnableTrueColor=false;
  _bEnableTransparency=false;
  _bEnableTrueColor=false;
  dNumImages = 0;
  Geo= new CGeoParams(); 

  cairo=NULL;
  rField = NULL;
  gField=NULL;
  bField = NULL;
  numField = NULL;
  trueColorAVG_RGBA=false;
  
  TTFFontLocation = "/usr/X11R6/lib/X11/fonts/truetype/verdana.ttf";
  const char *fontLoc=getenv("ADAGUC_FONT");
  if(fontLoc!=NULL){
    TTFFontLocation = strdup(fontLoc); 
  }
  TTFFontSize = 9;
  
  BGColorR=0;
  BGColorG=0;
  BGColorB=0;
  backgroundAlpha=255;
  
  numImagesAdded = 0;
  currentGraphicsRenderer = CDRAWIMAGERENDERER_GD;
  //CDBDebug("TTFFontLocation = %s",TTFFontLocation);
}
void CDrawImage::destroyImage(){
  //CDBDebug("[destroy] CDrawImage");

  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_GD){
    if(dPaletteCreated==1){
      for(int j=0;j<256;j++)if(_colors[j]!=-1)gdImageColorDeallocate(image,_colors[j]);
    }
    dPaletteCreated=0;
  }
  if(dImageCreated==1){
    if(currentGraphicsRenderer==CDRAWIMAGERENDERER_GD){
      gdImageDestroy(image);
    };  
  }
  dImageCreated=0;
  
  for(size_t j=0;j<legends.size();j++){
    delete legends[j];
  }
  legends.clear();
 
  delete cairo; cairo=NULL;
  if(rField!=NULL){
    delete[] rField; rField = NULL;
    delete[] gField;gField=NULL;
    delete[] bField;bField = NULL;
    delete[] numField;numField = NULL;
  }
}

CDrawImage::~CDrawImage(){
//   CDBDebug("[DESC] CDrawImage %dx%d", Geo->dWidth, Geo->dHeight);
  destroyImage();
  delete Geo; Geo=NULL;
}

int CDrawImage::createImage(const char *fn){
  //CDBDebug("CreateImage from file");
  _bEnableTrueColor=true;
  _bEnableTransparency=true;

  cairo_surface_t *surface=cairo_image_surface_create_from_png(fn);
  currentGraphicsRenderer = CDRAWIMAGERENDERER_CAIRO;
  createImage(cairo_image_surface_get_width(surface), cairo_image_surface_get_height(surface));
  cairo->setToSurface(surface);
  cairo_surface_destroy(surface);
  return 0;
}

int CDrawImage::createImage(int _dW,int _dH){
  //CDBDebug("CreateImage from WH");
  Geo->dWidth=_dW;
  Geo->dHeight=_dH;
  return createImage(Geo);
}

int CDrawImage::createImage(CGeoParams *_Geo){
  //CDBDebug("CreateImage from GeoParams");
#ifdef MEASURETIME
  StopWatch_Stop("start createImage of size %d %d, truecolor=[%d], transparency = [%d], currentGraphicsRenderer [%d]",_Geo->dWidth,_Geo->dHeight,_bEnableTrueColor,_bEnableTransparency,currentGraphicsRenderer);
#endif  
  if(currentGraphicsRenderer == -1){
    CDBError("currentGraphicsRenderer not set.");
    return 1;
  }
  if(dImageCreated==1){CDBError("createImage: image already created");return 1;}
  
  Geo->copy(_Geo);
  //CDBDebug("BLA %d",_bEnableTrueColor);
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    //Always true color

    if(_bEnableTransparency==false){
      cairo = new CCairoPlotter(Geo->dWidth, Geo->dHeight, TTFFontSize, TTFFontLocation ,BGColorR,BGColorG,BGColorB,255);
    }else{
      cairo = new CCairoPlotter(Geo->dWidth, Geo->dHeight, TTFFontSize, TTFFontLocation ,0,0,0,0);
    }
  }
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_GD){
    image = gdImageCreate(Geo->dWidth,Geo->dHeight);
    gdFTUseFontConfig(1);
  }
  dImageCreated=1;
#ifdef MEASURETIME
  StopWatch_Stop("image created with renderer %d.",currentGraphicsRenderer);
#endif  

  return 0;

}




int CDrawImage::getClosestGDColor(unsigned char r,unsigned char g,unsigned char b){
  int key = r+g*256+b*65536;
  int color;
  myColorIter=myColorMap.find(key);
  if(myColorIter==myColorMap.end()){
    
    int transparentColor = gdImageGetTransparent(image);
    float closestD = -1;
    int closestI = -1;;
    //CDBDebug("Transparent color = %d",transparentColor);
    for(int j=255;j>=0;j--){
      if(j!=transparentColor){
        int ri=gdImageRed(image, j);
        int gi=gdImageGreen(image, j);
        int bi=gdImageBlue(image, j);
        
        
        float d  = sqrt((ri-r)*(ri-r))+sqrt((gi-g)*(gi-g))+sqrt((bi-b)*(bi-b));
        //CDBDebug("%d %d %d %d %f",j,ri,gi,bi,d);
        if(closestI==-1){
          closestD = d;
          closestI = j;
        }else{
          if(d<closestD){
            closestD = d;
            closestI = j;
          }
        }
      }
      
    }
    
    //CDBDebug("Found %d",closestI);
    
    color = closestI;//gdImageColorClosest(image,r,g,b);
    myColorMap[key]=color;
  }else{
    color=(*myColorIter).second;
  }
  return color;
}

int CDrawImage::printImagePng8(bool useBitAlpha){
  if(dImageCreated==0){CDBError("print: image not created");return 1;}

  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    CDBDebug("printImagePng8 CAIRO");
    cairo->writeToPng8Stream(stdout,backgroundAlpha,useBitAlpha);
  }else if(currentGraphicsRenderer==CDRAWIMAGERENDERER_GD){
    CDBDebug("printImagePng8 GF");
    gdImagePng(image, stdout);
  }else{
    CDBDebug("No graphics renderer!!!");
    return 1;
  }
  return 0;
}

int CDrawImage::printImagePng24(){
  if(dImageCreated==0){CDBError("print: image not created");return 1;}
  
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    cairo->writeToPng24Stream(stdout,backgroundAlpha);
  }
  
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_GD){
    CDBError("gdImagePNG does not support 24 bit");
    return 1;
  }
  return 0;
}


int CDrawImage::printImagePng32(){
  if(dImageCreated==0){CDBError("print: image not created");return 1;}
  
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    cairo->writeToPng32Stream(stdout,backgroundAlpha);
  }
  
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_GD){
    CDBError("gdImagePNG does not support 32 bit");
    return 1;
  }
  return 0;
}
int CDrawImage::printImageWebP32(){
  if(dImageCreated==0){CDBError("print: image not created");return 1;}
  
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    cairo->writeToWebP32Stream(stdout,backgroundAlpha);
  }
  
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_GD){
    CDBError("gdImagePNG does not support webp");
    return 1;
  }
  return 0;
}

int CDrawImage::printImageGif(){
  if(currentGraphicsRenderer == CDRAWIMAGERENDERER_GD){
    if(dImageCreated==0){CDBError("print: image not created");return 1;}
    if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
      CDBError("TrueColor with gif images is not supported");
      return 1;
    }
    if(numImagesAdded == 0){
      gdImageGif(image, stdout);
    }else{
      gdImageGifAnimEnd(stdout);
    }
  }
  if(currentGraphicsRenderer == CDRAWIMAGERENDERER_CAIRO){
    CDBError("Cairo supports no GIF output");
    return 1;
  }
  return 0;
}

void CDrawImage::drawVector(int x,int y,double direction, double strength,int color){
  CColor col=getColorForIndex(color);
  drawVector(x, y ,direction, strength, col, 1.0);
}
  
void CDrawImage::drawVector(int x,int y,double direction, double strength,int color, float linewidth){
  CColor col=getColorForIndex(color);
  drawVector(x, y ,direction, strength, col, linewidth);
}

void CDrawImage::drawVector(int x,int y,double direction, double strength, CColor color, float linewidth){
  double wx1,wy1,wx2,wy2,dx1,dy1;
  if(fabs(strength)<1){
    setPixel(x,y,color);
    return;
  }
  
  bool startatxy=true;
  
  //strength=strength/2;
  dx1=cos(direction)*(strength);
  dy1=sin(direction)*(strength); 
  
  // arrow shaft
  if (startatxy) {
    wx1=double(x)+dx1;wy1=double(y)-dy1; //
    wx2=double(x);wy2=double(y);
  } else {
    wx1=double(x)+dx1;wy1=double(y)-dy1; // arrow point
    wx2=double(x)-dx1;wy2=double(y)+dy1;
  }

  strength=(3+strength);

  // arrow point
  float hx1,hy1,hx2,hy2,hx3,hy3;
  hx1=wx1+cos(direction-2.5)*(strength/2.8f);
  hy1=wy1-sin(direction-2.5)*(strength/2.8f);
  hx2=wx1+cos(direction+2.5)*(strength/2.8f);
  hy2=wy1-sin(direction+2.5)*(strength/2.8f);
  hx3=wx1+(cos(direction-2.5)+cos(direction+2.5))/2*(strength/2.8f);
  hy3=wy1-(sin(direction+2.5)+sin(direction-2.5))/2*(strength/2.8f);
  
 // line(wx1,wy1,hx1,hy1,linewidth,color);
 // line(wx1,wy1,hx2,hy2,linewidth,color);
  poly(hx1, hy1, wx1, wy1, hx2, hy2, linewidth, color, false);
//  line(wx1,wy1,wx2,wy2,linewidth,color);
  line(wx2,wy2,hx3,hy3,linewidth,color);
//  setPixelIndexed(x, y, 252);
  //circle(x+1, y+1, 1, color);
}

#define xCor(l,d) ((int)(l*cos(d)+0.5))
#define yCor(l,d) ((int)(l*sin(d)+0.5))

void CDrawImage::drawVector2(int x,int y,double direction, double strength, int radius, CColor color, float linewidth){
  if(fabs(strength)<1){
    setPixel(x,y,color);
    return;
  }
  
  int ARROW_LENGTH=radius + 16;
  float tipX = x+(int)(ARROW_LENGTH*cos(direction));
  float tipY = y+(int)(ARROW_LENGTH*sin(direction));

  int i2=8+(int)linewidth;
  int i1=2*i2;
  
  float hx1=tipX-xCor(i1, direction+0.5);
  float hy1=tipY-yCor(i1,direction+0.5);
  float hx2=tipX-xCor(i2,direction);
  float hy2=tipY-yCor(i2,direction);
  float hx3=tipX-xCor(i1, direction-0.5);
  float hy3=tipY-yCor(i1,direction-0.5);
  
  poly(tipX, tipY, hx1, hy1, hx2, hy2, hx3, hy3, linewidth, color, true);
}

#define MSTOKNOTS (3600./1852.)

#define round(x) (int(x+0.5)) // Only for positive values!!!

void CDrawImage::drawBarb(int x,int y,double direction, double strength, int color, bool toKnots, bool flip){
  CColor col=getColorForIndex(color);
  drawBarb(x, y, direction, strength, col, 0.5, toKnots, flip);
}

void CDrawImage::drawBarb(int x,int y,double direction, double strength,int color, float lineWidth, bool toKnots, bool flip){
  CColor col=getColorForIndex(color);
  drawBarb(x, y, direction, strength, col, lineWidth, toKnots, flip);
}

void CDrawImage::drawBarb(int x,int y,double direction, double strength, CColor color, float lineWidth, bool toKnots, bool flip){
  double wx1,wy1,wx2,wy2,dx1,dy1;
  int strengthInKnots=round(strength);
  if (toKnots) {
    strengthInKnots = round(strength*3600/1852.);
  }

  if(strengthInKnots<=2){
	// draw a circle
    circle(x, y, 6, color,lineWidth);
    return;
  }

  float pi=3.141592;

  //direction=direction+pi;
  
  int shaftLength=30;

  int nPennants=strengthInKnots/50;
  int nBarbs=(strengthInKnots % 50)/10;
  int rest=(strengthInKnots % 50)%10;
  int nhalfBarbs;
  if (rest<=2) {
  nhalfBarbs=0;
  }else if (rest<=7) {
  nhalfBarbs=1;
  } else {
  nhalfBarbs=0;
  nBarbs++;
  }

  float flipFactor=flip?-1:1; 
  int barbLength=int(-10*flipFactor);
  
  dx1=cos(direction)*(shaftLength);
  dy1=sin(direction)*(shaftLength);
  
/*  wx1=double(x);wy1=double(y);  //wind barb top (flag side)
  wx2=double(x)+dx1;wy2=double(y)-dy1;  //wind barb root*/
  wx1=double(x)-dx1;wy1=double(y)+dy1;  //wind barb top (flag side)
  wx2=double(x);wy2=double(y);  //wind barb root

  circle(int(wx2), int(wy2), 2, color,lineWidth);
  int nrPos=10;

  int pos=0;
  for (int i=0;i<nPennants;i++) {
  double wx3=wx1+pos*dx1/nrPos;
  double wy3=wy1-pos*dy1/nrPos;
  pos++;
  double hx3=wx1+pos*dx1/nrPos+cos(pi+direction+pi/2)*barbLength;
  double hy3=wy1-pos*dy1/nrPos-sin(pi+direction+pi/2)*barbLength;
  pos++;
  double wx4=wx1+pos*dx1/nrPos;
  double wy4=wy1-pos*dy1/nrPos;
    poly(wx3,wy3,hx3,hy3,wx4, wy4, color, true);
  }
  if (nPennants>0) pos++;
  for (int i=0; i<nBarbs;i++) {
  double wx3=wx1+pos*dx1/nrPos;
  double wy3=wy1-pos*dy1/nrPos;
  double hx3=wx3-cos(pi/2-direction+(2-float(flipFactor)*0.1)*pi/2)*barbLength; //was: +cos
  double hy3=wy3-sin(pi/2-direction+(2-float(flipFactor)*0.1)*pi/2)*barbLength; // was: -sin

  line(wx3, wy3, hx3, hy3, lineWidth,color);
  pos++;
  }

  if ((nPennants+nBarbs)==0) pos++;
  if (nhalfBarbs>0){
  double wx3=wx1+pos*dx1/nrPos;
  double wy3=wy1-pos*dy1/nrPos;
  double hx3=wx3-cos(pi/2-direction+(2-float(flipFactor)*0.1)*pi/2)*barbLength/2;
  double hy3=wy3-sin(pi/2-direction+(2-float(flipFactor)*0.1)*pi/2)*barbLength/2;
    line(wx3, wy3, hx3, hy3,lineWidth, color);
  pos++;
  }

  line(wx1,wy1,wx2,wy2,lineWidth,color);
}

void CDrawImage::circle(int x, int y, int r, int color,float lineWidth) {
    if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    cairo->setColor(currentLegend->CDIred[color],currentLegend->CDIgreen[color],currentLegend->CDIblue[color],255);
    cairo->circle(x, y, r,lineWidth);
  }else {
    gdImageArc(image, x-1, y-1, r*2+1, r*2+1, 0, 360, _colors[color]);
  }
}

void CDrawImage::circle(int x, int y, int r, CColor color,float lineWidth) {
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    cairo->setColor(color.r,color.g,color.b,color.a);
    cairo->circle(x, y, r,lineWidth);
  }else {
    int gdcolor=getClosestGDColor(color.r,color.g,color.b);
    gdImageArc(image, x-1, y-1, r*2+1, r*2+1, 0, 360,gdcolor);
  }
}

void CDrawImage::circle(int x, int y, int r, int color) {
  circle(x,y,r,color,1.0);
}

void CDrawImage::circle(int x, int y, int r, CColor col) {
  circle(x,y,r,col,1.0);
}


void CDrawImage::poly(float x1,float y1,float x2,float y2,float x3, float y3, int color, bool fill){
  CColor col=getColorForIndex(color);
  poly(x1, y1, x2, y2, x3, y3, col, fill);
}

void CDrawImage::poly(float x1,float y1,float x2,float y2,float x3, float y3, CColor color, bool fill){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    float ptx[3]={x1, x2, x3};
    float pty[3]={y1,y2,y3};
    cairo->setFillColor(color.r, color.g, color.b, color.a);
//    currentLegend->CDIred[color],currentLegend->CDIgreen[color],currentLegend->CDIblue[color],255);
    cairo->poly(ptx, pty, 3, true, fill);
  } else {
    int colorIndex=getClosestGDColor(color.r, color.g, color.b);
    gdPoint pt[4];
    pt[0].x=int(x1);
    pt[1].x=int(x2);
    pt[2].x=int(x3);
    pt[3].x=int(x1);
    pt[0].y=int(y1);
    pt[1].y=int(y2);
    pt[2].y=int(y3);
    pt[3].y=int(y1);
    if (fill) {
        gdImageFilledPolygon(image, pt, 4, _colors[colorIndex]);
    } else {
        gdImagePolygon(image, pt, 4, _colors[colorIndex]);
    }
  }
}


void CDrawImage::poly(float x1,float y1,float x2,float y2,float x3, float y3, float lineWidth, CColor color, bool fill){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    float ptx[3]={x1, x2, x3};
    float pty[3]={y1,y2,y3};
    cairo->setFillColor(color.r, color.g, color.b, color.a);
//    currentLegend->CDIred[color],currentLegend->CDIgreen[color],currentLegend->CDIblue[color],255);
    cairo->poly(ptx, pty, 3, lineWidth, true, fill);
  } else {
    int colorIndex=getClosestGDColor(color.r, color.g, color.b);
    gdPoint pt[4];
    pt[0].x=int(x1);
    pt[1].x=int(x2);
    pt[2].x=int(x3);
    pt[3].x=int(x1);
    pt[0].y=int(y1);
    pt[1].y=int(y2);
    pt[2].y=int(y3);
    pt[3].y=int(y1);
    if (fill) {
        gdImageFilledPolygon(image, pt, 4, _colors[colorIndex]);
    } else {
        gdImagePolygon(image, pt, 4, _colors[colorIndex]);
    }
  }
}

void CDrawImage::poly(float x1,float y1,float x2,float y2,float x3, float y3, float x4, float y4, float lineWidth, CColor color, bool fill){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    float ptx[4]={x1, x2, x3, x4};
    float pty[4]={y1, y2, y3, y4};
    cairo->setFillColor(color.r, color.g, color.b, color.a);
//    currentLegend->CDIred[color],currentLegend->CDIgreen[color],currentLegend->CDIblue[color],255);
    cairo->poly(ptx, pty, 4, lineWidth, true, fill);
  } else {
    int colorIndex=getClosestGDColor(color.r, color.g, color.b);
    gdPoint pt[5];
    pt[0].x=int(x1);
    pt[1].x=int(x2);
    pt[2].x=int(x3);
    pt[3].x=int(x4);
    pt[4].x=int(x1);
    pt[0].y=int(y1);
    pt[1].y=int(y2);
    pt[2].y=int(y3);
    pt[3].y=int(y4);
    pt[4].y=int(y1);
    if (fill) {
        gdImageFilledPolygon(image, pt, 5, _colors[colorIndex]);
    } else {
        gdImagePolygon(image, pt, 5, _colors[colorIndex]);
    }
  }
}

void CDrawImage::poly(float *x, float*y, int n, float lineWidth, CColor color, bool close, bool fill){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    cairo->setColor(color.r, color.g, color.b, color.a);
    cairo->setFillColor(color.r, color.g, color.b, color.a);
    //    currentLegend->CDIred[color],currentLegend->CDIgreen[color],currentLegend->CDIblue[color],255);
    cairo->poly(x, y, n, lineWidth, close, fill);
  } else {
    int colorIndex=getClosestGDColor(color.r, color.g, color.b);
    gdPoint pt[n];  
    for (int i=0; i<n;i++) {
      pt[i].x=int(x[i]);
      pt[i].y=int(y[i]);
    }
    if (fill) {
      gdImageFilledPolygon(image, pt, 5, _colors[colorIndex]);
    } else {
      gdImagePolygon(image, pt, 5, _colors[colorIndex]);
    }
  }
}


void CDrawImage::line(float x1, float y1, float x2, float y2,int color){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    if(currentLegend==NULL)return;
    if(color>=0&&color<256){
      cairo->setColor(currentLegend->CDIred[color],currentLegend->CDIgreen[color],currentLegend->CDIblue[color],255);
      cairo->line(x1,y1,x2,y2);
    }
  }else{
    gdImageLine(image, int(x1),int(y1),int(x2),int(y2),_colors[color]);
  }
}

void CDrawImage::line(float x1, float y1, float x2, float y2,CColor ccolor){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    if(currentLegend==NULL)return;
    cairo->setColor(ccolor.r,ccolor.g,ccolor.b,ccolor.a);
    cairo->line(x1,y1,x2,y2);
  }else{
    int gdcolor=getClosestGDColor(ccolor.r,ccolor.g,ccolor.b);
    gdImageLine(image, int(x1),int(y1),int(x2),int(y2),gdcolor);
  }
}

void CDrawImage::moveTo(float x1,float y1){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    if(currentLegend==NULL)return;
    cairo->moveTo(x1,y1);
  }else{
    lineMoveToX=x1;
    lineMoveToY=y1;
  }
}

void CDrawImage::lineTo(float x2, float y2,float w,CColor ccolor){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    if(currentLegend==NULL)return;
    cairo->setColor(ccolor.r,ccolor.g,ccolor.b,ccolor.a);
    cairo->lineTo(x2,y2,w);
  }else{
    int gdcolor=getClosestGDColor(ccolor.r,ccolor.g,ccolor.b);
    gdImageLine(image, int(lineMoveToX),int(lineMoveToY),int(x2),int(y2),gdcolor);
    lineMoveToX=x2;lineMoveToY=y2;
  }
  
}

void CDrawImage::endLine(){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    if(currentLegend==NULL)return;
    cairo->endLine();
  }else{
  }
}

CColor CDrawImage::getColorForIndex(int colorIndex){
  CColor color;
  if(currentLegend==NULL)return color;
  if(colorIndex>=0&&colorIndex<256){
    color =CColor(currentLegend->CDIred[colorIndex],currentLegend->CDIgreen[colorIndex],currentLegend->CDIblue[colorIndex],currentLegend->CDIalpha[colorIndex]);
  }
  return color;
}



void CDrawImage::line(float x1,float y1,float x2,float y2,float w,CColor ccolor){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    if(currentLegend==NULL)return;
      cairo->setColor(ccolor.r,ccolor.g,ccolor.b,ccolor.a);
      cairo->line(x1,y1,x2,y2,w);
  }else{
    gdImageSetThickness(image, int(w)*1);
    int gdcolor=getClosestGDColor(ccolor.r,ccolor.g,ccolor.b);
    gdImageLine(image, int(x1),int(y1),int(x2),int(y2),gdcolor);
  }
}

void CDrawImage::line(float x1,float y1,float x2,float y2,float w,int color){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    if(currentLegend==NULL)return;
    if(color>=0&&color<256){
      cairo->setColor(currentLegend->CDIred[color],currentLegend->CDIgreen[color],currentLegend->CDIblue[color],255);
      cairo->line(x1,y1,x2,y2,w);
    }
  }else{
    gdImageSetThickness(image, int(w)*1);
    gdImageLine(image, int(x1),int(y1),int(x2),int(y2),_colors[color]);
  }
}



void CDrawImage::setPixelIndexed(int x,int y,int color){
  
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    if(currentLegend==NULL)return;
    if(color>=0&&color<256){
//       if(currentLegend->CDIalpha[color]==255){
//     cairo-> pixel(x,y,currentLegend->CDIred[color],currentLegend->CDIgreen[color],currentLegend->CDIblue[color]);
//       }else{
      
        if(currentLegend->CDIalpha[color]>0){
              cairo-> pixel_blend(x,y,currentLegend->CDIred[color],currentLegend->CDIgreen[color],currentLegend->CDIblue[color],currentLegend->CDIalpha[color]);
        }else if(currentLegend->CDIalpha[color]<0){
              cairo-> pixel_overwrite(x,y,currentLegend->CDIred[color],currentLegend->CDIgreen[color],currentLegend->CDIblue[color],-(currentLegend->CDIalpha[color]+1));
        }
//       }
    }
  }else{
    gdImageSetPixel(image, x,y,colors[color]);
  }
}

void CDrawImage::getPixelTrueColor(int x,int y,unsigned char &r,unsigned char &g,unsigned char &b,unsigned char &a){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    cairo->getPixel(x,y,r,g,b,a);
  }else{
    int dTranspColor;
    if(currentGraphicsRenderer==CDRAWIMAGERENDERER_GD){
      dTranspColor=gdImageGetTransparent(image);
    }
    int color = gdImageGetPixel(image, x, y);
    r= gdImageRed(image,color);
    g= gdImageGreen(image,color);
    b= gdImageBlue(image,color);
    a= 255-gdImageAlpha(image,color)*2;
    if(color == dTranspColor){
      a=0;
    }
  }
}

void CDrawImage::setPixelTrueColor(int x,int y,unsigned int color){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    cairo->pixel_blend(x,y,color,color/256,color/(256*256),255);
  }else{
    gdImageSetPixel(image, x,y,color);
  }
}

const char* toHex8(char *data,unsigned char hex){
  unsigned char a=hex/16;
  unsigned char b=hex%16;
  data[0]=a<10?a+48:a+55;
  data[1]=b<10?b+48:b+55;
  data[2]='\0';
  return data;
}

void CDrawImage::getHexColorForColorIndex(CT::string *hexValue,int color){
  if(currentLegend==NULL)return;
  char data[3];
  hexValue->print("#%s%s%s",toHex8(data, currentLegend->CDIred[color]),toHex8(data, currentLegend->CDIgreen[color]),toHex8(data, currentLegend->CDIblue[color]));
}



void CDrawImage::setPixelTrueColor(int x,int y,unsigned char r,unsigned char g,unsigned char b){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    cairo->pixel_blend(x,y,r,g,b,255);
  }else{
    if(_bEnableTrueColor){
      gdImageSetPixel(image, x,y,r+g*256+b*65536);
    }else{
      gdImageSetPixel(image, x,y,getClosestGDColor(r,g,b));
    }
  }
}

void CDrawImage::setPixel(int x,int y,CColor &color){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    cairo->pixel_blend(x,y,color.r,color.g,color.b,color.a);
  }else{
    int key = color.r+color.g*256+color.b*65536;
    int icolor;
    myColorIter=myColorMap.find(key);
    if(myColorIter==myColorMap.end()){
      icolor = gdImageColorClosest(image,color.r,color.g,color.b);
      myColorMap[key]=icolor;
    }else{
      icolor=(*myColorIter).second;
    }
    gdImageSetPixel(image, x,y,icolor);
  }
}

/*(int CDrawImage::getClosestColorIndex(CColor color){
  return  gdImageColorClosest(image,color.r,color.g,color.b);
  int key = color.r+color.g*256+color.b*65536;
  int icolor;
  myColorIter=myColorMap.find(key);
  if(myColorIter==myColorMap.end()){
    icolor = gdImageColorClosest(image,color.r,color.g,color.b);
    myColorMap[key]=icolor;
  }else{
    icolor=(*myColorIter).second;
  }
  return icolor;
}*/

void CDrawImage::setPixelTrueColorOverWrite(int x,int y,unsigned char r,unsigned char g,unsigned char b,unsigned char a){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    cairo->pixel_overwrite(x,y,r,g,b,a);
  }else{
      int key = r+g*256+b*65536;
      int color;
      myColorIter=myColorMap.find(key);
      if(myColorIter==myColorMap.end()){
        color = gdImageColorClosest(image,r,g,b);
        myColorMap[key]=color;
      }else{
        color=(*myColorIter).second;
      }
      gdImageSetPixel(image, x,y,color);
  }
}

void CDrawImage::setPixelTrueColor(int x,int y,unsigned char r,unsigned char g,unsigned char b,unsigned char a){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    cairo->pixel_blend(x,y,r,g,b,a);
  }else{
      int key = r+g*256+b*65536;
      int color;
      myColorIter=myColorMap.find(key);
      if(myColorIter==myColorMap.end()){
        color = gdImageColorClosest(image,r,g,b);
        myColorMap[key]=color;
      }else{
        color=(*myColorIter).second;
      }
      gdImageSetPixel(image, x,y,color);
  }
}

void CDrawImage::setText(const char * text, size_t length,int x,int y,int color,int fontSize){
  CColor col=getColorForIndex(color);
  setText(text, length, x, y, col, fontSize);  
    //gdImageString (image, gdFontSmall, x,  y, (unsigned char *)text,_colors[color]);
}

void CDrawImage::setText(const char * text, size_t length,int x,int y, CColor color,int fontSize){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    if(currentLegend==NULL)return;
    cairo->setColor(color.r, color.g, color.b, color.a);
    cairo->drawText(x,y+10,0,text);
  }else{
     int colorIndex=getClosestGDColor(color.r, color.g, color.b);
    char *pszText=new char[length+1];
    strncpy(pszText,text,length);
    pszText[length]='\0';
    if(fontSize==-1)gdImageString (image, gdFontSmall, x,  y, (unsigned char *)pszText,colorIndex);
    if(fontSize==0)gdImageString (image, gdFontMediumBold, x,  y, (unsigned char *)pszText, colorIndex);
    if(fontSize==1)gdImageString (image, gdFontLarge, x,  y, (unsigned char *)pszText, colorIndex);
    delete[] pszText;
  }
}

void CDrawImage::setTextStroke(const char * text, size_t length,int x,int y, int fgcolor,int bgcolor, int fontSize){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    //Not yet supported...
    setText(text, length,x,y,fgcolor,fontSize);
  }else{
    fgcolor=colors[fgcolor];
    bgcolor=colors[bgcolor];
    char *pszText=new char[length+1];
    strncpy(pszText,text,length);
    pszText[length]='\0';
  
    for(int dy=-1;dy<2;dy=dy+1)
      for(int dx=-1;dx<2;dx=dx+1)
        if(!(dx==0&&dy==0)){
      if(fontSize==-1)gdImageString (image, gdFontSmall, x+dx,  y+dy, (unsigned char *)pszText, bgcolor);
          if(fontSize==0)gdImageString (image, gdFontMediumBold, x+dx,  y+dy, (unsigned char *)pszText, bgcolor);
          if(fontSize==1)gdImageString (image, gdFontLarge, x+dx,  y+dy, (unsigned char *)pszText, bgcolor);
        }
    if(fontSize==-1)gdImageString (image, gdFontSmall, x,  y, (unsigned char *)pszText, fgcolor);
    if(fontSize==0)gdImageString (image, gdFontMediumBold, x,  y, (unsigned char *)pszText, fgcolor);
    if(fontSize==1)gdImageString (image, gdFontLarge, x,  y, (unsigned char *)pszText, fgcolor);
  
    delete[] pszText;
  }
}

void CDrawImage::drawText(int x,int y,float angle,const char *text,unsigned char colorIndex){
  CDrawImage::drawText( x, y, angle,text,CColor (currentLegend->CDIred[colorIndex],currentLegend->CDIgreen[colorIndex],currentLegend->CDIblue[colorIndex],255));
}

void CDrawImage::drawText(int x,int y,float angle,const char *text,CColor fgcolor){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    cairo->setColor(fgcolor.r,fgcolor.g,fgcolor.b,fgcolor.a);
    cairo->drawText(x,y,angle,text);
  }else{
    char *_text = new char[strlen(text)+1];
    memcpy(_text,text,strlen(text)+1);
    int tcolor=getClosestGDColor(fgcolor.r,fgcolor.g,fgcolor.b);

    gdImageStringFT(image, &brect[0], tcolor, (char*)TTFFontLocation, 8.0f, angle,  x,  y, (char*)_text);
    delete[] _text;
    //drawTextAngle(text, strlen(text),angle, x, y, 240,8);
  }
}



void CDrawImage::drawText(int x,int y,const char *fontfile, float size, float angle,const char *text,unsigned char colorIndex){
  CColor color(currentLegend->CDIred[colorIndex],currentLegend->CDIgreen[colorIndex],currentLegend->CDIblue[colorIndex],255);
  drawText(x,y,fontfile, size, angle,text,color);
}
 
int CDrawImage::drawTextArea(int x,int y,const char *fontfile, float size, float angle,const char *_text,CColor fgcolor,CColor bgcolor){
    CT::string text;
      int offset = 0;
      CT::string title = _text;
      int length = title.length();
       CCairoPlotter * ftTitle = new CCairoPlotter (Geo->dWidth,Geo->dHeight,(cairo->getByteBuffer()),size,fontfile);
    // ftTitle->setColor(fgcolor.r,fgcolor.g,fgcolor.b,fgcolor.a);
     
//      freeType->drawFilledText(x,y,angle,text);
    //float textScale = 1;
      float textY = 0;
      int width = Geo->dWidth-x;
      int w,h;
      
      do{
        
     
        do{
          text.copy((const char*)(title.c_str()+offset),length);
          ftTitle->getTextSize(w,h,0.0,text.c_str());
          length--;
          //if(!needsLineBreak)if(w>width-10)needsLineBreak = true;
        }while(w>width&&length>=0);
        length++;
      
        if(length+offset<(int)title.length()){
          int sl = length;
          while(text.charAt(sl+offset)!=' '&&sl>0){
            sl--;
          }
          if(sl>0)length=(sl+1);
        }

//         if(length>int(title.length())){
//           length = title.length();
//         }
//         if(offset>int(title.length())){
//             break;
//         }
//         
        text.copy((const char*)(title.c_str()+offset),length);
        if(bgcolor.a!=0){
          ftTitle->setColor(bgcolor.r,bgcolor.g,bgcolor.b,0);
          ftTitle->setFillColor(bgcolor.r,bgcolor.g,bgcolor.b,bgcolor.a);
          ftTitle->filledRectangle(x-3,y+textY+3,x+w+3,y-h-3);
        }
        ftTitle->setColor(fgcolor.r,fgcolor.g,fgcolor.b,fgcolor.a);
        ftTitle->drawText(x,y+textY,0.0,text.c_str());textY+=size*1.5;  
        
        offset+=length;
        //length+=(length);
//         if(length>int(title.length())-1){
//           length = title.length();
//           break;
//         }
      }while(length<(int)(title.length())-1&&(int)offset<(int)title.length()-1);
      cairo->isAlphaUsed|=ftTitle->isAlphaUsed; //remember ftTile's isAlphaUsed flag
      delete ftTitle;
      
      return textY;
}
 
void CDrawImage::drawText(int x,int y,const char *fontfile, float size, float angle,const char *text,CColor fgcolor,CColor bgcolor){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
     CCairoPlotter * freeType = this->getCairoPlotter(fontfile, size, Geo->dWidth, Geo->dHeight, cairo->getByteBuffer());
     freeType->setColor(fgcolor.r,fgcolor.g,fgcolor.b,fgcolor.a);
     freeType->setFillColor(bgcolor.r,bgcolor.g,bgcolor.b,bgcolor.a);
     freeType->drawFilledText(x,y,angle,text);
     cairo->isAlphaUsed|=freeType->isAlphaUsed; //remember freetype's isAlphaUsed flag
   }else{
     char *_text = new char[strlen(text)+1];
     memcpy(_text,text,strlen(text)+1);
     int tcolor=getClosestGDColor(fgcolor.r,fgcolor.g,fgcolor.b);
     if(_bEnableTrueColor)tcolor=-tcolor;
     gdImageStringFT(image, &brect[0],   tcolor, (char*)TTFFontLocation, size, angle,  x,  y, (char*)_text);
     delete[] _text;
   }
 }
void CDrawImage::setDisc(int x,int y,int discRadius, int fillCol, int lineCol){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    if(currentLegend==NULL)return;
    cairo->setFillColor(currentLegend->CDIred[fillCol],currentLegend->CDIgreen[fillCol],currentLegend->CDIblue[fillCol],currentLegend->CDIalpha[fillCol]);
    cairo->setColor(currentLegend->CDIred[lineCol],currentLegend->CDIgreen[lineCol],currentLegend->CDIblue[lineCol],currentLegend->CDIalpha[fillCol]);
    cairo->filledcircle(x, y, discRadius);
    //cairo->setColor(textcolor.r,textcolor.g,textcolor.b,textcolor.a);
    cairo->circle(x, y, discRadius, 1);
//    circle( x,  y,  discRadius,lineCol,1);
  }else{
    gdImageFilledEllipse(image, x, y, discRadius*2, discRadius*2, fillCol);
    circle( x,  y,  discRadius,lineCol,1);    
  }
}

void CDrawImage::setDisc(int x,int y,int discRadius, CColor fillColor, CColor lineColor){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    if(currentLegend==NULL)return;
    cairo->setFillColor(fillColor.r,fillColor.g,fillColor.b,fillColor.a);
    cairo->setColor(lineColor.r,lineColor.g,lineColor.b,lineColor.a);
    cairo->filledcircle(x, y, discRadius);
//    circle( x,  y,  discRadius,lineColor,1);
  }else{
    int fillCol=getClosestGDColor(fillColor.r,fillColor.g,fillColor.b);
    gdImageFilledEllipse(image, x, y, discRadius*2, discRadius*2, fillCol);
    circle( x,  y, discRadius,lineColor,1);    
  }
}
void CDrawImage::setTextDisc(int x,int y,int discRadius, const char *text,const char *fontfile, float fontsize,CColor textcolor,CColor fillcolor, CColor lineColor){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    if(currentLegend==NULL)return;
    cairo->setFillColor(fillcolor.r,fillcolor.g,fillcolor.b,fillcolor.a);
    cairo->setColor(lineColor.r,lineColor.g,lineColor.b,lineColor.a);
    cairo->filledcircle(x, y, discRadius);
    //cairo->setColor(textcolor.r,textcolor.g,textcolor.b,textcolor.a);
    
    
//    circle( x,  y,  discRadius,lineColor,1);
    drawCenteredText( x, y,fontfile, fontsize, 0,text, textcolor);
  }else{
    int fillCol=getClosestGDColor(fillcolor.r,fillcolor.g,fillcolor.b);
    gdImageFilledEllipse(image, x, y, discRadius*2, discRadius*2, fillCol);
    circle( x,  y,  discRadius,lineColor,1);    
    drawCenteredText( x, y,fontfile, fontsize, 0,text, textcolor);
  }
}

void CDrawImage::drawAnchoredText(int x,int y,const char *fontfile, float size, float angle,const char *text,CColor color, int anchor){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    CCairoPlotter * freeType = this->getCairoPlotter(fontfile, size, Geo->dWidth, Geo->dHeight, cairo->getByteBuffer());
    freeType->setColor(color.r,color.g,color.b,color.a);
    freeType->drawAnchoredText(x,y,angle,text,anchor);
    cairo->isAlphaUsed|=freeType->isAlphaUsed; //remember freetype's isAlphaUsed flag
  }else{
    //TODO GD renderer does not center text yet
    char *_text = new char[strlen(text)+1];
    memcpy(_text,text,strlen(text)+1);
    int tcolor=getClosestGDColor(color.r,color.g,color.b);
    if(_bEnableTrueColor)tcolor=-tcolor;
    gdImageStringFT(image, &brect[0],   tcolor, (char*)fontfile, size, angle,  x,  y, (char*)_text);
    delete[] _text;
  }
}


CCairoPlotter *CDrawImage::getCairoPlotter(const char *fontfile, float size, int w, int h, unsigned char *b) {
  CT::string _key;
  _key.print("%s_%f_%d_%d", fontfile, size, w, h);
  // std::string key = _key.c_str();
  std::map<CT::string,CCairoPlotter*>::iterator myCCairoPlotterIter = myCCairoPlotterMap.find(_key);
  if(myCCairoPlotterIter==myCCairoPlotterMap.end()){
    CCairoPlotter * cairoPlotter = new CCairoPlotter (w,h,b,size,fontfile);
    myCCairoPlotterMap[_key] = cairoPlotter;
    return cairoPlotter;
  } else {
    return (*myCCairoPlotterIter).second;
  } 
}
  
  

void CDrawImage::drawCenteredText(int x,int y,const char *fontfile, float size, float angle,const char *text,CColor color){
  
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    CCairoPlotter * freeType = this->getCairoPlotter(fontfile, size, Geo->dWidth, Geo->dHeight, cairo->getByteBuffer());
    freeType->setColor(color.r,color.g,color.b,color.a);
    freeType->drawCenteredText(x,y,angle,text);
    cairo->isAlphaUsed|=freeType->isAlphaUsed; //remember freetype's isAlphaUsed flag
  }else{
    //TODO GD renderer does not center text yet
    char *_text = new char[strlen(text)+1];
    memcpy(_text,text,strlen(text)+1);
    int tcolor=getClosestGDColor(color.r,color.g,color.b);
    if(_bEnableTrueColor)tcolor=-tcolor;
    gdImageStringFT(NULL, &brect[0], tcolor, (char*)TTFFontLocation, 8.0f, angle,  x,  y, (char*)_text);

    int sw=brect[2]-brect[0];
    int sh=brect[5]-brect[3];
    gdImageStringFT(image, &brect[0],   tcolor, (char*)fontfile, size, angle,  x-sw/2,  y-sh/2, (char*)_text);
    delete[] _text;
  }
}

void CDrawImage::drawText(int x,int y,const char *fontfile, float size, float angle,const char *text,CColor color){
  
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    CCairoPlotter * freeType = this->getCairoPlotter(fontfile, size, Geo->dWidth, Geo->dHeight, cairo->getByteBuffer());
    freeType->setColor(color.r,color.g,color.b,color.a);
    freeType->drawText(x,y,angle,text);
    cairo->isAlphaUsed|=freeType->isAlphaUsed; //remember freetype's isAlphaUsed flag
  }else{
    char *_text = new char[strlen(text)+1];
    memcpy(_text,text,strlen(text)+1);
    int tcolor=getClosestGDColor(color.r,color.g,color.b);
    if(_bEnableTrueColor)tcolor=-tcolor;
    gdImageStringFT(image, &brect[0],   tcolor, (char*)fontfile, size, angle,  x,  y, (char*)_text);
    delete[] _text;
  }
}

int CDrawImage::create685Palette(){
  currentLegend=NULL;
  //CDBDebug("Create 685Palette");
  const char *paletteName685="685Palette";
  
  for(size_t j=0;j<legends.size();j++){
    if(legends[j]->legendName.equals(paletteName685)){
      CDBDebug("Found legend");
      currentLegend=legends[j];
      return 0;
    }
  }
  
  if(currentLegend ==NULL){
    currentLegend = new CLegend();
    currentLegend->id = legends.size();
    currentLegend->legendName=paletteName685;
    legends.push_back(currentLegend);
  }
  
  if(dImageCreated==0){CDBError("createGDPalette: image not created");return 1;}
  
  int j=0;
  for(int r=0;r<6;r++)
    for(int g=0;g<8;g++)
      for(int b=0;b<5;b++){
        addColor(j++,r*51  ,g*36  ,b *63 );
      }
  
  addColor(240,0  ,0  ,0  );
  addColor(241,32,32  ,32  );
  addColor(242,64 ,64,64  );
  addColor(243,96,96,96  );
  //addColor(244,64  ,64  ,192);
  addColor(244,64  ,64  ,255);
  addColor(245,128,128  ,255);
  addColor(246,64  ,64,192);
  addColor(247,32,32,32);
  addColor(248,0  ,0  ,0  );
  addColor(249,255,255,191);
  addColor(250,191,232,255);
  addColor(251,204,204,204);
  addColor(252,160,160,160);
  addColor(253,192,192,192);
  addColor(254,224,224,224);
  addColor(255,255,255,255);
  
  copyPalette();
  return 0;
}

int CDrawImage::_createStandard(){

  addColor(240,0  ,0  ,0  );
  addColor(241,32,32  ,32  );
  addColor(242,64 ,64,64  );
  addColor(243,96,96,96  );
  //addColor(244,64  ,64  ,192);
  addColor(244,64  ,64  ,255);
  addColor(245,128,128  ,255);
  addColor(246,64  ,64,192);
  addColor(247,32,32,32);
  addColor(248,0  ,0  ,0  );
  addColor(249,255,255,191);
  addColor(250,191,232,255);
  addColor(251,204,204,204);
  addColor(252,160,160,160);
  addColor(253,192,192,192);
  addColor(254,224,224,224);
  addColor(255,255,255,255);
  
  copyPalette();
  return 0;
}


int CDrawImage::createGDPalette(CServerConfig::XMLE_Legend *legend){
  
  currentLegend=NULL;
  if(legend!=NULL){
    for(size_t j=0;j<legends.size();j++){
      if(legends[j]->legendName.equals(legend->attr.name.c_str())){
        currentLegend=legends[j];
        return 0;
      }
    }
  }
  //CDBDebug("Create legend %s",legend->attr.name.c_str());
  if(currentLegend == NULL){
    currentLegend = new CLegend();
    currentLegend->id = legends.size();
    currentLegend->legendName=legend->attr.name.c_str();
    legends.push_back(currentLegend);
  }
  
  
  if(dImageCreated==0){CDBError("createGDPalette: image not created");return 1;}
 
  
  for(int j=0;j<256;j++){
    currentLegend->CDIred[j]=0;
    currentLegend->CDIgreen[j]=0;
    currentLegend->CDIblue[j]=0;
    currentLegend->CDIalpha[j]=255;
    
  }
  if(legend==NULL){
    return _createStandard();
  }
  if(legend->attr.type.equals("colorRange")){
    
    float cx;
    float rc[4];
    for(size_t j=0;j<legend->palette.size();j++){
      CServerConfig::XMLE_palette *pbegin = legend->palette[j];
      CServerConfig::XMLE_palette *pnext = legend->palette[j];
      if(j<legend->palette.size()-1){
        pnext = legend->palette[j+1];
      }
      

      if(pbegin->attr.index>255)pbegin->attr.index=255;
      if(pbegin->attr.index<0)  pbegin->attr.index=0;
      if(pnext->attr.index>255)pnext->attr.index=255;
      if(pnext->attr.index<0)  pnext->attr.index=0;
      float dif = pnext->attr.index-pbegin->attr.index;
      if(dif<0.5f)dif=1;
      rc[0]=float(pnext->attr.red  -pbegin->attr.red)/dif;
      rc[1]=float(pnext->attr.green-pbegin->attr.green)/dif;
      rc[2]=float(pnext->attr.blue -pbegin->attr.blue)/dif;
      rc[3]=float(pnext->attr.alpha -pbegin->attr.alpha)/dif;

  
      for(int i=pbegin->attr.index;i<pnext->attr.index+1;i++){
        if(i>=0&&i<240){
          cx=float(i-pbegin->attr.index);
          currentLegend->CDIred[i]  =int(rc[0]*cx)+pbegin->attr.red;
          currentLegend->CDIgreen[i]=int(rc[1]*cx)+pbegin->attr.green;
          currentLegend->CDIblue[i] =int(rc[2]*cx)+pbegin->attr.blue;
          currentLegend->CDIalpha[i]=int(rc[3]*cx)+pbegin->attr.alpha;
          if(currentLegend->CDIred[i]==0)currentLegend->CDIred[i]=1;//for transparency
        }
      }
    }
    
    /*for(int j=0;j<240;j++){
      CDBDebug("%d %d %d %d",currentLegend->CDIred[j],currentLegend->CDIgreen[j],currentLegend->CDIblue[j],currentLegend->CDIalpha[j]);
    }*/
    
    return _createStandard();
  }
  if(legend->attr.type.equals("interval")){
  
    for(size_t j=0;j<legend->palette.size();j++){
    
      if(legend->palette[j]->attr.index!=-1){
        
        int startIndex = legend->palette[j]->attr.index;
        int stopIndex = 240;
        if(j<legend->palette.size()-1)stopIndex = legend->palette[j+1]->attr.index;
        
        for(int i=startIndex;i<stopIndex;i++){
          if(i>=0&&i<240){
            currentLegend->CDIred[i]=legend->palette[j]->attr.red;
            currentLegend->CDIgreen[i]=legend->palette[j]->attr.green;
            currentLegend->CDIblue[i]=legend->palette[j]->attr.blue;
            currentLegend->CDIalpha[i]=legend->palette[j]->attr.alpha;
            if(currentLegend->CDIred[i]==0)currentLegend->CDIred[i]=1;//for transparency
          }
        }
      }else{
        for(int i=legend->palette[j]->attr.min;i<=legend->palette[j]->attr.max;i++){
          if(i>=0&&i<240){
            currentLegend->CDIred[i]=legend->palette[j]->attr.red;
            currentLegend->CDIgreen[i]=legend->palette[j]->attr.green;
            currentLegend->CDIblue[i]=legend->palette[j]->attr.blue;
            currentLegend->CDIalpha[i]=legend->palette[j]->attr.alpha;
            if(currentLegend->CDIred[i]==0)currentLegend->CDIred[i]=1;//for transparency
          }
        }
      }
    }
    return _createStandard();
  }
  if(legend->attr.type.equals("svg")){
    if(legend->attr.file.empty()){
      CDBError("Legend type file has no file attribute specified");
      return 1;
    }
    CDBDebug("Reading file %s",legend->attr.file.c_str());
    
    CXMLParserElement element;
    
    try{
      element.parse(legend->attr.file.c_str());
      CXMLParser::XMLElement::XMLElementPointerList stops = element.get("svg")->get("g")->get("defs")->get("linearGradient")->getList("stop");
      float cx;
      float rc[4];
      unsigned char prev_red, prev_green, prev_blue, prev_alpha;
      int prev_offset;
      for(size_t j=0;j<stops.size();j++){
        //CDBDebug("%s",stops.get(j)->toString().c_str());
        int offset = (int)(stops.get(j)->getAttrValue("offset").toFloat()*2.4);
        CT::string color = stops.get(j)->getAttrValue("stop-color").c_str()+4;
        color.setSize(color.length()-1);
        CT::string *colors = color.splitToArray(",");
        if(colors->count!=3){
          delete[] colors;
          CDBError("Number of specified colors is unequal to three");
          return 1;
        }
        unsigned char red = colors[0].toInt();
        unsigned char green = colors[1].toInt();
        unsigned char blue = colors[2].toInt();
        delete[] colors;
        unsigned char alpha = (char)(stops.get(j)->getAttrValue("stop-opacity").toFloat()*255);
        //CDBDebug("I%d R%d G%d B%d A%d",offset,red,green,blue,alpha);
        if(offset>255)offset=255;else if(offset<0)  offset=0;
        /*---------------*/
       
        if(j==0){
          prev_red = red;
          prev_green = green;
          prev_blue = blue;
          prev_alpha = alpha;
          prev_offset = offset;
        }else{
          float dif = offset-prev_offset;
          //CDBDebug("dif %f",dif);
          if(dif<0.5f)dif=1;
          rc[0]=float(prev_red  -red)/dif;
          rc[1]=float(prev_green-green)/dif;
          rc[2]=float(prev_blue -blue)/dif;
          rc[3]=float(prev_alpha -alpha)/dif;

      
          for(int i=prev_offset;i<offset+1;i++){
            if(i>=0&&i<240){
              cx=float(i-prev_offset);
              currentLegend->CDIred[i]  =int(rc[0]*cx)+red;
              currentLegend->CDIgreen[i]=int(rc[1]*cx)+green;
              currentLegend->CDIblue[i] =int(rc[2]*cx)+blue;
              currentLegend->CDIalpha[i]=int(rc[3]*cx)+alpha;
              if(currentLegend->CDIred[i]==0)currentLegend->CDIred[i]=1;//for transparency
            }
          }
        }
        prev_red = red;
        prev_green = green;
        prev_blue = blue;
        prev_alpha = alpha;
        prev_offset = offset;
          /*---------------*/
      }
      
    }catch(int e){
      CT::string message=CXMLParser::getErrorMessage(e);
      CDBError("%s\n",message.c_str());
      return 1;
    }
    return _createStandard();
  }
  return 1;
}

void  CDrawImage::rectangle( int x1, int y1, int x2, int y2,CColor innercolor,CColor outercolor){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
  cairo->setFillColor(innercolor.r,innercolor.g,innercolor.b,innercolor.a);
  cairo->setColor(outercolor.r,outercolor.g,outercolor.b,outercolor.a);
  cairo->filledRectangle(x1,y1,x2,y2);
  }else{
    int gdinnercolor=getClosestGDColor(innercolor.r,innercolor.g,innercolor.b);
    int gdoutercolor=getClosestGDColor(outercolor.r,outercolor.g,outercolor.b);
    if(innercolor.a==255){
      gdImageFilledRectangle (image,x1+1,y1+1,x2-1,y2-1, gdinnercolor);
    }
    gdImageRectangle (image,x1,y1,x2,y2, gdoutercolor);
    
  }
}

void CDrawImage::rectangle( int x1, int y1, int x2, int y2,int innercolor,int outercolor){
  if(currentLegend==NULL)return;
  if(innercolor>=0&&innercolor<255&&outercolor>=0&&outercolor<255){
    if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
      cairo->setColor(currentLegend->CDIred[outercolor],currentLegend->CDIgreen[outercolor],currentLegend->CDIblue[outercolor],255);
      cairo->setFillColor(currentLegend->CDIred[innercolor],currentLegend->CDIgreen[innercolor],currentLegend->CDIblue[innercolor],255);
      cairo->filledRectangle(x1,y1,x2,y2);
    }else{
      //Check for transparency
      if(isColorTransparent(innercolor)){
        //In case of transparency, draw a checkerboard
        for(int x=x1;x<x2;x=x+6){
          for(int y=y1;y<y2;y=y+3){
            int tx=x+((y%6)/3)*3;
            if(tx+2>x2)tx=x2-2;
            gdImageFilledRectangle (image,tx,y,tx+2,y+2, getClosestGDColor(64,64,64));
          }
        }
       }else{
         gdImageFilledRectangle (image,x1+1,y1+1,x2-1,y2-1, colors[innercolor]);
       }
        //gdImageFilledRectangle (image,x1+1,y1+1,x2-1,y2-1, _colors[239]);
      gdImageRectangle (image,x1,y1,x2,y2, colors[outercolor]);
    }
  }
}

void CDrawImage::rectangle( int x1, int y1, int x2, int y2,int outercolor){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
    line( x1, y1, x2, y1,1,outercolor);
    line( x2, y1, x2, y2,1,outercolor);
    line( x2, y2, x1, y2,1,outercolor);
    line( x1, y2, x1, y1,1,outercolor);
  }else{
    gdImageRectangle (image,x1,y1,x2,y2, colors[outercolor]);
  }
}

int CDrawImage::addColor(int Color,unsigned char R,unsigned char G,unsigned char B){
  if(currentLegend==NULL)return 0;
  currentLegend->CDIred[Color]=R;
  currentLegend->CDIgreen[Color]=G;
  currentLegend->CDIblue[Color]=B;
  currentLegend->CDIalpha[Color]=255;
  return 0;
}

int CDrawImage::copyPalette(){
  
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_GD){
    
    //Only one legend is supported for palette based images.
    if(dPaletteCreated==0){
      _colors[255] = gdImageColorAllocate(image,BGColorR,BGColorG,BGColorB); 
      if(_bEnableTransparency){
        gdImageColorTransparent(image,_colors[255]);
      }
      //CDBDebug("CopyPalette");
      for(int j=0;j<255;j++){
        _colors[j] = gdImageColorAllocate(image,currentLegend->CDIred[j],currentLegend->CDIgreen[j],currentLegend->CDIblue[j]); 
      }
      gdTranspColor=gdImageGetTransparent(image);
    }
  
  }
  for(int j=0;j<256;j++){
    if(dPaletteCreated==0){colors[j]=_colors[j];}
    
    if(j<240){
      if(currentLegend->CDIred[j]==0&&currentLegend->CDIgreen[j]==0&&currentLegend->CDIblue[j]==0){
        if(dPaletteCreated==0){colors[j]=_colors[255];}
        currentLegend->CDIalpha[j]=0;
      }
    }
  }
  dPaletteCreated=1;
  return 0;
}
int s=0;
/**
 * Add image is only called when an image is added to an existing image (EG. only for animations).
 * 
 */

int CDrawImage::addImage(int delay){

  //CDBDebug("Render: %d",currentGraphicsRenderer);
  if(currentGraphicsRenderer == CDRAWIMAGERENDERER_GD){
    //Add the current active image:
    gdImageGifAnimAdd(image, stdout, 0, 0, 0, delay, gdDisposalRestorePrevious, NULL);
    
    //This image is added to the GIF container, it should now be destroyed.
    //Immediately make a new image for next round.
    destroyImage();

    //Make sure a new image is available for drawing
    if(currentGraphicsRenderer==CDRAWIMAGERENDERER_GD){
      image = gdImageCreate(Geo->dWidth,Geo->dHeight);
    }else{
      image = gdImageCreateTrueColor(Geo->dWidth,Geo->dHeight);
      gdImageSaveAlpha( image, true );
    }
    dImageCreated=1;
    currentLegend=legends[0];    
    copyPalette();
  }
  numImagesAdded++;
  
  return 0;
}

int CDrawImage::beginAnimation(){
  numImagesAdded = 0;
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_GD){
    if(_bEnableTrueColor == false){
      gdImageGifAnimBegin(image, stdout, 1, 0);
    }
  }
  return 0; 
}

int CDrawImage::endAnimation(){
  return 0;
}

void CDrawImage::enableTransparency(bool enable){
  _bEnableTransparency=enable;
}
void CDrawImage::setBGColor(unsigned char R,unsigned char G,unsigned char B){
  BGColorR=R;
  BGColorG=G;
  BGColorB=B;
}

void CDrawImage::setTrueColor(bool enable){
  _bEnableTrueColor=enable;
  if(enable){
    setRenderer(CDRAWIMAGERENDERER_CAIRO);
  }else{
    setRenderer(CDRAWIMAGERENDERER_GD);
  }
}


bool CDrawImage::isPixelTransparent(int &x,int &y){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_GD){
    int color = gdImageGetPixel(image, x, y);
    if(color!=gdTranspColor||127!=gdImageAlpha(image,color)){
      return false;
    }
    return true;
  }else{

    unsigned char r,g,b,a;
    cairo-> getPixel(x,y,r,g,b,a);

    if(a==0)return true;else {
      if(r == BGColorR && g == BGColorG && b== BGColorB && 1==2){
        return true;
      }else{
        return false;
      }
    }
  }
  return false;
}


bool CDrawImage::isColorTransparent(int &color){
  
  if(currentLegend==NULL){
    return true;
  }
  if(color<0||color>255)return true;
  if(currentLegend->CDIred[color]==0&&currentLegend->CDIgreen[color]==0&&currentLegend->CDIblue[color]==0)return true;
  return false;
}

/**
 * Returns a value indicating how width the image is based on whether there are pixels set.
 * Useful for cropping the image
 */

void CDrawImage:: getCanvasSize(int &x1,int &y1,int &w,int &h){
  w=0;
  x1=Geo->dWidth;
  for(int y=0;y<Geo->dHeight;y++){
    for(int x=w;x<Geo->dWidth;x++)if(!isPixelTransparent(x,y))w=x;
    for(int x=0;x<x1;x++)if(!isPixelTransparent(x,y))x1=x;
  }
 
  w=w-x1+1;
  
  h=0;
  y1=Geo->dHeight;
  
  for(int x=x1;x<w;x++){
    for(int y=h;y<Geo->dHeight;y++)if(!isPixelTransparent(x,y))h=y;
    for(int y=0;y<y1;y++)if(!isPixelTransparent(x,y))y1=y;
  }

  h=h-y1+1;
  if(w<0)w=0;
  if(h<0)h=0;
}

int CDrawImage::clonePalette(CDrawImage *drawImage){
  if(drawImage->currentLegend == NULL){
    currentLegend  = NULL;
    return 0;
  }
  currentLegend = new CLegend();
  currentLegend->id = legends.size();
  currentLegend->legendName=drawImage->currentLegend->legendName.c_str();
  legends.push_back(currentLegend);
  for(int j=0;j<256;j++){
    currentLegend->CDIred[j]=drawImage->currentLegend->CDIred[j];
    currentLegend->CDIgreen[j]=drawImage->currentLegend->CDIgreen[j];
    currentLegend->CDIblue[j]=drawImage->currentLegend->CDIblue[j];
    currentLegend->CDIalpha[j]=drawImage->currentLegend->CDIalpha[j];
  }
  BGColorR=drawImage->BGColorR;
  BGColorG=drawImage->BGColorG;
  BGColorB=drawImage->BGColorB;
  copyPalette();  
  return 0;
}

/**
 * Creates a new image with the same settings but with different size as the source image
 */
int CDrawImage::createImage(CDrawImage *image,int width,int height){
  //CDBDebug("CreateImage from image");
  #ifdef MEASURETIME
  CDBDebug("createImage(CDrawImage *image,int width,int height)");
#endif
  //setTrueColor(image->getTrueColor());
//  CDBDebug("Creating image");
  enableTransparency(image->_bEnableTransparency);
  currentGraphicsRenderer = image->currentGraphicsRenderer;
  //colorType = image->colorType;
  setTTFFontLocation(image->TTFFontLocation);
  
  setTTFFontSize(image->TTFFontSize);
//  CDBDebug("Creating image 2");
  createImage(width,height);
//  CDBDebug("Creating image palette");
  clonePalette(image);
//  CDBDebug("New image created");
  return 0;
}

    
int CDrawImage::setCanvasSize(int x,int y,int width,int height){
  CDrawImage temp;
  temp.createImage(this,width,height);
 
  temp.draw(0,0,x,y,this);
 
  destroyImage();

  createImage(&temp,width,height);
  draw(0,0,0,0,&temp);
  temp.destroyImage();
  return 0;
}

int CDrawImage::draw(int destx, int desty,int sourcex,int sourcey,CDrawImage *simage){
  unsigned char r,g,b,a;
  int dTranspColor;
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_GD){
    dTranspColor=gdImageGetTransparent(simage->image);
  }
  for(int y=0;y<simage->Geo->dHeight;y++){
    for(int x=0;x<simage->Geo->dWidth;x++){
      int sx=x+sourcex;int sy=y+sourcey;int dx=x+destx;int dy=y+desty;
      if(sx>=0&&sy>=0&&dx>=0&&dy>=0&&
        sx<simage->Geo->dWidth&&sy<simage->Geo->dHeight&&dx<Geo->dWidth&&dy<Geo->dHeight){
        //Get source r,g,b,a
        if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
          simage->cairo->getPixel(sx,sy,r,g,b,a);
        }else{
          int color = gdImageGetPixel(simage->image, x+sourcex, y+sourcey);
          r= gdImageRed(simage->image,color);
          g= gdImageGreen(simage->image,color);
          b= gdImageBlue(simage->image,color);
          a= 255-gdImageAlpha(simage->image,color)*2;
          if(color == dTranspColor){
            a=0;
          }
        }
        //Set r,g,b,a to dest
        if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
          cairo-> pixel_blend(dx,dy,r,g,b,a);
        }else{
          if(a>128){
            gdImageSetPixel(image,x+destx,y+desty,getClosestGDColor(r,g,b));
          }
        }
      }
    }
  }
  return 0;
}
int CDrawImage::drawrotated(int destx, int desty,int sourcex,int sourcey,CDrawImage *simage){
  unsigned char r,g,b,a;
  int dTranspColor;
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_GD){
    dTranspColor=gdImageGetTransparent(simage->image);
  }
  for(int y=0;y<simage->Geo->dHeight;y++){
    for(int x=0;x<simage->Geo->dWidth;x++){
      int sx=x+sourcex;int sy=y+sourcey;int dx=simage->Geo->dHeight-y-desty;int dy=x+destx;
      if(sx>=0&&sy>=0&&dx>=0&&dy>=0&&
        sx<simage->Geo->dWidth&&sy<simage->Geo->dHeight&&dx<Geo->dWidth&&dy<Geo->dHeight){
        //Get source r,g,b,a
        if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
          simage->cairo->getPixel(sx,sy,r,g,b,a);
        }else{
          int color = gdImageGetPixel(simage->image, x+sourcex, y+sourcey);
          r= gdImageRed(simage->image,color);
          g= gdImageGreen(simage->image,color);
          b= gdImageBlue(simage->image,color);
          a= 255-gdImageAlpha(simage->image,color)*2;
          if(color == dTranspColor){
            a=0;
          }
        }
        //Set r,g,b,a to dest
        if(currentGraphicsRenderer==CDRAWIMAGERENDERER_CAIRO){
          cairo-> pixel_blend(dx,dy,r,g,b,a);
        }else{
          if(a>128){
            gdImageSetPixel(image,dx,dy,getClosestGDColor(r,g,b));
          }
        }
        }
    }
  }
  return 0;
}
/**
 * Crops image 
 * @param int paddingW the padding to keep in pixels in width. Set to -1 if no crop in width is desired
 * @param int paddingH the padding to keep in pixels in height. Set to -1 if no crop in height is desired
 */
void CDrawImage::crop(int paddingW, int paddingH){
 // return;
  int x,y,w,h;
  getCanvasSize(x,y,w,h);
  
  int x1=x-paddingW;
  int y1=y-paddingH;
  int w1=w+paddingW*2;
  int h1=h+paddingH*2;
  if(x1<0){x1=0;};if(y1<0){y1=0;}
  if(x1>Geo->dWidth){x1=Geo->dWidth;};if(y1>Geo->dHeight){y1=Geo->dHeight;}
  if(paddingW<0){
    x1=0;w1=Geo->dWidth;
  }
  if(paddingH<0){
    y1=0;h1=Geo->dHeight;
  }
  if(h1>Geo->dHeight-y1)h1=Geo->dHeight-y1;
  if(w1>Geo->dWidth-x1)w1=Geo->dWidth-x1;
  
  setCanvasSize(x1,y1,w1,h1);
}

/**
 * Crops image with desired padding
 * @param padding The number of empty pixels surrounding the new image
 */
void CDrawImage::crop(int padding){
  crop(padding,padding);
}

void CDrawImage::rotate() {
  int w = Geo->dWidth;
  int h = Geo->dHeight;
//   if(this->Geo->dWidth!=1)w=srvParam->Geo->dWidth;
//   if(srvParam->Geo->dHeight!=1)h=srvParam->Geo->dHeight;
//   if (requestedWidth>requestedHeight) {
//     int x,y,w,h;
//     getCanvasSize(x,y,w,h);
//     // rotate image 90 degrees counter clockwise
//         
//   }    
  CDrawImage temp;
  temp.createImage(this,w,h);
  
  temp.draw(0,0,0,0,this);
  
  destroyImage();
  CDBDebug("Creating rotated legend %d,%d", h, w);
  createImage(&temp,h,w);
  drawrotated(0,0,0,0,&temp);
  temp.destroyImage();
//  return 0;
}

unsigned char* const CDrawImage::getCanvasMemory(){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_GD){
    CDBError("Unable to return canvas memory for indexed colors");
    return NULL;
  }
  return cairo->getByteBuffer();
  
}


void CDrawImage::setCanvasColorType(int colorType){
  if(colorType == CDRAWIMAGE_COLORTYPE_ARGB){
    setRenderer(CDRAWIMAGERENDERER_CAIRO);
    _bEnableTrueColor = true;
  }  else{
    setRenderer(CDRAWIMAGERENDERER_GD);
  }
}

int CDrawImage::getCanvasColorType(){
  if(currentGraphicsRenderer==CDRAWIMAGERENDERER_GD){
    return CDRAWIMAGE_COLORTYPE_INDEXED;
  }
  return CDRAWIMAGE_COLORTYPE_ARGB;
}

int CDrawImage::getRenderer(){
  return currentGraphicsRenderer;
}


void CDrawImage::setRenderer(int type){
  currentGraphicsRenderer=type;
}

int CDrawImage::getHeight() {
  return Geo->dHeight;
}

int CDrawImage::getWidth() {
  return Geo->dWidth;
}
