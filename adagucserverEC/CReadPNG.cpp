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


const char *CReadPNG::className="CReadPNG";
const char *CReadPNG::CPNGRaster::className="CPNGRaster";

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define PNG_DEBUG 3
#include <png.h>

// #define CREADPNG_DEBUG


// https://aiddata.rvo.nl/projects

CReadPNG::CPNGRaster * CReadPNG::read_png_file(const char* file_name, bool pngReadHeaderOnly) {
  CDBDebug("CReadPNG::open %s", file_name);
  png_byte color_type;
  png_byte bit_depth;
  
  png_structp png_ptr;
  png_infop info_ptr;
//   int number_of_passes;
  png_bytep * row_pointers;
  
  unsigned char header[8];    // 8 is the maximum size that can be checked
  
  /* open file and test for it being a png */
  FILE *fp = fopen(file_name, "rb");
  if (!fp) {
    CDBError("[read_png_file] File %s could not be opened for reading", file_name);
    return NULL;
  }
  fread(header, 1, 8, fp);
  if (png_sig_cmp(header, 0, 8)) {
    CDBError("[read_png_file] File %s is not recognized as a PNG file", file_name);
    return NULL;
  }
  
  /* initialize stuff */
  png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  
  if (!png_ptr) {
    CDBError("[read_png_file] png_create_read_struct failed");
    return NULL;
  }
  
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    CDBError("[read_png_file] png_create_info_struct failed");
    return NULL;
  }
  
  if (setjmp(png_jmpbuf(png_ptr))) {
    CDBError("[read_png_file] Error during init_io");
    return NULL;
  }
  
  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);
  
  png_read_info(png_ptr, info_ptr);
  
  CReadPNG::CPNGRaster *pngRaster = new CPNGRaster();
  
  pngRaster->width = png_get_image_width(png_ptr, info_ptr);
  pngRaster->height = png_get_image_height(png_ptr, info_ptr);
  CDBDebug("CReadPNG::open PNG of size [%dx%d]", pngRaster->width, pngRaster->height );
  color_type = png_get_color_type(png_ptr, info_ptr);
  bit_depth = png_get_bit_depth(png_ptr, info_ptr);
  
//  number_of_passes = png_set_interlace_handling(png_ptr);
  png_read_update_info(png_ptr, info_ptr);
  
  if(pngReadHeaderOnly == true){
    return pngRaster;
  }
  
  #ifdef CREADPNG_DEBUG
  CDBDebug("read png data");
  #endif
  
  /* read file */
  if (setjmp(png_jmpbuf(png_ptr))) {
    CDBError("[read_png_file] Error during read_image");
    return NULL;
  }
  
  
  row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * pngRaster->height);
  
  for (size_t y=0; y<pngRaster->height; y++)
    row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));
#ifdef CREADPNG_DEBUG
  CDBDebug("start png_read_image");
#endif
  png_read_image(png_ptr, row_pointers);\

#ifdef CREADPNG_DEBUG
  CDBDebug("/done png_read_image");  
  CDBDebug("PNG data: [%d, %d, %d, %d]", pngRaster->width, pngRaster->height, (int)bit_depth, color_type);
#endif  
  int bpp = 4;
  int pngwidthdivisor = 1;
  if(bit_depth==4){

    pngwidthdivisor=2;
  }
  if(bit_depth==1){
    
    pngwidthdivisor=8;
  }
  if(color_type==2){
     bpp = 3;
  }
  int num_palette;
  png_colorp palette;
  if(color_type==3){
    bpp = 1;
    png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette);
  }
  
  if (pngRaster->data !=NULL) {
    CDBDebug("pngRaster->data already defined, return");
    return pngRaster;
  }

  
  pngRaster->data = new unsigned char[pngRaster->width*pngRaster->height*4];
  for (size_t y=0; y<pngRaster->height; y++){
    unsigned char *line = row_pointers[y];
    for(size_t x=0;x<pngRaster->width/pngwidthdivisor;x+=1){
      
      
      if(bpp==1){
        unsigned char i  = line[x*bpp+0];
        if(bit_depth==1){
          int d=1;
          for(int j=0;j<8;j++){
            pngRaster->data[x*32 + j*4 + 0 +(y*pngRaster->width*4)]= palette[(i&d)/d].red; 
            pngRaster->data[x*32 + j*4 + 1 +(y*pngRaster->width*4)]= palette[(i&d)/d].green; 
            pngRaster->data[x*32 + j*4 + 2 +(y*pngRaster->width*4)]= palette[(i&d)/d].blue; 
            pngRaster->data[x*32 + j*4 + 3 +(y*pngRaster->width*4)]= 255;
            d=d+d;
          }
        }
        if(bit_depth==4){
          pngRaster->data[x*8 + 0 +(y*pngRaster->width*4)]= palette[i/16].red; 
          pngRaster->data[x*8 + 1 +(y*pngRaster->width*4)]= palette[i/16].green; 
          pngRaster->data[x*8 + 2 +(y*pngRaster->width*4)]= palette[i/16].blue; 
          pngRaster->data[x*8 + 3 +(y*pngRaster->width*4)]= 255;
          pngRaster->data[x*8 + 4 +(y*pngRaster->width*4)]= palette[i%16].red; 
          pngRaster->data[x*8 + 5 +(y*pngRaster->width*4)]= palette[i%16].green; 
          pngRaster->data[x*8 + 6 +(y*pngRaster->width*4)]= palette[i%16].blue; 
          pngRaster->data[x*8 + 7 +(y*pngRaster->width*4)]= 255;
        }
        if(bit_depth==8){
          pngRaster->data[x*4 + 0 +(y*pngRaster->width*4)]= palette[i].red; 
          pngRaster->data[x*4 + 1 +(y*pngRaster->width*4)]= palette[i].green; 
          pngRaster->data[x*4 + 2 +(y*pngRaster->width*4)]= palette[i].blue; 
          pngRaster->data[x*4 + 3 +(y*pngRaster->width*4)]= 255;
        }
      }else{
        pngRaster->data[x*4 + 0 +(y*pngRaster->width*4)]= line[x*bpp+0]; 
        if(bpp==3 || bpp ==4){
          pngRaster->data[x*4 + 1 +(y*pngRaster->width*4)]= line[x*bpp+1];
          pngRaster->data[x*4 + 2 +(y*pngRaster->width*4)]= line[x*bpp+2];
        }
        if(bpp==4){
          pngRaster->data[x*4 + 3 +(y*pngRaster->width*4)]= line[x*4+3];
        }else{
          pngRaster->data[x*4 + 3 +(y*pngRaster->width*4)]= 255;
        }
      }

    }
  }
  
  fclose(fp);
#ifdef CREADPNG_DEBUG  
  CDBDebug("done");
#endif  
  pngRaster->hasOnlyHeaders = false;
  return pngRaster;
}

