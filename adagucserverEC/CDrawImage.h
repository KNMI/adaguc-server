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
#include "CTString.h"
#include "CColor.h"
#include "Definitions.h"
#include "CStopWatch.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "Types/GeoParameters.h"
#include "CServerError.h"
#include "CServerConfig_CPPXSD.h"
#include <cmath>
#include "CCairoPlotter.h"
#include "CColor.h"
#include "CRectangleText.h"

float convertValueToClass(float val, float interval);

#define CDRAWIMAGE_COLORTYPE_INDEXED 1
#define CDRAWIMAGE_COLORTYPE_ARGB 2

class CLegend {
public:
  int id;
  unsigned char CDIred[256], CDIgreen[256], CDIblue[256];
  short CDIalpha[256]; // Currently alpha of 0 and 255 is supported, but nothin in between.
  CT::string legendName;
};

static CColor drawPointTextOutlineColor = CColor(255, 255, 255, 0);

class CDrawImage {
private:
  std::vector<CLegend *> legends;
  CLegend *currentLegend = nullptr;
  int dImageCreated;
  DEF_ERRORFUNCTION();
  int _createStandard();
  int dPaletteCreated;
  unsigned char BGColorR, BGColorG, BGColorB;
  bool _bEnableTransparency;
  bool _bEnableTrueColor;
  unsigned char backgroundAlpha;
  CCairoPlotter *cairo;
  const char *TTFFontLocation;
  float TTFFontSize;
  std::map<CT::string, CCairoPlotter *> myCCairoPlotterMap;
  CCairoPlotter *getCairoPlotter(const char *fontfile, float size, int w, int h, unsigned char *b);

public:
  float *rField, *gField, *bField;
  int *numField;
  bool trueColorAVG_RGBA; // TODO: is always false?
  int _colors[256];

  int colors[256];
  GeoParameters geoParams;
  CDrawImage();
  ~CDrawImage();

public:
  int createImage(int _dW, int _dH);
  int createImage(GeoParameters &_Geo);
  int createImage(const char *fn);
  int createImage(CDrawImage *image, int width, int height);
  int printImagePng8(bool useBitAlpha);
  int printImagePng24();
  int printImagePng32();
  int printImageWebP32(int quality);
  int printImageGif();                                      // TODO: to be removed?
  int createGDPalette(CServerConfig::XMLE_Legend *palette); // TODO: to be removed?
  int create685Palette();
  int clonePalette(CDrawImage *drawImage);

  void drawBarb(int x, int y, double direction, double viewDirCorrection, double strength, CColor barbColor, float linewidth, bool toKnots, bool flip, bool drawText, double fontSize = 12,
                CColor textColor = CColor(255, 255, 255, 0), CColor outlineColor = CColor(255, 255, 255, 255), double outlineWidth = 1.0);
  void drawText(int x, int y, float angle, const char *text, unsigned char colorIndex);
  void drawText(int x, int y, float angle, const char *text, CColor fgcolor);
  void drawText(int x, int y, const char *fontfile, float size, float angle, const char *text, unsigned char colorIndex);
  void drawText(int x, int y, const char *fontfile, float size, float angle, const char *text, CColor fgcolor);
  void drawText(int x, int y, const char *fontfile, float size, float angle, const char *text, CColor fgcolor, CColor bgcolor);
  void drawAnchoredText(int x, int y, const char *fontfile, float size, float angle, const char *text, CColor color, int anchor);
  void drawCenteredText(int x, int y, const char *fontfile, float size, float angle, const char *text, CColor color, CColor textOutlineColor = drawPointTextOutlineColor);
  void drawCenteredTextNoOverlap(int x, int y, const char *fontfile, float size, float angle, int padding, const char *text, CColor color, bool noOverlap, std::vector<CRectangleText> &rects);
  int drawTextArea(int x, int y, const char *fontfile, float size, float angle, const char *text, CColor fgcolor, CColor bgcolor);

  // void drawTextAngle(const char * text, size_t length,double angle,int x,int y,int color,int fontSize);
  void drawVector(int x, int y, double direction, double strength, int color);
  void drawVector(int x, int y, double direction, double strength, int color, float linewidth);
  void drawVector(int x, int y, double direction, double strength, CColor color, float linewidth);
  void drawVector2(int x, int y, double direction, double strength, int radius, CColor color, float linewidth);
  void destroyImage();
  void line(float x1, float y1, float x2, float y2, int color);
  void line(float x1, float y1, float x2, float y2, CColor color);
  void line(float x1, float y1, float x2, float y2, float w, int color);
  void line(float x1, float y1, float x2, float y2, float w, CColor color);
  void moveTo(float x1, float y1);
  void lineTo(float x1, float y1, float w, CColor color);
  void endLine();
  void endLine(const double *dashes, int num_dashes);

  void poly(float x1, float y1, float x2, float y2, float x3, float y3, int c, bool fill);
  void poly(float x1, float y1, float x2, float y2, float x3, float y3, CColor color, bool fill);
  void poly(float x1, float y1, float x2, float y2, float x3, float y3, float lineWidth, CColor color, bool fill);
  void poly(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float lineWidth, CColor color, bool fill);
  void poly(float *x, float *y, int n, float lineWidth, CColor lineColor, CColor fillColor, bool close, bool fill);
  void circle(int x, int y, int r, int color);
  void circle(int x, int y, int r, CColor col);

  void circle(int x, int y, int r, int color, float lineWidth);
  void circle(int x, int y, int r, CColor color, float lineWidth);
  void setPixelIndexed(int x, int y, int color);
  void setPixelTrueColor(int x, int y, unsigned int color);
  void setPixelTrueColor(int x, int y, unsigned char r, unsigned char g, unsigned char b);
  void setPixelTrueColor(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
  void getPixelTrueColor(int x, int y, unsigned char &r, unsigned char &g, unsigned char &b, unsigned char &a);
  void setPixelTrueColorOverWrite(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
  void setPixel(int x, int y, CColor &color);

  int getWidth();
  int getHeight();

  void getHexColorForColorIndex(CT::string *hexValue, int colorIndex);
  void setText(const char *text, size_t length, int x, int y, int color, int fontSize);
  void setText(const char *text, size_t length, int x, int y, CColor color, int fontSize);
  // void setTextDisc(const char *text, size_t length, int x, int y, int r, CColor color, const char *fontfile,int fontSize);
  void setDisc(int x, int y, int discRadius, CColor fillColor, CColor lineColor);
  void setDisc(int x, int y, int discRadius, int fillCol, int lineCol);
  void setDisc(int x, int y, float discRadius, CColor fillColor, CColor lineColor);
  void setEllipse(int x, int y, float discRadiusX, float discRadiusY, float rotation, CColor fillColor, CColor lineColor);
  void setTextDisc(int x, int y, int discRadius, const char *text, const char *fontfile, float fontsize, CColor textcolor, CColor fillcolor, CColor lineColor);
  void setTextStroke(int x, int y, float angle, const char *text, const char *fontFile, float fontsize, float strokeWidth, CColor bgcolor, CColor fgcolor);
  void rectangle(int x1, int y1, int x2, int y2, int innercolor, int outercolor);
  void rectangle(int x1, int y1, int x2, int y2, int outercolor);
  void rectangle(int x1, int y1, int x2, int y2, CColor innercolor, CColor outercolor);
  CColor getColorForIndex(int index);
  int copyPalette();
  int addColor(int Color, unsigned char R, unsigned char G, unsigned char B);
  void enableTransparency(bool enable);
  void setBGColor(unsigned char R, unsigned char G, unsigned char B);
  void setTrueColor(bool enable);
  bool getTrueColor() { return _bEnableTrueColor; }

  void setTTFFontLocation(const char *_TTFFontLocation) { TTFFontLocation = _TTFFontLocation; }
  void setTTFFontSize(float _TTFFontSize) { TTFFontSize = _TTFFontSize; }
  const char *getFontLocation();
  float getFontSize();

  bool isPixelTransparent(int &x, int &y);
  bool isColorTransparent(int &color);
  void getCanvasSize(int &x, int &y, int &w, int &h);

  /**
   * @param alpha The transparency of the resulting image written as PNG: 0 is transparent, 255 is opaque
   */
  void setBackGroundAlpha(unsigned char alpha) { backgroundAlpha = alpha; }

  int setCanvasSize(int x, int y, int width, int height);
  int draw(int destx, int desty, int sourcex, int sourcey, CDrawImage *simage);
  int drawrotated(int destx, int desty, int sourcex, int sourcey, CDrawImage *simage);
  void crop(int paddingW, int paddingH);
  void crop(int padding);

  void rotate();
  /**
   * Returns canvas memory in case of true color images
   */
  unsigned char *getCanvasMemory() const;

  /**
   * Returns canvas colortype, either COLORTYPE_INDEXED or COLORTYPE_ARGB
   */
  int getCanvasColorType();

  /**
   * Sets canvas colortype, either COLORTYPE_INDEXED or COLORTYPE_ARGB
   */
  void setCanvasColorType(int colorType);

  /**
   * Get renderer width of the given text
   */
  int getTextWidth(CT::string text, const std::string &fontPath, int fontSize, int angle);
};

#endif
