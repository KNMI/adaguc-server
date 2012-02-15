#ifndef ADAGUC_USE_CAIRO
#include <png.h>
#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>
#include <freetype/ftstroke.h>
#include <math.h>
#include FT_FREETYPE_H
#include <CTypes.h>

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
      RGBAByteBuffer[p+0]=sr;
      RGBAByteBuffer[p+1]=sg;
      RGBAByteBuffer[p+2]=sb;
      RGBAByteBuffer[p+3]=tf;
    }
  }
};

class CXiaolinWuLine:public CPlotter{
  //Xiaolin Wu's line algorithm
  private:
    DEF_ERRORFUNCTION();
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
  
  void pixel(float x,float y){
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
  void pixel(int x,int y, unsigned char r,unsigned char g,unsigned char b,unsigned char a){
    if(x<0||y<0)return;
    if(x>=width||y>=height)return;
    size_t p=x+y*width;p*=4;
    RGBAByteBuffer[p+0]=r;
    RGBAByteBuffer[p+1]=g;
    RGBAByteBuffer[p+2]=b;
    RGBAByteBuffer[p+3]=a;

  }
  void pixelBlend(int x,int y, unsigned char r,unsigned char g,unsigned char b,unsigned char a){
    if(x<0||y<0)return;
    if(x>=width||y>=height)return;
    float a1=1-(float(a)/255);//*alpha;
    if(a1==0){

      size_t p=x+y*width;p*=4;
      RGBAByteBuffer[p+0]=r;
      RGBAByteBuffer[p+1]=g;
      RGBAByteBuffer[p+2]=b;
      RGBAByteBuffer[p+3]=255;
    }else{
      size_t p=x+y*width;p*=4;
      // ALpha is increased
      float sf=RGBAByteBuffer[p+3];  
      float alphaRatio=(1-sf/255);
      float tf=sf+a*alphaRatio;if(tf>255)tf=255;  
      if(sf==0.0f)a1=0;
      float a2=1-a1;//1-alphaRatio;
      //CDBDebug("Ratio: a1=%2.2f a2=%2.2f",a1,sf);
      
      float sr=RGBAByteBuffer[p+0];sr=sr*a1+r*a2;if(sr>255)sr=255;
      float sg=RGBAByteBuffer[p+1];sg=sg*a1+g*a2;if(sg>255)sg=255;
      float sb=RGBAByteBuffer[p+2];sb=sb*a1+b*a2;if(sb>255)sb=255;
      RGBAByteBuffer[p+0]=sr;
      RGBAByteBuffer[p+1]=sg;
      RGBAByteBuffer[p+2]=sb;
      RGBAByteBuffer[p+3]=tf;
    }

  }
  void rectangle(float x1,float y1,float x2,float y2){
    line_noaa(x1,y1,x2,y1);
    line_noaa(x2,y1,x2,y2+1);
    line_noaa(x2,y2,x1,y2);
    line_noaa(x1,y2,x1,y1);
  }
  void filledRectangle(float x1,float y1,float x2,float y2){
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
  void line_noaa(float x1,float y1,float x2,float y2) {
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
        plot(x,y,1);
        y+=gradient;
      }
    }else{
       for(int x=x1;x<x2;x++){
        plot(y,x,1);
        y+=gradient;
      }
    }
  }
  void line(float x1,float y1,float x2,float y2) {
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
        plot(xpxl1, ypxl1, rfpart(yend) * xgap);
        plot(xpxl1, ypxl1 + 1, fpart(yend) * xgap);
      }else{
        plot(ypxl1, xpxl1, rfpart(yend) * xgap);
        plot(ypxl1 + 1, xpxl1, fpart(yend) * xgap);
      }
      float intery = yend + gradient; // first y-intersection for the main loop
      
      // handle second endpoint
      xend = int (x2+0.5);
      yend = y2 + gradient * (xend - x2);
      xgap = fpart(x2 + 0.5);
      float xpxl2 = xend;  // this will be used in the main loop
      float ypxl2 = int(yend);
      if(xyIsSwapped==0){
        plot (xpxl2, ypxl2, rfpart (yend) * xgap);
        plot (xpxl2, ypxl2 + 1, fpart (yend) * xgap);
      }else{
        plot (ypxl2, xpxl2, rfpart (yend) * xgap);
        plot (ypxl2 + 1, xpxl2, fpart (yend) * xgap);
      }
      
      // main loop
      if(xyIsSwapped==0){
        for(float x = xpxl1 + 1;x< xpxl2 ;x++){
            plot (x, int(intery), rfpart (intery));
            plot (x, int(intery) + 1, fpart (intery));
            intery = intery + gradient;
        }
      }else for(float x = xpxl1 + 1;x< xpxl2;x++){
          plot (int(intery), x,rfpart (intery));
          plot (int(intery) + 1, x,fpart (intery));
          intery = intery + gradient;
      }
  }
};


class CFreeType{
  private:
    DEF_ERRORFUNCTION();
    float fontSize;
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
            alpha/=255;
            
            //r=255;g=255;b=255;
            //plot( x+left,  y+top, alpha);
            //r=0;g=0;b=0;
            wuLine->plot( x+left,  y+top, alpha);
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
        CDBError("the font file %s could be opened and read, but it appears that its font format is unsupported",fontLocation);
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
    CXiaolinWuLine *wuLine;
  public:
    CFreeType(int width,int height,unsigned char* RGBAByteBuffer,float fontSize,const char *fontLocation){
      //this->RGBAByteBuffer=RGBAByteBuffer;
      //this->width=width;
      //this->height=height;
      this->fontSize=fontSize;
      wuLine = new CXiaolinWuLine(width,height,RGBAByteBuffer);
//      r=0;g=0;b=0;a=255;
      TTFFontLocation=fontLocation;
      library=NULL;
      face=NULL;
    }
    ~CFreeType(){
      if(library!=NULL){
        FT_Done_FreeType(library);library=NULL;face=NULL;
      }
      delete wuLine;
    }
    void setColor(unsigned char r,unsigned char g,unsigned char b,unsigned char a){
     wuLine->setColor(r,g,b,a);
    } 
    void setFillColor(unsigned char r,unsigned char g,unsigned char b,unsigned char a){
      wuLine->setFillColor(r,g,b,a);
    } 
    /*
     void line_noaa(float x1,float y1,float x2,float y2) {
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
        plot(x,y,1);
        y+=gradient;
      }
    }else{
       for(int x=x1;x<x2;x++){
        plot(y,x,1);
        y+=gradient;
      }
    }
  }*/
  
  int drawFilledText(int x,int y,float angle,const char *text){
    if(library==NULL){
      int status  = initializeFreeType();
      if(status != 0){
        return 1;
      }
    }
    
    FT_Matrix matrix; /* transformation matrix */
    /* set up matrix */
    matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
    matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
    matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
    matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L ); /* the pen position in 26.6 cartesian space coordinates */
    
    //Draw text :)
    int my_target_height = 8,error;
    int num_chars=strlen(text);
    int orgr=wuLine->r;
    int orgg=wuLine->g;
    int orgb=wuLine->b;
    int orga=wuLine->a;
    
    FT_Vector pen; /* untransformed origin */
    pen.x = x * 64; pen.y = ( my_target_height - y ) * 64;
    wuLine->setColor(255,255,255,0);
    wuLine->filledRectangle( pen.x/64-5, 
                     my_target_height-(pen.y)/64 +8,
                     (pen.x)/64,
                     my_target_height-(pen.y )/64-fontSize-4);
    for ( int n = 0; n < num_chars; n++ ) { /* set transformation */
      FT_Set_Transform( face, &matrix, &pen ); /* load glyph image into the  face->glyph (erase previous one) */
      error = FT_Load_Char( face, text[n], FT_LOAD_RENDER );
      if ( error ){CDBError("unable toFT_Load_Char");return 1;
      }
      /* now, draw to our target surface (convert position) */
      
      //setFillColor(255,255,255,100);
      
      wuLine->setColor(255,255,255,0);
      wuLine->filledRectangle( pen.x/64, 
                       my_target_height-(pen.y)/64 +5,
                       (pen.x+face->glyph->advance.x)/64,
                       my_target_height-(pen.y )/64-fontSize-4);
      wuLine->setColor(orgr,orgg,orgb,orga);
      renderFont( & face->glyph->bitmap,  face->glyph->bitmap_left, my_target_height -  face->glyph->bitmap_top );
      /* increment pen position */
      pen.x +=  face->glyph->advance.x; pen.y +=  face->glyph->advance.y;
    }
    wuLine->setColor(255,255,255,0);
    wuLine->filledRectangle( pen.x/64, 
                     my_target_height-(pen.y)/64 +5,
                     (pen.x)/64+5,
                     my_target_height-(pen.y )/64-fontSize-4);
    wuLine->setColor(orgr,orgg,orgb,orga);
    return 0;
  }
  
    int drawFreeTypeText(int x,int y,float angle,const char *text){
      //Draw text :)
      if(library==NULL){
        int status  = initializeFreeType();
        if(status != 0){
          return 1;
        }
        };
      int error;
      
      //
        FT_Stroker  stroker = NULL; 
      error = FT_Stroker_New( library, &stroker ); 
      
      //
      
      
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
      
      //line_noaa(matrix.xx+x,matrix.xy+y,matrix.yx+x,matrix.yy+y);
      /* start at (300,200) */ 
      pen.x = x * 64; pen.y = ( my_target_height - y ) * 64;

      for ( n = 0; n < num_chars; n++ ) { /* set transformation */  
        
        
        FT_Set_Transform( face, &matrix, &pen ); /* load glyph image into the slot (erase previous one) */  
        //error = FT_Load_Char( face, text[n], FT_LOAD_RENDER ); 
        //error = FT_Load_Glyph(face, text[n]-29, FT_LOAD_RENDER);
        int glyphIndex = FT_Get_Char_Index(face,(unsigned char)(text[n]));
        error = FT_Load_Glyph(face,glyphIndex, FT_LOAD_RENDER);

        if ( error ){
          CDBError("unable toFT_Load_Char");
          return 1;
        }
        /* now, draw to our target surface (convert position) */ 
        renderFont( &slot->bitmap, slot->bitmap_left, my_target_height - slot->bitmap_top ); 
        /* increment pen position */  
        //char t[2];t[1]=0;t[0]=text[n];
        //printf("%s %d\n",t,face->glyph->linearHoriAdvance);
        //plot(slot->bitmap_left, my_target_height - slot->bitmap_top, 1);
        
        pen.x += slot->advance.x; pen.y += slot->advance.y; 
      }
      return 0;
    }
};
int writeRGBAPng(int width,int height,unsigned char *aggBytebuf,FILE *file,bool trueColor);

#endif