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

#include "Definitions.h"
#ifdef ADAGUC_USE_CAIRO
/*
 * CCairoPlotter2.h
 *
 *  Created on: Nov 11, 2011
 *      Author: vreedede
 */

#ifndef CCAIROPLOTTER_H_
#define CCAIROPLOTTER_H_
#include <png.h>
#include <cairo.h>
#include "CDebugger.h"
#include "CTypes.h"
#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <freetype/ftoutln.h>
#include <freetype/fttrigon.h>
#include FT_FREETYPE_H
#include <stdio.h>
#include <math.h>

#include "COctTreeColorQuantizer.h"

cairo_status_t writerFunc(void *closure, const unsigned char *data, unsigned int length);
class CCairoPlotter {
private:
  DEF_ERRORFUNCTION();
  unsigned char* ARGBByteBuffer;
  float rr, rg,rb,ra;
  unsigned char fr,fg,fb;float fa;
  float rfr, rfb, rfg, rfa;
  cairo_surface_t *surface;
  cairo_t *cr;
  FILE *fp;
  int width, height,stride;
  float fontSize;
  const char *fontLocation;
  bool initializationFailed;
  FT_Library library;
  FT_Face face;
  unsigned char r,g,b;float a;
//   void _plot(int x, int y, float alpha);
  void _swap(float &x,float &y);
  void _swap(int &x,int &y);
  static const cairo_format_t FORMAT=CAIRO_FORMAT_ARGB32;
  bool byteBufferPointerIsOwned;
  void _cairoPlotterInit(int width,int height,float fontSize, const char*fontLocation);
  int _drawFreeTypeText(int x,int y,int &w,int &h,float angle,const char *text,bool render);
public:
  bool isAlphaUsed;
  
  CCairoPlotter(int width,int height, float fontSize, const char*fontLocation,unsigned char r,unsigned char g,unsigned char b,unsigned char a);
  CCairoPlotter(int width,int height, unsigned char * _ARGBByteBuffer, float fontSize, const char*fontLocation);

  ~CCairoPlotter() ;
  int writeARGBPng(int width,int height,unsigned char *ARGBByteBuffer,FILE *file,int bitDepth,bool use8bitpalAlpha);
  int renderFont(FT_Bitmap *bitmap,int left,int top);
  int initializeFreeType();
  
  int getTextSize(int &w,int &h,float angle,const char *text);
  int drawAnchoredText(int x, int y, float angle,const char *text, int anchor);
  int drawCenteredText(int x, int y, float angle,const char *text);
  int drawFilledText(int x,int y,float angle,const char *text);
  void initFont() ;
  void setColor(unsigned char r,unsigned char g,unsigned char b,unsigned char a);
  void setFillColor(unsigned char r,unsigned char g,unsigned char b,unsigned char a);
//   void pixel(int x,int y);

//   void pixel(int x,int y, unsigned char r,unsigned char g,unsigned char b);
//   void pixel(int x,int y, unsigned char r,unsigned char g,unsigned char b,unsigned char a);
  void getPixel(int x,int y, unsigned char &r,unsigned char &g,unsigned char &b,unsigned char &a);
  
  void pixel_overwrite(int x,int y, unsigned char r,unsigned char g,unsigned char b,unsigned char a);
  void pixel_blend(int x,int y, unsigned char r,unsigned char g,unsigned char b,unsigned char a);
  
  unsigned char *getByteBuffer();
  void rectangle(int x1,int y1,int x2,int y2);
  void filledRectangle(int x1,int y1,int x2,int y2);
  void line_noaa(int x1,int y1,int x2,int y2) ;
  void moveTo(float x1,float y1) ;
  void lineTo(float x1,float y1) ;
  void lineTo(float x1,float y1,float width) ;
  void endLine();
  void line(float x1,float y1,float x2,float y2);
  void line(float x1,float y1,float x2,float y2,float width) ;
  void circle(int x, int y, int r) ;
  void filledcircle(int x, int y, int r) ;
  void filledEllipse(int x, int y, float rX, float rY, float rotation) ;
  void circle(int x, int y, int r,float lineWidth);
  void poly(float x[], float y[], int n, bool closePath, bool fill) ;
  void poly(float x[], float y[], int n, float lineWidth, bool closePath, bool fill) ;
  void drawText(int x, int y,double angle, const char *text);

  void writeToPng8Stream(FILE *fp,unsigned char alpha,bool use8bitpalAlpha);
  void writeToPng24Stream(FILE *fp,unsigned char alpha);
  void writeToPng32Stream(FILE *fp,unsigned char alpha);
  void writeToWebP32Stream(FILE *fp,unsigned char alpha);
  void setToSurface(cairo_surface_t *png) ;
};





#endif /* CCAIROPLOTTER_H_ */
#endif
