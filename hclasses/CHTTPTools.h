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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "CTypes.h"

class CHTTPTools{
  
  private:
  DEF_ERRORFUNCTION();  
  struct MemoryStruct {
    char *memory;
    size_t size;
  };
  
  
  static void *myrealloc(void *ptr, size_t size){
    /* There might be a realloc() out there that doesn't like reallocing
    NULL pointers, so we take care of it here */ 
    if(ptr)
      return realloc(ptr, size);
    else
      return malloc(size);
  }
  
  static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data){
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;
  
    mem->memory = (char*)myrealloc(((void*)mem->memory), mem->size + realsize + 1);
    if (mem->memory) {
      memcpy(&(mem->memory[mem->size]), ptr, realsize);
      mem->size += realsize;
      mem->memory[mem->size] = 0;
    }
    return realsize;
  }
  public:
  
  /**
   * getBuffer reads data form an URL into a buffer. The buffer and length arguments are set.
   * @param url The URL to read
   * @param buffer The buffer to use, must be a NULL pointer and should be freed with free
   * @param length The length of the buffer, is set by this function after succesful completion
   * @param maxFileSize The maximum allowed size of file in bytes, refuse to download if larger
   * @return Zero on succes
   */
  int static getBuffer(const char * url,char * &buffer, size_t &length, long maxFileSize = 0){
    CDBDebug("Getting [%s]",url);
    if(buffer!=NULL){
      CDBError("curl buffer is not empty");
      return 1;
    }
    CURL *curl_handle;
  
    struct MemoryStruct chunk;
  
    chunk.memory=NULL; /* we expect realloc(NULL, size) to work */ 
    chunk.size = 0;    /* no data at this point */ 
  
    if(curl_global_init(CURL_GLOBAL_ALL)!=0){
      CDBError("curl_global_init failed");
    }
  
    /* init the curl session */ 
    curl_handle = curl_easy_init();

    if(curl_handle == NULL){
      CDBError("curl_easy_init failed");
    }
  
    /* specify URL to get */ 
    if(curl_easy_setopt(curl_handle, CURLOPT_URL, url)!=0){
      CDBError("curl_easy_setopt failed CURLOPT_URL");
    }

    /* the maximum allowed size of file in bytes, refuse to download if larger */
    if(maxFileSize > 0){
      if(curl_easy_setopt(curl_handle, CURLOPT_MAXFILESIZE, maxFileSize) != 0){
        CDBError("curl_easy_setopt failed CURLOPT_MAXFILESIZE");
      }
    }
  
    /* send all data to this function  */ 
    if( curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback)!=0){
      CDBError("curl_easy_setopt failed CURLOPT_WRITEFUNCTION");
    }
  
    /* we pass our 'chunk' struct to the callback function */ 
    if(curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk)!=0){
      CDBError("curl_easy_setopt failed CURLOPT_WRITEDATA");
    }
  
    /* some servers don't like requests that are made without a user-agent
    field, so we provide one */ 
    //if(curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0")!=0){
    if(curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Fedora; Linux x86_64; rv:51.0) Gecko/20100101 Firefox/51.0")!=0){
      CDBError("curl_easy_setopt failed CURLOPT_USERAGENT");
    }

    /* get it! */
    CURLcode response = curl_easy_perform(curl_handle);

    if(response !=0){
      CDBError("curl_easy_perform failed with error: %s", curl_easy_strerror(response));
    }
  
    /* cleanup curl stuff */ 
    curl_easy_cleanup(curl_handle);
    
    length = chunk.size;
    
    if(length==0){
      CDBError("CURL chunk.size == 0");
      return 2;
    }
    
    buffer=chunk.memory;
  
    /* we're done with libcurl, so clean it up */ 
    curl_global_cleanup();
    
    return 0;
  }
  
  CT::string static getString(const char * url, float maxFileSize = 0){
    char *buffer = NULL;
    size_t length = 0;
    int status = getBuffer(url,buffer,length, maxFileSize);
    if(status!=0)throw 1;
    if(length==0)throw 2;
    return CT::string(buffer,length);
  }

};
#endif
#endif

