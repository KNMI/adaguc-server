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
class CPlotter{
  public:
  unsigned char* RGBAByteBuffer;
  int width, height;
  unsigned char r,g,b;float a;
  void plot(int x, int y, float alpha){
    //plot the pixel at (x, y) with brightness c (where 0 ≤ c ≤ 1)
    if(x<0||y<0)return;
    if(x>=width||y>=height)return;
    size_t p=x+y*width;p*=4;
    float a1=1-(a/255)*alpha;
    if(a1==0){
      RGBAByteBuffer[p+0]=r;
      RGBAByteBuffer[p+1]=g;
      RGBAByteBuffer[p+2]=b;
      RGBAByteBuffer[p+3]=255;
    }else{
      // ALpha is increased
      float sf=RGBAByteBuffer[p+3];  
      float alphaRatio=(alpha*(1-sf/255));
      float tf=sf+a*alphaRatio;if(tf>255)tf=255;  
      float a2=1-a1;//1-alphaRatio;
      float sr=RGBAByteBuffer[p+0];sr=sr*a1+r*a2;if(sr>255)sr=255;
      float sg=RGBAByteBuffer[p+1];sg=sg*a1+g*a2;if(sg>255)sg=255;
      float sb=RGBAByteBuffer[p+2];sb=sb*a1+b*a2;if(sb>255)sb=255;
      RGBAByteBuffer[p+0]=(unsigned char)sr;
      RGBAByteBuffer[p+1]=(unsigned char)sg;
      RGBAByteBuffer[p+2]=(unsigned char)sb;
      RGBAByteBuffer[p+3]=(unsigned char)tf;
    }
  }
};

class CXiaolinWuLine:private CPlotter{
  //Xiaolin Wu's line algorithm
  private:
 
  unsigned char fr,fg,fb;float fa;
  
  float fpart(float x) {
    double a;
    return modf (x , &a);
  }

  float rfpart(float x) {;
      return 1.0 - fpart(x);
  }

  void swap(float &x,float &y){
    float a=x; x=y;  y=a;
  }
  void swap(int &x,int &y){
    int a=x; x=y;  y=a;
  }
  public:
  CXiaolinWuLine(int width,int height,unsigned char* RGBAByteBuffer){
    this->RGBAByteBuffer=RGBAByteBuffer;
    this->width=width;
    this->height=height;
    r=0;g=0;b=0;a=255;
    fr=0;fg=0;fb=0;fa=255;
  }
  void setColor(unsigned char r,unsigned char g,unsigned char b,unsigned char a){
    this->r=r;
    this->g=g;
    this->b=b;
    this->a=(float)a;
  }
  void setFillColor(unsigned char r,unsigned char g,unsigned char b,unsigned char a){
    this->fr=r;
    this->fg=g;
    this->fb=b;
    this->fa=(float)a;
  }
  
  void pixel(int x,int y){
    plot( x, y, 1);
  }
  void pixel(int x,int y, unsigned char r,unsigned char g,unsigned char b){
    if(x<0||y<0)return;
    if(x>=width||y>=height)return;
    size_t p=x+y*width;p*=4;
    RGBAByteBuffer[p+0]=r;
    RGBAByteBuffer[p+1]=g;
    RGBAByteBuffer[p+2]=b;
    RGBAByteBuffer[p+3]=255;

  }
  void rectangle(int x1,int y1,int x2,int y2){
    line_noaa(x1,y1,x2,y1);
    line_noaa(x2,y1,x2,y2+1);
    line_noaa(x2,y2,x1,y2);
    line_noaa(x1,y2,x1,y1);
  }
  void filledRectangle(int x1,int y1,int x2,int y2){
    if(y1>y2)swap(y1,y2);
    int h=y2-y1;
    unsigned char tr,tg,tb;float ta;
    
    tr=r;tg=g;tb=b;ta=a;
    r=fr;g=fg;b=fb;a=fa;
    
    for(int j=0;j<h;j++){
      line_noaa(x1,y1+j,x2,y1+j);
    }
    r=tr;g=tg;b=tb;a=ta;
    rectangle(x1,y1,x2,y2);
  }
  void line_noaa(int x1,int y1,int x2,int y2) {
    int xyIsSwapped=0;
    float dx = x2 - x1;
    float dy = y2 - y1;
    if(fabs(dx) < fabs(dy)){
      swap(x1, y1);
      swap(x2, y2);
      swap(dx, dy);
      xyIsSwapped=1;
    }
    if(x2 < x1){
      swap(x1, x2);
      swap(y1, y2);
    }
    float gradient = dy / dx;
    float y=y1;
    if(xyIsSwapped==0){
      for(int x=x1;x<x2;x++){
        plot((int)x,(int)y,1);
        y+=gradient;
      }
    }else{
       for(int x=x1;x<x2;x++){
        plot((int)y,(int)x,1);
        y+=gradient;
      }
    }
  }
  void line(float x1,float y1,float x2, float y2) {
      //Xiaolin Wu's line algorithm
      float dx = x2 - x1;
      float dy = y2 - y1;
      int xyIsSwapped=0;
      if(fabs(dx) < fabs(dy)){
        swap(x1, y1);
        swap(x2, y2);
        swap(dx, dy);
        xyIsSwapped=1;
      }
      if(x2 < x1){
        swap(x1, x2);
        swap(y1, y2);
      }
      float gradient = dy / dx;
      
      // handle first endpoint
      float xend = int(x1+0.5);
      float yend = y1 + gradient * (xend - x1);
      float xgap = rfpart(x1 + 0.5);
      float xpxl1 = xend ; // this will be used in the main loop
      float ypxl1 = int(yend);
      if(xyIsSwapped==0){
        plot((int)xpxl1, (int)ypxl1, rfpart(yend) * xgap);
        plot((int)xpxl1, (int)ypxl1 + 1, fpart(yend) * xgap);
      }else{
        plot((int)ypxl1, (int)xpxl1, rfpart(yend) * xgap);
        plot((int)ypxl1 + 1, (int)xpxl1, fpart(yend) * xgap);
      }
      float intery = yend + gradient; // first y-intersection for the main loop
      
      // handle second endpoint
      xend = int (x2+0.5);
      yend = y2 + gradient * (xend - x2);
      xgap = fpart(x2 + 0.5);
      float xpxl2 = xend;  // this will be used in the main loop
      float ypxl2 = int(yend);
      if(xyIsSwapped==0){
        plot ((int)xpxl2, (int)ypxl2, rfpart (yend) * xgap);
        plot ((int)xpxl2, (int)ypxl2 + 1, fpart (yend) * xgap);
      }else{
        plot ((int)ypxl2, (int)xpxl2, rfpart (yend) * xgap);
        plot ((int)ypxl2 + 1, (int)xpxl2, fpart (yend) * xgap);
      }
      
      // main loop
      if(xyIsSwapped==0){
        for(float x = xpxl1 + 1;x< xpxl2 ;x++){
            plot ((int)x, int(intery), rfpart (intery));
            plot ((int)x, int(intery) + 1, fpart (intery));
            intery = intery + gradient;
        }
      }else for(float x = xpxl1 + 1;x< xpxl2;x++){
          plot (int(intery), (int)x,rfpart (intery));
          plot (int(intery) + 1, (int)x,fpart (intery));
          intery = intery + gradient;
      }
  }
};


class CFreeType:private CPlotter{
  private:
    DEF_ERRORFUNCTION();
    int fontSize;
    bool initializationFailed;
    char *fontConfig;
    FT_Library library;
    FT_Face face;
    const char *TTFFontLocation;
    int renderFont(FT_Bitmap *bitmap,int left,int top){
      for(int y=0;y<bitmap->rows;y++){
        for(int x=0;x<bitmap->width;x++){
          size_t p=(x+y*bitmap->width);
          if(bitmap->buffer[p]!=0){
            float alpha=bitmap->buffer[p];
            alpha/=256;
            
            //r=255;g=255;b=255;
            //plot( x+left,  y+top, alpha);
            //r=0;g=0;b=0;
            plot( x+left,  y+top, alpha);
          }
        }
      }
      return 0;
    }
     int initializeFreeType(){
      if(library!=NULL){
        CDBError("Freetype is already intialized");
        return 1;
      };
      int error = FT_Init_FreeType( &library );
      const char * fontLocation = TTFFontLocation;
      if ( error ) { 
        CDBError("an error occurred during freetype library initialization");
        if(library!=NULL){FT_Done_FreeType(library);library=NULL;face=NULL; }
        return 1;
      }
      error = FT_New_Face( library,fontLocation , 0, &face ); 
      if ( error == FT_Err_Unknown_File_Format ) { 
        CDBError("the font file could be opened and read, but it appears that its font format is unsupported");
        if(library!=NULL){FT_Done_FreeType(library);library=NULL;face=NULL; }
        return 1;
      } else if ( error ) {
        CDBError("Unable to initialize freetype: Could not read fontfile %s",fontLocation);
        if(library!=NULL){FT_Done_FreeType(library);library=NULL;face=NULL; }
        return 1;
      }
      
      error = FT_Set_Char_Size( face, /* handle to face object */  
                                0, /* char_width in 1/64th of points */ 
                                fontSize*64, /* char_height in 1/64th of points */ 
                                100, /* horizontal device resolution */
                                100 ); /* vertical device resolution */
      if ( error ) {
        CDBError("unable to set character size");
        if(library!=NULL){FT_Done_FreeType(library);library=NULL;face=NULL; }
        return 1;
      }
      return 0;
    }

  public:
    CFreeType(int width,int height,unsigned char* RGBAByteBuffer,int fontSize,const char *fontLocation){
      this->RGBAByteBuffer=RGBAByteBuffer;
      this->width=width;
      this->height=height;
      this->fontSize=fontSize;
      r=0;g=0;b=0;a=255;
      TTFFontLocation=fontLocation;
      library=NULL;
      face=NULL;
      initializationFailed=false;
    }
    ~CFreeType(){
      if(library!=NULL){
        FT_Done_FreeType(library);library=NULL;face=NULL;
      }
    }
    void setColor(unsigned char r,unsigned char g,unsigned char b,unsigned char a){
      this->r=r;
      this->g=g;
      this->b=b;
      this->a=(float)a;
    }    
    int drawFreeTypeText(int x,int y,float angle,const char *text){
      //Draw text :)
      if(initializationFailed==true)return 1;
      if(library==NULL){
        int status  = initializeFreeType();
        if(status != 0){
          initializationFailed=true;
          return 1;
        }
      };
      int error;
      FT_GlyphSlot slot; FT_Matrix matrix; /* transformation matrix */
      FT_Vector pen; /* untransformed origin */
      int n;
      int my_target_height = 8;
      int num_chars=strlen(text);
      slot = face->glyph; /* a small shortcut */
      /* set up matrix */
      matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
      matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
      matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
      matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L ); /* the pen position in 26.6 cartesian space coordinates */
      /* start at (300,200) */ 
      pen.x = x * 64; pen.y = ( my_target_height - y ) * 64;
      for ( n = 0; n < num_chars; n++ ) { /* set transformation */  
        FT_Set_Transform( face, &matrix, &pen ); /* load glyph image into the slot (erase previous one) */  
        error = FT_Load_Char( face, text[n], FT_LOAD_RENDER ); 
        if ( error ){
          CDBError("unable toFT_Load_Char");
          return 1;
        }
        /* now, draw to our target surface (convert position) */ 
        renderFont( &slot->bitmap, slot->bitmap_left, my_target_height - slot->bitmap_top ); 
        /* increment pen position */  
        pen.x += slot->advance.x; pen.y += slot->advance.y; 
      }
      return 0;
    }
};
#endif //ADAGUC_USE_CAIRO

class CLegend{
public:
  int id;
  unsigned char CDIred[256],CDIgreen[256],CDIblue[256],CDIalpha[256];//Currently alpha of 0 and 255 is supported, but nothin in between.
  CT::string legendName;
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
    bool _bAntiAliased;
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
    int TTFFontSize;
    char *fontConfig ;
  public:
    
    int _colors[256];
    gdImagePtr image;
    
    int colors[256];
    CGeoParams *Geo;
    CDrawImage();
    ~CDrawImage();
    int createImage(int _dW,int _dH);
    int createImage(CGeoParams *_Geo);
    int printImagePng();
    int printImageGif();
    int createGDPalette(CServerConfig::XMLE_Legend *palette);

    int create685Palette();
    
    void drawBarb(int x,int y,double direction, double strength,int color,bool toKnots,bool flip);
    void drawText(int x,int y,float angle,const char *text,unsigned char colorIndex);
    void drawTextAngle(const char * text, size_t length,double angle,int x,int y,int color,int fontSize);
    void drawVector(int x,int y,double direction, double strength,int color);
    void destroyImage();
    void line(float x1,float y1,float x2,float y2,int color);
    void line(float x1,float y1,float x2,float y2,float w,int color);
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
    int copyPalette();
    int addImage(int delay);
    int beginAnimation();
    int endAnimation();
    int addColor(int Color,unsigned char R,unsigned char G,unsigned char B);
    void enableTransparency(bool enable);
    void setBGColor(unsigned char R,unsigned char G,unsigned char B);
    void setTrueColor(bool enable);
    bool getTrueColor(){return _bEnableTrueColor;}
    
    void setAntiAliased(bool enable){      _bAntiAliased=enable;   };
    bool getAntialiased(){return _bAntiAliased;}
    
    void setTTFFontLocation(const char *_TTFFontLocation){TTFFontLocation=_TTFFontLocation;  }
    void setTTFFontSize(int _TTFFontSize){  TTFFontSize=_TTFFontSize; }
};

#endif

