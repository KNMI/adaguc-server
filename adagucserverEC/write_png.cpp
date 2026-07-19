#include <png.h>
#include <errno.h>
#include <string.h>
#include <cstdint>
#include "CDebugger.h"
#include "COctTreeColorQuantizer.h"
#include "write_png.h"
#include "CStopWatch.h"

// #define MEASURETIME

void prepare24Bpp(png_structp png_ptr, png_infop info_ptr, int width, int height) {
  png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_byte a[1];
  png_color_16 trans_values[1];
  a[0] = 0;
  trans_values[0].index = 0;
  trans_values[0].red = 0;
  trans_values[0].green = 0;
  trans_values[0].blue = 0;
  png_set_tRNS(png_ptr, info_ptr, a, 1, trans_values);
}

void prepare8Bpp(OctreeType **outTree, unsigned char *ARGBByteBuffer, png_structp png_ptr, png_infop info_ptr, int width, int height, bool use8bitpalAlpha) {
  png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
  png_color palette[256];
  png_byte a[256];
  png_color_16 trans_values[256];
#ifdef MEASURETIME
  StopWatch_Stop("Creating octtree for color quantization");
#endif

  const uint32_t *src32 = reinterpret_cast<const uint32_t *>(ARGBByteBuffer);
  const int totalPixels = width * height;
  OctreeType *tree = NULL;

  RGBType color;
  bool insertedVisiblePixel = false;
  bool has_active_run = false;
  uint32_t last_pixel = 0;
  uint run_length = 0;

  auto insertGroupedPixels = [&](uint32_t pixel, uint count) {
    uint8_t alpha = (pixel >> 24) & 0xFF;
    if (!use8bitpalAlpha && alpha <= 64) return;

    insertedVisiblePixel = true;
    color.r = (pixel >> 16) & 0xFF;
    color.g = (pixel >> 8) & 0xFF;
    color.realblue = pixel & 0xFF;
    color.realalpha = alpha;
    // if use8bitpalAlpha, store top 3 bits of alpha + bottom 5 bits of blue
    color.b = use8bitpalAlpha ? ((color.realblue >> 3) + ((alpha >> 5) << 5)) : color.realblue;

    InsertTreeCount(&tree, &color, -1, count);
  };

  // Instead of inserting individual pixels into the octree, group identical sequential pixels together and add them once.
  // This keeps palette generation identical, while reducing octree traversal.
  for (int j = 0; j < totalPixels; j++) {
    uint32_t current_pixel = src32[j];
    uint8_t alpha = (current_pixel >> 24) & 0xFF;

    // Encountered transparant pixel, end current run
    if (!use8bitpalAlpha && alpha <= 64) {
      if (has_active_run) {
        insertGroupedPixels(last_pixel, run_length);
        has_active_run = false;
      }
      continue;
    }

    // This pixel isn't grouped together yet, start new run
    if (!has_active_run) {
      last_pixel = current_pixel;
      run_length = 1;
      has_active_run = true;
      continue;
    }

    // Current pixel is same as the last pixel, extend run
    if (current_pixel == last_pixel) {
      run_length++;
      continue;
    }

    // Encountered a different pixel color, insert last run
    insertGroupedPixels(last_pixel, run_length);

    last_pixel = current_pixel;
    run_length = 1;
  }

  // Store the last pixels
  if (has_active_run) {
    insertGroupedPixels(last_pixel, run_length);
  }

  if (!insertedVisiblePixel) {
    color.r = 0;
    color.g = 0;
    color.b = 0;
    color.realblue = 0;
    color.realalpha = 0;

    InsertTreeCount(&tree, &color, -1, 1);
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

  // Set PNG palette
  int numColors = 0;
  RGBType table[256];
  MakePaletteTable(tree, table, &numColors);

  palette[0].red = 0;
  palette[0].green = 0;
  palette[0].blue = 0;

  if (use8bitpalAlpha) {
    if (numColors > 255) numColors = 255;
    int numAlphaColors = 0;
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
    if (numColors > 254) numColors = 254;
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

  *outTree = tree;
}

void write24BppPayload(png_structp png_ptr, int width, int height, unsigned char *ARGBByteBuffer) {
  int i;
  png_bytep row_ptr = 0;
  int s = width * 4;
  unsigned char *RGBRow = new unsigned char[s];
  for (i = 0; i < height; i = i + 1) {

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
  delete[] RGBRow;
}

void write8BppPayload(png_structp png_ptr, OctreeType *tree, int width, int height, unsigned char *ARGBByteBuffer, bool use8bitpalAlpha) {
  struct ColorCacheEntry {
    uint32_t key;
    unsigned char index;
    bool valid;
  };

  // Small cache to avoid octree lookups
  ColorCacheEntry color_lookup[4096] = {};
  const int stride = width * 4;

  unsigned char *imageBuffer = new unsigned char[width * height];
  png_bytep *row_ptrs = new png_bytep[height];

  for (int y = 0; y < height; y++) {
    row_ptrs[y] = imageBuffer + (y * width);
  }

  for (int y = 0; y < height; y++) {
    unsigned char *dst = row_ptrs[y];
    unsigned char *src = ARGBByteBuffer + (y * stride);

    for (int x = 0; x < width; x++, src += 4) {
      uint8_t b = src[0];
      uint8_t g = src[1];
      uint8_t r = src[2];
      uint8_t a = src[3];

      if (!use8bitpalAlpha && a <= 64) {
        *dst++ = 0;
        continue;
      }

      uint8_t qb = use8bitpalAlpha ? ((b >> 3) + ((a >> 5) << 5)) : b;
      uint32_t key = (uint32_t(r) << 16) | (uint32_t(g) << 8) | uint32_t(qb);

      // Most images contain many repeated colors, making octree traversal not needed.
      auto &entry = color_lookup[key & 4095];
      if (entry.valid && entry.key == key) {
        *dst++ = entry.index;
        continue;
      }

      RGBType color;
      color.r = r;
      color.g = g;
      color.b = qb;

      // Traverse octree until the leaf containing this color is reached
      unsigned char idx = FindLeaf(tree, color)->index;

      // Index 0 is reserved for transparency in non-alpha-palette mode
      if (!use8bitpalAlpha) idx += 1;

      entry.valid = true;
      entry.key = key;
      entry.index = idx;

      *dst++ = idx;
    }
  }

#ifdef MEASURETIME
  StopWatch_Stop("Before png_write_image");
#endif
  png_write_image(png_ptr, row_ptrs);

  delete[] row_ptrs;
  delete[] imageBuffer;
}

int writePng(int width, int height, unsigned char *ARGBByteBuffer, FILE *file, int bitDepth, bool use8bitpalAlpha) {
  OctreeType *tree = NULL;

#ifdef MEASURETIME
  StopWatch_Stop("Start writePNG");
#endif

  png_structp png_ptr;
  png_infop info_ptr;
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  // png_set_compression_level(png_ptr, 1); // TODO
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
  if (bitDepth == 24) {
    prepare24Bpp(png_ptr, info_ptr, width, height);
  } else if (bitDepth == 8) {
    prepare8Bpp(&tree, ARGBByteBuffer, png_ptr, info_ptr, width, height, use8bitpalAlpha);
  }

  png_set_filter(png_ptr, 0, PNG_FILTER_NONE);
  png_write_info(png_ptr, info_ptr);

#ifdef MEASURETIME
  StopWatch_Stop("Headers written");
#endif

  if (bitDepth == 24) {
    write24BppPayload(png_ptr, width, height, ARGBByteBuffer);
  } else if (bitDepth == 8) {
    write8BppPayload(png_ptr, tree, width, height, ARGBByteBuffer, use8bitpalAlpha);
  }
#ifdef MEASURETIME
  StopWatch_Stop("PNG image written");
#endif

  png_write_end(png_ptr, NULL);
  png_destroy_write_struct(&png_ptr, &info_ptr);

#ifdef MEASURETIME
  StopWatch_Stop("End writePNG");
#endif
  return 0;
}