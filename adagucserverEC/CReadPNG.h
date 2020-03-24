/******************************************************************************
 * 
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
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


#ifndef CREADPNG_H
#define CREADPNG_H
#include "CDebugger.h"

// #define CREADPNG_DEBUG
class CReadPNG  {
  private: 
    DEF_ERRORFUNCTION();
  
  public:
    class CPNGRaster {
      private: 
        DEF_ERRORFUNCTION();
      public:
      CPNGRaster(){
        data = NULL;
        hasOnlyHeaders = true;
      }
      ~CPNGRaster() {
        CDBDebug("~CPNGRaster");
        if(data!=NULL){
          delete[] data;
          data = NULL;
        }
      }
      bool hasOnlyHeaders;
      unsigned char * data;
      size_t width, height;
    };
    
    static CPNGRaster* read_png_file(const char* file_name, bool readHeaderOnly);
};

#endif
