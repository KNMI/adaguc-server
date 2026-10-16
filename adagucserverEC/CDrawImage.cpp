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

#include "CDrawImage.h"
#include "CDebugger.h"
#include "CColor.h"
#include "Definitions.h"
#include "CStopWatch.h"
#include "Types/GeoParameters.h"
#include "CServerError.h"
#include "CXMLParser.h"
#include <cairo-ft.h>
#include "write_png.h"
#include <cstdint>
#include <algorithm>

#ifdef ADAGUC_USE_WEBP
#include "webp/encode.h"
#include "webp/decode.h"
#include "webp/types.h"
#endif

#define CAIROPLOTTER_COLOR_BYTE_TO_NORMAL 1 / 255.0

float convertValueToClass(float val, float interval) {
  float f = int(val / interval);
  if (val < 0) f -= 1;
  return f * interval;
}

static cairo_status_t pngWriterFunc(void *closure, const unsigned char *data, unsigned int length) {
  FILE *fp = (FILE *)closure;
  int nrec = fwrite(data, length, 1, fp);
  if (nrec == 1) {
    return CAIRO_STATUS_SUCCESS;
  }
  return CAIRO_STATUS_WRITE_ERROR;
}

static int drawBarbTriangle(cairo_t *cr, int x, int y, int nPennants, double direction, double shaftLength, double barbLengthWithFlip, int nrPos) {
  int pos = 0;
  double dx1 = cos(direction) * (shaftLength);
  double dy1 = sin(direction) * (shaftLength);
  double wx1 = double(x) - dx1;
  double wy1 = double(y) + dy1; // wind barb top (flag side)

  for (int i = 0; i < nPennants; i++) {
    double wx3 = wx1 + pos * dx1 / nrPos;
    double wy3 = wy1 - pos * dy1 / nrPos;
    pos++;
    double hx3 = wx1 + pos * dx1 / nrPos + cos(M_PI + direction + M_PI / 2) * barbLengthWithFlip;
    double hy3 = wy1 - pos * dy1 / nrPos - sin(M_PI + direction + M_PI / 2) * barbLengthWithFlip;
    pos++;
    double wx4 = wx1 + pos * dx1 / nrPos;
    double wy4 = wy1 - pos * dy1 / nrPos;

    double ptx[3] = {wx3, hx3, wx4};
    double pty[3] = {wy3, hy3, wy4};

    cairo_move_to(cr, ptx[0], pty[0]);
    for (int j = 1; j < 3; j++) {
      cairo_line_to(cr, ptx[j], pty[j]);
    }
  }
  return pos;
}

#ifdef ADAGUC_USE_WEBP
static int webPWriter(const uint8_t *data, size_t data_size, const WebPPicture *const pic) {
  FILE *const out = (FILE *)pic->custom_ptr;
  return data_size ? (fwrite(data, data_size, 1, out) == 1) : 1;
}
#endif

CDrawImage::CDrawImage() {
  dImageCreated = 0;
  dPaletteCreated = 0;
  currentLegend = NULL;
  _bEnableTrueColor = false;
  _bEnableTransparency = false;
  _bEnableTrueColor = false;
  rField = NULL;
  gField = NULL;
  bField = NULL;
  numField = NULL;

  TTFFontLocation = "/usr/X11R6/lib/X11/fonts/truetype/verdana.ttf"; // TODO: this location does not exist in the docker container
  const char *fontLoc = getenv("ADAGUC_FONT");
  if (fontLoc != NULL) {
    TTFFontLocation = fontLoc;
  }
  TTFFontSize = 9;

  BGColorR = 0;
  BGColorG = 0;
  BGColorB = 0;
  backgroundAlpha = 255;
}

void CDrawImage::destroyImage() {
  dImageCreated = 0;

  for (size_t j = 0; j < legends.size(); j++) {
    delete legends[j];
  }
  legends.clear();

  if (cr != nullptr) {
    cairo_destroy(cr);
    cr = nullptr;
  }
  if (surface != nullptr) {
    cairo_surface_destroy(surface);
    surface = nullptr;
  }
  if (byteBufferPointerIsOwned && ARGBByteBuffer != nullptr) {
    delete[] ARGBByteBuffer;
  }
  ARGBByteBuffer = nullptr;
  byteBufferPointerIsOwned = false;

  if (rField != NULL) {
    delete[] rField;
    rField = NULL;
    delete[] gField;
    gField = NULL;
    delete[] bField;
    bField = NULL;
    delete[] numField;
    numField = NULL;
  }
}

void CDrawImage::_closeFreeType() {
  for (auto &kv: fontFaceCache) {
    if (kv.second != nullptr) {
      FT_Done_Face(kv.second);
    }
  }
  fontFaceCache.clear();
  if (ftLibrary != nullptr) {
    FT_Done_FreeType(ftLibrary);
    ftLibrary = nullptr;
  }
  currentFace = nullptr;
}

CDrawImage::~CDrawImage() {
  destroyImage();
  // The font-face cache outlives destroyImage() (see createImage(CDrawImage*,...)/setCanvasSize,
  // which destroy and recreate the Cairo surface on the same CDrawImage): an FT_Face has no
  // dependency on the pixel buffer, so it stays valid across that cycle and is only torn down here.
  _closeFreeType();
}

int CDrawImage::createImage(const char *fn) {
  _bEnableTrueColor = true;
  _bEnableTransparency = true;

  cairo_surface_t *pngSurface = cairo_image_surface_create_from_png(fn);
  createImage(cairo_image_surface_get_width(pngSurface), cairo_image_surface_get_height(pngSurface));
  _setToSurface(pngSurface);
  cairo_surface_destroy(pngSurface);
  return 0;
}

int CDrawImage::createImage(int _dW, int _dH) {
  geoParams.width = _dW;
  geoParams.height = _dH;
  return createImage(geoParams);
}

int CDrawImage::createImage(GeoParameters &_Geo) {
  StopWatch_Measure("start createImage of size");
  if (dImageCreated == 1) {
    CDBError("createImage: image already created");
    return 1;
  }

  geoParams = _Geo;
  // Always true color

  unsigned char initR = 0, initG = 0, initB = 0, initA = 255;
  if (_bEnableTransparency == false) {
    initR = BGColorR;
    initG = BGColorG;
    initB = BGColorB;
    initA = 255;
  } else {
    initR = 0;
    initG = 0;
    initB = 0;
    initA = 0;
  }

  stride = cairo_format_stride_for_width(CAIRO_PIXEL_FORMAT, geoParams.width);
  size_t bufferSize = size_t(geoParams.height) * stride;
  ARGBByteBuffer = new unsigned char[bufferSize];
  byteBufferPointerIsOwned = true;

  // Fill ARGBByteBuffer with the desired background color. It's faster to write per 32-bit pixel
  // instead of per 8-bit color channel.
  uint32_t pixel = (uint32_t(initA) << 24) | (uint32_t(initR) << 16) | (uint32_t(initG) << 8) | uint32_t(initB);
  auto *quadBuffer = reinterpret_cast<uint32_t *>(ARGBByteBuffer);
  std::fill_n(quadBuffer, bufferSize / 4, pixel);

  surface = cairo_image_surface_create_for_data(ARGBByteBuffer, CAIRO_PIXEL_FORMAT, geoParams.width, geoParams.height, stride);
  cr = cairo_create(surface);
  isAlphaUsed = false;

  r = 0;
  g = 0;
  b = 0;
  a = 255;
  rr = r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
  rg = g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
  rb = b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
  ra = 1;
  fr = 0;
  fg = 0;
  fb = 0;
  fa = 0;
  rfr = rfg = rfb = 0;
  rfa = 0;

  dImageCreated = 1;
  StopWatch_Measure("image created");

  return 0;
}

int CDrawImage::printImagePng8(bool useBitAlpha) {
  if (dImageCreated == 0) {
    CDBError("print: image not created");
    return 1;
  }

  _writeToPng8Stream(stdout, useBitAlpha);
  return 0;
}

int CDrawImage::printImagePng24() {
  if (dImageCreated == 0) {
    CDBError("print: image not created");
    return 1;
  }

  _writeToPng24Stream(stdout);
  return 0;
}

int CDrawImage::printImagePng32() {
  if (dImageCreated == 0) {
    CDBError("print: image not created");
    return 1;
  }

  _writeToPng32Stream(stdout, backgroundAlpha);
  return 0;
}
int CDrawImage::printImageWebP32(int quality) {
  if (dImageCreated == 0) {
    CDBError("print: image not created");
    return 1;
  }

  _writeToWebP32Stream(stdout, quality);
  return 0;
}

void CDrawImage::drawVector(int x, int y, double direction, double strength, const CColor &color, float linewidth) {
  double wx1, wy1, wx2, wy2, dx1, dy1;
  if (fabs(strength) < 1) {
    setPixel(x, y, color);
    return;
  }

  bool startatxy = true;

  dx1 = cos(direction) * (strength);
  dy1 = sin(direction) * (strength);

  // arrow shaft
  if (startatxy) {
    wx1 = double(x) + dx1;
    wy1 = double(y) - dy1; //
    wx2 = double(x);
    wy2 = double(y);
  } else {
    wx1 = double(x) + dx1;
    wy1 = double(y) - dy1; // arrow point
    wx2 = double(x) - dx1;
    wy2 = double(y) + dy1;
  }

  strength = (3 + strength);

  // arrow point
  float hx1, hy1, hx2, hy2, hx3, hy3;
  hx1 = wx1 + cos(direction - 2.5) * (strength / 2.8f);
  hy1 = wy1 - sin(direction - 2.5) * (strength / 2.8f);
  hx2 = wx1 + cos(direction + 2.5) * (strength / 2.8f);
  hy2 = wy1 - sin(direction + 2.5) * (strength / 2.8f);
  hx3 = wx1 + (cos(direction - 2.5) + cos(direction + 2.5)) / 2 * (strength / 2.8f);
  hy3 = wy1 - (sin(direction + 2.5) + sin(direction - 2.5)) / 2 * (strength / 2.8f);

  // Render triangle
  _setColor(color.r, color.g, color.b, color.a);
  _setFillColor(color.r, color.g, color.b, color.a);
  poly(hx1, hy1, wx1, wy1, hx2, hy2, linewidth, color, false);

  // Render shaft
  line(wx2, wy2, hx3, hy3, linewidth, color);
}

#define xCor(l, d) ((int)(l * cos(d) + 0.5))
#define yCor(l, d) ((int)(l * sin(d) + 0.5))

void CDrawImage::drawVector2(int x, int y, double direction, double strength, int radius, const CColor &color, float linewidth) {
  if (fabs(strength) < 1) {
    setPixel(x, y, color);
    return;
  }

  int ARROW_LENGTH = radius + 16;
  float tipX = x + (int)(ARROW_LENGTH * cos(direction));
  float tipY = y + (int)(ARROW_LENGTH * sin(direction));

  int i2 = 8 + (int)linewidth;
  int i1 = 2 * i2;

  float hx1 = tipX - xCor(i1, direction + 0.5);
  float hy1 = tipY - yCor(i1, direction + 0.5);
  float hx2 = tipX - xCor(i2, direction);
  float hy2 = tipY - yCor(i2, direction);
  float hx3 = tipX - xCor(i1, direction - 0.5);
  float hy3 = tipY - yCor(i1, direction - 0.5);

  poly(tipX, tipY, hx1, hy1, hx2, hy2, hx3, hy3, linewidth, color, true);
}

#define MSTOKNOTS (3600. / 1852.)

#define round(x) (int(x + 0.5)) // Only for positive values!!!

void CDrawImage::drawBarb(int x, int y, double direction, double viewDirCorrection, double strength, const CColor &barbColor, float lineWidth, bool toKnots, bool flip, bool drawText, double fontSize,
                          const CColor &textColor, const CColor &outlineColor, double outlineWidth) {
  // If no linewidth, no outline should be drawn, set inner barblineWidth to 0.8 to ensure we draw a barb

  double barblineWidth = lineWidth == 0 ? 0.8 : lineWidth;
  double barbOutlineWidth = lineWidth == 0 ? 0 : outlineWidth;

  // Barb text, like setTextStroke's bgcolor!=0 branch, always uses the default font face - only
  // fontSize (rendered size) is configurable per call, not the typeface.
  currentFace = _getFontFace(TTFFontLocation, TTFFontSize);
  _drawBarb(x, y, direction, viewDirCorrection, strength, barbColor, outlineColor, barblineWidth, toKnots, flip, drawText, fontSize, textColor, barbOutlineWidth);
}

void CDrawImage::circle(int x, int y, int r, const CColor &color, float lineWidth) {
  _setColor(color.r, color.g, color.b, color.a);
  _circle(x, y, r, lineWidth);
}

void CDrawImage::poly(float x1, float y1, float x2, float y2, float x3, float y3, const CColor &color, bool fill) {
  float ptx[3] = {x1, x2, x3};
  float pty[3] = {y1, y2, y3};
  _setFillColor(color.r, color.g, color.b, color.a);
  _poly(ptx, pty, 3, true, fill);
}

void CDrawImage::poly(float x1, float y1, float x2, float y2, float x3, float y3, float lineWidth, const CColor &color, bool fill) {
  float ptx[3] = {x1, x2, x3};
  float pty[3] = {y1, y2, y3};
  _setFillColor(color.r, color.g, color.b, color.a);
  _poly(ptx, pty, 3, lineWidth, true, fill);
}

void CDrawImage::poly(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float lineWidth, const CColor &color, bool fill) {
  float ptx[4] = {x1, x2, x3, x4};
  float pty[4] = {y1, y2, y3, y4};
  _setFillColor(color.r, color.g, color.b, color.a);
  _poly(ptx, pty, 4, lineWidth, true, fill);
}

void CDrawImage::poly(float *x, float *y, int n, float lineWidth, const CColor &lineColor, const CColor &fillColor, bool close, bool fill) {
  _setColor(lineColor.r, lineColor.g, lineColor.b, lineColor.a);
  _setFillColor(fillColor.r, fillColor.g, fillColor.b, fillColor.a);
  _poly(x, y, n, lineWidth, close, fill);
}

void CDrawImage::poly(const std::vector<f8point> &polyPoints, float lineWidth, const CColor &lineColor, const CColor &fillColor, bool close, bool fill) {
  _setColor(lineColor.r, lineColor.g, lineColor.b, lineColor.a);
  _setFillColor(fillColor.r, fillColor.g, fillColor.b, fillColor.a);
  _poly(polyPoints, lineWidth, close, fill);
}

void CDrawImage::line(float x1, float y1, float x2, float y2, const CColor &ccolor) {
  if (currentLegend == NULL) return;
  _setColor(ccolor.r, ccolor.g, ccolor.b, ccolor.a);
  _line(x1, y1, x2, y2);
}

void CDrawImage::moveTo(float x1, float y1) {
  if (currentLegend == NULL) return;
  _moveTo(x1, y1);
}

void CDrawImage::lineTo(float x2, float y2, float w, const CColor &ccolor) {
  if (currentLegend == NULL) return;
  _setColor(ccolor.r, ccolor.g, ccolor.b, ccolor.a);
  _lineTo(x2, y2, w);
}

void CDrawImage::endLine(const double *dashes, int num_dashes) {
  if (currentLegend == NULL) return;
  _endLine(dashes, num_dashes);
}

CColor CDrawImage::getCColorFromLegendPaletteIndex(int colorIndex) const {
  // Transparent, not CColor()'s default opaque black: callers gate drawing on alpha>0, and an
  // out-of-range index (no legend, or a genuine off-by-one in a caller's own index math) must
  // mean "nothing to draw" rather than "draw opaque black".
  CColor color(0, 0, 0, 0);
  if (currentLegend == NULL) return color;
  if (colorIndex >= 0 && colorIndex < 256) {
    color = currentLegend->colors[colorIndex];
  }
  return color;
}

void CDrawImage::setPixelByLegendPaletteIndex(int x, int y, int colorIndex) {
  if (currentLegend == NULL) return;
  if (colorIndex >= 0 && colorIndex < 256) {
    const CColor &c = currentLegend->colors[colorIndex];
    if (c.a > 0) {
      _pixel_blend(x, y, c.r, c.g, c.b, c.a);
    }
  }
}

void CDrawImage::line(float x1, float y1, float x2, float y2, float w, const CColor &ccolor) {
  if (currentLegend == NULL) return;
  _setColor(ccolor.r, ccolor.g, ccolor.b, ccolor.a);
  _line(x1, y1, x2, y2, w);
}

void CDrawImage::getPixelTrueColor(int x, int y, unsigned char &r, unsigned char &g, unsigned char &b, unsigned char &a) { _getPixel(x, y, r, g, b, a); }

void CDrawImage::setPixelTrueColor(int x, int y, unsigned int color) { _pixel_blend(x, y, color, color / 256, color / (256 * 256), 255); }

void CDrawImage::setPixel(int x, int y, const CColor &color) { _pixel_blend(x, y, color.r, color.g, color.b, color.a); }

void CDrawImage::setPixelTrueColorOverWrite(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a) { _pixel_overwrite(x, y, r, g, b, a); }

void CDrawImage::setPixelTrueColor(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a) { _pixel_blend(x, y, r, g, b, a); }

void CDrawImage::setText(const char *text, int x, int y, const CColor &color) {
  if (currentLegend == NULL) return;
  currentFace = _getFontFace(TTFFontLocation, TTFFontSize);
  currentFontSize = TTFFontSize;
  _drawText(x, y + 10, 0, text, color);
}

void CDrawImage::setTextStroke(int x, int y, float angle, const char *text, const char *fontFile, float fontSize, float strokeWidth, const CColor &bgcolor, const CColor &fgcolor) {
  if (bgcolor.a == 0) {
    drawText(x, y, fontFile, fontSize, angle, text, fgcolor);
  } else {
    // Pre-existing behavior: this branch always uses the default font face (fontFile is only
    // honored by the plain-drawText path above) - fontSize here only scales the rendered glyphs.
    currentFace = _getFontFace(TTFFontLocation, TTFFontSize);
    _drawStrokedText(x, y, -angle, text, fontSize * 1.4, strokeWidth, bgcolor, fgcolor);
  }
}

void CDrawImage::drawText(int x, int y, float angle, const char *text, const CColor &fgcolor) {
  currentFace = _getFontFace(TTFFontLocation, TTFFontSize);
  currentFontSize = TTFFontSize;
  _drawText(x, y, angle, text, fgcolor);
}

int CDrawImage::drawTextArea(int x, int y, const char *fontfile, float size, float, const char *_text, const CColor &fgcolor, const CColor &bgcolor) {
  std::string text;
  int offset = 0;
  std::string title = _text;
  int length = title.length();
  currentFace = _getFontFace(fontfile, size);
  currentFontSize = size;
  float textY = 0;
  int width = geoParams.width - x;
  int widthOfText, heightOfText;
  //
  do {
    do {
      text.assign((const char *)(title.c_str() + offset), length);
      _getTextSize(widthOfText, heightOfText, 0.0, text.c_str());
      length--;
    } while (widthOfText > width && length >= 0);
    length++;
    if (length + offset < (int)title.length()) {
      int sl = length;
      while (text[sl] != ' ' && sl > 0) {
        sl--;
      }
      if (sl > 0) length = (sl + 1);
    }
    text.assign((const char *)(title.c_str() + offset), length);
    _getTextSize(widthOfText, heightOfText, 0.0, text.c_str());
    if (bgcolor.a != 0) {
      _fillRect(x - 3, y + textY + 5, x + widthOfText + 3, y + textY - 3 - heightOfText, bgcolor);
    }
    _drawText(x, y + textY, 0.0, text.c_str(), fgcolor);
    textY += size * 1.5;

    offset += length;
  } while (length < (int)(title.length()) - 1 && (int)offset < (int)title.length() - 1);

  return textY;
}

void CDrawImage::drawText(int x, int y, const char *fontfile, float size, float angle, const char *text, const CColor &fgcolor, const CColor &bgcolor) {
  currentFace = _getFontFace(fontfile, size);
  currentFontSize = size;
  _drawFilledText(x, y, angle, text, fgcolor, bgcolor);
}
void CDrawImage::setEllipse(int x, int y, float discRadiusX, float discRadiusY, float rotation, const CColor &fillColor, const CColor &lineColor) {
  if (currentLegend == NULL) return;
  _setFillColor(fillColor.r, fillColor.g, fillColor.b, fillColor.a);
  _setColor(lineColor.r, lineColor.g, lineColor.b, lineColor.a);
  _filledEllipse(x, y, discRadiusX, discRadiusY, rotation);
}

// Also serves callers with an integer radius: int -> float is a lossless implicit conversion, and
// _filledcircle() truncates to int either way.
void CDrawImage::setDisc(int x, int y, float discRadius, const CColor &fillColor, const CColor &lineColor) {
  if (currentLegend == NULL) return;
  _setFillColor(fillColor.r, fillColor.g, fillColor.b, fillColor.a);
  _setColor(lineColor.r, lineColor.g, lineColor.b, lineColor.a);
  _filledcircle(x, y, discRadius);
}

void CDrawImage::setTextDisc(int x, int y, int discRadius, const char *text, const char *fontfile, float fontsize, const CColor &textcolor, const CColor &fillcolor, const CColor &lineColor) {
  if (currentLegend == NULL) return;
  _setFillColor(fillcolor.r, fillcolor.g, fillcolor.b, fillcolor.a);
  _setColor(lineColor.r, lineColor.g, lineColor.b, lineColor.a);
  _filledcircle(x, y, discRadius);
  drawCenteredText(x, y, fontfile, fontsize, 0, text, textcolor);
}

void CDrawImage::drawCenteredText(int x, int y, const char *fontfile, float size, float angle, const char *text, const CColor &color, const CColor &textOutlineColor) {
  currentFace = _getFontFace(fontfile, size);
  currentFontSize = size;
  if (textOutlineColor.a == 0) {
    _drawCenteredText(x, y, angle, text, color);
  } else {
    _drawStrokedText(x, y, angle, text, size * 1.4, 1, textOutlineColor, color, true);
  }
}

void CDrawImage::drawCenteredTextNoOverlap(int x, int y, const char *fontFile, float size, float angle, int padding, const char *text, const CColor &color, bool noOverlap,
                                           std::vector<CRectangleText> &rects) {
  if (size <= 0) { // size 0 means do not draw label
    return;
  }

  int w, h;
  float radAngle = angle * M_PI / 180;
  CRectangleText rect;

  currentFace = _getFontFace(fontFile, size);
  currentFontSize = size;
  _getTextSize(w, h, radAngle, text);
  rect.init(x, y, (x + w), (y + h), angle, padding, text, fontFile, size, color);

  if (noOverlap) {
    for (size_t j = 0; j < rects.size(); j++) {
      if (rects[j].overlaps(rect)) {
        return;
      }
    }
  }
  rects.push_back(rect);
}

void CDrawImage::drawText(int x, int y, const char *fontfile, float size, float angle, const char *text, const CColor &color) {
  currentFace = _getFontFace(fontfile, size);
  currentFontSize = size;
  _drawText(x, y, angle, text, color);
}

int CDrawImage::create685Palette() {
  currentLegend = NULL;
  const char *paletteName685 = "685Palette";

  for (size_t j = 0; j < legends.size(); j++) {
    if (legends[j]->legendName == (paletteName685)) {
      CDBDebug("Found legend");
      currentLegend = legends[j];
      return 0;
    }
  }

  if (currentLegend == NULL) {
    currentLegend = new CLegend();
    currentLegend->legendName = paletteName685;
    legends.push_back(currentLegend);
  }

  if (dImageCreated == 0) {
    CDBError("createPalette: image not created");
    return 1;
  }

  int j = 0;
  for (int r = 0; r < 6; r++)
    for (int g = 0; g < 8; g++)
      for (int b = 0; b < 5; b++) {
        addColor(j++, r * 51, g * 36, b * 63);
      }

  // Indices 240-255 are the same fixed "standard" entries as every other palette type.
  return _createStandard();
}

int CDrawImage::_createStandard() {

  addColor(240, 0, 0, 0);
  addColor(241, 32, 32, 32);
  addColor(242, 64, 64, 64);
  addColor(243, 96, 96, 96);
  addColor(244, 64, 64, 255);
  addColor(245, 128, 128, 255);
  addColor(246, 64, 64, 192);
  addColor(247, 32, 32, 32);
  addColor(248, 0, 0, 0);
  addColor(249, 255, 255, 191);
  addColor(250, 191, 232, 255);
  addColor(251, 204, 204, 204);
  addColor(252, 160, 160, 160);
  addColor(253, 192, 192, 192);
  addColor(254, 224, 224, 224);
  addColor(255, 255, 255, 255);

  copyPalette();
  return 0;
}

static void setPaletteColor(CLegend *legend, int index, int red, int green, int blue, int alpha) {
  unsigned char r = (unsigned char)red;
  if (r == 0) r = 1; // avoid copyPalette()'s auto-transparency rule mistaking this for an unset entry
  legend->colors[index] = CColor(r, (unsigned char)green, (unsigned char)blue, (unsigned char)(alpha < 0 ? 0 : alpha));
}

int CDrawImage::createPalette(CServerConfig::XMLE_Legend *legend) {
  currentLegend = NULL;
  if (legend != NULL) {
    for (size_t j = 0; j < legends.size(); j++) {
      if (legends[j]->legendName == (legend->attr.name.c_str())) {
        currentLegend = legends[j];
        return 0;
      }
    }
  }
  if (currentLegend == NULL) {
    currentLegend = new CLegend();
    currentLegend->legendName = legend->attr.name.c_str();
    legends.push_back(currentLegend);
  }

  if (dImageCreated == 0) {
    CDBError("createPalette: image not created");
    return 1;
  }

  for (int j = 0; j < 256; j++) {
    currentLegend->colors[j] = CColor(0, 0, 0, 255);
  }
  if (legend == NULL) {
    return _createStandard();
  }
  if (legend->attr.type == ("colorRange")) {

    float cx;
    float rc[4];
    for (size_t j = 0; j < legend->palette.size(); j++) {
      CServerConfig::XMLE_palette *pbegin = &legend->palette[j];
      CServerConfig::XMLE_palette *pnext = &legend->palette[j];
      if (j < legend->palette.size() - 1) {
        pnext = &legend->palette[j + 1];
      }

      if (pbegin->attr.index > 255) pbegin->attr.index = 255;
      if (pbegin->attr.index < 0) pbegin->attr.index = 0;
      if (pnext->attr.index > 255) pnext->attr.index = 255;
      if (pnext->attr.index < 0) pnext->attr.index = 0;
      float dif = pnext->attr.index - pbegin->attr.index;
      if (dif < 0.5f) dif = 1;
      rc[0] = float(pnext->attr.red - pbegin->attr.red) / dif;
      rc[1] = float(pnext->attr.green - pbegin->attr.green) / dif;
      rc[2] = float(pnext->attr.blue - pbegin->attr.blue) / dif;
      rc[3] = float(pnext->attr.alpha - pbegin->attr.alpha) / dif;

      for (int i = pbegin->attr.index; i < pnext->attr.index + 1; i++) {
        if (i >= 0 && i < 240) {
          cx = float(i - pbegin->attr.index);
          setPaletteColor(currentLegend, i, int(rc[0] * cx) + pbegin->attr.red, int(rc[1] * cx) + pbegin->attr.green, int(rc[2] * cx) + pbegin->attr.blue, int(rc[3] * cx) + pbegin->attr.alpha);
        }
      }
    }

    return _createStandard();
  }
  if (legend->attr.type == ("interval")) {

    for (size_t j = 0; j < legend->palette.size(); j++) {

      if (legend->palette[j].attr.index != -1) {

        int startIndex = legend->palette[j].attr.index;
        int stopIndex = 240;
        if (j < legend->palette.size() - 1) stopIndex = legend->palette[j + 1].attr.index;

        for (int i = startIndex; i < stopIndex; i++) {
          if (i >= 0 && i < 240) {
            auto &attr = legend->palette[j].attr;
            setPaletteColor(currentLegend, i, attr.red, attr.green, attr.blue, attr.alpha);
          }
        }
      } else {
        for (int i = legend->palette[j].attr.min; i <= legend->palette[j].attr.max; i++) {
          if (i >= 0 && i < 240) {
            auto &attr = legend->palette[j].attr;
            setPaletteColor(currentLegend, i, attr.red, attr.green, attr.blue, attr.alpha);
          }
        }
      }
    }
    return _createStandard();
  }
  if (legend->attr.type == ("svg")) {
    if (legend->attr.file.empty()) {
      CDBError("Legend type file has no file attribute specified");
      return 1;
    }
    CDBDebug("Reading file %s", legend->attr.file.c_str());

    CXMLParserElement element;

    try {
      element.parseFile(legend->attr.file.c_str());
      auto stops = element.getThrows("svg")->getThrows("g")->getThrows("defs")->getThrows("linearGradient")->getList("stop");
      float cx;
      float rc[4];
      unsigned char prev_red = 0, prev_green = 0, prev_blue = 0, prev_alpha = 0;
      int prev_offset = 0;
      for (size_t j = 0; j < stops.size(); j++) {
        int offset = (int)(std::stof(stops.at(j).getAttrValue("offset")) * 2.4);
        std::string color = stops.at(j).getAttrValue("stop-color").c_str() + 4;
        color.resize(color.length() - 1);
        auto colors = CT::split(color, ",");
        if (colors.size() != 3) {
          CDBError("Number of specified colors is unequal to three");
          return 1;
        }
        unsigned char red = atoi(colors[0].c_str());
        unsigned char green = atoi(colors[1].c_str());
        unsigned char blue = atoi(colors[2].c_str());
        unsigned char alpha = (char)(std::stof(stops.at(j).getAttrValue("stop-opacity")) * 255);
        if (offset > 255)
          offset = 255;
        else if (offset < 0)
          offset = 0;
        /*---------------*/

        if (j == 0) {
          prev_red = red;
          prev_green = green;
          prev_blue = blue;
          prev_alpha = alpha;
          prev_offset = offset;
        } else {
          float dif = offset - prev_offset;
          if (dif < 0.5f) dif = 1;
          rc[0] = float(prev_red - red) / dif;
          rc[1] = float(prev_green - green) / dif;
          rc[2] = float(prev_blue - blue) / dif;
          rc[3] = float(prev_alpha - alpha) / dif;

          for (int i = prev_offset; i < offset + 1; i++) {
            if (i >= 0 && i < 240) {
              cx = float(i - prev_offset);
              setPaletteColor(currentLegend, i, int(rc[0] * cx) + red, int(rc[1] * cx) + green, int(rc[2] * cx) + blue, int(rc[3] * cx) + alpha);
            }
          }
        }
        prev_red = red;
        prev_green = green;
        prev_blue = blue;
        prev_alpha = alpha;
        prev_offset = offset;
        /*---------------*/
      }

    } catch (int e) {
      std::string message = CXMLParser::getErrorMessage(e);
      CDBError("%s\n", message.c_str());
      return 1;
    }
    return _createStandard();
  }
  return 1;
}

void CDrawImage::rectangle(int x1, int y1, int x2, int y2, const CColor &innercolor, const CColor &outercolor) {
  _setFillColor(innercolor.r, innercolor.g, innercolor.b, innercolor.a);
  _setColor(outercolor.r, outercolor.g, outercolor.b, outercolor.a);
  _filledRectangle(x1, y1, x2, y2);
}

int CDrawImage::addColor(int Color, unsigned char R, unsigned char G, unsigned char B) {
  if (currentLegend == NULL) return 0;
  currentLegend->colors[Color] = CColor(R, G, B, 255);
  return 0;
}

int CDrawImage::copyPalette() {
  for (int j = 0; j < 256; j++) {
    if (dPaletteCreated == 0) {
      colors[j] = _colors[j];
    }

    if (j < 240) {
      CColor &c = currentLegend->colors[j];
      if (c.r == 0 && c.g == 0 && c.b == 0) {
        if (dPaletteCreated == 0) {
          colors[j] = _colors[255];
        }
        c.a = 0;
      }
    }
  }
  dPaletteCreated = 1;
  return 0;
}

void CDrawImage::enableTransparency(bool enable) { _bEnableTransparency = enable; }
void CDrawImage::setBGColor(unsigned char R, unsigned char G, unsigned char B) {
  BGColorR = R;
  BGColorG = G;
  BGColorB = B;
}

void CDrawImage::setTrueColor(bool enable) { _bEnableTrueColor = enable; }

bool CDrawImage::isPixelTransparent(int x, int y) const {
  unsigned char r, g, b, a;
  _getPixel(x, y, r, g, b, a);

  if (a == 0)
    return true;
  else {
    if (r == BGColorR && g == BGColorG && b == BGColorB && 1 == 2) {
      return true;
    } else {
      return false;
    }
  }
  return false;
}

/**
 * Returns a value indicating how width the image is based on whether there are pixels set.
 * Useful for cropping the image
 */

void CDrawImage::getCanvasSize(int &x1, int &y1, int &w, int &h) const {
  w = 0;
  x1 = geoParams.width;
  for (int y = 0; y < geoParams.height; y++) {
    for (int x = w; x < geoParams.width; x++)
      if (!isPixelTransparent(x, y)) w = x;
    for (int x = 0; x < x1; x++)
      if (!isPixelTransparent(x, y)) x1 = x;
  }

  w = w - x1 + 1;

  h = 0;
  y1 = geoParams.height;

  for (int x = x1; x < w; x++) {
    for (int y = h; y < geoParams.height; y++)
      if (!isPixelTransparent(x, y)) h = y;
    for (int y = 0; y < y1; y++)
      if (!isPixelTransparent(x, y)) y1 = y;
  }

  h = h - y1 + 1;
  if (w < 0) w = 0;
  if (h < 0) h = 0;
}

int CDrawImage::clonePalette(const CDrawImage *drawImage) {
  if (drawImage->currentLegend == NULL) {
    currentLegend = NULL;
    return 0;
  }
  currentLegend = new CLegend(*drawImage->currentLegend);
  legends.push_back(currentLegend);
  BGColorR = drawImage->BGColorR;
  BGColorG = drawImage->BGColorG;
  BGColorB = drawImage->BGColorB;
  copyPalette();
  return 0;
}

/**
 * Creates a new image with the same settings but with different size as the source image
 */
int CDrawImage::createImage(const CDrawImage *image, int width, int height) {
  if (height < 0) {
    height = 0;
  }
  if (width < 0) {
    width = 0;
  }
  StopWatch_Measure("createImage(CDrawImage *image,int width,int height)");
  enableTransparency(image->_bEnableTransparency);
  setTTFFontLocation(image->TTFFontLocation);

  setTTFFontSize(image->TTFFontSize);
  createImage(width, height);
  clonePalette(image);
  return 0;
}

int CDrawImage::setCanvasSize(int x, int y, int width, int height) {
  CDrawImage temp;
  temp.createImage(this, width, height);
  temp.draw(0, 0, x, y, this);

  destroyImage();

  createImage(&temp, width, height);
  draw(0, 0, 0, 0, &temp);
  temp.destroyImage();
  return 0;
}

int CDrawImage::draw(int destx, int desty, int sourcex, int sourcey, const CDrawImage *simage) {
  unsigned char r, g, b, a;
  for (int y = 0; y < simage->geoParams.height; y++) {
    for (int x = 0; x < simage->geoParams.width; x++) {
      int sx = x + sourcex;
      int sy = y + sourcey;
      int dx = x + destx;
      int dy = y + desty;
      if (sx >= 0 && sy >= 0 && dx >= 0 && dy >= 0 && sx < simage->geoParams.width && sy < simage->geoParams.height && dx < geoParams.width && dy < geoParams.height) {
        // Get source r,g,b,a
        simage->_getPixel(sx, sy, r, g, b, a);
        // Set r,g,b,a to dest
        _pixel_blend(dx, dy, r, g, b, a);
      }
    }
  }
  return 0;
}

int CDrawImage::drawrotated(int destx, int desty, int sourcex, int sourcey, const CDrawImage *simage) {
  unsigned char r, g, b, a;
  for (int y = 0; y < simage->geoParams.height; y++) {
    for (int x = 0; x < simage->geoParams.width; x++) {
      int sx = x + sourcex;
      int sy = y + sourcey;
      int dx = simage->geoParams.height - y - desty;
      int dy = x + destx;
      if (sx >= 0 && sy >= 0 && dx >= 0 && dy >= 0 && sx < simage->geoParams.width && sy < simage->geoParams.height && dx < geoParams.width && dy < geoParams.height) {
        // Get source r,g,b,a
        simage->_getPixel(sx, sy, r, g, b, a);
        // Set r,g,b,a to dest
        _pixel_blend(dx, dy, r, g, b, a);
      }
    }
  }
  return 0;
}

/**
 * Crops image
 * @param int paddingW the padding to keep in pixels in width. Set to -1 if no crop in width is desired
 * @param int paddingH the padding to keep in pixels in height. Set to -1 if no crop in height is desired
 */
void CDrawImage::crop(int paddingW, int paddingH) {
  int x, y, w, h;
  getCanvasSize(x, y, w, h);

  int x1 = x - paddingW;
  int y1 = y - paddingH;
  int w1 = w + paddingW * 2;
  int h1 = h + paddingH * 2;
  if (x1 < 0) {
    x1 = 0;
  };
  if (y1 < 0) {
    y1 = 0;
  }
  if (x1 > geoParams.width) {
    x1 = geoParams.width;
  };
  if (y1 > geoParams.height) {
    y1 = geoParams.height;
  }
  if (paddingW < 0) {
    x1 = 0;
    w1 = geoParams.width;
  }
  if (paddingH < 0) {
    y1 = 0;
    h1 = geoParams.height;
  }
  if (h1 > geoParams.height - y1) h1 = geoParams.height - y1;
  if (w1 > geoParams.width - x1) w1 = geoParams.width - x1;

  setCanvasSize(x1, y1, w1, h1);
}

/**
 * Crops image with desired padding
 * @param padding The number of empty pixels surrounding the new image
 */
void CDrawImage::crop(int padding) { crop(padding, padding); }

void CDrawImage::rotate() {
  int w = geoParams.width;
  int h = geoParams.height;

  CDrawImage temp;
  temp.createImage(this, w, h);

  temp.draw(0, 0, 0, 0, this);

  destroyImage();
  CDBDebug("Creating rotated legend %d,%d", h, w);
  createImage(&temp, h, w);
  drawrotated(0, 0, 0, 0, &temp);
  temp.destroyImage();
}

unsigned char *CDrawImage::getCanvasMemory() const { return ARGBByteBuffer; }

int CDrawImage::getHeight() const { return geoParams.height; }

int CDrawImage::getWidth() const { return geoParams.width; }

std::string CDrawImage::getFontLocation() const { return this->TTFFontLocation; }

float CDrawImage::getFontSize() const { return this->TTFFontSize; }

int CDrawImage::getTextWidth(const std::string &text, const std::string &fontfile, float size, int angle) {
  currentFace = _getFontFace(fontfile, size);
  currentFontSize = size;
  int w = 0, h = 0;
  _getTextSize(w, h, angle, text.c_str());
  return w;
}

/* -------------------------------------------------------------------------------------------- *
 * Cairo/FreeType primitives below. These used to live in a separate CCairoPlotter class that
 * CDrawImage merely delegated to; folded in directly since every instance shared the same single
 * pixel buffer anyway (the one allocated in createImage() above) - see CDrawImage.h for why
 * alternate fonts only need a swapped-in FT_Face now, not a whole second Cairo context.
 * -------------------------------------------------------------------------------------------- */

void CDrawImage::_swap(float &x, float &y) const {
  float t = x;
  x = y;
  y = t;
}
void CDrawImage::_swap(int &x, int &y) const {
  int t = x;
  x = y;
  y = t;
}

void CDrawImage::_setColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
  this->r = r;
  rr = r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
  this->g = g;
  rg = g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
  this->b = b;
  rb = b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
  this->a = (float)a;
  ra = a * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
}

void CDrawImage::_setFillColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
  fr = r;
  rfr = r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
  fg = g;
  rfg = g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
  fb = b;
  rfb = b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
  fa = float(a);
  rfa = a * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
}

void CDrawImage::_getPixel(int x, int y, unsigned char &r, unsigned char &g, unsigned char &b, unsigned char &a) const {
  if (x < 0 || y < 0 || x >= geoParams.width || y >= geoParams.height) {
    r = 0;
    b = 0;
    g = 0;
    a = 0;
  }
  size_t p = x * 4 + y * stride;
  b = ARGBByteBuffer[p];
  g = ARGBByteBuffer[p + 1];
  r = ARGBByteBuffer[p + 2];
  a = ARGBByteBuffer[p + 3];
}

void CDrawImage::_pixel_overwrite(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
  if (x < 0 || y < 0 || x >= geoParams.width || y >= geoParams.height) return;
  if (a != 0 && a != 255) {
    isAlphaUsed = true;
  }
  size_t p = x * 4 + y * stride;
  ARGBByteBuffer[p] = b;
  ARGBByteBuffer[p + 1] = g;
  ARGBByteBuffer[p + 2] = r;
  ARGBByteBuffer[p + 3] = a;
}

void CDrawImage::_pixel_blend(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
  if (x < 0 || y < 0 || x >= geoParams.width || y >= geoParams.height) return;

  size_t p = x * 4 + y * stride;
  // Pixel is transparent
  if (a != 255) {

    float destBlue = b;
    float destGreen = g;
    float destRed = r;
    float destAlpha = a;

    unsigned char origAlphaC = ARGBByteBuffer[p + 3];

    // Background is transparent, combine Background with pixel
    if (origAlphaC != 255) {
      float origAlpha = float(origAlphaC);

      float origBlue = ARGBByteBuffer[p];
      float origGreen = ARGBByteBuffer[p + 1];
      float origRed = ARGBByteBuffer[p + 2];

      destBlue = destBlue * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
      destGreen = destGreen * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
      destRed = destRed * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
      destAlpha = destAlpha * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;

      origBlue = origBlue * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
      origGreen = origGreen * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
      origRed = origRed * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
      origAlpha = origAlpha * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;

      float A1 = origAlpha * (1 - destAlpha);
      float A2 = (destAlpha + A1);

      float newBlue = (destBlue * destAlpha + origBlue * A1) / A2;
      float newGreen = (destGreen * destAlpha + origGreen * A1) / A2;
      float newRed = (destRed * destAlpha + origRed * A1) / A2;
      float newAlpha = origAlpha + destAlpha * (1 - origAlpha);

      unsigned char aa = newAlpha * 255.;
      ARGBByteBuffer[p] = newBlue * 255;
      ARGBByteBuffer[p + 1] = newGreen * 255;
      ARGBByteBuffer[p + 2] = newRed * 255;
      ARGBByteBuffer[p + 3] = aa;
      if (aa != 255 && aa != 0) {
        isAlphaUsed = true;
      }
    } else {
      //         Background is not transparent, but pixel is. Alpha can be left to 255, adjust color values
      float origBlue = ARGBByteBuffer[p];
      float origGreen = ARGBByteBuffer[p + 1];
      float origRed = ARGBByteBuffer[p + 2];
      destBlue = destBlue * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
      destGreen = destGreen * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
      destRed = destRed * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
      destAlpha = destAlpha * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
      origBlue = origBlue * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
      origGreen = origGreen * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
      origRed = origRed * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
      float A1 = 1 - destAlpha;
      float newBlue = (destBlue * destAlpha + origBlue * A1);
      float newGreen = (destGreen * destAlpha + origGreen * A1);
      float newRed = (destRed * destAlpha + origRed * A1);
      ARGBByteBuffer[p] = newBlue * 255;
      ARGBByteBuffer[p + 1] = newGreen * 255;
      ARGBByteBuffer[p + 2] = newRed * 255;
      ARGBByteBuffer[p + 3] = 255;
    }
  } else {
    ARGBByteBuffer[p] = b;
    ARGBByteBuffer[p + 1] = g;
    ARGBByteBuffer[p + 2] = r;
    ARGBByteBuffer[p + 3] = 255;
  }
}

void CDrawImage::_rectangle(int x1, int y1, int x2, int y2) {
  cairo_rectangle(cr, x1 + 0.5, y1 + 0.5, x2 - x1, y2 - y1);
  cairo_set_source_rgba(cr, rr, rg, rb, a);
  cairo_stroke(cr);
}

void CDrawImage::_filledRectangle(int x1, int y1, int x2, int y2) {
  if (y1 > y2) _swap(y1, y2);

  cairo_rectangle(cr, x1, y1, x2 - x1, y2 - y1);
  cairo_set_source_rgba(cr, rfr, rfg, rfb, rfa);
  cairo_fill(cr);
  cairo_antialias_t aa = cairo_get_antialias(cr);
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
  cairo_set_line_width(cr, 0.5);
  _rectangle(x1, y1, x2, y2);
  cairo_set_antialias(cr, aa);
}

void CDrawImage::_moveTo(float x1, float y1) { cairo_move_to(cr, x1 + 0.5, y1 + 0.5); }

void CDrawImage::_lineTo(float x1, float y1, float width) {
  cairo_set_source_rgba(cr, rr, rg, rb, ra);
  cairo_set_line_width(cr, width);
  cairo_line_to(cr, x1 + 0.5, y1 + 0.5);
}

void CDrawImage::_endLine(const double *dashes, int num_dashes) {
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
  if (dashes != nullptr) {
    cairo_set_dash(cr, dashes, num_dashes, 0);
  }
  cairo_stroke(cr);
  if (dashes != nullptr) {
    cairo_set_dash(cr, 0, 0, 0);
  }
}

void CDrawImage::_line(float x1, float y1, float x2, float y2) { _line(x1, y1, x2, y2, 0.9f); }

void CDrawImage::_line(float x1, float y1, float x2, float y2, float width) {
  cairo_set_source_rgba(cr, rr, rg, rb, ra);
  cairo_move_to(cr, x1 + 0.5, y1 + 0.5);
  cairo_line_to(cr, x2 + 0.5, y2 + 0.5);
  cairo_set_line_width(cr, width);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
  cairo_stroke(cr);
}

void CDrawImage::_filledcircle(int x, int y, int r) {
  cairo_save(cr);
  cairo_set_source_rgba(cr, rfr, rfg, rfb, rfa);
  cairo_arc(cr, x, y, r, 0, 2 * M_PI);
  cairo_fill(cr);

  cairo_arc(cr, x, y, r, 0, 2 * M_PI);
  cairo_set_line_width(cr, 0.8);
  cairo_set_source_rgba(cr, rr, rg, rb, ra);
  cairo_stroke(cr);
  cairo_restore(cr);
}

void CDrawImage::_filledEllipse(int x, int y, float rX, float rY, float rotation) {
  cairo_save(cr);
  cairo_set_source_rgba(cr, rfr, rfg, rfb, rfa);
  cairo_translate(cr, x, y);
  cairo_rotate(cr, rotation);
  cairo_scale(cr, rX, rY);
  cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);
  cairo_fill(cr);
  cairo_restore(cr);
}

void CDrawImage::_circle(int x, int y, int r, float lineWidth) {
  double lWx = lineWidth;
  double lWy = lineWidth;
  cairo_device_to_user_distance(cr, &lWx, &lWy);
  cairo_set_source_rgba(cr, rr, rg, rb, ra);
  cairo_set_line_width(cr, lWx);
  cairo_arc(cr, x, y, r, 0, 2 * M_PI);
  cairo_stroke(cr);
}

void CDrawImage::_poly(float x[], float y[], int n, bool closePath, bool fill) {
  cairo_move_to(cr, x[0], y[0]);
  cairo_antialias_t aa = cairo_get_antialias(cr);
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
  for (int i = 1; i < n; i++) {
    cairo_line_to(cr, x[i], y[i]);
  }
  if (closePath) {
    cairo_close_path(cr);
    if (fill) {
      cairo_set_source_rgba(cr, rfr, rfg, rfb, rfa);
      cairo_fill_preserve(cr);
    }
  }
  cairo_set_source_rgba(cr, rr, rg, rb, ra);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
  cairo_stroke(cr);
  cairo_set_antialias(cr, aa);
}

void CDrawImage::_poly(const std::vector<f8point> &polyPoints, float lineWidth, bool closePath, bool fill) {
  if (polyPoints.size() < 2) return;
  double lWx = lineWidth;
  double lWy = lineWidth;

  cairo_antialias_t aa = cairo_get_antialias(cr);
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

  cairo_device_to_user_distance(cr, &lWx, &lWy);
  cairo_set_line_width(cr, lWx);

  cairo_move_to(cr, polyPoints[0].x, polyPoints[0].y);
  for (const auto &polyPoint: polyPoints) {
    cairo_line_to(cr, polyPoint.x, polyPoint.y);
  }
  if (closePath) {
    cairo_close_path(cr);
    if (fill) {
      cairo_set_source_rgba(cr, rfr, rfg, rfb, rfa);
      cairo_fill_preserve(cr);
    }
  }
  cairo_set_source_rgba(cr, rr, rg, rb, ra);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
  cairo_stroke(cr);
  cairo_set_antialias(cr, aa);
}

void CDrawImage::_poly(float x[], float y[], int n, float lineWidth, bool closePath, bool fill) {
  double lWx = lineWidth;
  double lWy = lineWidth;

  cairo_antialias_t aa = cairo_get_antialias(cr);
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);

  cairo_device_to_user_distance(cr, &lWx, &lWy);
  cairo_set_line_width(cr, lWx);

  cairo_move_to(cr, x[0], y[0]);
  for (int i = 1; i < n; i++) {
    cairo_line_to(cr, x[i], y[i]);
  }
  if (closePath) {
    cairo_close_path(cr);
    if (fill) {
      cairo_set_source_rgba(cr, rfr, rfg, rfb, rfa);
      cairo_fill_preserve(cr);
    }
  }
  cairo_set_source_rgba(cr, rr, rg, rb, ra);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
  cairo_stroke(cr);
  cairo_set_antialias(cr, aa);
}

void CDrawImage::_writeToPng24Stream(FILE *fp) { writePng(geoParams.width, geoParams.height, ARGBByteBuffer, fp, 24, false); }

void CDrawImage::_writeToPng8Stream(FILE *fp, bool use8bitpalAlpha) { writePng(geoParams.width, geoParams.height, ARGBByteBuffer, fp, 8, use8bitpalAlpha); }

void CDrawImage::_writeToPng32Stream(FILE *fp, unsigned char alpha) {
  if (isAlphaUsed) {
    for (int y = 0; y < geoParams.height; y++) {
      for (int x = 0; x < geoParams.width; x++) {
        size_t p = x * 4 + y * stride;
        if (ARGBByteBuffer[p + 3] != 255) {
          float pa = ARGBByteBuffer[p + 3];
          ARGBByteBuffer[p] = (unsigned char)((float(ARGBByteBuffer[p]) * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL) * float(pa));
          ARGBByteBuffer[p + 1] = (unsigned char)((float(ARGBByteBuffer[p + 1]) * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL) * float(pa));
          ARGBByteBuffer[p + 2] = (unsigned char)((float(ARGBByteBuffer[p + 2]) * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL) * float(pa));
        }
      }
    }
  }
  cairo_set_operator(cr, CAIRO_OPERATOR_DEST_IN);
  if (alpha != 255) {
    cairo_paint_with_alpha(cr, float(alpha) * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL);
  }
  cairo_surface_flush(surface);
  cairo_surface_write_to_png_stream(surface, pngWriterFunc, (void *)fp);
}

void CDrawImage::_setToSurface(cairo_surface_t *png) {
  cairo_set_source_surface(this->cr, png, 0, 0);
  cairo_paint(this->cr);
}

void CDrawImage::_writeToWebP32Stream(FILE *fp, int quality) {
#ifdef ADAGUC_USE_WEBP
  // https://developers.google.com/speed/webp/docs/api

  WebPConfig config;
  WebPPicture picture;
  if (!WebPPictureInit(&picture) || !WebPConfigInit(&config)) {
    CDBError("Error! WEBP Version mismatch!\n");
    return;
  }

  config.lossless = quality == 100 ? 1 : 0; // Lossless encoding (0=lossy(default), 1=lossless).
  config.quality = quality;                 // between 0 and 100. For lossy, 0 gives the smallest
  config.method = 0;                        // quality/speed trade-off (0=fast, 6=slower-better)
  config.alpha_quality = 100;               // Between 0 (smallest size) and 100 (lossless). Default is 100.
  config.pass = 1;                          // number of entropy-analysis passes (in [1..10]).
  config.preprocessing = 0;                 // preprocessing filter (0=none, 1=segment-smooth)
  config.partitions = 0;                    // log2(number of token partitions) in [0..3] Default is set to 0 for easier progressive decoding.
  config.partition_limit = 100;             // quality degradation allowed to fit the 512k limit on prediction modes coding (0: no degradation, 100: maximum possible degradation).
  // Lossless needs ARGB input. For lossy, YUVA input is faster: the RGB to YUV conversion is then done during import.
  // The encoded result is the same for both.
  picture.use_argb = config.lossless ? 1 : 0; // To select between ARGB and YUVA input.
  config.thread_level = 1; // ~20-30% faster encoding, same output size (benchmarked on 1200x1000-3600x3000 lossy encodes).
  picture.width = geoParams.width;
  picture.height = geoParams.height;
  picture.writer = webPWriter;
  picture.custom_ptr = (void *)fp;

  if (!WebPValidateConfig(&config)) {
    CDBError("Error! Invalid configuration.");
    return;
  }

  StopWatch_Measure("writeToWebP32Stream: start import %dx%d", geoParams.width, geoParams.height);
  bool importOk = WebPPictureImportBGRA(&picture, ARGBByteBuffer, stride);
  if (!importOk) {
    CDBError("Error! Cannot import picture for WebP");
    WebPPictureFree(&picture);
    return;
  }
  StopWatch_Measure("writeToWebP32Stream: done import, start encode");

  if (!WebPEncode(&config, &picture)) {
    CDBError("Error!  Cannot encode picture as WebP");
  }
  StopWatch_Measure("writeToWebP32Stream: done encode");
  WebPPictureFree(&picture);

#else
  CDBError("-DADAGUC_USE_WEBP not enabled");
#endif
}

void CDrawImage::_drawBarb(int x, int y, double uncorrectedDirection, double viewDirCorrection, double strength, const CColor &barbColor, const CColor &outlineColor, float lineWidth, bool toKnots, bool flip,
                           bool drawText, double fontSize, const CColor &textColor, double outlineWidth) {
  // Barb settings
  float centerDiscRadius = 3;
  int shaftLength = 37;
  int barbLength = 12;
  int halfBarbLength = 6;
  double direction = uncorrectedDirection + viewDirCorrection;

  // Preserve path
  cairo_save(cr);
  cairo_path_t *cp = cairo_copy_path(cr);
  cairo_new_path(cr);
  cairo_set_line_width(cr, lineWidth);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);

  int strengthInKnots = round(strength);
  if (toKnots) {
    strengthInKnots = round(strength * 3600 / 1852.);
  }

  // Rounded to the nearest 5 kts
  int strengthInKnotsRoundedToFive = round((strengthInKnots + 2) / 5) * 5;

  int nPennants = strengthInKnotsRoundedToFive / 50;
  int nBarbs = (strengthInKnotsRoundedToFive % 50) / 10 + 0.5;
  bool hasHalfBarb = strengthInKnotsRoundedToFive % 10 >= 5;
  float flipFactor = flip ? -1 : 1;
  int barbLengthWithFlip = int(-barbLength * flipFactor);
  int halfBarbLengthWithFlip = int(-halfBarbLength * flipFactor);

  if (strengthInKnotsRoundedToFive <= 2) {
    // https://www.weather.gov/hfo/windbarbinfo
    // When wind speeds are 2 kts or less, a small open circle is used.
    cairo_arc(cr, x, y, 6, 0, 2 * M_PI);
  } else {

    double dx1 = cos(direction) * (shaftLength);
    double dy1 = sin(direction) * (shaftLength);

    double wx1 = double(x) - dx1;
    double wy1 = double(y) + dy1; // wind barb top (flag side)

    // Draw small center circle
    cairo_arc(cr, x, y, centerDiscRadius, 0, 2 * M_PI);

    // Draw main shaft from center to end
    cairo_move_to(cr, wx1, wy1);
    cairo_line_to(cr, x - cos(direction) * (centerDiscRadius), y + sin(direction) * (centerDiscRadius));

    // Draw flags
    int nrPos = 10;
    int pos = drawBarbTriangle(cr, x, y, nPennants, direction, shaftLength, barbLengthWithFlip, nrPos);

    // Draw full Barb
    if (nPennants > 0) pos++;
    for (int i = 0; i < nBarbs; i++) {
      double wx3 = wx1 + pos * dx1 / nrPos;
      double wy3 = wy1 - pos * dy1 / nrPos;
      double hx3 = wx3 - cos(M_PI / 2 - direction + (2 - float(flipFactor) * 0.1) * M_PI / 2) * barbLengthWithFlip; // was: +cos
      double hy3 = wy3 - sin(M_PI / 2 - direction + (2 - float(flipFactor) * 0.1) * M_PI / 2) * barbLengthWithFlip; // was: -sin
      cairo_move_to(cr, wx3, wy3);
      cairo_line_to(cr, hx3, hy3);
      pos++;
    }

    if ((nPennants + nBarbs) == 0) pos++;

    // Draw half Barb
    if (hasHalfBarb) {
      double wx3 = wx1 + pos * dx1 / nrPos;
      double wy3 = wy1 - pos * dy1 / nrPos;
      double hx3 = wx3 - cos(M_PI / 2 - direction + (2 - float(flipFactor) * 0.1) * M_PI / 2) * halfBarbLengthWithFlip;
      double hy3 = wy3 - sin(M_PI / 2 - direction + (2 - float(flipFactor) * 0.1) * M_PI / 2) * halfBarbLengthWithFlip;

      cairo_move_to(cr, wx3, wy3);
      cairo_line_to(cr, hx3, hy3);
      pos++;
    }
  }
  // No dash
  cairo_set_dash(cr, 0, 0, 0);

  if (outlineWidth > 0) {
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, outlineColor.r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, outlineColor.g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, outlineColor.b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, .2);
    cairo_set_line_width(cr, 1.0 + outlineWidth);
    cairo_stroke_preserve(cr);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(cr, outlineColor.r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, outlineColor.g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, outlineColor.b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, 1);
    cairo_set_line_width(cr, outlineWidth);
    cairo_stroke_preserve(cr);
  }

  // Stroke thin version
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, barbColor.r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, barbColor.g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, barbColor.b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, 1);
  cairo_set_line_width(cr, lineWidth);
  cairo_stroke(cr);

  if (strengthInKnotsRoundedToFive > 2) {
    drawBarbTriangle(cr, x, y, nPennants, direction, shaftLength, barbLengthWithFlip, 10);
    cairo_close_path(cr);
    cairo_set_source_rgba(cr, barbColor.r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, barbColor.g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, barbColor.b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, 1);
    cairo_fill_preserve(cr);
  }

  // End end restore
  cairo_close_path(cr);
  cairo_restore(cr);
  cairo_append_path(cr, cp);
  cairo_path_destroy(cp);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  if (drawText) {
    CColor barbTextColor = textColor.a < 1 ? barbColor : textColor;
    bool showDirection = false;
    bool drawOutline = outlineWidth > .01;
    if (showDirection == false) {
      std::string text;
      text = CT::printf("%d", strengthInKnots);

      // If speed is really low, draw the text below the circle
      double textDirection = strengthInKnotsRoundedToFive <= 2 ? -M_PI / 2.1 : direction;
      this->_drawStrokedText(x - cos(textDirection + M_PI) * 15 - 5, y + sin(textDirection + M_PI) * 12 + 5, 0, text.c_str(), fontSize, 1 * drawOutline, outlineColor, barbTextColor);
    } else {
      double degrees = fmod(((270 - ((uncorrectedDirection) * (180 / M_PI)))), 360);
      std::string text;
      text = CT::printf("%02d %03d°", strengthInKnots, int(round(degrees)));

      // If speed is really low, draw the text below the circle
      double textDirection = strengthInKnotsRoundedToFive <= 2 ? -M_PI / 2.1 : direction;
      int tx = x - cos(textDirection + M_PI) * 15 - 5 - text.length();
      int ty = y + sin(textDirection + M_PI) * 12 + 5;
      this->_drawStrokedText(tx, ty, 0, text.c_str(), fontSize, 1 * drawOutline, outlineColor, barbTextColor);
    }
  }
}

FT_Face CDrawImage::_getFontFace(const std::string &fontfile, float size) {
  std::string key = CT::printf("%s_%f", fontfile.c_str(), size);
  auto it = fontFaceCache.find(key);
  if (it != fontFaceCache.end()) {
    return it->second;
  }

  if (ftLibrary == nullptr) {
    int error = FT_Init_FreeType(&ftLibrary);
    if (error) {
      CDBError("an error occurred during freetype library initialization");
      ftLibrary = nullptr;
      fontFaceCache[key] = nullptr;
      return nullptr;
    }
  }

  FT_Face face = nullptr;
  int error = FT_New_Face(ftLibrary, fontfile.c_str(), 0, &face);
  if (error == FT_Err_Unknown_File_Format) {
    CDBError("the font file could be opened and read, but it appears that its font format is unsupported %s", fontfile.c_str());
    fontFaceCache[key] = nullptr;
    return nullptr;
  } else if (error) {
    CDBError("Unable to initialize freetype: Could not read fontfile %s", fontfile.c_str());
    fontFaceCache[key] = nullptr;
    return nullptr;
  }

  error = FT_Set_Char_Size(face,        /* handle to face object */
                           0,           /* char_width in 1/64th of points */
                           int(size * 64), /* char_height in 1/64th of points */
                           100,         /* horizontal device resolution */
                           100);        /* vertical device resolution */
  if (error) {
    CDBError("unable to set character size");
    FT_Done_Face(face);
    fontFaceCache[key] = nullptr;
    return nullptr;
  }

  fontFaceCache[key] = face;
  return face;
}

cairo_font_face_t *CDrawImage::_setCairoFont() {
  if (currentFace == nullptr) return nullptr;
  cairo_font_face_t *ct = cairo_ft_font_face_create_for_ft_face(currentFace, 0);
  cairo_set_font_face(cr, ct);
  // cairo_set_font_size() takes plain user-space (pixel) units, with no DPI scaling - unlike the
  // old FT_Set_Char_Size(..., 100, 100) call _getFontFace() still makes for every cached face
  // (kept so a raw FT_Face lookup elsewhere would size correctly too). Apply the same 100dpi
  // scale here so a given fontSize value keeps rendering at its historical pixel size.
  cairo_set_font_size(cr, currentFontSize * (100.0 / 72.0));
  return ct;
}

void CDrawImage::_fillRect(double x1, double y1, double x2, double y2, const CColor &color) {
  if (y1 > y2) std::swap(y1, y2);
  if (x1 > x2) std::swap(x1, x2);
  cairo_rectangle(cr, x1, y1, x2 - x1, y2 - y1);
  cairo_set_source_rgba(cr, color.r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, color.g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, color.b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL,
                        color.a * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL);
  cairo_fill(cr);
}

int CDrawImage::_getTextSize(int &w, int &h, float angle, const char *text) {
  w = 0;
  h = 0;
  if (currentFace == nullptr || text == nullptr || text[0] == '\0') return 1;

  cairo_save(cr);
  cairo_font_face_t *ct = _setCairoFont();
  cairo_text_extents_t te;
  cairo_text_extents(cr, text, &te);
  cairo_restore(cr);
  cairo_font_face_destroy(ct);

  // Angle matters here: callers use (w,h) as a screen-space occupancy box (rotated tick labels,
  // overlap checks for angled point labels), not just the along-baseline text length.
  if (angle == 0.0f) {
    w = (int)lround(te.x_advance);
    h = (int)lround(te.height);
  } else {
    double c = fabs(cos(angle)), s = fabs(sin(angle));
    w = (int)lround(te.x_advance * c + te.height * s);
    h = (int)lround(te.x_advance * s + te.height * c);
  }
  return 0;
}

int CDrawImage::_drawText(int x, int y, double angle, const char *text, const CColor &color) {
  if (currentFace == nullptr || text == nullptr || text[0] == '\0') return 1;

  cairo_save(cr);
  cairo_font_face_t *ct = _setCairoFont();
  cairo_path_t *cp = cairo_copy_path(cr);
  cairo_new_path(cr);

  cairo_move_to(cr, x, y);
  // Negated: the old manual FreeType glyph walk rotated in FreeType's native Y-up space, which is
  // the mirror image of Cairo's Y-down rotation for the same angle value. Every external caller of
  // the plain drawText()/setText() family was tuned against that FreeType handedness (see e.g.
  // setTextStroke(), which negates the angle only on its Cairo-native _drawStrokedText() branch).
  cairo_rotate(cr, -angle);
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
  cairo_set_source_rgba(cr, color.r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, color.g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, color.b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL,
                        color.a * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL);
  cairo_show_text(cr, text);

  cairo_restore(cr);
  cairo_append_path(cr, cp);
  cairo_path_destroy(cp);
  cairo_font_face_destroy(ct);
  return 0;
}

int CDrawImage::_drawCenteredText(int x, int y, float angle, const char *text, const CColor &color) {
  if (currentFace == nullptr || text == nullptr || text[0] == '\0') return 1;

  cairo_save(cr);
  cairo_font_face_t *ct = _setCairoFont();
  cairo_path_t *cp = cairo_copy_path(cr);
  cairo_new_path(cr);

  cairo_text_extents_t te;
  cairo_text_extents(cr, text, &te);
  double cx = x - (te.x_bearing + te.width / 2);
  double cy = y - (te.y_bearing + te.height / 2);

  cairo_move_to(cr, cx, cy);
  cairo_rotate(cr, -angle); // see _drawText()'s comment on Y-up vs Y-down rotation handedness
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
  cairo_set_source_rgba(cr, color.r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, color.g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, color.b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL,
                        color.a * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL);
  cairo_show_text(cr, text);

  cairo_restore(cr);
  cairo_append_path(cr, cp);
  cairo_path_destroy(cp);
  cairo_font_face_destroy(ct);
  return 0;
}

int CDrawImage::_drawFilledText(int x, int y, float angle, const char *text, const CColor &fgcolor, const CColor &bgcolor) {
  if (text == nullptr || text[0] == '\0') return 0;
  if (currentFace == nullptr) return 1;

  cairo_save(cr);
  cairo_font_face_t *ct = _setCairoFont();
  cairo_path_t *cp = cairo_copy_path(cr);
  cairo_new_path(cr);

  cairo_text_extents_t te;
  cairo_text_extents(cr, text, &te);

  cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
  _fillRect(x - 5, y + 8, x + te.x_advance + 5, y - currentFontSize - 4, bgcolor);

  cairo_move_to(cr, x, y);
  cairo_rotate(cr, -angle); // see _drawText()'s comment on Y-up vs Y-down rotation handedness
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
  cairo_set_source_rgba(cr, fgcolor.r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, fgcolor.g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, fgcolor.b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL,
                        fgcolor.a * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL);
  cairo_show_text(cr, text);

  cairo_restore(cr);
  cairo_append_path(cr, cp);
  cairo_path_destroy(cp);
  cairo_font_face_destroy(ct);
  return 0;
}

void CDrawImage::_drawStrokedText(int x, int y, double angle, const char *text, float fontSize, float strokeWidth, const CColor &bgcolor, const CColor &fgcolor, bool centerText) {
  if (currentFace == nullptr) {
    return;
  }

  cairo_save(cr);

  cairo_font_face_t *ct = cairo_ft_font_face_create_for_ft_face(currentFace, 0);
  cairo_set_font_face(cr, ct);
  cairo_set_font_size(cr, fontSize);

  // Save the current path, because we might be drawing something like contour lines, which should not be stroked.
  cairo_path_t *cp = cairo_copy_path(cr);

  cairo_new_path(cr);
  cairo_set_dash(cr, 0, 0, 0);

  if (centerText) {
    cairo_text_extents_t te;
    cairo_text_extents(cr, text, &te);
    x = x - (te.x_bearing + te.width / 2);
    y = y - (te.y_bearing + te.height / 2);
  }

  cairo_move_to(cr, x, y);
  cairo_rotate(cr, angle);
  cairo_text_path(cr, text);
  if (strokeWidth > 0) {
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, bgcolor.r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, bgcolor.g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, bgcolor.b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, .2);
    cairo_set_line_width(cr, 2.5 + strokeWidth);
    cairo_stroke_preserve(cr);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(cr, bgcolor.r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, bgcolor.g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, bgcolor.b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, 1);
    cairo_set_line_width(cr, 1.5 + strokeWidth);
    cairo_stroke_preserve(cr);
  }

  cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, fgcolor.r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, fgcolor.g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, fgcolor.b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, 1);
  cairo_fill(cr);

  cairo_close_path(cr);

  cairo_restore(cr);
  // Put the original path back
  cairo_append_path(cr, cp);
  cairo_path_destroy(cp);
  cairo_font_face_destroy(ct);
}
