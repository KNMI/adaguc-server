/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Packages PNG into a NetCDF file
 * Author:   Maarten Plieger (KNMI)
 * Date:     2017-08-11
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

#include "CReadPNG.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <algorithm>
#include <cfenv>
#include <cmath>
#define PNG_DEBUG 3
#include <png.h>

// #define CREADPNG_DEBUG

// https://aiddata.rvo.nl/projects

CPNGRaster::~CPNGRaster() {
  if (data != NULL) {
    delete[] data;
    data = NULL;
  }
}

CPNGRaster *CReadPNG_read_png_file(const char *file_name, bool pngReadHeaderOnly) {
  if (pngReadHeaderOnly) {
    CDBDebug("open image %s", file_name);
  } else {
    CDBDebug("open only headers %s", file_name);
  }
  png_byte color_type;
  png_byte bit_depth;

  png_structp png_ptr = nullptr;
  png_infop info_ptr = nullptr;
  //   int number_of_passes;
  png_bytep *row_pointers = nullptr;

  unsigned char header[8]; // 8 is the maximum size that can be checked

  /* open file and test for it being a png */
  FILE *fp = fopen(file_name, "rb");
  if (!fp) {
    CDBError("[read_png_file] File %s could not be opened for reading", file_name);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return NULL;
  }
  (void)!fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8)) {
    CDBError("[read_png_file] File %s is not recognized as a PNG file", file_name);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return NULL;
  }

  /* initialize stuff */
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!png_ptr) {
    CDBError("[read_png_file] png_create_read_struct failed");
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return NULL;
  }

  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    CDBError("[read_png_file] png_create_info_struct failed");
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return NULL;
  }

  if (setjmp(png_jmpbuf(png_ptr))) {
    CDBError("[read_png_file] Error during init_io");
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return NULL;
  }

  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);

  png_read_info(png_ptr, info_ptr);

  CPNGRaster *pngRaster = new CPNGRaster();

  pngRaster->width = png_get_image_width(png_ptr, info_ptr);
  pngRaster->height = png_get_image_height(png_ptr, info_ptr);
  // CDBDebug("open PNG of size [%dx%d]", pngRaster->width, pngRaster->height );
  color_type = png_get_color_type(png_ptr, info_ptr);
  bit_depth = png_get_bit_depth(png_ptr, info_ptr);

  /* Read text properties from PNG file */
  png_textp text_ptr;
  int num_text = 0;
  png_get_text(png_ptr, info_ptr, &text_ptr, &num_text);

  for (int j = 0; j < num_text; j++) {
    pngRaster->headers.add({.key = text_ptr[j].key, .value = text_ptr[j].text});
  }

  png_read_update_info(png_ptr, info_ptr);

  if (pngReadHeaderOnly == true) {
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return pngRaster;
  }

#ifdef CREADPNG_DEBUG
  CDBDebug("read png data");
#endif

  /* read file */
  if (setjmp(png_jmpbuf(png_ptr))) {
    CDBError("[read_png_file] Error during read_image");
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return NULL;
  }

  row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * pngRaster->height);

  for (size_t y = 0; y < pngRaster->height; y++) row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png_ptr, info_ptr));
#ifdef CREADPNG_DEBUG
  CDBDebug("start png_read_image");
#endif
  png_read_image(png_ptr, row_pointers);

#ifdef CREADPNG_DEBUG
  CDBDebug("/done png_read_image");
  CDBDebug("PNG data: [%d, %d, %d, %d]", pngRaster->width, pngRaster->height, (int)bit_depth, color_type);
#endif

  int pngwidthdivisor = 1;
  if (bit_depth == 4) {

    pngwidthdivisor = 2;
  }
  if (bit_depth == 1) {

    pngwidthdivisor = 8;
  }
  int num_palette;
  png_colorp palette;
  if (color_type == 3) {
    png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
  }

  if (pngRaster->data != NULL) {
    CDBDebug("pngRaster->data already defined, return");
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return pngRaster;
  }

  pngRaster->data = new unsigned char[pngRaster->width * pngRaster->height * 4];
  size_t c = std::ceil((pngRaster->width / pngwidthdivisor) + 1.f);
  size_t calcWidth = std::min(pngRaster->width, c);
  for (size_t y = 0; y < pngRaster->height; y++) {
    unsigned char *line = row_pointers[y];
    for (size_t x = 0; x < calcWidth; x += 1) {

      /*  Each pixel is a palette index, using PLTE */
      if (color_type == 3) {
        unsigned char i = line[x * 1 + 0];
        if (bit_depth == 1) { // 1 byte is 8 pixels (colormap of 2 colors)
          int d = 128;
          for (size_t j = 0; j < 8; j++) {
            size_t nx = x * 32 + j * 4;
            if (nx + 3 < pngRaster->width * 4) {
              size_t b = nx + 0 + (y * pngRaster->width * 4);
              pngRaster->data[b + 0] = palette[(i & d) / d].red;
              pngRaster->data[b + 1] = palette[(i & d) / d].green;
              pngRaster->data[b + 2] = palette[(i & d) / d].blue;
              pngRaster->data[b + 3] = 255;
            }
            d = d / 2;
          }
        }
        if (bit_depth == 4) { // 1 byte is 2 pixels (colormap of 16 colors)
          size_t b = x * 8 + (y * pngRaster->width * 4);
          pngRaster->data[b + 0] = palette[i / 16].red;
          pngRaster->data[b + 1] = palette[i / 16].green;
          pngRaster->data[b + 2] = palette[i / 16].blue;
          pngRaster->data[b + 3] = 255;
          pngRaster->data[b + 4] = palette[i % 16].red;
          pngRaster->data[b + 5] = palette[i % 16].green;
          pngRaster->data[b + 6] = palette[i % 16].blue;
          pngRaster->data[b + 7] = 255;
        }
        if (bit_depth == 8) { // 1 byte is 1 pixel (colormap of 256 colors)
          size_t b = x * 4 + 0 + (y * pngRaster->width * 4);
          pngRaster->data[b + 0] = palette[i].red;
          pngRaster->data[b + 1] = palette[i].green;
          pngRaster->data[b + 2] = palette[i].blue;
          pngRaster->data[b + 3] = 255;
        }
      }
      /*  Each pixel is an R,G,B triple (2) or Each pixel is an R,G,B triple, followed by an alpha sample. (6)*/
      if (color_type == 2 || color_type == 6) {

        int bpp = 0;
        int bbpo = 1;

        /* Each pixel is an R,G,B triple: 3 bytes per pixel */
        if (color_type == 2) {
          if (bit_depth == 8) {
            bpp = 3;
            bbpo = 1;
          }
          if (bit_depth == 16) {
            bpp = 6;
            bbpo = 2;
          }
        }

        /*  Each pixel is an R,G,B triple, followed by an alpha sample.*/
        if (color_type == 6) {
          if (bit_depth == 8) {
            bpp = 4;
            bbpo = 1;
          }
          if (bit_depth == 16) {
            bpp = 8;
            bbpo = 2;
          }
        }
        /* Red */
        pngRaster->data[x * 4 + 0 + (y * pngRaster->width * 4)] = line[x * bpp + 0];

        /* Green and blue */
        pngRaster->data[x * 4 + 1 + (y * pngRaster->width * 4)] = line[x * bpp + 1 * bbpo];
        pngRaster->data[x * 4 + 2 + (y * pngRaster->width * 4)] = line[x * bpp + 2 * bbpo];

        /* Alpha */
        if (bpp == 4 || bpp == 8) {
          pngRaster->data[x * 4 + 3 + (y * pngRaster->width * 4)] = line[x * bpp + 3 * bbpo];
        } else {
          pngRaster->data[x * 4 + 3 + (y * pngRaster->width * 4)] = 255;
        }
      }
    }
  }

  if (row_pointers) {
    for (size_t y = 0; y < pngRaster->height; y++) {
      free(row_pointers[y]);
    }
    free(row_pointers);
    row_pointers = NULL;
  }
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  fclose(fp);
#ifdef CREADPNG_DEBUG
  CDBDebug("done");
#endif
  pngRaster->hasOnlyHeaders = false;
  return pngRaster;
}
