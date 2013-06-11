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

#ifndef ADAGUC_USE_CAIRO
#include "CDrawAA.h"
DEF_ERRORMAIN();
const char *CFreeType::className="CFreeType";
const char *CXiaolinWuLine::className="CXiaolinWuLine";
int writeRGBAPng(int width,int height,unsigned char *RGBAByteBuffer,FILE *file,bool trueColor){
  
  
#ifdef MEASURETIME
  StopWatch_Stop("start writeRGBAPng.");
#endif  

  png_structp png_ptr;
  png_infop info_ptr;
  png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  
#ifdef MEASURETIME
  StopWatch_Stop("LINE %d",__LINE__);
#endif  
  if (!png_ptr){CDBError("png_create_write_struct failed");return 1;}
  
  info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr){
    png_destroy_write_struct(&png_ptr, NULL);
    CDBError("png_create_info_struct failed");
    return 1;
  }

  if (setjmp(png_jmpbuf(png_ptr))){
    CDBError("Error during init_io");
    return 1;
  }

  png_init_io(png_ptr, file);

  /* write header */
  if (setjmp(png_jmpbuf(png_ptr))){
    CDBError("Error during writing header");
    return 1;
  }
  /*png_set_IHDR(png_ptr, info_ptr, width, height, 8, 
               PNG_COLOR_TYPE_RGB_ALPHA,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
  PNG_FILTER_TYPE_DEFAULT);*/
  if(trueColor){
  /*png_set_IHDR(png_ptr, info_ptr, width, height, 8, 
               PNG_COLOR_TYPE_RGB_ALPHA,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
    PNG_FILTER_TYPE_BASE);*/
  png_set_IHDR(png_ptr, info_ptr, width, height, 8, 
               PNG_COLOR_TYPE_RGB_ALPHA,
               PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_BASE,
               PNG_FILTER_TYPE_BASE);
  
  }else{
    png_set_IHDR(png_ptr, info_ptr, width, height,
                 8, PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_NONE);
  }
  
  
  png_set_filter (png_ptr, 0, PNG_FILTER_NONE);
  png_set_compression_level (png_ptr, -1);
  
  //png_set_invert_alpha(png_ptr);
  png_write_info(png_ptr, info_ptr);
  
  /* write bytes */
  if (setjmp(png_jmpbuf(png_ptr))){
    CDBError("Error during writing bytes");
    return 1;
  }
  png_set_packing(png_ptr);
#ifdef MEASURETIME
  StopWatch_Stop("LINE %d",__LINE__);
#endif  

  int i;
  png_bytep row_ptr = 0;
  if(trueColor){
    for (i = 0; i < height; i=i+1){
      row_ptr = RGBAByteBuffer + 4 * i * width;
      png_write_rows(png_ptr, &row_ptr, 1);
    }
  }else{
    for (i = 0; i < height; i++){
      row_ptr = RGBAByteBuffer + 1 * i * width;
      png_write_rows(png_ptr, &row_ptr, 1);
    }
  }
  
#ifdef MEASURETIME
  StopWatch_Stop("LINE %d",__LINE__);
#endif  

  /* end write */
  if (setjmp(png_jmpbuf(png_ptr))){
    CDBError("Error during end of write");
    return 1;
  }

  png_write_end(png_ptr, NULL);
  
#ifdef MEASURETIME
  StopWatch_Stop("end writeRGBAPng.");
#endif  
  return 0;
}
#endif
