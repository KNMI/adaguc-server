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

#ifdef ENABLE_CURL

#ifndef CHTTPTOOLS_H
#define CHTTPTOOLS_H

#include "CDebugger.h"
#include "CTypes.h"
class CHTTPTools {

private:
  DEF_ERRORFUNCTION();
  struct MemoryStruct {
    char *memory;
    size_t size;
  };

  static void *myrealloc(void *ptr, size_t size);

  static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data);

public:
  /**
   * getBuffer reads data form an URL into a buffer. The buffer and length arguments are set.
   * @param url The URL to read
   * @param buffer The buffer to use, must be a NULL pointer and should be freed with free
   * @param length The length of the buffer, is set by this function after succesful completion
   * @param maxFileSize The maximum allowed size of file in bytes, refuse to download if larger
   * @return Zero on succes
   */
  int static getBuffer(const char *url, char *&buffer, size_t &length, long maxFileSize = 0);

  CT::string static getString(const char *url, float maxFileSize = 0);
};
#endif
#endif
