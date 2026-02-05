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

#include "CDrawImage.h"
#include "CXMLParser.h"

float convertValueToClass(float val, float interval) {
  float f = int(val / interval);
  if (val < 0) f -= 1;
  return f * interval;
}

CDrawImage::CDrawImage() {
  // CDBDebug("[CONS] CDrawImage");
  dImageCreated = 0;
  dPaletteCreated = 0;
  currentLegend = NULL;
  _bEnableTrueColor = false;
  _bEnableTransparency = false;
  _bEnableTrueColor = false;
  cairo = NULL;
  rField = NULL;
  gField = NULL;
  bField = NULL;
  numField = NULL;

  TTFFontLocation = "/usr/X11R6/lib/X11/fonts/truetype/verdana.ttf"; // TODO: this location does not exist in the docker container
  const char *fontLoc = getenv("ADAGUC_FONT");
  if (fontLoc != NULL) {
    TTFFontLocation = strdup(fontLoc);
  }
  TTFFontSize = 9;

  BGColorR = 0;
  BGColorG = 0;
  BGColorB = 0;
  backgroundAlpha = 255;
}

void CDrawImage::destroyImage() {
  // CDBDebug("[destroy] CDrawImage");

  dImageCreated = 0;

  for (size_t j = 0; j < legends.size(); j++) {
    delete legends[j];
  }
  legends.clear();

  delete cairo;
  cairo = NULL;
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

CDrawImage::~CDrawImage() {
  //   CDBDebug("[DESC] CDrawImage %dx%d", Geo.dWidth, Geo.dHeight);
  destroyImage();
  std::map<CT::string, CCairoPlotter *>::iterator myCCairoPlotterIter = myCCairoPlotterMap.begin();
  while (myCCairoPlotterIter != myCCairoPlotterMap.end()) {
    delete myCCairoPlotterIter->second;
    myCCairoPlotterIter++;
  }
  myCCairoPlotterMap.clear();
}

int CDrawImage::createImage(const char *fn) {
  // CDBDebug("CreateImage from file");
  _bEnableTrueColor = true;
  _bEnableTransparency = true;

  cairo_surface_t *surface = cairo_image_surface_create_from_png(fn);
  createImage(cairo_image_surface_get_width(surface), cairo_image_surface_get_height(surface));
  cairo->setToSurface(surface);
  cairo_surface_destroy(surface);
  return 0;
}

int CDrawImage::createImage(int _dW, int _dH) {
  // CDBDebug("CreateImage from WH");
  geoParams.width = _dW;
  geoParams.height = _dH;
  return createImage(geoParams);
}

int CDrawImage::createImage(GeoParameters &_Geo) {
  // CDBDebug("CreateImage from GeoParams");
#ifdef MEASURETIME
  StopWatch_Stop("start createImage of size %d %d, truecolor=[%d], transparency = [%d], currentGraphicsRenderer [%d]", _Geo.dWidth, _Geo.dHeight, _bEnableTrueColor, _bEnableTransparency,
                 currentGraphicsRenderer);
#endif
  if (dImageCreated == 1) {
    CDBError("createImage: image already created");
    return 1;
  }

  geoParams = _Geo;
  // Always true color

  if (_bEnableTransparency == false) {
    cairo = new CCairoPlotter(geoParams.width, geoParams.height, TTFFontSize, TTFFontLocation, BGColorR, BGColorG, BGColorB, 255);
  } else {
    cairo = new CCairoPlotter(geoParams.width, geoParams.height, TTFFontSize, TTFFontLocation, 0, 0, 0, 0);
  }
  dImageCreated = 1;
#ifdef MEASURETIME
  StopWatch_Stop("image created with renderer %d.", currentGraphicsRenderer);
#endif

  return 0;
}

int CDrawImage::printImagePng8(bool useBitAlpha) {
  if (dImageCreated == 0) {
    CDBError("print: image not created");
    return 1;
  }

  cairo->writeToPng8Stream(stdout, backgroundAlpha, useBitAlpha);
  return 0;
}

int CDrawImage::printImagePng24() {
  if (dImageCreated == 0) {
    CDBError("print: image not created");
    return 1;
  }

  cairo->writeToPng24Stream(stdout, backgroundAlpha);
  return 0;
}

int CDrawImage::printImagePng32() {
  if (dImageCreated == 0) {
    CDBError("print: image not created");
    return 1;
  }

  cairo->writeToPng32Stream(stdout, backgroundAlpha);
  return 0;
}
int CDrawImage::printImageWebP32(int quality) {
  if (dImageCreated == 0) {
    CDBError("print: image not created");
    return 1;
  }

  cairo->writeToWebP32Stream(stdout, backgroundAlpha, quality);
  return 0;
}

void CDrawImage::drawVector(int x, int y, double direction, double strength, int color) {
  LineStyle lineStyle = {
    .lineWidth = 1.0,
    .lineColor = getColorForIndex(color),
    .lineOutlineColor = CColor(255, 255, 255, 255),
    .lineOutlineWidth = 0,
  };
  drawVector(x, y, direction, strength, lineStyle);
}

void CDrawImage::drawVector(int x, int y, double direction, double strength, int color, float linewidth) {
  LineStyle lineStyle = {
    .lineWidth = linewidth,
    .lineColor = getColorForIndex(color),
    .lineOutlineColor = CColor(255, 255, 255, 255),
    .lineOutlineWidth = 0,
  };
  drawVector(x, y, direction, strength, lineStyle);
}

void CDrawImage::drawVector(int x, int y, double direction, double strength, LineStyle lineStyle) {
  // TODO: Support setting outline

  double wx1, wy1, wx2, wy2, dx1, dy1;
  if (fabs(strength) < 1) {
    setPixel(x, y, lineStyle.lineColor);
    return;
  }

  bool startatxy = true;

  // strength=strength/2;
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
  CColor color = lineStyle.lineColor;
  cairo->setColor(color.r, color.g, color.b, color.a);
  cairo->setFillColor(color.r, color.g, color.b, color.a);
  poly(hx1, hy1, wx1, wy1, hx2, hy2, lineStyle.lineWidth, color, false);

  // Render shaft
  line(wx2, wy2, hx3, hy3, lineStyle.lineWidth, color);

  // TODO: should `poly` and `line` also accept `LineStyle` structs?
}

#define xCor(l, d) ((int)(l * cos(d) + 0.5))
#define yCor(l, d) ((int)(l * sin(d) + 0.5))

void CDrawImage::drawVector2(int x, int y, double direction, double strength, int radius, CColor color, float linewidth) {
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

void CDrawImage::drawBarb(int x, int y, double direction, double viewDirCorrection, double strength, bool toKnots, bool flip, bool drawVectorPlotValue, LineStyle lineStyle, TextStyle textStyle) {
  // If no linewidth, no outline should be drawn, set inner barblineWidth to 0.8 to ensure we draw a barb
  if (lineStyle.lineWidth == 0) {
    lineStyle.lineWidth = 0.8;
    lineStyle.lineOutlineWidth = 0;
  }

  cairo->drawBarb(x, y, direction, viewDirCorrection, strength, toKnots, flip, drawVectorPlotValue, lineStyle, textStyle);
}

void CDrawImage::circle(int x, int y, int r, int color, float lineWidth) {
  cairo->setColor(currentLegend->CDIred[color], currentLegend->CDIgreen[color], currentLegend->CDIblue[color], 255);
  cairo->circle(x, y, r, lineWidth);
}

void CDrawImage::circle(int x, int y, int r, CColor color, float lineWidth) {
  cairo->setColor(color.r, color.g, color.b, color.a);
  cairo->circle(x, y, r, lineWidth);
}

void CDrawImage::circle(int x, int y, int r, int color) { circle(x, y, r, color, 1.0); }

void CDrawImage::circle(int x, int y, int r, CColor col) { circle(x, y, r, col, 1.0); }

void CDrawImage::poly(float x1, float y1, float x2, float y2, float x3, float y3, int color, bool fill) {
  CColor col = getColorForIndex(color);
  poly(x1, y1, x2, y2, x3, y3, col, fill);
}

void CDrawImage::poly(float x1, float y1, float x2, float y2, float x3, float y3, CColor color, bool fill) {
  float ptx[3] = {x1, x2, x3};
  float pty[3] = {y1, y2, y3};
  cairo->setFillColor(color.r, color.g, color.b, color.a);
  cairo->poly(ptx, pty, 3, true, fill);
}

void CDrawImage::poly(float x1, float y1, float x2, float y2, float x3, float y3, float lineWidth, CColor color, bool fill) {
  float ptx[3] = {x1, x2, x3};
  float pty[3] = {y1, y2, y3};
  cairo->setFillColor(color.r, color.g, color.b, color.a);
  cairo->poly(ptx, pty, 3, lineWidth, true, fill);
}

void CDrawImage::poly(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4, float lineWidth, CColor color, bool fill) {
  float ptx[4] = {x1, x2, x3, x4};
  float pty[4] = {y1, y2, y3, y4};
  cairo->setFillColor(color.r, color.g, color.b, color.a);
  cairo->poly(ptx, pty, 4, lineWidth, true, fill);
}

void CDrawImage::poly(float *x, float *y, int n, float lineWidth, CColor lineColor, CColor fillColor, bool close, bool fill) {
  cairo->setColor(lineColor.r, lineColor.g, lineColor.b, lineColor.a);
  cairo->setFillColor(fillColor.r, fillColor.g, fillColor.b, fillColor.a);
  cairo->poly(x, y, n, lineWidth, close, fill);
}

void CDrawImage::line(float x1, float y1, float x2, float y2, int color) {
  if (currentLegend == NULL) return;
  if (color >= 0 && color < 256) {
    cairo->setColor(currentLegend->CDIred[color], currentLegend->CDIgreen[color], currentLegend->CDIblue[color], 255);
    cairo->line(x1, y1, x2, y2);
  }
}

void CDrawImage::line(float x1, float y1, float x2, float y2, CColor ccolor) {
  if (currentLegend == NULL) return;
  cairo->setColor(ccolor.r, ccolor.g, ccolor.b, ccolor.a);
  cairo->line(x1, y1, x2, y2);
}

void CDrawImage::moveTo(float x1, float y1) {
  if (currentLegend == NULL) return;
  cairo->moveTo(x1, y1);
}

void CDrawImage::lineTo(float x2, float y2, float w, CColor ccolor) {
  if (currentLegend == NULL) return;
  cairo->setColor(ccolor.r, ccolor.g, ccolor.b, ccolor.a);
  cairo->lineTo(x2, y2, w);
}

void CDrawImage::endLine() {
  if (currentLegend == NULL) return;
  cairo->endLine();
}

void CDrawImage::endLine(const double *dashes, int num_dashes) {
  if (currentLegend == NULL) return;
  cairo->endLine(dashes, num_dashes);
}

CColor CDrawImage::getColorForIndex(int colorIndex) {
  CColor color;
  if (currentLegend == NULL) return color;
  if (colorIndex >= 0 && colorIndex < 256) {
    color = CColor(currentLegend->CDIred[colorIndex], currentLegend->CDIgreen[colorIndex], currentLegend->CDIblue[colorIndex], currentLegend->CDIalpha[colorIndex]);
  }
  return color;
}

void CDrawImage::line(float x1, float y1, float x2, float y2, float w, CColor ccolor) {
  if (currentLegend == NULL) return;
  cairo->setColor(ccolor.r, ccolor.g, ccolor.b, ccolor.a);
  cairo->line(x1, y1, x2, y2, w);
}

void CDrawImage::line(float x1, float y1, float x2, float y2, float w, int color) {
  if (currentLegend == NULL) return;
  if (color >= 0 && color < 256) {
    cairo->setColor(currentLegend->CDIred[color], currentLegend->CDIgreen[color], currentLegend->CDIblue[color], 255);
    cairo->line(x1, y1, x2, y2, w);
  }
}

void CDrawImage::setPixelIndexed(int x, int y, int color) {
  if (currentLegend == NULL) return;
  if (color >= 0 && color < 256) {
    if (currentLegend->CDIalpha[color] > 0) {
      cairo->pixel_blend(x, y, currentLegend->CDIred[color], currentLegend->CDIgreen[color], currentLegend->CDIblue[color], currentLegend->CDIalpha[color]);
    } else if (currentLegend->CDIalpha[color] < 0) {
      cairo->pixel_overwrite(x, y, currentLegend->CDIred[color], currentLegend->CDIgreen[color], currentLegend->CDIblue[color], -(currentLegend->CDIalpha[color] + 1));
    }
  }
}

void CDrawImage::getPixelTrueColor(int x, int y, unsigned char &r, unsigned char &g, unsigned char &b, unsigned char &a) { cairo->getPixel(x, y, r, g, b, a); }

void CDrawImage::setPixelTrueColor(int x, int y, unsigned int color) { cairo->pixel_blend(x, y, color, color / 256, color / (256 * 256), 255); }

const char *toHex8(char *data, unsigned char hex) {
  unsigned char a = hex / 16;
  unsigned char b = hex % 16;
  data[0] = a < 10 ? a + 48 : a + 55;
  data[1] = b < 10 ? b + 48 : b + 55;
  data[2] = '\0';
  return data;
}

void CDrawImage::getHexColorForColorIndex(CT::string *hexValue, int color) {
  if (currentLegend == NULL) return;
  char data[3];
  hexValue->print("#%s%s%s", toHex8(data, currentLegend->CDIred[color]), toHex8(data, currentLegend->CDIgreen[color]), toHex8(data, currentLegend->CDIblue[color]));
}

void CDrawImage::setPixelTrueColor(int x, int y, unsigned char r, unsigned char g, unsigned char b) { cairo->pixel_blend(x, y, r, g, b, 255); }

void CDrawImage::setPixel(int x, int y, CColor &color) { cairo->pixel_blend(x, y, color.r, color.g, color.b, color.a); }

void CDrawImage::setPixelTrueColorOverWrite(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a) { cairo->pixel_overwrite(x, y, r, g, b, a); }

void CDrawImage::setPixelTrueColor(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a) { cairo->pixel_blend(x, y, r, g, b, a); }

void CDrawImage::setText(const char *text, int x, int y, int color) {
  CColor col = getColorForIndex(color);
  setText(text, x, y, col);
}

void CDrawImage::setText(const char *text, int x, int y, CColor color) {
  if (currentLegend == NULL) return;
  cairo->setColor(color.r, color.g, color.b, color.a);
  cairo->drawText(x, y + 10, 0, text);
}

void CDrawImage::setTextStroke(int x, int y, float angle, const char *text, const char *fontFile, float fontSize, float strokeWidth, CColor bgcolor, CColor fgcolor) {
  // TODO: this should receive a TextStyle struct

  if (bgcolor.a == 0) {
    drawText(x, y, fontFile, fontSize, angle, text, fgcolor);
  } else {
    TextStyle textStyle = { .textColor = fgcolor, .fontSize=fontSize * 1.4, .textOutlineColor = bgcolor, .textOutlineWidth = strokeWidth};
    cairo->drawStrokedText(x, y, -angle, text, textStyle);
  }
}

void CDrawImage::drawText(int x, int y, float angle, const char *text, unsigned char colorIndex) {
  CDrawImage::drawText(x, y, angle, text, CColor(currentLegend->CDIred[colorIndex], currentLegend->CDIgreen[colorIndex], currentLegend->CDIblue[colorIndex], 255));
}

void CDrawImage::drawText(int x, int y, float angle, const char *text, CColor fgcolor) {
  cairo->setColor(fgcolor.r, fgcolor.g, fgcolor.b, fgcolor.a);
  cairo->drawText(x, y, angle, text);
}

void CDrawImage::drawText(int x, int y, const char *fontfile, float size, float angle, const char *text, unsigned char colorIndex) {
  if (!currentLegend) {
    return;
  }
  CColor color(currentLegend->CDIred[colorIndex], currentLegend->CDIgreen[colorIndex], currentLegend->CDIblue[colorIndex], 255);
  drawText(x, y, fontfile, size, angle, text, color);
}

int CDrawImage::drawTextArea(int x, int y, const char *fontfile, float size, float, const char *_text, CColor fgcolor, CColor bgcolor) {
  CT::string text;
  int offset = 0;
  CT::string title = _text;
  int length = title.length();
  CCairoPlotter *ftTitle = new CCairoPlotter(geoParams.width, geoParams.height, (cairo->getByteBuffer()), size, fontfile);
  float textY = 0;
  int width = geoParams.width - x;
  int widthOfText, heightOfText;
  //
  do {
    do {
      text.copy((const char *)(title.c_str() + offset), length);
      ftTitle->getTextSize(widthOfText, heightOfText, 0.0, text.c_str());
      length--;
      // if(!needsLineBreak)if(w>width-10)needsLineBreak = true;
    } while (widthOfText > width && length >= 0);
    length++;
    if (length + offset < (int)title.length()) {
      int sl = length;
      while (text.charAt(sl) != ' ' && sl > 0) {
        sl--;
      }
      if (sl > 0) length = (sl + 1);
    }
    text.copy((const char *)(title.c_str() + offset), length);
    ftTitle->getTextSize(widthOfText, heightOfText, 0.0, text.c_str());
    if (bgcolor.a != 0) {
      ftTitle->setColor(bgcolor.r, bgcolor.g, bgcolor.b, 0);
      ftTitle->setFillColor(bgcolor.r, bgcolor.g, bgcolor.b, bgcolor.a);
      ftTitle->filledRectangle(x - 3, y + textY + 5, x + widthOfText + 3, y + textY - 3 - heightOfText);
    }
    ftTitle->setColor(fgcolor.r, fgcolor.g, fgcolor.b, fgcolor.a);
    ftTitle->drawText(x, y + textY, 0.0, text.c_str());
    textY += size * 1.5;

    offset += length;
  } while (length < (int)(title.length()) - 1 && (int)offset < (int)title.length() - 1);
  cairo->isAlphaUsed |= ftTitle->isAlphaUsed; // remember ftTile's isAlphaUsed flag
  delete ftTitle;

  return textY;
}

void CDrawImage::drawText(int x, int y, const char *fontfile, float size, float angle, const char *text, CColor fgcolor, CColor bgcolor) {
  CCairoPlotter *freeType = this->getCairoPlotter(fontfile, size, geoParams.width, geoParams.height, cairo->getByteBuffer());
  freeType->setColor(fgcolor.r, fgcolor.g, fgcolor.b, fgcolor.a);
  freeType->setFillColor(bgcolor.r, bgcolor.g, bgcolor.b, bgcolor.a);
  freeType->drawFilledText(x, y, angle, text);
  cairo->isAlphaUsed |= freeType->isAlphaUsed; // remember freetype's isAlphaUsed flag
}
void CDrawImage::setDisc(int x, int y, int discRadius, int fillCol, int lineCol) {
  if (currentLegend == NULL) return;
  cairo->setFillColor(currentLegend->CDIred[fillCol], currentLegend->CDIgreen[fillCol], currentLegend->CDIblue[fillCol], currentLegend->CDIalpha[fillCol]);
  cairo->setColor(currentLegend->CDIred[lineCol], currentLegend->CDIgreen[lineCol], currentLegend->CDIblue[lineCol], currentLegend->CDIalpha[fillCol]);
  cairo->filledcircle(x, y, discRadius);
  cairo->circle(x, y, discRadius, 1);
}

void CDrawImage::setDisc(int x, int y, int discRadius, CColor fillColor, CColor lineColor) {
  if (currentLegend == NULL) return;
  cairo->setFillColor(fillColor.r, fillColor.g, fillColor.b, fillColor.a);
  cairo->setColor(lineColor.r, lineColor.g, lineColor.b, lineColor.a);
  cairo->filledcircle(x, y, discRadius);
}

void CDrawImage::setEllipse(int x, int y, float discRadiusX, float discRadiusY, float rotation, CColor fillColor, CColor lineColor) {
  if (currentLegend == NULL) return;
  cairo->setFillColor(fillColor.r, fillColor.g, fillColor.b, fillColor.a);
  cairo->setColor(lineColor.r, lineColor.g, lineColor.b, lineColor.a);
  cairo->filledEllipse(x, y, discRadiusX, discRadiusY, rotation);
}

void CDrawImage::setDisc(int x, int y, float discRadius, CColor fillColor, CColor lineColor) {
  if (currentLegend == NULL) return;
  cairo->setFillColor(fillColor.r, fillColor.g, fillColor.b, fillColor.a);
  cairo->setColor(lineColor.r, lineColor.g, lineColor.b, lineColor.a);
  cairo->filledcircle(x, y, discRadius);
}

void CDrawImage::setTextDisc(int x, int y, int discRadius, const char *text, const char *fontfile, float fontsize, CColor textcolor, CColor fillcolor, CColor lineColor) {
  if (currentLegend == NULL) return;
  cairo->setFillColor(fillcolor.r, fillcolor.g, fillcolor.b, fillcolor.a);
  cairo->setColor(lineColor.r, lineColor.g, lineColor.b, lineColor.a);
  cairo->filledcircle(x, y, discRadius);
  drawCenteredText(x, y, fontfile, fontsize, 0, text, textcolor);
}

void CDrawImage::drawAnchoredText(int x, int y, const char *fontfile, float size, float angle, const char *text, CColor color, int anchor) {
  CCairoPlotter *freeType = this->getCairoPlotter(fontfile, size, geoParams.width, geoParams.height, cairo->getByteBuffer());
  freeType->setColor(color.r, color.g, color.b, color.a);
  freeType->drawAnchoredText(x, y, angle, text, anchor);
  cairo->isAlphaUsed |= freeType->isAlphaUsed; // remember freetype's isAlphaUsed flag
}

CCairoPlotter *CDrawImage::getCairoPlotter(const char *fontfile, float size, int w, int h, unsigned char *b) {
  CT::string _key;
  _key.print("%s_%f_%d_%d", fontfile, size, w, h);

  std::map<CT::string, CCairoPlotter *>::iterator myCCairoPlotterIter = myCCairoPlotterMap.find(_key);
  if (myCCairoPlotterIter == myCCairoPlotterMap.end()) {
    CCairoPlotter *cairoPlotter = new CCairoPlotter(w, h, b, size, fontfile);
    myCCairoPlotterMap[_key] = cairoPlotter;
    return cairoPlotter;
  } else {
    return (*myCCairoPlotterIter).second;
  }
}

void CDrawImage::drawCenteredText(int x, int y, const char *fontfile, float size, float angle, const char *text, CColor color, CColor textOutlineColor) {
  CCairoPlotter *freeType = this->getCairoPlotter(fontfile, size, geoParams.width, geoParams.height, cairo->getByteBuffer());
  freeType->setColor(color.r, color.g, color.b, color.a);
  if (textOutlineColor.a == 0) {
    freeType->drawCenteredText(x, y, angle, text);
  } else {
    TextStyle textStyle = { .textColor = color, .fontSize=size * 1.4, .textOutlineColor = textOutlineColor, .textOutlineWidth = 1 };
    freeType->drawStrokedText(x, y, angle, text, textStyle, true);
  }

  cairo->isAlphaUsed |= freeType->isAlphaUsed; // remember freetype's isAlphaUsed flag
}

void CDrawImage::drawCenteredTextNoOverlap(int x, int y, const char *fontFile, float size, float angle, int padding, const char *text, CColor color, bool noOverlap,
                                           std::vector<CRectangleText> &rects) {
  if (size <= 0) { // size 0 means do not draw label
    return;
  }

  int w, h;
  float radAngle = angle * M_PI / 180;
  CRectangleText rect;

  CCairoPlotter *freeType = this->getCairoPlotter(fontFile, size, geoParams.width, geoParams.height, cairo->getByteBuffer());
  freeType->setColor(color.r, color.g, color.b, color.a);
  freeType->getTextSize(w, h, radAngle, text);
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

void CDrawImage::drawText(int x, int y, const char *fontfile, float size, float angle, const char *text, CColor color) {
  CCairoPlotter *freeType = this->getCairoPlotter(fontfile, size, geoParams.width, geoParams.height, cairo->getByteBuffer());
  freeType->setColor(color.r, color.g, color.b, color.a);
  freeType->drawText(x, y, angle, text);
  cairo->isAlphaUsed |= freeType->isAlphaUsed; // remember freetype's isAlphaUsed flag
}

int CDrawImage::create685Palette() {
  currentLegend = NULL;
  // CDBDebug("Create 685Palette");
  const char *paletteName685 = "685Palette";

  for (size_t j = 0; j < legends.size(); j++) {
    if (legends[j]->legendName.equals(paletteName685)) {
      CDBDebug("Found legend");
      currentLegend = legends[j];
      return 0;
    }
  }

  if (currentLegend == NULL) {
    currentLegend = new CLegend();
    currentLegend->id = legends.size();
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

  addColor(240, 0, 0, 0);
  addColor(241, 32, 32, 32);
  addColor(242, 64, 64, 64);
  addColor(243, 96, 96, 96);
  // addColor(244,64  ,64  ,192);
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

int CDrawImage::_createStandard() {

  addColor(240, 0, 0, 0);
  addColor(241, 32, 32, 32);
  addColor(242, 64, 64, 64);
  addColor(243, 96, 96, 96);
  // addColor(244,64  ,64  ,192);
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

int CDrawImage::createPalette(CServerConfig::XMLE_Legend *legend) {
  currentLegend = NULL;
  if (legend != NULL) {
    for (size_t j = 0; j < legends.size(); j++) {
      if (legends[j]->legendName.equals(legend->attr.name.c_str())) {
        currentLegend = legends[j];
        return 0;
      }
    }
  }
  // CDBDebug("Create legend %s",legend->attr.name.c_str());
  if (currentLegend == NULL) {
    currentLegend = new CLegend();
    currentLegend->id = legends.size();
    currentLegend->legendName = legend->attr.name.c_str();
    legends.push_back(currentLegend);
  }

  if (dImageCreated == 0) {
    CDBError("createPalette: image not created");
    return 1;
  }

  for (int j = 0; j < 256; j++) {
    currentLegend->CDIred[j] = 0;
    currentLegend->CDIgreen[j] = 0;
    currentLegend->CDIblue[j] = 0;
    currentLegend->CDIalpha[j] = 255;
  }
  if (legend == NULL) {
    return _createStandard();
  }
  if (legend->attr.type.equals("colorRange")) {

    float cx;
    float rc[4];
    for (size_t j = 0; j < legend->palette.size(); j++) {
      CServerConfig::XMLE_palette *pbegin = legend->palette[j];
      CServerConfig::XMLE_palette *pnext = legend->palette[j];
      if (j < legend->palette.size() - 1) {
        pnext = legend->palette[j + 1];
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
          currentLegend->CDIred[i] = int(rc[0] * cx) + pbegin->attr.red;
          currentLegend->CDIgreen[i] = int(rc[1] * cx) + pbegin->attr.green;
          currentLegend->CDIblue[i] = int(rc[2] * cx) + pbegin->attr.blue;
          currentLegend->CDIalpha[i] = int(rc[3] * cx) + pbegin->attr.alpha;
          if (currentLegend->CDIred[i] == 0) currentLegend->CDIred[i] = 1; // for transparency
        }
      }
    }

    /*for(int j=0;j<240;j++){
      CDBDebug("%d %d %d %d",currentLegend->CDIred[j],currentLegend->CDIgreen[j],currentLegend->CDIblue[j],currentLegend->CDIalpha[j]);
    }*/

    return _createStandard();
  }
  if (legend->attr.type.equals("interval")) {

    for (size_t j = 0; j < legend->palette.size(); j++) {

      if (legend->palette[j]->attr.index != -1) {

        int startIndex = legend->palette[j]->attr.index;
        int stopIndex = 240;
        if (j < legend->palette.size() - 1) stopIndex = legend->palette[j + 1]->attr.index;

        for (int i = startIndex; i < stopIndex; i++) {
          if (i >= 0 && i < 240) {
            currentLegend->CDIred[i] = legend->palette[j]->attr.red;
            currentLegend->CDIgreen[i] = legend->palette[j]->attr.green;
            currentLegend->CDIblue[i] = legend->palette[j]->attr.blue;
            currentLegend->CDIalpha[i] = legend->palette[j]->attr.alpha;
            if (currentLegend->CDIred[i] == 0) currentLegend->CDIred[i] = 1; // for transparency
          }
        }
      } else {
        for (int i = legend->palette[j]->attr.min; i <= legend->palette[j]->attr.max; i++) {
          if (i >= 0 && i < 240) {
            currentLegend->CDIred[i] = legend->palette[j]->attr.red;
            currentLegend->CDIgreen[i] = legend->palette[j]->attr.green;
            currentLegend->CDIblue[i] = legend->palette[j]->attr.blue;
            currentLegend->CDIalpha[i] = legend->palette[j]->attr.alpha;
            if (currentLegend->CDIred[i] == 0) currentLegend->CDIred[i] = 1; // for transparency
          }
        }
      }
    }
    return _createStandard();
  }
  if (legend->attr.type.equals("svg")) {
    if (legend->attr.file.empty()) {
      CDBError("Legend type file has no file attribute specified");
      return 1;
    }
    CDBDebug("Reading file %s", legend->attr.file.c_str());

    CXMLParserElement element;

    try {
      element.parse(legend->attr.file.c_str());
      CXMLParser::XMLElement::XMLElementPointerList stops = element.get("svg")->get("g")->get("defs")->get("linearGradient")->getList("stop");
      float cx;
      float rc[4];
      unsigned char prev_red, prev_green, prev_blue, prev_alpha;
      int prev_offset;
      for (size_t j = 0; j < stops.size(); j++) {
        // CDBDebug("%s",stops.get(j)->toString().c_str());
        int offset = (int)(stops.get(j)->getAttrValue("offset").toFloat() * 2.4);
        CT::string color = stops.get(j)->getAttrValue("stop-color").c_str() + 4;
        color.setSize(color.length() - 1);
        auto colors = color.split(",");
        if (colors.size() != 3) {
          CDBError("Number of specified colors is unequal to three");
          return 1;
        }
        unsigned char red = colors[0].toInt();
        unsigned char green = colors[1].toInt();
        unsigned char blue = colors[2].toInt();
        unsigned char alpha = (char)(stops.get(j)->getAttrValue("stop-opacity").toFloat() * 255);
        // CDBDebug("I%d R%d G%d B%d A%d",offset,red,green,blue,alpha);
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
          // CDBDebug("dif %f",dif);
          if (dif < 0.5f) dif = 1;
          rc[0] = float(prev_red - red) / dif;
          rc[1] = float(prev_green - green) / dif;
          rc[2] = float(prev_blue - blue) / dif;
          rc[3] = float(prev_alpha - alpha) / dif;

          for (int i = prev_offset; i < offset + 1; i++) {
            if (i >= 0 && i < 240) {
              cx = float(i - prev_offset);
              currentLegend->CDIred[i] = int(rc[0] * cx) + red;
              currentLegend->CDIgreen[i] = int(rc[1] * cx) + green;
              currentLegend->CDIblue[i] = int(rc[2] * cx) + blue;
              currentLegend->CDIalpha[i] = int(rc[3] * cx) + alpha;
              if (currentLegend->CDIred[i] == 0) currentLegend->CDIred[i] = 1; // for transparency
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
      CT::string message = CXMLParser::getErrorMessage(e);
      CDBError("%s\n", message.c_str());
      return 1;
    }
    return _createStandard();
  }
  return 1;
}

void CDrawImage::rectangle(int x1, int y1, int x2, int y2, CColor innercolor, CColor outercolor) {
  cairo->setFillColor(innercolor.r, innercolor.g, innercolor.b, innercolor.a);
  cairo->setColor(outercolor.r, outercolor.g, outercolor.b, outercolor.a);
  cairo->filledRectangle(x1, y1, x2, y2);
}

void CDrawImage::rectangle(int x1, int y1, int x2, int y2, int innercolor, int outercolor) {
  if (currentLegend == NULL) return;
  if (innercolor >= 0 && innercolor < 255 && outercolor >= 0 && outercolor < 255) {
    cairo->setColor(currentLegend->CDIred[outercolor], currentLegend->CDIgreen[outercolor], currentLegend->CDIblue[outercolor], 255);
    cairo->setFillColor(currentLegend->CDIred[innercolor], currentLegend->CDIgreen[innercolor], currentLegend->CDIblue[innercolor], 255);
    cairo->filledRectangle(x1, y1, x2, y2);
  }
}

void CDrawImage::rectangle(int x1, int y1, int x2, int y2, int outercolor) {
  line(x1, y1, x2, y1, 1, outercolor);
  line(x2, y1, x2, y2, 1, outercolor);
  line(x2, y2, x1, y2, 1, outercolor);
  line(x1, y2, x1, y1, 1, outercolor);
}

int CDrawImage::addColor(int Color, unsigned char R, unsigned char G, unsigned char B) {
  if (currentLegend == NULL) return 0;
  currentLegend->CDIred[Color] = R;
  currentLegend->CDIgreen[Color] = G;
  currentLegend->CDIblue[Color] = B;
  currentLegend->CDIalpha[Color] = 255;
  return 0;
}

int CDrawImage::copyPalette() {
  for (int j = 0; j < 256; j++) {
    if (dPaletteCreated == 0) {
      colors[j] = _colors[j];
    }

    if (j < 240) {
      if (currentLegend->CDIred[j] == 0 && currentLegend->CDIgreen[j] == 0 && currentLegend->CDIblue[j] == 0) {
        if (dPaletteCreated == 0) {
          colors[j] = _colors[255];
        }
        currentLegend->CDIalpha[j] = 0;
      }
    }
  }
  dPaletteCreated = 1;
  return 0;
}
int s = 0;

void CDrawImage::enableTransparency(bool enable) { _bEnableTransparency = enable; }
void CDrawImage::setBGColor(unsigned char R, unsigned char G, unsigned char B) {
  BGColorR = R;
  BGColorG = G;
  BGColorB = B;
}

void CDrawImage::setTrueColor(bool enable) { _bEnableTrueColor = enable; }

bool CDrawImage::isPixelTransparent(int &x, int &y) {
  unsigned char r, g, b, a;
  cairo->getPixel(x, y, r, g, b, a);

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

bool CDrawImage::isColorTransparent(int &color) {
  if (currentLegend == NULL) {
    return true;
  }
  if (color < 0 || color > 255) return true;
  if (currentLegend->CDIred[color] == 0 && currentLegend->CDIgreen[color] == 0 && currentLegend->CDIblue[color] == 0) return true;
  return false;
}

/**
 * Returns a value indicating how width the image is based on whether there are pixels set.
 * Useful for cropping the image
 */

void CDrawImage::getCanvasSize(int &x1, int &y1, int &w, int &h) {
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

int CDrawImage::clonePalette(CDrawImage *drawImage) {
  if (drawImage->currentLegend == NULL) {
    currentLegend = NULL;
    return 0;
  }
  currentLegend = new CLegend();
  currentLegend->id = legends.size();
  currentLegend->legendName = drawImage->currentLegend->legendName.c_str();
  legends.push_back(currentLegend);
  for (int j = 0; j < 256; j++) {
    currentLegend->CDIred[j] = drawImage->currentLegend->CDIred[j];
    currentLegend->CDIgreen[j] = drawImage->currentLegend->CDIgreen[j];
    currentLegend->CDIblue[j] = drawImage->currentLegend->CDIblue[j];
    currentLegend->CDIalpha[j] = drawImage->currentLegend->CDIalpha[j];
  }
  BGColorR = drawImage->BGColorR;
  BGColorG = drawImage->BGColorG;
  BGColorB = drawImage->BGColorB;
  copyPalette();
  return 0;
}

/**
 * Creates a new image with the same settings but with different size as the source image
 */
int CDrawImage::createImage(CDrawImage *image, int width, int height) {
  if (height < 0) {
    height = 0;
  }
  if (width < 0) {
    width = 0;
  }
// CDBDebug("CreateImage from image");
#ifdef MEASURETIME
  CDBDebug("createImage(CDrawImage *image,int width,int height)");
#endif
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

int CDrawImage::draw(int destx, int desty, int sourcex, int sourcey, CDrawImage *simage) {
  unsigned char r, g, b, a;
  for (int y = 0; y < simage->geoParams.height; y++) {
    for (int x = 0; x < simage->geoParams.width; x++) {
      int sx = x + sourcex;
      int sy = y + sourcey;
      int dx = x + destx;
      int dy = y + desty;
      if (sx >= 0 && sy >= 0 && dx >= 0 && dy >= 0 && sx < simage->geoParams.width && sy < simage->geoParams.height && dx < geoParams.width && dy < geoParams.height) {
        // Get source r,g,b,a
        simage->cairo->getPixel(sx, sy, r, g, b, a);
        // Set r,g,b,a to dest
        cairo->pixel_blend(dx, dy, r, g, b, a);
      }
    }
  }
  return 0;
}

int CDrawImage::drawrotated(int destx, int desty, int sourcex, int sourcey, CDrawImage *simage) {
  unsigned char r, g, b, a;
  for (int y = 0; y < simage->geoParams.height; y++) {
    for (int x = 0; x < simage->geoParams.width; x++) {
      int sx = x + sourcex;
      int sy = y + sourcey;
      int dx = simage->geoParams.height - y - desty;
      int dy = x + destx;
      if (sx >= 0 && sy >= 0 && dx >= 0 && dy >= 0 && sx < simage->geoParams.width && sy < simage->geoParams.height && dx < geoParams.width && dy < geoParams.height) {
        // Get source r,g,b,a
        simage->cairo->getPixel(sx, sy, r, g, b, a);
        // Set r,g,b,a to dest
        cairo->pixel_blend(dx, dy, r, g, b, a);
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
  // return;
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
  //  return 0;
}

unsigned char *CDrawImage::getCanvasMemory() const { return cairo->getByteBuffer(); }

void CDrawImage::setCanvasColorType(int colorType) {
  if (colorType == CDRAWIMAGE_COLORTYPE_ARGB) {
    _bEnableTrueColor = true;
  }
}

int CDrawImage::getCanvasColorType() { return CDRAWIMAGE_COLORTYPE_ARGB; }

int CDrawImage::getHeight() { return geoParams.height; }

int CDrawImage::getWidth() { return geoParams.width; }

const char *CDrawImage::getFontLocation() { return this->TTFFontLocation; }

float CDrawImage::getFontSize() { return this->TTFFontSize; }

int CDrawImage::getTextWidth(CT::string text, const std::string &, int angle) {
  int w = 0, h = 0;
  cairo->getTextSize(w, h, angle, text.c_str());
  return w;
}
