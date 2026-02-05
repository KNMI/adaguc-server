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
// #define MEASURETIME

#include <cairo-ft.h>
#include "CStopWatch.h"

cairo_status_t writerFunc(void *closure, const unsigned char *data, unsigned int length) {
  FILE *fp = (FILE *)closure;
  int nrec = fwrite(data, length, 1, fp);
  if (nrec == 1) {
    return CAIRO_STATUS_SUCCESS;
  }

  return CAIRO_STATUS_WRITE_ERROR;
}

void CCairoPlotter::_swap(float &x, float &y) {
  float a = x;
  x = y;
  y = a;
}
void CCairoPlotter::_swap(int &x, int &y) {
  int a = x;
  x = y;
  y = a;
}

void CCairoPlotter::_cairoPlotterInit(int width, int height, float fontSize, const char *fontLocation) {
  this->width = width;
  this->height = height;
  this->fontSize = fontSize;
  this->fontLocation = fontLocation;
  this->stride = cairo_format_stride_for_width(FORMAT, width);
  this->isAlphaUsed = false;

  surface = cairo_image_surface_create_for_data(ARGBByteBuffer, CCairoPlotter::FORMAT, width, height, stride);
  cr = cairo_create(this->surface);
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

  library = NULL;
  initializationFailed = false;

  initFont();
  // CDBDebug("constructor");
}

void CCairoPlotter::pixel_overwrite(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {

  if (x < 0 || y < 0 || x >= width || y >= height) return;
  if (a != 0 && a != 255) {
    isAlphaUsed = true;
  }
  size_t p = x * 4 + y * stride;
  ARGBByteBuffer[p] = b;
  ARGBByteBuffer[p + 1] = g;
  ARGBByteBuffer[p + 2] = r;
  ARGBByteBuffer[p + 3] = a;
}

void CCairoPlotter::pixel_blend(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
  if (x < 0 || y < 0 || x >= width || y >= height) return;

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
      // newAlpha = 1;

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

CCairoPlotter::CCairoPlotter(int width, int height, float fontSize, const char *fontLocation, unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
  byteBufferPointerIsOwned = true;
  stride = cairo_format_stride_for_width(FORMAT, width);
  size_t bufferSize = size_t(height) * stride;
  ARGBByteBuffer = new unsigned char[bufferSize];
  if (a == 0 && r == 0 && g == 0 && b == 0) {
    for (size_t j = 0; j < bufferSize; j++) {
      ARGBByteBuffer[j] = 0;
    }
  } else {
    for (size_t j = 0; j < bufferSize / 4; j++) {
      ARGBByteBuffer[j * 4 + 3] = a;
      ARGBByteBuffer[j * 4 + 2] = r;
      ARGBByteBuffer[j * 4 + 1] = g;
      ARGBByteBuffer[j * 4 + 0] = b;
    }
  }
  _cairoPlotterInit(width, height, fontSize, fontLocation);
}

CCairoPlotter::CCairoPlotter(int width, int height, unsigned char *_ARGBByteBuffer, float fontSize, const char *fontLocation) {
  byteBufferPointerIsOwned = false;
  this->width = width;
  this->height = height;
  this->fontSize = fontSize;
  this->fontLocation = fontLocation;
  stride = cairo_format_stride_for_width(FORMAT, width);
  this->ARGBByteBuffer = (_ARGBByteBuffer);
  surface = cairo_image_surface_create_for_data(ARGBByteBuffer, CCairoPlotter::FORMAT, width, height, stride);
  cr = cairo_create(this->surface);
  library = NULL;
  initializationFailed = false;

  initFont();
}

CCairoPlotter::~CCairoPlotter() {
  cairo_destroy(cr);
  cairo_surface_destroy(surface);
  if (byteBufferPointerIsOwned) {
    delete[] ARGBByteBuffer;
    ARGBByteBuffer = NULL;
  }
}

int CCairoPlotter::renderFont(FT_Bitmap *bitmap, int left, int top) {
  for (int y = 0; y < (int)bitmap->rows; y++) {
    for (int x = 0; x < (int)bitmap->width; x++) {
      size_t p = (x + y * bitmap->width);

      if (bitmap->buffer[p] != 0) {
        float alpha = bitmap->buffer[p];
        // alpha/=256;

        // r=255;g=255;b=255;
        // plot( x+left,  y+top, alpha);
        // r=0;g=0;b=0;
        //           _plot( x+left,  y+top, alpha);
        pixel_blend(x + left, y + top, r, g, b, alpha);
      }
    }
  }
  return 0;
}
int CCairoPlotter::initializeFreeType() {
  // CDBDebug("initializeFreeType(%d)\n", library == NULL);
  if (library != NULL) {
    CDBError("Freetype is already intialized");
    return 1;
  };
  int error = FT_Init_FreeType(&library);
  const char *fontLocation = this->fontLocation;
  if (error) {
    CDBError("an error occurred during freetype library initialization");
    if (library != NULL) {
      FT_Done_FreeType(library);
      library = NULL;
      face = NULL;
    }
    return 1;
  }
  error = FT_New_Face(library, fontLocation, 0, &face);
  if (error == FT_Err_Unknown_File_Format) {
    CDBError("the font file could be opened and read, but it appears that its font format is unsupported %s", fontLocation);
    if (library != NULL) {
      FT_Done_FreeType(library);
      library = NULL;
      face = NULL;
    }
    return 1;
  } else if (error) {
    CDBError("Unable to initialize freetype: Could not read fontfile %s", fontLocation);
    if (library != NULL) {
      FT_Done_FreeType(library);
      library = NULL;
      face = NULL;
    }
    return 1;
  }

  error = FT_Set_Char_Size(face,               /* handle to face object */
                           0,                  /* char_width in 1/64th of points */
                           int(fontSize * 64), /* char_height in 1/64th of points */
                           100,                /* horizontal device resolution */
                           100);               /* vertical device resolution */
  if (error) {
    CDBError("unable to set character size");
    if (library != NULL) {
      FT_Done_FreeType(library);
      library = NULL;
      face = NULL;
    }
    return 1;
  }
  return 0;
}

// Aux function to support unicode characters (such as en dash)
// A character is represented by a variable number of bytes (1 to 4)
// First byte indicates the length
// The pattern is identified through massk
//   - 1 byte (ASCII), top bit must be 0.
//   - 2 bytes (mask: 0xE0, match 0xC0)
//   - 3 bytes (mask: 0xF0, match 0xE0)
//   - 4 bytes (not supported by this function)
const char *decode_utf8_char(const char *p, const char *end, uint32_t *out_char) {
  unsigned char c = (unsigned char)*p;

  // Case where the pointer is not valid
  if (p >= end) {
    *out_char = '?';
    return p;
  }

  if (c < 0x80) {
    // 1 byte (ASCII)
    *out_char = c;
    return p + 1;
  }

  if ((c & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80) {
    // 2 bytes

    if (p + 1 >= end) {
      *out_char = '?';
      return p + 1;
    }

    *out_char = ((c & 0x1F) << 6) | (p[1] & 0x3F);
    return p + 2;
  }

  if ((c & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 && (p[2] & 0xC0) == 0x80) {
    // 2 bytes

    if (p + 2 >= end) {
      *out_char = '?';
      return p + 1;
    }

    *out_char = ((c & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
    return p + 3;
  }

  // Default case (error or unsupported)
  *out_char = '?';
  return p + 1;
}

int CCairoPlotter::_drawFreeTypeText(int x, int y, int &w, int &h, float angle, const char *text, bool render) {
  // Draw text :)

  w = 0;
  h = 0;
  if (library == NULL) {
    int status = initializeFreeType();
    if (status != 0) {
      return 1;
    }
  };
  FT_GlyphSlot slot;
  FT_Matrix matrix; /* transformation matrix */
  FT_Vector pen;    /* untransformed origin */
  int my_target_height = 8;
  slot = face->glyph; /* a small shortcut */
  /* set up matrix */
  matrix.xx = (FT_Fixed)(cos(angle) * 0x10000L);
  matrix.xy = (FT_Fixed)(-sin(angle) * 0x10000L);
  matrix.yx = (FT_Fixed)(sin(angle) * 0x10000L);
  matrix.yy = (FT_Fixed)(cos(angle) * 0x10000L); /* the pen position in 26.6 cartesian space coordinates */

  /* start at (300,200) */
  pen.x = x * 64;
  pen.y = (my_target_height - y) * 64;

  /* Using UTF-8 standard */

  const char *p = text;
  const char *end = text + strlen(text);
  while (*p) {
    uint32_t codepoint;
    p = decode_utf8_char(p, end, &codepoint);
    // CDBDebug("* Decoded char U+%04X from: %.*s", codepoint, (int)(p - prev), prev);

    FT_Set_Transform(face, &matrix, &pen);
    int glyphIndex = FT_Get_Char_Index(face, codepoint);
    if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER) == 0) {
      if (render) {
        renderFont(&slot->bitmap, slot->bitmap_left, my_target_height - slot->bitmap_top);
      }
      pen.x += slot->advance.x;
      pen.y += slot->advance.y;
      w += slot->advance.x / 64;
      if ((int)slot->bitmap.rows > h) h = (int)slot->bitmap.rows;
    }
  }

  return 0;
}

int CCairoPlotter::getTextSize(int &w, int &h, float angle, const char *text) { return _drawFreeTypeText(0, 0, w, h, angle, text, false); }

int CCairoPlotter::drawAnchoredText(int x, int y, float angle, const char *text, int anchor) {
  int w = 0, h = 0;
  getTextSize(w, h, angle, text);
  //    CDBDebug("[w,h]=>[%d,%d] %s at [%d,%d] %d,%d\n", w, h, text, x, y, anchor, anchor % 4);
  switch (anchor % 4) {
  case 0:
    _drawFreeTypeText(x, y, w, h, angle, text, true);
    break;
  case 1:
    _drawFreeTypeText(x, y + h, w, h, angle, text, true);
    break;
  case 2:
    _drawFreeTypeText(x - w, y + h, w, h, angle, text, true);
    break;
  case 3:
    _drawFreeTypeText(x - w, y, w, h, angle, text, true);
    break;
  }
  return 0;
}

int CCairoPlotter::drawCenteredText(int x, int y, float angle, const char *text) {
  int w = 0, h = 0;
  getTextSize(w, h, angle, text);
  //    CDBDebug("[w,h]=>[%d,%d] at [%d,%d] (%d)\n", w, h, x, y, isAlphaUsed);
  return _drawFreeTypeText(x - w / 2, y + h / 2, w, h, angle, text, true);
}

int CCairoPlotter::drawFilledText(int x, int y, float angle, const char *text) {
  if (text == NULL) return 0;
  if (strlen(text) == 0) return 0;
  if (initializationFailed == true) return 1;
  if (library == NULL) {
    int status = initializeFreeType();
    if (status != 0) {
      initializationFailed = true;
      return 1;
    }
  };

  FT_Matrix matrix; /* transformation matrix */
  /* set up matrix */
  matrix.xx = (FT_Fixed)(cos(angle) * 0x10000L);
  matrix.xy = (FT_Fixed)(-sin(angle) * 0x10000L);
  matrix.yx = (FT_Fixed)(sin(angle) * 0x10000L);
  matrix.yy = (FT_Fixed)(cos(angle) * 0x10000L); /* the pen position in 26.6 cartesian space coordinates */

  // Draw text :)
  int my_target_height = 8, error;
  int num_chars = strlen(text);
  int orgr = this->r;
  int orgg = this->g;
  int orgb = this->b;
  int orga = int(this->a);

  FT_Vector pen; /* untransformed origin */
  pen.x = x * 64;
  pen.y = (my_target_height - y) * 64;
  setColor(255, 255, 255, 0);
  filledRectangle(pen.x / 64 - 5, my_target_height - (pen.y) / 64 + 8, (pen.x) / 64, my_target_height - (pen.y) / 64 - int(fontSize) - 4);
  for (int n = 0; n < num_chars; n++) {    /* set transformation */
    FT_Set_Transform(face, &matrix, &pen); /* load glyph image into the  face->glyph (erase previous one) */
    error = FT_Load_Char(face, text[n], FT_LOAD_RENDER);
    if (error) {
      CDBError("unable toFT_Load_Char");
      return 1;
    }
    /* now, draw to our target surface (convert position) */

    // setFillColor(255,255,255,100);

    setColor(255, 255, 255, 0);
    filledRectangle(pen.x / 64, my_target_height - (pen.y) / 64 + 5, (pen.x + face->glyph->advance.x) / 64, my_target_height - (pen.y) / 64 - int(fontSize) - 4);
    setColor(orgr, orgg, orgb, orga);
    renderFont(&face->glyph->bitmap, face->glyph->bitmap_left, my_target_height - face->glyph->bitmap_top);
    /* increment pen position */
    pen.x += face->glyph->advance.x;
    pen.y += face->glyph->advance.y;
  }
  setColor(255, 255, 255, 0);
  filledRectangle(pen.x / 64, my_target_height - (pen.y) / 64 + 5, (pen.x) / 64 + 5, my_target_height - (pen.y) / 64 - int(fontSize) - 4);
  setColor(orgr, orgg, orgb, orga);
  return 0;
}

void CCairoPlotter::initFont() {}

void CCairoPlotter::setColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
  this->r = r;
  rr = r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
  this->g = g;
  rg = g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
  this->b = b;
  rb = b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
  this->a = (float)a;
  ra = a * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
}

void CCairoPlotter::setFillColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
  fr = r;
  rfr = r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
  fg = g;
  rfg = g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
  fb = b;
  rfb = b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
  fa = float(a);
  rfa = a * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL;
}

void CCairoPlotter::getPixel(int x, int y, unsigned char &r, unsigned char &g, unsigned char &b, unsigned char &a) {
  if (x < 0 || y < 0 || x >= width || y >= height) {
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

unsigned char *CCairoPlotter::getByteBuffer() { return ARGBByteBuffer; }
/*
  void flush() {
    cairo_surface_flush(surface);
  }*/

void CCairoPlotter::rectangle(int x1, int y1, int x2, int y2) {
  cairo_rectangle(cr, x1 + 0.5, y1 + 0.5, x2 - x1, y2 - y1);
  cairo_set_source_rgba(cr, rr, rg, rb, a);
  cairo_stroke(cr);
}

void CCairoPlotter::filledRectangle(int x1, int y1, int x2, int y2) {
  if (y1 > y2) _swap(y1, y2);

  cairo_rectangle(cr, x1, y1, x2 - x1, y2 - y1);
  cairo_set_source_rgba(cr, rfr, rfg, rfb, rfa);
  cairo_fill(cr);
  cairo_antialias_t aa = cairo_get_antialias(cr);
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
  cairo_set_line_width(cr, 0.5);
  rectangle(x1, y1, x2, y2);
  cairo_set_antialias(cr, aa);
}

void CCairoPlotter::line_noaa(int x1, int y1, int x2, int y2) {
  cairo_set_source_rgba(cr, rr, rg, rb, ra);
  cairo_antialias_t aa = cairo_get_antialias(cr);
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
  cairo_move_to(cr, x1, y1);
  cairo_line_to(cr, x2, y2);
  cairo_set_line_width(cr, 1);
  cairo_stroke(cr);
  cairo_set_antialias(cr, aa);
}

void CCairoPlotter::moveTo(float x1, float y1) { cairo_move_to(cr, x1 + 0.5, y1 + 0.5); }
void CCairoPlotter::lineTo(float x1, float y1) {
  cairo_set_source_rgba(cr, rr, rg, rb, ra);
  cairo_line_to(cr, x1 + 0.5, y1 + 0.5);
  cairo_set_line_width(cr, 0.9);
  cairo_stroke(cr);
}

void CCairoPlotter::lineTo(float x1, float y1, float width) {
  cairo_set_source_rgba(cr, rr, rg, rb, ra);
  cairo_set_line_width(cr, width);
  cairo_line_to(cr, x1 + 0.5, y1 + 0.5);
}

void CCairoPlotter::endLine() {
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
  cairo_stroke(cr);
}

void CCairoPlotter::endLine(const double *dashes, int num_dashes) {
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_MITER);
  cairo_set_dash(cr, dashes, num_dashes, 0);
  cairo_stroke(cr);
  cairo_set_dash(cr, 0, 0, 0);
}

void CCairoPlotter::line(float x1, float y1, float x2, float y2) {
  cairo_set_source_rgba(cr, rr, rg, rb, ra);
  cairo_move_to(cr, x1 + 0.5, y1 + 0.5);
  cairo_line_to(cr, x2 + 0.5, y2 + 0.5);
  cairo_set_line_width(cr, 0.9);
  cairo_stroke(cr);
}
void CCairoPlotter::line(float x1, float y1, float x2, float y2, float width) {
  cairo_set_source_rgba(cr, rr, rg, rb, ra);
  cairo_move_to(cr, x1 + 0.5, y1 + 0.5);
  cairo_line_to(cr, x2 + 0.5, y2 + 0.5);
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
  // cairo_set_line_width(cr, 1.0);
  // cairo_set_source_rgba(cr, 1, 0.2, 0.2, 0.6);
  cairo_set_source_rgba(cr, rfr, rfg, rfb, rfa);
  cairo_arc(cr, x, y, r, 0, 2 * M_PI);
  cairo_fill(cr);

  cairo_arc(cr, x, y, r, 0, 2 * M_PI);
  cairo_set_line_width(cr, 0.8);
  cairo_set_source_rgba(cr, rr, rg, rb, ra);
  cairo_stroke(cr);
  cairo_restore(cr);
}

void CCairoPlotter::filledEllipse(int x, int y, float rX, float rY, float rotation) {
  cairo_save(cr);
  cairo_set_source_rgba(cr, rfr, rfg, rfb, rfa);
  cairo_translate(cr, x, y);
  cairo_rotate(cr, rotation);
  cairo_scale(cr, rX, rY);
  cairo_arc(cr, 0, 0, 1, 0, 2 * M_PI);
  cairo_fill(cr);
  cairo_restore(cr);
}

void CCairoPlotter::circle(int x, int y, int r, float lineWidth) {
  double lWx = lineWidth;
  double lWy = lineWidth;
  cairo_device_to_user_distance(cr, &lWx, &lWy);
  cairo_set_source_rgba(cr, rr, rg, rb, ra);
  cairo_set_line_width(cr, lWx);
  cairo_arc(cr, x, y, r, 0, 2 * M_PI);
  cairo_stroke(cr);
}

void CCairoPlotter::poly(float x[], float y[], int n, bool closePath, bool fill) {
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
  cairo_stroke(cr);
  cairo_set_antialias(cr, aa);
}

void CCairoPlotter::poly(float x[], float y[], int n, float lineWidth, bool closePath, bool fill) {
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
  cairo_stroke(cr);
  cairo_set_antialias(cr, aa);
}

void CCairoPlotter::drawText(int x, int y, double angle, const char *text) {
  int w, h;
  _drawFreeTypeText(x, y, w, h, angle, text, true);
}

// void CCairoPlotter::drawStrokedText(int x, int y, double angle, const char *text, float fontSize, float strokeWidth, CColor bgcolor, CColor fgcolor, bool centerText) {
void CCairoPlotter::drawStrokedText(int x, int y, double angle, const char *text, TextStyle textStyle, bool centerText) {
  if (library == NULL) {
    int status = initializeFreeType();
    if (status != 0) {
      // TODO
    }
  }

  cairo_save(cr);

  cairo_font_face_t *ct = cairo_ft_font_face_create_for_ft_face(face, 0);
  cairo_set_font_face(cr, ct);
  cairo_set_font_size(cr, textStyle.fontSize);

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
  if (textStyle.textOutlineWidth > 0) {
    CColor bgcolor = textStyle.textOutlineColor;

    // textStyle.textOutlineWidth /= 2; // TODO: fix this
    // strokeWidth = 2;
    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, bgcolor.r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, bgcolor.g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, bgcolor.b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, .2);
    cairo_set_line_width(cr, 2.5 + textStyle.textOutlineWidth);
    cairo_stroke_preserve(cr);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(cr, bgcolor.r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, bgcolor.g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, bgcolor.b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, 1);
    cairo_set_line_width(cr, 1.5 + textStyle.textOutlineWidth);
    cairo_stroke_preserve(cr);
  }

  CColor fgcolor = textStyle.textColor;
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_BEST);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, fgcolor.r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, fgcolor.g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, fgcolor.b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, 1);
  cairo_fill(cr);

  cairo_close_path(cr);

  cairo_restore(cr);
  // Put the original path back
  cairo_append_path(cr, cp);
  cairo_path_destroy(cp);
}

void CCairoPlotter::writeToPng24Stream(FILE *fp, unsigned char) { writeARGBPng(width, height, ARGBByteBuffer, fp, 24, false); }

void CCairoPlotter::writeToPng8Stream(FILE *fp, unsigned char, bool use8bitpalAlpha) { writeARGBPng(width, height, ARGBByteBuffer, fp, 8, use8bitpalAlpha); }

void CCairoPlotter::writeToPng32Stream(FILE *fp, unsigned char alpha) {
  if (isAlphaUsed) {
    CDBDebug("Alpha was used");
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        size_t p = x * 4 + y * stride;
        if (ARGBByteBuffer[p + 3] != 255) {
          float a = ARGBByteBuffer[p + 3];
          ARGBByteBuffer[p] = (unsigned char)((float(ARGBByteBuffer[p]) * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL) * float(a));
          ARGBByteBuffer[p + 1] = (unsigned char)((float(ARGBByteBuffer[p + 1]) * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL) * float(a));
          ARGBByteBuffer[p + 2] = (unsigned char)((float(ARGBByteBuffer[p + 2]) * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL) * float(a));
        }
      }
    }
  }
  cairo_set_operator(cr, CAIRO_OPERATOR_DEST_IN);
  if (alpha != 255) {
    cairo_paint_with_alpha(cr, float(alpha) * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL);
  }
  cairo_surface_flush(surface);
  this->fp = fp;
  cairo_surface_write_to_png_stream(surface, writerFunc, (void *)fp);
}

int CCairoPlotter::writeARGBPng(int width, int height, unsigned char *ARGBByteBuffer, FILE *file, int bitDepth, bool use8bitpalAlpha) {
  // bool use8bitpalAlpha = true;

  // CDBDebug("Using png library directly to write PNG");
  OctreeType *tree = NULL;

#ifdef MEASURETIME
  StopWatch_Stop("start writeRGBAPng.");
#endif

  png_structp png_ptr;
  png_infop info_ptr;
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

#ifdef MEASURETIME
  StopWatch_Stop("png_create_write_struct written");
#endif
  if (!png_ptr) {
    CDBError("png_create_write_struct failed");
    return 1;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_write_struct(&png_ptr, NULL);
    CDBError("png_create_info_struct failed");
    return 1;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    CDBError("Error during init_io");
    return 1;
  }

  png_init_io(png_ptr, file);

  /* write header */
  if (setjmp(png_jmpbuf(png_ptr))) {
    CDBError("Error during writing header");
    return 1;
  }
  /*png_set_IHDR(png_ptr, info_ptr, width, height, 8,
   *          PNG_COLOR_TYPE_RGB_ALPHA,
   *          PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
   P NG*_FILTER_TYPE_DEFAULT);*/
  if (bitDepth == 32) {
    /*png_set_IHDR(png_ptr, info_ptr, width, height, 8,
     *          PNG_COLOR_TYPE_RGB_ALPHA,
     *          PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
     P *NG_FILTER_TYPE_BASE);*/
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

  } else if (bitDepth == 24) {
    /*png_set_IHDR(png_ptr, info_ptr, width, height, 8,
     *          PNG_COLOR_TYPE_RGB_ALPHA,
     *          PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
     P *NG_FILTER_TYPE_BASE);*/
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_byte a[1];
    png_color_16 trans_values[1];
    a[0] = 0;
    trans_values[0].index = 0;
    trans_values[0].red = 0;
    trans_values[0].green = 0;
    trans_values[0].blue = 0;
    png_set_tRNS(png_ptr, info_ptr, a, 1, trans_values);

  } else if (bitDepth == 8) {
    // CDBDebug("Starting header");
    png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    // CDBDebug("Finished header");

    png_color palette[256];
    png_byte a[256];
    png_color_16 trans_values[256];
#ifdef MEASURETIME
    StopWatch_Stop("Creating octtree for color quantization");
#endif

    if (use8bitpalAlpha) {
      for (int j = 0; j < width * height; j = j + 1) {
        RGBType color;
        color.b = ARGBByteBuffer[0 + j * 4] / 8 + int(ARGBByteBuffer[3 + j * 4] / 32) * 32;
        color.g = ARGBByteBuffer[1 + j * 4];
        color.r = ARGBByteBuffer[2 + j * 4];
        color.realblue = ARGBByteBuffer[0 + j * 4];
        color.realalpha = ARGBByteBuffer[3 + j * 4];
        InsertTree(&tree, &color, -1);
      }
    } else {
      bool something = false;
      for (int j = 0; j < width * height; j = j + 1) {
        RGBType color;
        if ((ARGBByteBuffer[3 + j * 4] > 64)) {
          something = true;
          color.b = ARGBByteBuffer[0 + j * 4];
          color.g = ARGBByteBuffer[1 + j * 4];
          color.r = ARGBByteBuffer[2 + j * 4];
          color.realblue = ARGBByteBuffer[0 + j * 4];
          color.realalpha = ARGBByteBuffer[3 + j * 4];
          InsertTree(&tree, &color, -1);
        }
      }
      if (!something) {
        RGBType color;
        color.r = 0;
        color.g = 0;
        color.b = 0;
        color.realblue = 0;
        color.realalpha = 0;
        InsertTree(&tree, &color, -1);
      }
    }

#ifdef MEASURETIME
    StopWatch_Stop("Tree filled, starting reduction");
#endif
    if (use8bitpalAlpha) {
      while (TotalLeafNodes() > 255) {
        ReduceTree();
      }
    } else {
      while (TotalLeafNodes() > 254) {
        ReduceTree();
      }
    }
#ifdef MEASURETIME
    StopWatch_Stop("Tree reduction completed");
#endif

    if (use8bitpalAlpha) {
      int numColors = 0;
      RGBType table[256];
      MakePaletteTable(tree, table, &numColors);
      if (numColors > 255) numColors = 255;
      int numAlphaColors = 0;
      palette[0].red = 0;
      palette[0].green = 0;
      palette[0].blue = 0;
      for (int j = 0; j < 256 && j < numColors; j++) {
        palette[j].red = table[j].r;
        palette[j].green = table[j].g;
        palette[j].blue = table[j].realblue;
        unsigned char alpha = table[j].realalpha;
        a[numAlphaColors] = alpha;
        numAlphaColors++;
      }
      png_set_PLTE(png_ptr, info_ptr, palette, numColors);
      png_set_tRNS(png_ptr, info_ptr, a, numAlphaColors, trans_values);
    } else {
      int numColors = 0;
      RGBType table[256];
      MakePaletteTable(tree, table, &numColors);
      if (numColors > 254) numColors = 254;
      palette[0].red = 0;
      palette[0].green = 0;
      palette[0].blue = 0;
      for (int j = 1; j < 256 && j < numColors + 1; j++) {
        palette[j].red = table[j - 1].r;
        palette[j].green = table[j - 1].g;
        palette[j].blue = table[j - 1].realblue;
      }
      png_set_PLTE(png_ptr, info_ptr, palette, 255);

      a[0] = 0;
      trans_values[0].index = 0;
      trans_values[0].red = 0;
      trans_values[0].green = 0;
      trans_values[0].blue = 0;
      png_set_tRNS(png_ptr, info_ptr, a, 1, trans_values);
    }
  }

  png_set_filter(png_ptr, 0, PNG_FILTER_NONE);
  png_write_info(png_ptr, info_ptr);

  /* write bytes */
  if (setjmp(png_jmpbuf(png_ptr))) {
    CDBError("Error during writing bytes");
    return 1;
  }
  png_set_packing(png_ptr);
#ifdef MEASURETIME
  StopWatch_Stop("Headers written");
#endif

  int i;
  png_bytep row_ptr = 0;

  if (bitDepth == 32) {

    int s = width * 4;
    for (i = 0; i < height; i = i + 1) {

      unsigned char RGBARow[s];
      int p = 0;
      int start = i * s;
      int stop = start + s;
      for (int x = start; x < stop; x += 4) {
        RGBARow[p++] = ARGBByteBuffer[2 + x];
        RGBARow[p++] = ARGBByteBuffer[1 + x];
        RGBARow[p++] = ARGBByteBuffer[0 + x];
        RGBARow[p++] = ARGBByteBuffer[3 + x];
      }

      row_ptr = RGBARow;
      png_write_rows(png_ptr, &row_ptr, 1);
    }
  } else if (bitDepth == 24) {
#ifdef MEASURETIME
    StopWatch_Stop("Start 24BIT FILL");
#endif
    int s = width * 4;
    for (i = 0; i < height; i = i + 1) {

      unsigned char RGBRow[s];
      int p = 0;
      int start = i * s;
      int stop = start + s;
      for (int x = start; x < stop; x += 4) {
        if (ARGBByteBuffer[3 + x] > 127) {
          if (ARGBByteBuffer[2 + x] == 0 && ARGBByteBuffer[1 + x] == 0 && ARGBByteBuffer[0 + x] == 0) {
            RGBRow[p++] = 1;
            RGBRow[p++] = 0;
            RGBRow[p++] = 0;
          } else {
            RGBRow[p++] = ARGBByteBuffer[2 + x];
            RGBRow[p++] = ARGBByteBuffer[1 + x];
            RGBRow[p++] = ARGBByteBuffer[0 + x];
          }
        } else {
          RGBRow[p++] = 0;
          RGBRow[p++] = 0;
          RGBRow[p++] = 0;
        }
      }

      row_ptr = RGBRow;
      png_write_rows(png_ptr, &row_ptr, 1);
    }
#ifdef MEASURETIME
    StopWatch_Stop("Finished 24BIT FILL");
#endif
  } else if (bitDepth == 8) {
    int s = width * 4;

#ifdef MEASURETIME
    StopWatch_Stop("Starting color quantization");
#endif
    for (i = 0; i < height; i++) {
      unsigned char RGBARow[s];
      int p = 0;
      int start = i * s;
      int stop = start + s;
      for (int x = start; x < stop; x += 4) {
        // if(ARGBByteBuffer[3+x]>128){

        RGBType color;
        color.g = ARGBByteBuffer[1 + x];
        color.r = ARGBByteBuffer[2 + x];
        if (use8bitpalAlpha) {
          color.b = ARGBByteBuffer[0 + x] / 8 + int(ARGBByteBuffer[3 + x] / 32) * 32;
          RGBARow[p++] = QuantizeColorMapped(tree, &color);

        } else {
          color.b = ARGBByteBuffer[0 + x];
          if (ARGBByteBuffer[3 + x] > 64) {
            RGBARow[p++] = QuantizeColorMapped(tree, &color) + 1;
          } else {
            RGBARow[p++] = 0;
          }
        }
      }

      row_ptr = RGBARow;
      png_write_rows(png_ptr, &row_ptr, 1);
    }
  }

#ifdef MEASURETIME
  StopWatch_Stop("PNG data written");
#endif

  /* end write */
  if (setjmp(png_jmpbuf(png_ptr))) {
    CDBError("Error during end of write");
    return 1;
  }

  png_write_end(png_ptr, NULL);
  png_destroy_write_struct(&png_ptr, &info_ptr);
#ifdef MEASURETIME
  StopWatch_Stop("end writeRGBAPng.");
#endif
  return 0;
}

void CCairoPlotter::setToSurface(cairo_surface_t *png) {
  cairo_set_source_surface(this->cr, png, 0, 0);
  cairo_paint(this->cr);
  //   cairo_surface_destroy(surface);
}

#ifdef ADAGUC_USE_WEBP
#include "webp/encode.h"
#include "webp/decode.h"
#include "webp/types.h"

static int MyWriter(const uint8_t *data, size_t data_size, const WebPPicture *const pic) {
  FILE *const out = (FILE *)pic->custom_ptr;
  return data_size ? (fwrite(data, data_size, 1, out) == 1) : 1;
}

#endif

void CCairoPlotter::writeToWebP32Stream(FILE *fp, unsigned char, int quality) {
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
  picture.use_argb = 1;                     // To select between ARGB and YUVA input.
  config.thread_level = 1;
  picture.width = width;
  picture.height = height;
  picture.writer = MyWriter;
  picture.custom_ptr = (void *)fp;
  if (!WebPPictureAlloc(&picture)) return; // memory error

  if (!WebPValidateConfig(&config)) {
    CDBError("Error! Invalid configuration.");
    return;
  }

  WebPPictureImportBGRA(&picture, ARGBByteBuffer, stride);

  if (!WebPEncode(&config, &picture)) {
    CDBError("Error!  Cannot encode picture as WebP");
  }
  WebPPictureFree(&picture);

#else
  CDBError("-DADAGUC_USE_WEBP not enabled");
#endif
}

#endif

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

// void CCairoPlotter::drawBarb(int x, int y, double uncorrectedDirection, double viewDirCorrection, double strength, CColor barbColor, CColor outlineColor, float lineWidth, bool toKnots, bool flip,
//                              bool drawText, double fontSize, CColor textColor, double outlineWidth) {

void CCairoPlotter::drawBarb(int x, int y, double uncorrectedDirection, double viewDirCorrection, double strength, bool toKnots, bool flip, bool drawVectorPlotValue, LineStyle lineStyle, TextStyle textStyle) {
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
  cairo_set_line_width(cr, lineStyle.lineWidth);
  cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);

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

  if (lineStyle.lineOutlineWidth > 0) {
    CColor outlineColor = lineStyle.lineOutlineColor;

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cr, outlineColor.r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, outlineColor.g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, outlineColor.b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, .2);
    cairo_set_line_width(cr, 1.0 + lineStyle.lineOutlineWidth);
    cairo_stroke_preserve(cr);

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba(cr, outlineColor.r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, outlineColor.g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, outlineColor.b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, 1);
    cairo_set_line_width(cr, lineStyle.lineOutlineWidth);
    cairo_stroke_preserve(cr);
  }

  // Stroke thin version
  CColor barbColor = lineStyle.lineColor;
  cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
  cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
  cairo_set_source_rgba(cr, barbColor.r * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, barbColor.g * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, barbColor.b * CAIROPLOTTER_COLOR_BYTE_TO_NORMAL, 1);
  cairo_set_line_width(cr, lineStyle.lineWidth);
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

  if (drawVectorPlotValue) {
    textStyle.textColor = textStyle.textColor.a < 1 ? barbColor : textStyle.textColor;

    bool showDirection = false;
    // bool drawOutline = vectorStyle.outlineWidth > .01;
    if (showDirection == false) {
      CT::string text;
      text.print("%d", strengthInKnots);

      // If speed is really low, draw the text below the circle
      double textDirection = strengthInKnotsRoundedToFive <= 2 ? -M_PI / 2.1 : direction;
      // this->drawStrokedText(x - cos(textDirection + M_PI) * 15 - 5, y + sin(textDirection + M_PI) * 12 + 5, 0, text.c_str(), fontSize, outlineWidth, outlineColor, barbTextColor);

      textStyle.textOutlineWidth = 1 * (textStyle.textOutlineWidth > 0.01);
      this->drawStrokedText(x - cos(textDirection + M_PI) * 15 - 5, y + sin(textDirection + M_PI) * 12 + 5, 0, text.c_str(), textStyle);

    } else {
      double degrees = fmod(((270 - ((uncorrectedDirection) * (180 / M_PI)))), 360);
      CT::string text;
      text.print("%02d %03d°", strengthInKnots, int(round(degrees)));

      // If speed is really low, draw the text below the circle
      double textDirection = strengthInKnotsRoundedToFive <= 2 ? -M_PI / 2.1 : direction;
      int tx = x - cos(textDirection + M_PI) * 15 - 5 - text.length();
      int ty = y + sin(textDirection + M_PI) * 12 + 5;
      this->drawStrokedText(tx, ty, 0, text.c_str(), textStyle);
    }
  }
}