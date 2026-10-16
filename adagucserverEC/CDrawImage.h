/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl, GST - GeoSpatialTeam KNMI
 * Date:     2026-09-10
 *
 ******************************************************************************
 *
 * Copyright 2026, Royal Netherlands Meteorological Institute (KNMI)
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
#include "CTString.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include "CServerConfig_CPPXSD.h"
#include <cmath>
#include <cairo.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include "CColor.h"
#include "CRectangleText.h"
#include "Types/GeoParameters.h"

float convertValueToClass(float val, float interval);

struct CLegend {
  CColor colors[256];
  std::string legendName;
};

static CColor drawPointTextOutlineColor = CColor(255, 255, 255, 0);

// Named colors reused across call sites instead of each hand-rolling its own RGBA literal.
// Several correspond to entries of the legacy 240-255 "standard" index palette (see
// CDrawImage::_createStandard()) - the index is noted for anyone cross-referencing that palette.
static const CColor standardCColorBlack = CColor(0, 0, 0, 255);            // standard palette index 240/248
static const CColor standardCColorDarkGray = CColor(32, 32, 32, 255);      // standard palette index 241/247
static const CColor standardCColorLightBlue = CColor(64, 64, 255, 255);    // standard palette index 244
static const CColor standardCColorPaleYellow = CColor(255, 255, 191, 255); // standard palette index 249
static const CColor standardCColorLightGray = CColor(204, 204, 204, 255);  // standard palette index 251
static const CColor standardCColorPaleGray = CColor(224, 224, 224, 255);   // standard palette index 254
static const CColor standardCColorTranslucentBlack = CColor(0, 0, 0, 128);

class CDrawImage {
private:
  std::vector<CLegend *> legends;
  CLegend *currentLegend = nullptr;
  int dImageCreated;

  int _createStandard();
  int dPaletteCreated;
  unsigned char BGColorR, BGColorG, BGColorB;
  bool _bEnableTransparency;
  bool _bEnableTrueColor;
  unsigned char backgroundAlpha;
  std::string TTFFontLocation;
  float TTFFontSize;

  // --- Cairo canvas: a single shared surface/context for the whole image. Every drawing method
  // (including text, via a swapped-in FT_Face - see getFontFace()) operates on this same cr, so
  // there is exactly one pixel buffer and one Cairo context per CDrawImage, not one per font.
  unsigned char *ARGBByteBuffer = nullptr;
  bool byteBufferPointerIsOwned = false;
  cairo_surface_t *surface = nullptr;
  cairo_t *cr = nullptr;
  int stride = 0;
  bool isAlphaUsed = false;
  static constexpr cairo_format_t CAIRO_PIXEL_FORMAT = CAIRO_FORMAT_ARGB32;

  // Current stroke/fill color state, set by _setColor()/_setFillColor() before each primitive -
  // mirrors how Cairo itself keeps "current color" as context state rather than a draw argument.
  float rr = 0, rg = 0, rb = 0, ra = 1;
  unsigned char r = 0, g = 0, b = 0;
  float a = 255;
  float rfr = 0, rfg = 0, rfb = 0, rfa = 0;
  unsigned char fr = 0, fg = 0, fb = 0;
  float fa = 0;

  void _swap(float &x, float &y) const;
  void _swap(int &x, int &y) const;

  void _setColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
  void _setFillColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
  void _getPixel(int x, int y, unsigned char &r, unsigned char &g, unsigned char &b, unsigned char &a) const;
  void _pixel_overwrite(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
  void _pixel_blend(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
  void _rectangle(int x1, int y1, int x2, int y2);
  void _filledRectangle(int x1, int y1, int x2, int y2);
  void _moveTo(float x1, float y1);
  void _lineTo(float x1, float y1, float width);
  void _endLine(const double *dashes = nullptr, int num_dashes = 0);
  void _line(float x1, float y1, float x2, float y2);
  void _line(float x1, float y1, float x2, float y2, float width);
  void _filledcircle(int x, int y, int r);
  void _filledEllipse(int x, int y, float rX, float rY, float rotation);
  void _circle(int x, int y, int r, float lineWidth);
  void _poly(float x[], float y[], int n, bool closePath, bool fill);
  void _poly(float x[], float y[], int n, float lineWidth, bool closePath, bool fill);
  void _poly(const std::vector<f8point> &polyPoints, float lineWidth, bool closePath, bool fill);
  void _drawBarb(int x, int y, double direction, double viewDirCorrection, double strength, const CColor &color, const CColor &outlineColor, float lineWidth, bool toKnots, bool flip, bool drawText,
                 double fontSize, const CColor &textColor, double outlineWidth);

  void _writeToPng8Stream(FILE *fp, bool use8bitpalAlpha);
  void _writeToPng24Stream(FILE *fp);
  void _writeToPng32Stream(FILE *fp, unsigned char alpha);
  void _writeToWebP32Stream(FILE *fp, int quality);
  void _setToSurface(cairo_surface_t *png);

  // --- Text rendering: one shared FreeType library, one cached FT_Face per (font file, size).
  // Drawing and measuring both go through Cairo's own text API (cairo_show_text/cairo_text_extents,
  // fed by the FT_Face via cairo_ft_font_face_create_for_ft_face) rather than walking glyphs by
  // hand, so swapping fonts only means swapping currentFace - never a new rendering context.
  FT_Library ftLibrary = nullptr;
  std::map<std::string, FT_Face> fontFaceCache;
  FT_Face currentFace = nullptr;
  float currentFontSize = 0;

  FT_Face _getFontFace(const std::string &fontfile, float size);
  void _closeFreeType();
  cairo_font_face_t *_setCairoFont();
  void _fillRect(double x1, double y1, double x2, double y2, const CColor &color);
  int _getTextSize(int &w, int &h, float angle, const char *text);
  int _drawCenteredText(int x, int y, float angle, const char *text, const CColor &color);
  int _drawFilledText(int x, int y, float angle, const char *text, const CColor &fgcolor, const CColor &bgcolor);
  int _drawText(int x, int y, double angle, const char *text, const CColor &color);
  void _drawStrokedText(int x, int y, double angle, const char *text, float fontSize, float strokeWidth, const CColor &bgcolor, const CColor &fgcolor, bool centerText = false);

public:
  float *rField, *gField, *bField;
  int *numField;
  int _colors[256];

  int colors[256];
  GeoParameters geoParams;
  CDrawImage();
  ~CDrawImage();

public:
  int createImage(int _dW, int _dH);
  int createImage(GeoParameters &_Geo);
  int createImage(const char *fn);
  int createImage(const CDrawImage *image, int width, int height);
  int printImagePng8(bool useBitAlpha);
  int printImagePng24();
  int printImagePng32();
  int printImageWebP32(int quality);
  int createPalette(CServerConfig::XMLE_Legend *palette);
  int create685Palette();
  int clonePalette(const CDrawImage *drawImage);

  void drawBarb(int x, int y, double direction, double viewDirCorrection, double strength, const CColor &barbColor, float linewidth, bool toKnots, bool flip, bool drawText, double fontSize = 12,
                const CColor &textColor = CColor(255, 255, 255, 0), const CColor &outlineColor = CColor(255, 255, 255, 255), double outlineWidth = 1.0);
  void drawText(int x, int y, float angle, const char *text, const CColor &fgcolor);
  void drawText(int x, int y, const char *fontfile, float size, float angle, const char *text, const CColor &fgcolor);
  void drawText(int x, int y, const char *fontfile, float size, float angle, const char *text, const CColor &fgcolor, const CColor &bgcolor);
  void drawCenteredText(int x, int y, const char *fontfile, float size, float angle, const char *text, const CColor &color, const CColor &textOutlineColor = drawPointTextOutlineColor);
  void drawCenteredTextNoOverlap(int x, int y, const char *fontfile, float size, float angle, int padding, const char *text, const CColor &color, bool noOverlap, std::vector<CRectangleText> &rects);
  int drawTextArea(int x, int y, const char *fontfile, float size, float angle, const char *text, const CColor &fgcolor, const CColor &bgcolor);

  void drawVector(int x, int y, double direction, double strength, const CColor &color, float linewidth);
  void drawVector2(int x, int y, double direction, double strength, int radius, const CColor &color, float linewidth);
  void destroyImage();
  void line(float x1, float y1, float x2, float y2, const CColor &color);
  void line(float x1, float y1, float x2, float y2, float w, const CColor &color);
  void moveTo(float x1, float y1);
  void lineTo(float x1, float y1, float w, const CColor &color);
  void endLine(const double *dashes = nullptr, int num_dashes = 0);

  void poly(float x1, float y1, float x2, float y2, float x3, float y3, const CColor &color, bool fill);
  void poly(float x1, float y1, float x2, float y2, float x3, float y3, float lineWidth, const CColor &color, bool fill);
  void poly(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float lineWidth, const CColor &color, bool fill);
  void poly(float *x, float *y, int n, float lineWidth, const CColor &lineColor, const CColor &fillColor, bool close, bool fill);
  void poly(const std::vector<f8point> &polyPoints, float lineWidth, const CColor &lineColor, const CColor &fillColor, bool close, bool fill);
  void circle(int x, int y, int r, const CColor &color, float lineWidth = 1.0);
  void setPixelTrueColor(int x, int y, unsigned int color);
  void setPixelTrueColor(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);
  void getPixelTrueColor(int x, int y, unsigned char &r, unsigned char &g, unsigned char &b, unsigned char &a);
  void setPixelTrueColorOverWrite(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
  void setPixel(int x, int y, const CColor &color);
  // Looks the index up in the active legend/palette and blends it in one call - cheaper than a
  // caller doing getCColorFromLegendPaletteIndex() + its own alpha check for every pixel of a hot warp loop.
  void setPixelByLegendPaletteIndex(int x, int y, int colorIndex);

  int getWidth() const;
  int getHeight() const;

  void setText(const char *text, int x, int y, const CColor &color);
  void setDisc(int x, int y, float discRadius, const CColor &fillColor, const CColor &lineColor);
  void setEllipse(int x, int y, float discRadiusX, float discRadiusY, float rotation, const CColor &fillColor, const CColor &lineColor);
  void setTextDisc(int x, int y, int discRadius, const char *text, const char *fontfile, float fontsize, const CColor &textcolor, const CColor &fillcolor, const CColor &lineColor);
  void setTextStroke(int x, int y, float angle, const char *text, const char *fontFile, float fontsize, float strokeWidth, const CColor &bgcolor, const CColor &fgcolor);
  void rectangle(int x1, int y1, int x2, int y2, const CColor &innercolor, const CColor &outercolor);
  CColor getCColorFromLegendPaletteIndex(int index) const;
  int copyPalette();
  int addColor(int Color, unsigned char R, unsigned char G, unsigned char B);
  void enableTransparency(bool enable);
  void setBGColor(unsigned char R, unsigned char G, unsigned char B);
  void setTrueColor(bool enable);

  void setTTFFontLocation(const std::string &_TTFFontLocation) { TTFFontLocation = _TTFFontLocation; }
  void setTTFFontSize(float _TTFFontSize) { TTFFontSize = _TTFFontSize; }
  std::string getFontLocation() const;
  float getFontSize() const;

  bool isPixelTransparent(int x, int y) const;
  void getCanvasSize(int &x, int &y, int &w, int &h) const;

  /**
   * @param alpha The transparency of the resulting image written as PNG: 0 is transparent, 255 is opaque
   */
  void setBackGroundAlpha(unsigned char alpha) { backgroundAlpha = alpha; }

  int setCanvasSize(int x, int y, int width, int height);
  int draw(int destx, int desty, int sourcex, int sourcey, const CDrawImage *simage);
  int drawrotated(int destx, int desty, int sourcex, int sourcey, const CDrawImage *simage);
  void crop(int paddingW, int paddingH);
  void crop(int padding);

  void rotate();
  /**
   * Returns canvas memory in case of true color images
   */
  unsigned char *getCanvasMemory() const;

  /**
   * Rendered width in pixels of text at the given font/size/angle - e.g. used to center a label
   * within a reserved span without exceeding it (pass angle 0 for horizontal text).
   */
  int getTextWidth(const std::string &text, const std::string &fontPath, float size, int angle);
};

#endif
