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
#ifndef CMYCURL_H
#define CMYCURL_H
// #include "Definitions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

class MyCURL {
public:
  class ImageField {
  public:
    int w, h;
    unsigned char *rgba;
  };

private:
  struct MemoryStruct {
    char *memory;
    size_t size;
  };

  static void *myrealloc(void *ptr, size_t size) {
    /* There might be a realloc() out there that doesn't like reallocing
    NULL pointers, so we take care of it here */
    if (ptr)
      return realloc(ptr, size);
    else
      return malloc(size);
  }

  static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;

    mem->memory = (char *)myrealloc(((void *)mem->memory), mem->size + realsize + 1);
    if (mem->memory) {
      memcpy(&(mem->memory[mem->size]), ptr, realsize);
      mem->size += realsize;
      mem->memory[mem->size] = 0;
    }
    return realsize;
  }

public:
  int static getbuffer(const char *url, char *&buffer, size_t &length) {
    if (buffer != NULL) return 1;
    CURL *curl_handle;

    struct MemoryStruct chunk;

    chunk.memory = NULL; /* we expect realloc(NULL, size) to work */
    chunk.size = 0;      /* no data at this point */

    curl_global_init(CURL_GLOBAL_ALL);

    /* init the curl session */
    curl_handle = curl_easy_init();

    /* specify URL to get */
    curl_easy_setopt(curl_handle, CURLOPT_URL, url);

    /* send all data to this function  */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

    /* some servers don't like requests that are made without a user-agent
    field, so we provide one */
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

    /* get it! */
    curl_easy_perform(curl_handle);

    /* cleanup curl stuff */
    curl_easy_cleanup(curl_handle);

    length = chunk.size;

    if (length == 0) {
      return 2;
    }
    // buffer = new char[chunk.size];
    // memcpy(buffer,chunk.memory,chunk.size);

    buffer = chunk.memory;
    // if(chunk.memory)
    //      free(chunk.memory);

    /* we're done with libcurl, so clean it up */
    curl_global_cleanup();

    return 0;
  }
};
#endif
#endif
