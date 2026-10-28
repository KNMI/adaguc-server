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
#include <span>
#include <vector>
#include <iostream>
#include "CTString.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include "CServerConfig_CPPXSD.h"
#include <cmath>
#include <cairo.h>
#include "CColor.h"
#include "CRectangleText.h"
#include "Types/GeoParameters.h"

static const CColor drawPointTextOutlineColor = CColor(255, 255, 255, 0);

// Named colors; some match the old 240-255 "standard" palette indices, now unreachable.
static const CColor standardCColorBlack = CColor(0, 0, 0, 255);
static const CColor standardCColorDarkGray = CColor(32, 32, 32, 255);
static const CColor standardCColorLightBlue = CColor(64, 64, 255, 255);
static const CColor standardCColorPaleYellow = CColor(255, 255, 191, 255);
static const CColor standardCColorLightGray = CColor(204, 204, 204, 255);
static const CColor standardCColorPaleGray = CColor(224, 224, 224, 255);
static const CColor standardCColorTranslucentBlack = CColor(0, 0, 0, 128);

class CDrawImage {
private:
  int dImageCreated;

  int setCanvasSize(int x, int y, int width, int height);
  int drawrotated(int destx, int desty, int sourcex, int sourcey, const CDrawImage *simage);
  bool isPixelTransparent(int x, int y) const;
  void getCanvasSize(int &x, int &y, int &w, int &h) const;

  unsigned char BGColorR, BGColorG, BGColorB;
  bool _bEnableTransparency;
  bool _bEnableTrueColor;
  unsigned char backgroundAlpha;
  std::string TTFFontLocation;
  float TTFFontSize;

  // One shared Cairo surface/context per image; text drawing swaps in a cairo_font_face_t on it.
  std::vector<unsigned char> ARGBByteBuffer;
  cairo_surface_t *surface = nullptr;
  cairo_t *cr = nullptr;
  int stride = 0;
  static constexpr cairo_format_t CAIRO_PIXEL_FORMAT = CAIRO_FORMAT_ARGB32;

  // Current stroke/fill color, Cairo's [0,1] range, set by _setColor()/_setFillColor().
  float rr = 0, rg = 0, rb = 0, ra = 1;
  float rfr = 0, rfg = 0, rfb = 0, rfa = 0;

  void _setColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a);
  void _setFillColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a);

  // Inline so this inlines into the generic renderer's per-pixel call chain.
  void _getPixel(int x, int y, unsigned char &r, unsigned char &g, unsigned char &b, unsigned char &a) const {
    if (x < 0 || y < 0 || x >= geoParams.width || y >= geoParams.height) {
      r = 0;
      b = 0;
      g = 0;
      a = 0;
      return;
    }
    size_t p = x * 4 + y * stride;
    uint32_t pixel;
    memcpy(&pixel, &ARGBByteBuffer[p], sizeof(pixel));
    b = pixel;
    g = pixel >> 8;
    r = pixel >> 16;
    a = pixel >> 24;
  }

  // Packs B,G,R,A into the native-endian 0xAARRGGBB word CAIRO_FORMAT_ARGB32 stores.
  static uint32_t _packBGRA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) { return uint32_t(b) | (uint32_t(g) << 8) | (uint32_t(r) << 16) | (uint32_t(a) << 24); }

  void _pixel_overwrite(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    if (x < 0 || y < 0 || x >= geoParams.width || y >= geoParams.height) return;
    uint32_t pixel = _packPremultiplied(r, g, b, a);
    memcpy(&ARGBByteBuffer[x * 4 + y * stride], &pixel, sizeof(pixel));
  }

  // Straight (r,g,b,a) in, premultiplied word out - the form the buffer stores.
  static uint32_t _packPremultiplied(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    if (a == 255) return _packBGRA(r, g, b, 255);
    return _packBGRA(premultiplyChannel(r, a), premultiplyChannel(g, a), premultiplyChannel(b, a), a);
  }

  // Source-over blend: dst' = src + dst * (255 - srcAlpha) / 255, per channel.
  void _blendPremultiplied(size_t p, uint32_t src) {
    uint32_t srcAlpha = src >> 24;
    if (srcAlpha == 255) {
      memcpy(&ARGBByteBuffer[p], &src, sizeof(src));
      return;
    }
    if (srcAlpha == 0) return;
    uint32_t dst;
    memcpy(&dst, &ARGBByteBuffer[p], sizeof(dst));
    uint32_t keep = 255 - srcAlpha;
    uint32_t b = (src & 0xff) + div255((dst & 0xff) * keep);
    uint32_t g = ((src >> 8) & 0xff) + div255(((dst >> 8) & 0xff) * keep);
    uint32_t r = ((src >> 16) & 0xff) + div255(((dst >> 16) & 0xff) * keep);
    uint32_t a = srcAlpha + div255((dst >> 24) * keep);
    uint32_t out = b | (g << 8) | (r << 16) | (a << 24);
    memcpy(&ARGBByteBuffer[p], &out, sizeof(out));
  }

  void _pixel_blend(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    if (x < 0 || y < 0 || x >= geoParams.width || y >= geoParams.height) return;
    _blendPremultiplied(x * 4 + y * stride, _packPremultiplied(r, g, b, a));
  }

  void _rectangle(int x1, int y1, int x2, int y2);
  void _filledRectangle(int x1, int y1, int x2, int y2);
  void _moveTo(float x1, float y1);
  void _lineTo(float x1, float y1, float width);
  void _endLine(std::span<const double> dashes, bool roundCap);
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

  // --- Text: one cairo_font_face_t cached per font file, loaded via cairo's FreeType backend.
  std::map<std::string, cairo_font_face_t *> fontFaceCache;
  cairo_font_face_t *currentFace = nullptr;
  float currentFontSize = 0;

  cairo_font_face_t *_getFontFace(const std::string &fontfile);
  void _closeFontFaceCache();
  void _setCairoFont();
  void _fillRect(double x1, double y1, double x2, double y2, const CColor &color);
  int _getTextSize(int &w, int &h, float angle, const char *text);
  int _drawCenteredText(int x, int y, float angle, const char *text, const CColor &color);
  int _drawFilledText(int x, int y, float angle, const char *text, const CColor &fgcolor, const CColor &bgcolor);
  int _drawText(int x, int y, double angle, const char *text, const CColor &color);
  void _drawStrokedText(int x, int y, double angle, const char *text, float fontSize, float strokeWidth, const CColor &bgcolor, const CColor &fgcolor, bool centerText = false);

public:
  // --- Canvas lifecycle -------------------------------------------------------------------------
  GeoParameters geoParams;

  CDrawImage();
  ~CDrawImage();

  int createImage(int _dW, int _dH);
  int createImage(const GeoParameters &_Geo);
  int createImage(const char *fn);
  int createImage(const CDrawImage *image, int width, int height);
  void destroyImage();

  int getWidth() const;
  int getHeight() const;

  void enableTransparency(bool enable);
  void setBGColor(unsigned char R, unsigned char G, unsigned char B);
  void setTrueColor(bool enable);
  void setBackGroundAlpha(unsigned char alpha) { backgroundAlpha = alpha; }

  void setTTFFontLocation(const std::string &_TTFFontLocation) { TTFFontLocation = _TTFFontLocation; }
  void setTTFFontSize(float _TTFFontSize) { TTFFontSize = _TTFFontSize; }
  std::string getFontLocation() const;
  float getFontSize() const;

  // --- Pixel access -----------------------------------------------------------------------------
  // Canvas is premultiplied-alpha (CAIRO_FORMAT_ARGB32); setters premultiply on the way in, and
  // only the straight-color exporters (PNG8/24, WebP) unpremultiply on the way out.

  // floor(x / 255) without a division.
  static uint32_t div255(uint32_t x) { return (x + 1 + (x >> 8)) >> 8; }
  static unsigned char premultiplyChannel(unsigned char c, unsigned char a) { return div255(uint32_t(c) * a); }
  static unsigned char unpremultiplyChannel(unsigned char c, unsigned char a) {
    if (a == 255) return c;
    if (a == 0) return 0;
    uint32_t v = (uint32_t(c) * 255 + a / 2) / a;
    return v > 255 ? 255 : v;
  }

  void setPixel(int x, int y, const CColor &color) { _pixel_blend(x, y, color.r, color.g, color.b, color.a); }
  void setPixelTrueColor(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) { _pixel_blend(x, y, r, g, b, a); }
  void setPixelTrueColorOverWrite(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a) { _pixel_overwrite(x, y, r, g, b, a); }

  // --- Lines and shapes -------------------------------------------------------------------------

  void moveTo(float x1, float y1);
  void lineTo(float x1, float y1, float w, const CColor &color);
  // roundCap: avoids a seam where contour sub-paths meet at an angle around label gaps.
  void endLine(std::span<const double> dashes = {}, bool roundCap = false);

  void line(float x1, float y1, float x2, float y2, const CColor &color);
  void line(float x1, float y1, float x2, float y2, float w, const CColor &color);

  void poly(float x1, float y1, float x2, float y2, float x3, float y3, const CColor &color, bool fill);
  void poly(float x1, float y1, float x2, float y2, float x3, float y3, float lineWidth, const CColor &color, bool fill);
  void poly(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float lineWidth, const CColor &color, bool fill);
  void poly(float *x, float *y, int n, float lineWidth, const CColor &lineColor, const CColor &fillColor, bool close, bool fill);
  void poly(const std::vector<f8point> &polyPoints, float lineWidth, const CColor &lineColor, const CColor &fillColor, bool close, bool fill);

  void circle(int x, int y, int r, const CColor &color, float lineWidth = 1.0);
  void setDisc(int x, int y, float discRadius, const CColor &fillColor, const CColor &lineColor);
  void setEllipse(int x, int y, float discRadiusX, float discRadiusY, float rotation, const CColor &fillColor, const CColor &lineColor);
  void rectangle(int x1, int y1, int x2, int y2, const CColor &innercolor, const CColor &outercolor);

  // --- Text --------------------------------------------------------------------------------------

  void drawText(int x, int y, float angle, const char *text, const CColor &fgcolor);
  void drawText(int x, int y, const char *fontfile, float size, float angle, const char *text, const CColor &fgcolor);
  void drawText(int x, int y, const char *fontfile, float size, float angle, const char *text, const CColor &fgcolor, const CColor &bgcolor);
  void drawCenteredText(int x, int y, const char *fontfile, float size, float angle, const char *text, const CColor &color, const CColor &textOutlineColor = drawPointTextOutlineColor);
  void drawCenteredTextNoOverlap(int x, int y, const char *fontfile, float size, float angle, int padding, const char *text, const CColor &color, bool noOverlap, std::vector<CRectangleText> &rects);
  int drawTextArea(int x, int y, const char *fontfile, float size, float angle, const char *text, const CColor &fgcolor, const CColor &bgcolor);
  void setText(const char *text, int x, int y, const CColor &color);
  void setTextDisc(int x, int y, int discRadius, const char *text, const char *fontfile, float fontsize, const CColor &textcolor, const CColor &fillcolor, const CColor &lineColor);
  void setTextStroke(int x, int y, float angle, const char *text, const char *fontFile, float fontsize, float strokeWidth, const CColor &bgcolor, const CColor &fgcolor);
  int getTextWidth(const std::string &text, const std::string &fontPath, float size, int angle);

  // --- Meteorological symbols --------------------------------------------------------------------

  void drawBarb(int x, int y, double direction, double viewDirCorrection, double strength, const CColor &barbColor, float linewidth, bool toKnots, bool flip, bool drawText, double fontSize = 12,
                const CColor &textColor = CColor(255, 255, 255, 0), const CColor &outlineColor = CColor(255, 255, 255, 255), double outlineWidth = 1.0);
  void drawVector(int x, int y, double direction, double strength, const CColor &color, float linewidth);
  void drawVector2(int x, int y, double direction, double strength, int radius, const CColor &color, float linewidth);

  // --- Whole-image operations --------------------------------------------------------------------

  int draw(int destx, int desty, int sourcex, int sourcey, const CDrawImage *simage);
  void crop(int paddingW, int paddingH);
  void crop(int padding);
  void rotate();

  int printImagePng8(bool useBitAlpha);
  int printImagePng24();
  int printImagePng32();
  int printImageWebP32(int quality);
};

#endif
