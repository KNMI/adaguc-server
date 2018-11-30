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

#ifndef CDrawImage_H
#define CDrawImage_H

#include <map>
#include <iostream>
#include "CDebugger.h"
#include "CTypes.h"

#include "Definitions.h" 
#include "CStopWatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "CGeoParams.h"
#include "CServerError.h"
#include "CServerConfig_CPPXSD.h"
#include <math.h>
//#include <png.h>
#include <gd.h>
#include "gdfontl.h"
#include "gdfonts.h"
#include "gdfontmb.h"
#include "CCairoPlotter.h"

float convertValueToClass(float val,float interval);

#define CDRAWIMAGE_COLORTYPE_INDEXED 1
#define CDRAWIMAGE_COLORTYPE_ARGB    2

#define CDRAWIMAGERENDERER_GD 1
#define CDRAWIMAGERENDERER_CAIRO 2

class CLegend{
public:
  int id;
  unsigned char CDIred[256],CDIgreen[256],CDIblue[256];
  short CDIalpha[256];//Currently alpha of 0 and 255 is supported, but nothin in between.
  CT::string legendName;
};


class CColor{
  public:
    unsigned char r,g,b,a;
    CColor(){
      r=0;g=0;b=0;a=255;
    }
    CColor(unsigned char r,unsigned char g,unsigned char b,unsigned char a){
      this->r=r;
      this->g=g;
      this->b=b;
      this->a=a;
    }
    CColor(const char *color){
      parse(color);
    }
    /**
     * color can have format #RRGGBB or #RRGGBBAA
     */
    void parse(const char * color){
      if(color[0]=='#'){
        if(strlen(color)==7){
          r  =((color[1]>64)?color[1]-55:color[1]-48)*16+((color[2]>64)?color[2]-55:color[2]-48);
          g=((color[3]>64)?color[3]-55:color[3]-48)*16+((color[4]>64)?color[4]-55:color[4]-48);
          b =((color[5]>64)?color[5]-55:color[5]-48)*16+((color[6]>64)?color[6]-55:color[6]-48);
          a=255;
        }
        if(strlen(color)==9){
          r  =((color[1]>64)?color[1]-55:color[1]-48)*16+((color[2]>64)?color[2]-55:color[2]-48);
          g=((color[3]>64)?color[3]-55:color[3]-48)*16+((color[4]>64)?color[4]-55:color[4]-48);
          b =((color[5]>64)?color[5]-55:color[5]-48)*16+((color[6]>64)?color[6]-55:color[6]-48);
          a =((color[7]>64)?color[7]-55:color[7]-48)*16+((color[8]>64)?color[8]-55:color[8]-48);
        }
      }
    }
};


class CDrawImage{
public:
  /*
  #define CGRAPHICSIMAGE_IMAGETYPEINDEXED
  #define CGRAPHICSIMAGE_IMAGETYPEARGB
 
  class CGraphicsImage{
  public:
    CGraphicsImage();
    ~CGraphicsImage();
    int imageType;
    int width,height,stride;
    unsigned char* byteBuffer;
    int byteBufferPointerIsOwned;
  };
  
  class CPNGWriter{
  public:
    class {
      unsigned char    r,g,b,a;
    } RGBAType;
    int writeToPng8Stream(FILE *fp, CGraphicsImage* image);
    int writeToPng32Stream(FILE *fp, CGraphicsImage* image);
  };
  */
  private:
    std::vector<CLegend *>legends;
    CLegend * currentLegend;
    int dImageCreated;
    
    DEF_ERRORFUNCTION();
    int _createStandard();
    
    int dPaletteCreated;
    bool paletteCopied;
    unsigned char * rbuf;
    int dNumImages;
    
    unsigned char BGColorR,BGColorG,BGColorB;
    bool _bEnableTransparency;
    bool _bEnableTrueColor;
    unsigned char backgroundAlpha;
    //bool _bAntiAliased;
    int brect[8];
    CCairoPlotter *cairo;
    const char *TTFFontLocation;
    float TTFFontSize;
    //char *fontConfig ;
    
    std::map<int,int> myColorMap;
    std::map<int,int>::iterator myColorIter;
    std::map<CT::string,CCairoPlotter*> myCCairoPlotterMap;
    CCairoPlotter *getCairoPlotter(const char *fontfile, float size, int w, int h, unsigned char *b);
    int getClosestGDColor(unsigned char r,unsigned char g,unsigned char b);
    int gdTranspColor;
    float lineMoveToX,lineMoveToY;
    int numImagesAdded;
    int currentGraphicsRenderer;
  public:
    float *rField , *gField, *bField ;
    int *numField ;
    bool trueColorAVG_RGBA;
    int getRenderer();
    int _colors[256];
    gdImagePtr image;
    
    int colors[256];
    CGeoParams *Geo;
    CDrawImage();
    ~CDrawImage();
    int createImage(int _dW,int _dH);
    int createImage(CGeoParams *_Geo);
    int createImage(const char *fn);
    int createImage(CDrawImage *image,int width,int height);
    int printImagePng8(bool useBitAlpha);
    int printImagePng24();
    int printImagePng32();
    int printImageWebP32();
    int printImageGif();
    int createGDPalette(CServerConfig::XMLE_Legend *palette);
    int create685Palette();
    int clonePalette(CDrawImage *drawImage);
    
    void drawBarb(int x,int y,double direction, double strength,int color,bool toKnots,bool flip);
    void drawBarb(int x,int y,double direction, double strength,int color,float linewidth, bool toKnots,bool flip);
    void drawBarb(int x,int y,double direction, double strength,CColor color,float linewidth, bool toKnots,bool flip);
    void drawText(int x,int y,float angle,const char *text,unsigned char colorIndex);
    void drawText(int x,int y,float angle,const char *text,CColor fgcolor);
    void drawText(int x,int y,const char *fontfile, float size, float angle,const char *text,unsigned char colorIndex);
    void drawText(int x,int y,const char *fontfile, float size, float angle,const char *text,CColor fgcolor);
    void drawText(int x,int y,const char *fontfile, float size, float angle,const char *text,CColor fgcolor,CColor bgcolor);
    void drawAnchoredText(int x,int y,const char *fontfile, float size, float angle,const char *text,CColor color, int anchor);
    void drawCenteredText(int x,int y,const char *fontfile, float size, float angle,const char *text,CColor color);
    int drawTextArea(int x,int y,const char *fontfile, float size, float angle,const char *text,CColor fgcolor,CColor bgcolor);
    
    //void drawTextAngle(const char * text, size_t length,double angle,int x,int y,int color,int fontSize);
    void drawVector(int x,int y,double direction, double strength,int color);
    void drawVector(int x,int y,double direction, double strength,int color, float linewidth);
    void drawVector(int x,int y,double direction, double strength,CColor color, float linewidth);
    void drawVector2(int x,int y,double direction, double strength, int radius, CColor color, float linewidth);
    void destroyImage();
    void line(float x1,float y1,float x2,float y2,int color);
    void line(float x1,float y1,float x2,float y2,CColor color);
    void line(float x1,float y1,float x2,float y2,float w,int color);
    void line(float x1,float y1,float x2,float y2,float w,CColor color);
    void moveTo(float x1,float y1);
    void lineTo(float x1,float y1,float w,CColor color);
    void endLine();
    
    void poly(float x1, float y1, float x2, float y2, float x3, float y3, int c, bool fill);
    void poly(float x1, float y1, float x2, float y2, float x3, float y3, CColor color, bool fill);
    void poly(float x1, float y1, float x2, float y2, float x3, float y3, float lineWidth, CColor color, bool fill);
    void poly(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float lineWidth, CColor color, bool fill);
    void poly(float *x, float*y, int n, float lineWidth, CColor color, bool close, bool fill);
    void circle(int x, int y, int r, int color);
    void circle(int x, int y, int r, CColor col);

    void circle(int x, int y, int r, int color,float lineWidth);
    void circle(int x, int y, int r, CColor color,float lineWidth);
    void setPixelIndexed(int x,int y,int color);
    void setPixelTrueColor(int x,int y,unsigned int color);
    void setPixelTrueColor(int x,int y,unsigned char r,unsigned char g,unsigned char b);
    void setPixelTrueColor(int x,int y,unsigned char r,unsigned char g,unsigned char b,unsigned char a);
    void getPixelTrueColor(int x,int y,unsigned char &r,unsigned char &g,unsigned char &b,unsigned char &a);
    void setPixelTrueColorOverWrite(int x,int y,unsigned char r,unsigned char g,unsigned char b,unsigned char a);
    void setPixel(int x,int y,CColor &color);
    
    int getWidth();
    int getHeight();
    
    //int getClosestColorIndex(CColor color);
    void getHexColorForColorIndex(CT::string *hexValue,int colorIndex);
    void setText(const char * text, size_t length,int x,int y, int color,int fontSize);
    void setText(const char * text, size_t length,int x,int y, CColor color,int fontSize);
    //void setTextDisc(const char *text, size_t length, int x, int y, int r, CColor color, const char *fontfile,int fontSize);
    void setDisc(int x,int y,int discRadius, CColor fillColor, CColor lineColor);
    void setDisc(int x,int y,int discRadius, int fillCol, int lineCol);
    void setTextDisc(int x,int y,int discRadius, const char *text,const char *fontfile, float fontsize,CColor textcolor,CColor fillcolor, CColor lineColor);
    void setTextStroke(const char * text, size_t length,int x,int y, int fgcolor,int bgcolor, int fontSize);
    void rectangle( int x1, int y1, int x2, int y2,int innercolor,int outercolor);
    void rectangle( int x1, int y1, int x2, int y2,int outercolor);
    void rectangle( int x1, int y1, int x2, int y2,CColor innercolor,CColor outercolor);
    CColor getColorForIndex(int index);
    int copyPalette();
    int addImage(int delay);
    int beginAnimation();
    int endAnimation();
    int addColor(int Color,unsigned char R,unsigned char G,unsigned char B);
    void enableTransparency(bool enable);
    void setBGColor(unsigned char R,unsigned char G,unsigned char B);
    void setTrueColor(bool enable);
    bool getTrueColor(){return _bEnableTrueColor;}
    
    //void setAntiAliased(bool enable){      _bAntiAliased=enable;   };
    //bool getAntialiased(){return _bAntiAliased;}
    
    void setTTFFontLocation(const char *_TTFFontLocation){TTFFontLocation=_TTFFontLocation;  }
    void setTTFFontSize(float _TTFFontSize){  TTFFontSize=_TTFFontSize; }
    bool isPixelTransparent(int &x,int &y);
    bool isColorTransparent(int &color);
    void getCanvasSize(int &x,int &y,int &w,int &h);
    
    /**
     * @param alpha The transparency of the resulting image written as PNG: 0 is transparent, 255 is opaque
     */
    void setBackGroundAlpha(unsigned char alpha){
      backgroundAlpha=alpha;
    }
    
    int setCanvasSize(int x,int y,int width,int height);
    int draw(int destx, int desty,int sourcex,int sourcey,CDrawImage *simage);
    int drawrotated(int destx, int desty,int sourcex,int sourcey,CDrawImage *simage);    
    void crop(int paddingW,int paddingH);
    void crop(int padding);
    
    void rotate();
    /**
     * Returns canvas memory in case of true color images
     */
    unsigned char* const getCanvasMemory();
    
    /**
     * Returns canvas colortype, either COLORTYPE_INDEXED or COLORTYPE_ARGB
     */
    int getCanvasColorType();
    
    /**
     * Sets canvas colortype, either COLORTYPE_INDEXED or COLORTYPE_ARGB
     */
    void setCanvasColorType(int colorType);
    
    /**
     * Set renderer type, CDRAWIMAGERENDERER_CAIRO or CDRAWIMAGERENDERER_GD
     */
    void setRenderer(int renderer);
    

};

#endif

