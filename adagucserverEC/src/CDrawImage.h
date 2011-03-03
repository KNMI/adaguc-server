#ifndef CDrawImage_H
#define CDrawImage_H
#include "CDebugger.h"
#include "CTypes.h"

#include "Definitions.h" 
#include "CStopWatch.h"
#include <gd.h>
#include "gdfontl.h"
#include "gdfonts.h"
#include "gdfontmb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "CGeoParams.h"
#include "CServerError.h"
#include "CServerConfig_CPPXSD.h"
#include "agg.h"
#include <math.h>
#include <png.h>
#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>
#include FT_FREETYPE_H
float convertValueToClass(float val,float interval);

class CDrawImage{
  private:
    int dImageCreated;

    DEF_ERRORFUNCTION();
    int _createStandard();
    
    int dPaletteCreated;
    
    int dNumImages;
    unsigned char CDIred[256],CDIgreen[256],CDIblue[256];
    unsigned char BGColorR,BGColorG,BGColorB;
    bool _bEnableTransparency;
    bool _bEnableTrueColor;
    bool _bEnableAGG;
    unsigned char* aggBytebuf;
    agg::rendering_buffer *rbuf;
    agg::renderer<agg::span_rgba32> *renRGBA;
    agg::renderer<agg::span_mono8> *ren256;
    agg::rasterizer *ras;
    int writeAGGPng();
    int brect[8];
    char *fontConfig;
    FT_Library library;
    FT_Face face;
    int initializeFreeType();
    int renderFont(FT_Bitmap *bitmap,int left,int top,agg::rgba8 color);
    int drawFreeTypeText(int x,int y,float angle,const char *text,agg::rgba8 color);
    
    void draw_line(agg::rasterizer* ras,
                   double x1, double y1, 
                   double x2, double y2,
                   double width);
    const char *TTFFontLocation;
  public:
    
    int _colors[256];
    gdImagePtr image;
    
    int colors[256];
    CGeoParams *Geo;
    CDrawImage();
    ~CDrawImage();
    int createImage(int _dW,int _dH);
    int createImage(CGeoParams *_Geo);
    int printImage();
    int createGDPalette(CServerConfig::XMLE_Legend *palette);
    int createGDPalette();
    void drawVector(int x,int y,double direction, double strength,int color);
    void line(int x1,int y1,int x2,int y2,int color);
    void line(int x1,int y1,int x2,int y2,float w,int color);
    void setPixelIndexed(int x,int y,int color);
    void setPixelTrueColor(int x,int y,unsigned int color);
    void setPixelTrueColor(int x,int y,unsigned char r,unsigned char g,unsigned char b);
    void setPixelTrueColor(int x,int y,unsigned char r,unsigned char g,unsigned char b,unsigned char a);
    void getHexColorForColorIndex(CT::string *hexValue,int colorIndex);
    void setText(const char * text, size_t length,int x,int y, int color,int fontSize);
    void setTextStroke(const char * text, size_t length,int x,int y, int fgcolor,int bgcolor, int fontSize);
    void rectangle( int x1, int y1, int x2, int y2,int innercolor,int outercolor);
    void rectangle( int x1, int y1, int x2, int y2,int outercolor);
    int copyPalette();
    int addImage(int delay);
    int beginAnimation();
    int endAnimation();
    int addColor(int Color,unsigned char R,unsigned char G,unsigned char B);
    void enableTransparency(bool enable);
    void setBGColor(unsigned char R,unsigned char G,unsigned char B);
    void setTrueColor(bool enable);
    bool getTrueColor(){return _bEnableTrueColor;}
    void bla(const char * text, size_t length,double angle,int x,int y,int color,int fontSize);
    void setAntiAliased(bool enable){      _bEnableAGG=enable;   };
    bool getAntialiased(){return _bEnableAGG;}
    void drawText(int x,int y,float angle,int fontsize,const char *text,agg::rgba8 color);
    void setTTFFontLocation(const char *_TTFFontLocation){
      if(library!=NULL){
        FT_Done_FreeType(library);library=NULL;face=NULL;
      }
      TTFFontLocation=_TTFFontLocation;
    }
};

#endif

