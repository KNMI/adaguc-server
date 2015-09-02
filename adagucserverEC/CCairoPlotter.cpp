/******************************************************************************
 * 
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Ernst de Vreede vreedede "at" knmi.nl
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

#include "CCairoPlotter.h"
#ifdef ADAGUC_USE_CAIRO

const char *CCairoPlotter::className="CCairoPlotter";

cairo_status_t writerFunc(void *closure, const unsigned char *data, unsigned int length) {
  FILE *fp=(FILE *)closure;
  int nrec=fwrite(data, length, 1, fp);
  if (nrec==1) {
    return CAIRO_STATUS_SUCCESS;
  }

  return CAIRO_STATUS_WRITE_ERROR;
}

  void CCairoPlotter::_swap(float &x,float &y){
    float a=x; x=y;  y=a;
  }
  void CCairoPlotter::_swap(int &x,int &y){
    int a=x; x=y;  y=a;
  }
  
void CCairoPlotter::_cairoPlotterInit(int width,int height,float fontSize, const char*fontLocation){
    this->width=width;
    this->height=height;
    this->fontSize=fontSize;
    this->fontLocation=fontLocation;
    stride=cairo_format_stride_for_width(FORMAT, width);
    
  
    
    surface=cairo_image_surface_create_for_data(ARGBByteBuffer, CCairoPlotter::FORMAT, width, height, stride);
    cr=cairo_create(this->surface);
//    fprintf(stderr, "cairo status: %s\n", cairo_status_to_string(cairo_status(cr)));
    r=0;g=0;b=0;a=255;
    rr=r/256.l; rg=g/256.;rb=b/256.;ra=1;
    fr=0;fg=0;fb=0;fa=1;
    rfr=rfg=rfb=0;rfa=1;
    
    library=NULL;
    initializationFailed=false;
    
    initFont();
    //CDBDebug("constructor");
  }

void CCairoPlotter::pixelBlend(int x,int y, unsigned char newR,unsigned char newG,unsigned char newB,unsigned char newA){
  if(x<0||y<0)return;
  if(x>=width||y>=height)return;
  size_t p=x*4+y*stride;
  if(newA!=255){
    float oldB = float(ARGBByteBuffer[p]);
    float oldG = float(ARGBByteBuffer[p+1]);
    float oldR = float(ARGBByteBuffer[p+2]);
    float oldA = float(ARGBByteBuffer[p+3]);
    
    //float alpha = float(newA)/255.;
  
    //float nalpha = 1-alpha;
    
    
    float alphaRatio=(1-oldA/255);
    float tf=oldA+float(newA)*alphaRatio;if(tf>255)tf=255;  
    float a1=1-(float(newA)/255);//*alpha;
    if(oldA==0.0f)a1=0;
    float a2=1-a1;//1-alphaRatio;
    
    ARGBByteBuffer[p]  =(unsigned char)(float(newB)*a2+oldB*a1);
    ARGBByteBuffer[p+1]=(unsigned char)(float(newG)*a2+oldG*a1);
    ARGBByteBuffer[p+2]=(unsigned char)(float(newR)*a2+oldR*a1);
    ARGBByteBuffer[p+3]=(unsigned char)tf;//float(newA)*alpha+oldA*nalpha;
    //pixel(x,y,ARGBByteBuffer[p+2], ARGBByteBuffer[p+1], ARGBByteBuffer[p], ARGBByteBuffer[p+3]);
  }else{
  
  
    ARGBByteBuffer[p]=newB;
    ARGBByteBuffer[p+1]=newG;
    ARGBByteBuffer[p+2]=newR;
    ARGBByteBuffer[p+3]=newA;
  }
}
  
    void CCairoPlotter::pixel(int x,int y, unsigned char r,unsigned char g,unsigned char b,unsigned char a){
    if(x<0||y<0)return;
    if(x>=width||y>=height)return;
    size_t p=x*4+y*stride;
    if(a!=255){
    ARGBByteBuffer[p]=(unsigned char)((float(b)/256.0)*float(a));
    ARGBByteBuffer[p+1]=(unsigned char)((float(g)/256.0)*float(a));
    ARGBByteBuffer[p+2]=(unsigned char)((float(r)/256.0)*float(a));
    ARGBByteBuffer[p+3]=a;
    }else{
      ARGBByteBuffer[p]=b;
    ARGBByteBuffer[p+1]=g;
    ARGBByteBuffer[p+2]=r;
    ARGBByteBuffer[p+3]=a;
    }
  }
  
  
  
  
  void  CCairoPlotter::_plot(int x, int y, float alpha){
//    fprintf(stderr, "plot([%d,%d], %d,%d,%d,%f)\n", x, y, r, g, b,a);
//     
    cairo_surface_flush(surface);
    //plot the pixel at (x, y) with brightness c (where 0 ≤ c ≤ 1)
    if(x<0||y<0)return;
    if(x>=width||y>=height)return;
    size_t p=x*4+y*stride;
    float a1=1-(a/255)*alpha;
    if(a1==0){
      ARGBByteBuffer[p]=b;
      ARGBByteBuffer[p+1]=g;
      ARGBByteBuffer[p+2]=r;
      ARGBByteBuffer[p+3]=255;
    }else{
      // ALpha is increased
      float sf=ARGBByteBuffer[p+3];
      float alphaRatio=(alpha*(1-sf/255));
      float tf=sf+a*alphaRatio;if(tf>255)tf=255;
      float a2=1-a1;//1-alphaRatio;
      float sr=ARGBByteBuffer[p+2];sr=sr*a1+r*a2;if(sr>255)sr=255;
      float sg=ARGBByteBuffer[p+1];sg=sg*a1+g*a2;if(sg>255)sg=255;
      float sb=ARGBByteBuffer[p];sb=sb*a1+b*a2;if(sb>255)sb=255;
      ARGBByteBuffer[p]=(unsigned char)sb;
      ARGBByteBuffer[p+1]=(unsigned char)sg;
      ARGBByteBuffer[p+2]=(unsigned char)sr;
      ARGBByteBuffer[p+3]=(unsigned char)tf;
    }
    cairo_surface_mark_dirty(surface);
  }
  
  
  
  CCairoPlotter::CCairoPlotter(int width,int height, float fontSize, const char*fontLocation,unsigned char r,unsigned char g,unsigned char b,unsigned char a){
    byteBufferPointerIsOwned = true;
    stride=cairo_format_stride_for_width(FORMAT, width);
    size_t bufferSize = size_t(height)*stride;
    ARGBByteBuffer = new unsigned char[bufferSize];
    if(a==0&&r==0&&g==0&&b==0){
      for(size_t j=0;j<bufferSize;j++){
        ARGBByteBuffer[j]=0;
      }
    }else{
      for(size_t j=0;j<bufferSize/4;j++){
        ARGBByteBuffer[j*4+3]=a;
        ARGBByteBuffer[j*4+2]=r;
        ARGBByteBuffer[j*4+1]=g;
        ARGBByteBuffer[j*4+0]=b;
      }
    }
    _cairoPlotterInit(width,height,fontSize,fontLocation);
  }
  
  CCairoPlotter::CCairoPlotter(int width,int height, unsigned char * _ARGBByteBuffer, float fontSize, const char*fontLocation){
    byteBufferPointerIsOwned = false;
    this->width=width;
    this->height=height;
    this->fontSize=fontSize;
    this->fontLocation=fontLocation;
    stride=cairo_format_stride_for_width(FORMAT, width);
    //size_t bufferSize = size_t(height)*stride;
    //ARGBByteBuffer = new unsigned char[bufferSize];
    this->ARGBByteBuffer = (_ARGBByteBuffer);
    //for(size_t j=0;j<bufferSize;j++)ARGBByteBuffer[j]=0;
    surface=cairo_image_surface_create_for_data(ARGBByteBuffer, CCairoPlotter::FORMAT, width, height, stride);
    cr=cairo_create(this->surface);
    library=NULL;
    initializationFailed=false;
    
    initFont();
   
  }
  

  CCairoPlotter::~CCairoPlotter() {
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    if(byteBufferPointerIsOwned){
      delete[] ARGBByteBuffer;
      ARGBByteBuffer= NULL;
    }
  }

  int CCairoPlotter::renderFont(FT_Bitmap *bitmap,int left,int top){
    for(int y=0;y<(int)bitmap->rows;y++){
      for(int x=0;x<(int)bitmap->width;x++){
        size_t p=(x+y*bitmap->width);
        if(bitmap->buffer[p]!=0){
          float alpha=bitmap->buffer[p];
          alpha/=256;

          //r=255;g=255;b=255;
          //plot( x+left,  y+top, alpha);
          //r=0;g=0;b=0;
          _plot( x+left,  y+top, alpha);
        }
      }
    }
    return 0;
  }
   int CCairoPlotter::initializeFreeType(){
    //CDBDebug("initializeFreeType(%d)\n", library==NULL);
    if(library!=NULL){
      CDBError("Freetype is already intialized");
      return 1;
    };
    int error = FT_Init_FreeType( &library );
    const char * fontLocation = this->fontLocation;
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
                              int(fontSize*64), /* char_height in 1/64th of points */
                              100, /* horizontal device resolution */
                              100 ); /* vertical device resolution */
    if ( error ) {
      CDBError("unable to set character size");
      if(library!=NULL){FT_Done_FreeType(library);library=NULL;face=NULL; }
      return 1;
    }
    return 0;
  }
      int CCairoPlotter::_drawFreeTypeText(int x,int y,int &w,int &h,float angle,const char *text,bool render){
      //Draw text :)
      w=0;h=0;
      if(library==NULL){
        int status  = initializeFreeType();
        if(status != 0){
          return 1;
        }
        };
      int error;
      
      //
//         FT_Stroker  stroker = NULL; 
//       error = FT_Stroker_New( library, &stroker ); 
      
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
        if(render){
          renderFont( &slot->bitmap, slot->bitmap_left, my_target_height - slot->bitmap_top ); 
        }
        /* increment pen position */  
        //char t[2];t[1]=0;t[0]=text[n];
        //printf("%s %d\n",t,face->glyph->linearHoriAdvance);
        //plot(slot->bitmap_left, my_target_height - slot->bitmap_top, 1);
        
        if (int(slot->bitmap.rows) > h) h=(int)slot->bitmap.rows;
        
        pen.x += slot->advance.x; pen.y += slot->advance.y; 
        w += slot->advance.x/64;
//        h += slot->advance.y/64;
      }
   
      return 0;
    }
    
  int CCairoPlotter::getTextSize(int &w,int &h,float angle,const char *text){
    return _drawFreeTypeText(0,0,w,h,angle,text,false);
  }

  int CCairoPlotter::drawAnchoredText(int x, int y, float angle,const char *text, int anchor){
    int w,h;
    getTextSize(w, h, angle, text);
 //   CDBDebug("[w,h]=>[%d,%d] %s at [%d,%d] %d,%d\n", w, h, text, x, y, anchor, anchor % 4);
    switch(anchor%4) {
      case 0:
        _drawFreeTypeText( x, y, w, h, angle,text, true);
        break;
      case 1:
        _drawFreeTypeText( x, y+h, w, h,angle,text, true);
        break;
      case 2:
        _drawFreeTypeText( x-w, y+h, w, h,angle,text, true);
        break;
      case 3:
        _drawFreeTypeText( x-w, y, w, h,angle,text, true);
        break;
    }        
    return 0;
  }
  
  int CCairoPlotter::drawCenteredText(int x, int y, float angle,const char *text){
    int w,h;
    getTextSize(w, h, angle, text);
    //CDBDebug("[w,h]=>[%d,%d] at [%d,%d]\n", w, h, x, y);
    return _drawFreeTypeText( x-w/2, y+h/2,w,h,angle,text,true);
  }

  int CCairoPlotter::drawFilledText(int x,int y,float angle,const char *text){
    if(text==NULL)return 0;
    if(strlen(text)==0)return 0;
    if(initializationFailed==true)return 1;
    if(library==NULL){
      int status  = initializeFreeType();
      if(status != 0){
        initializationFailed=true;
        return 1;
      }
    };
    
    FT_Matrix matrix; /* transformation matrix */
    /* set up matrix */
    matrix.xx = (FT_Fixed)( cos( angle ) * 0x10000L );
    matrix.xy = (FT_Fixed)(-sin( angle ) * 0x10000L );
    matrix.yx = (FT_Fixed)( sin( angle ) * 0x10000L );
    matrix.yy = (FT_Fixed)( cos( angle ) * 0x10000L ); /* the pen position in 26.6 cartesian space coordinates */

    //Draw text :)
    int my_target_height = 8,error;
    int num_chars=strlen(text);
    int orgr=this->r;
    int orgg=this->g;
    int orgb=this->b;
    int orga=int(this->a);
    
    FT_Vector pen; /* untransformed origin */
    pen.x = x * 64; pen.y = ( my_target_height - y ) * 64;
    setColor(255,255,255,0);
    filledRectangle( pen.x/64-5, 
                     my_target_height-(pen.y)/64 +8,
                     (pen.x)/64,
                     my_target_height-(pen.y )/64-int(fontSize)-4);
    for ( int n = 0; n < num_chars; n++ ) { /* set transformation */
      FT_Set_Transform( face, &matrix, &pen ); /* load glyph image into the  face->glyph (erase previous one) */
      error = FT_Load_Char( face, text[n], FT_LOAD_RENDER );
      if ( error ){CDBError("unable toFT_Load_Char");return 1;
      }
      /* now, draw to our target surface (convert position) */
      
      //setFillColor(255,255,255,100);
     
      setColor(255,255,255,0);
      filledRectangle( pen.x/64, 
                       my_target_height-(pen.y)/64 +5,
                       (pen.x+face->glyph->advance.x)/64,
                       my_target_height-(pen.y )/64-int(fontSize)-4);
      setColor(orgr,orgg,orgb,orga);
      renderFont( & face->glyph->bitmap,  face->glyph->bitmap_left, my_target_height -  face->glyph->bitmap_top );
      /* increment pen position */
      pen.x +=  face->glyph->advance.x; pen.y +=  face->glyph->advance.y;
    }
    setColor(255,255,255,0);
    filledRectangle( pen.x/64, 
                 my_target_height-(pen.y)/64 +5,
                 (pen.x)/64+5,
                 my_target_height-(pen.y )/64-int(fontSize)-4);
    setColor(orgr,orgg,orgb,orga);
    return 0;
  }

  void CCairoPlotter::initFont() {
  }

  void CCairoPlotter::setColor(unsigned char r,unsigned char g,unsigned char b,unsigned char a){
    this->r=r;
    rr=r/256.;
    this->g=g;
    rg=g/256.;
    this->b=b;
    rb=b/256.;
    this->a=(float)a;
    ra=a/256.;
  }

  void CCairoPlotter::setFillColor(unsigned char r,unsigned char g,unsigned char b,unsigned char a){
    fr=r;
    rfr=r/256.;
    fg=g;
    rfg=g/256.;
    fb=b;
    rfb=b/256.;
    fa=float(a);
    rfa=a/256.;
  }

  void CCairoPlotter::pixel(int x,int y){
    _plot( x, y, 1);
  }

  
  
  void CCairoPlotter::pixel(int x,int y, unsigned char r,unsigned char g,unsigned char b){
    if(x<0||y<0)return;
    if(x>=width||y>=height)return;
    size_t p=x*4+y*stride;
    ARGBByteBuffer[p]=b;
    ARGBByteBuffer[p+1]=g;
    ARGBByteBuffer[p+2]=r;
    ARGBByteBuffer[p+3]=255;
  }
  
   
  
  void CCairoPlotter::getPixel(int x,int y, unsigned char &r,unsigned char &g,unsigned char &b,unsigned char &a){
    if(x<0||y<0||x>=width||y>=height){
      r=0;b=0;g=0;a=0;
    }
    size_t p=x*4+y*stride;
    b=ARGBByteBuffer[p];
    g=ARGBByteBuffer[p+1];
    r=ARGBByteBuffer[p+2];
    a=ARGBByteBuffer[p+3];
  }
  
  
  unsigned char *CCairoPlotter::getByteBuffer(){
    return ARGBByteBuffer;
  }
/*
  void flush() {
    cairo_surface_flush(surface);
  }*/

  void CCairoPlotter::rectangle(int x1,int y1,int x2,int y2){
    cairo_rectangle(cr, x1+0.5, y1+0.5, x2-x1, y2-y1);
    cairo_set_source_rgba(cr, rr, rg, rb, a);
    cairo_stroke(cr);
  }

  void CCairoPlotter::filledRectangle(int x1,int y1,int x2,int y2){
    if(y1>y2)_swap(y1,y2);
    
    cairo_rectangle(cr, x1, y1, x2-x1, y2-y1);
    cairo_set_source_rgba(cr, rfr, rfg, rfb, rfa);
    cairo_fill(cr);
    cairo_antialias_t aa=cairo_get_antialias(cr);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
    cairo_set_line_width(cr, 0.5);
    rectangle(x1,y1,x2,y2);
    cairo_set_antialias(cr, aa);
  }

  void CCairoPlotter::line_noaa(int x1,int y1,int x2,int y2) {
    cairo_set_source_rgba(cr, rr, rg, rb, ra);
    cairo_antialias_t aa=cairo_get_antialias(cr);
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
    cairo_move_to(cr, x1, y1);
    cairo_line_to(cr, x2, y2);
    cairo_set_line_width(cr, 0.9);
    cairo_stroke(cr);
    cairo_set_antialias(cr, aa);
  }


  void CCairoPlotter::moveTo(float x1,float y1) {
    cairo_move_to(cr, x1+0.5, y1+0.5);
  }
  void CCairoPlotter::lineTo(float x1,float y1) {
    cairo_set_source_rgba(cr, rr, rg, rb, ra);
    cairo_line_to(cr, x1+0.5, y1+0.5);
    cairo_set_line_width(cr, 0.9);
    cairo_stroke(cr);
  }
  
  void CCairoPlotter::lineTo(float x1,float y1,float width) {
    cairo_set_source_rgba(cr, rr, rg, rb, ra);
    cairo_set_line_width(cr,width);
    cairo_line_to(cr, x1+0.5, y1+0.5);

  }
  void CCairoPlotter::endLine(){
    cairo_set_line_join (cr, CAIRO_LINE_JOIN_MITER); 
    cairo_stroke(cr);
  }
  
  void CCairoPlotter::line(float x1,float y1,float x2,float y2) {
    cairo_set_source_rgba(cr, rr, rg, rb, ra);
    cairo_move_to(cr, x1+0.5, y1+0.5);
    cairo_line_to(cr, x2+0.5, y2+0.5);
    cairo_set_line_width(cr, 0.9);
    cairo_stroke(cr);
  }
  void CCairoPlotter::line(float x1,float y1,float x2,float y2,float width) {
    cairo_set_source_rgba(cr, rr, rg, rb, ra);
    cairo_move_to(cr, x1+0.5, y1+0.5);
    cairo_line_to(cr, x2+0.5, y2+0.5);
    cairo_set_line_width(cr, width);
    cairo_stroke(cr);
  }
//  /*cairo_status_t writeToPng(const char* fileName) {
//    return cairo_surface_write_to_png(surface, fileName);
//  }*/

  void CCairoPlotter::circle(int x, int y, int r) {
    circle(x, y, r, 1);
/*    
    cairo_set_line_width(cr, 1);
    cairo_arc(cr, x, y, r, 0, 2*M_PI);
    cairo_stroke(cr);
*/
  }

  void CCairoPlotter::filledcircle(int x, int y, int r) {
    cairo_save(cr);
//    cairo_set_line_width(cr, 1);
//   cairo_set_source_rgba(cr, rfr, rfg, rfb, rfa);
//    cairo_arc(cr, x, y, r, 0, 2*M_PI);
//    cairo_stroke_preserve(cr);
    cairo_set_source_rgba(cr, rfr, rfg, rfb, rfa);
    cairo_arc(cr, x, y, r, 0, 2*M_PI);
    cairo_fill(cr);
    cairo_restore(cr);
  }

  void CCairoPlotter::circle(int x, int y, int r,float lineWidth) {
    double lWx=lineWidth;
    double lWy=lineWidth;
    cairo_device_to_user_distance(cr, &lWx, &lWy);
    cairo_set_source_rgba(cr, rr, rg, rb, ra);
    cairo_set_line_width(cr, lWx);
    cairo_arc(cr, x, y, r, 0, 2*M_PI);
    cairo_stroke(cr);
  }
  
  void CCairoPlotter::poly(float x[], float y[], int n, bool closePath, bool fill) {
    cairo_move_to(cr, x[0], y[0]);
    for (int i=1; i<n; i++){
      cairo_line_to(cr, x[i], y[i]);
    }
    if (closePath) {
      cairo_close_path(cr);
      if (fill) {
        cairo_set_source_rgba(cr, rfr, rfg, rfb, rfa);
        cairo_fill_preserve(cr);
      }
      cairo_set_source_rgba(cr, rr, rg, rb ,ra);
      cairo_stroke(cr);
    }
  }

  void CCairoPlotter::poly(float x[], float y[], int n, float lineWidth, bool closePath, bool fill) {
    double lWx=lineWidth;
    double lWy=lineWidth;
    cairo_device_to_user_distance(cr, &lWx, &lWy);
    cairo_set_line_width(cr, lWx);

    cairo_move_to(cr, x[0], y[0]);
    for (int i=1; i<n; i++){
      cairo_line_to(cr, x[i], y[i]);
    }
    if (closePath) {
      cairo_close_path(cr);
      if (fill) {
        cairo_set_source_rgba(cr, rfr, rfg, rfb, rfa);
        cairo_fill_preserve(cr);
      }
      cairo_set_source_rgba(cr, rr, rg, rb ,ra);
      cairo_stroke(cr);
    }
  }
    
  void CCairoPlotter::drawText(int x, int y,double angle, const char *text) {
    int w,h;
    _drawFreeTypeText( x, y,w,h,angle,text,true);
  }

  void CCairoPlotter::writeToPngStream(FILE *fp) {
    cairo_surface_flush(surface);
    this->fp=fp;
    cairo_surface_write_to_png_stream(surface, writerFunc, (void *)fp);
  }
  
  void CCairoPlotter::writeToPngStream(FILE *fp,float alpha) {
    cairo_set_operator (cr,CAIRO_OPERATOR_DEST_IN);
    cairo_paint_with_alpha (cr, alpha);
    cairo_surface_flush(surface);
    this->fp=fp;
    cairo_surface_write_to_png_stream(surface, writerFunc, (void *)fp);
  }
  
  void CCairoPlotter::setToSurface(cairo_surface_t *png) {
    cairo_set_source_surface (this->cr, png, 0, 0);
    cairo_paint(this->cr);
 //   cairo_surface_destroy(surface);
  }
#endif
