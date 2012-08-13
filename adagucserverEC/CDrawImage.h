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
#include <png.h>
#include <gd.h>
#include "gdfontl.h"
#include "gdfonts.h"
#include "gdfontmb.h"
#ifdef ADAGUC_USE_CAIRO
#include "CCairoPlotter.h"
#else
  #include <ft2build.h>
  #include <freetype/freetype.h>
  #include <freetype/ftglyph.h>
  #include <freetype/ftoutln.h>
  #include <freetype/fttrigon.h>
  #include FT_FREETYPE_H
#endif

float convertValueToClass(float val,float interval);

#ifndef ADAGUC_USE_CAIRO
#include "CDrawAA.h"
#endif 

class CLegend{
public:
  int id;
  unsigned char CDIred[256],CDIgreen[256],CDIblue[256],CDIalpha[256];//Currently alpha of 0 and 255 is supported, but nothin in between.
  CT::string legendName;
};


class CColor{
  public:
    unsigned char r,g,b,a;
    CColor(unsigned char r,unsigned char g,unsigned char b,unsigned char a){
      this->r=r;
      this->g=g;
      this->b=b;
      this->a=a;
    }
};


class CDrawImage{
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
    //bool _bAntiAliased;
    int brect[8];
#ifdef ADAGUC_USE_CAIRO
    CCairoPlotter *cairo;
#else
    CXiaolinWuLine *wuLine;
    CFreeType *freeType;
    unsigned char *RGBAByteBuffer;
    int writeRGBAPng(int width,int height,unsigned char *RGBAByteBuffer,FILE *file,bool trueColor);
#endif
    const char *TTFFontLocation;
    float TTFFontSize;
    //char *fontConfig ;
    
    static std::map<int,int> myColorMap;
    static std::map<int,int>::iterator myColorIter;
    int getClosestGDColor(unsigned char r,unsigned char g,unsigned char b){
      int key = r+g*256+b*65536;
      int color;
      myColorIter=myColorMap.find(key);
      if(myColorIter==myColorMap.end()){
        color = gdImageColorClosest(image,r,g,b);
        myColorMap[key]=color;
      }else{
        color=(*myColorIter).second;
      }
      return color;
    }
    int gdTranspColor;
  public:
    
    int _colors[256];
    gdImagePtr image;
    
    int colors[256];
    CGeoParams *Geo;
    CDrawImage();
    ~CDrawImage();
    int createImage(int _dW,int _dH);
    int createImage(CGeoParams *_Geo);
    int createImage(CDrawImage *image,int width,int height);
    int printImagePng();
    int printImageGif();
    int createGDPalette(CServerConfig::XMLE_Legend *palette);
    int create685Palette();
    int clonePalette(CDrawImage *drawImage);
    
    void drawBarb(int x,int y,double direction, double strength,int color,bool toKnots,bool flip);
    void drawText(int x,int y,float angle,const char *text,unsigned char colorIndex);
    void drawText(int x,int y,const char *fontfile, float size, float angle,const char *text,unsigned char colorIndex);
    void drawText(int x,int y,const char *fontfile, float size, float angle,const char *text,CColor fgcolor);
    void drawText(int x,int y,const char *fontfile, float size, float angle,const char *text,CColor fgcolor,CColor bgcolor);
    //void drawTextAngle(const char * text, size_t length,double angle,int x,int y,int color,int fontSize);
    void drawVector(int x,int y,double direction, double strength,int color);
    void destroyImage();
    void line(float x1,float y1,float x2,float y2,int color);
    void line(float x1,float y1,float x2,float y2,CColor color);
    void line(float x1,float y1,float x2,float y2,float w,int color);
    void line(float x1,float y1,float x2,float y2,float w,CColor color);
    void poly(float x1, float y1, float x2, float y2, float x3, float y3, int c, bool fill);
    void circle(int x, int y, int r, int color);
    void setPixelIndexed(int x,int y,int color);
    void setPixelTrueColor(int x,int y,unsigned int color);
    void setPixelTrueColor(int x,int y,unsigned char r,unsigned char g,unsigned char b);
    void setPixelTrueColor(int x,int y,unsigned char r,unsigned char g,unsigned char b,unsigned char a);
    void getHexColorForColorIndex(CT::string *hexValue,int colorIndex);
    void setText(const char * text, size_t length,int x,int y, int color,int fontSize);
    void setTextStroke(const char * text, size_t length,int x,int y, int fgcolor,int bgcolor, int fontSize);
    void rectangle( int x1, int y1, int x2, int y2,int innercolor,int outercolor);
    void rectangle( int x1, int y1, int x2, int y2,int outercolor);
    void rectangle( int x1, int y1, int x2, int y2,CColor innercolor,CColor outercolor);
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
    
    int setCanvasSize(int x,int y,int width,int height);
    int draw(int destx, int desty,int sourcex,int sourcey,CDrawImage *simage);
    void crop(int paddingW,int paddingH);
    void crop(int padding);
    
    
};

#endif

