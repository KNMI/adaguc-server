#ifdef ENABLE_CURL
#ifndef CMYCURL_H
#define CMYCURL_H
#include "Definitions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#include <curl/curl.h>
//#include <curl/types.h>
//#include <curl/easy.h>

#include <gd.h>

class MyCURL{
  public:
  class ImageField{
    public:
      int w,h;
      unsigned char *rgba;
  };
  private:
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
  
  int static get(const char * url,char * &buffer, size_t &length){
    if(buffer!=NULL)return 1;
    CURL *curl_handle;
  
    struct MemoryStruct chunk;
  
    chunk.memory=NULL; /* we expect realloc(NULL, size) to work */ 
    chunk.size = 0;    /* no data at this point */ 
  
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
    buffer = new char[chunk.size];
    memcpy(buffer,chunk.memory,chunk.size);
    
    if(chunk.memory)
      free(chunk.memory);
  
    /* we're done with libcurl, so clean it up */ 
    curl_global_cleanup();
  
    return 0;
  }
  
  int static getGDImageField(const char * url, gdImagePtr &im){

    CURL *curl_handle;
  
    struct MemoryStruct chunk;
  
    chunk.memory=NULL; /* we expect realloc(NULL, size) to work */ 
    chunk.size = 0;    /* no data at this point */ 
  
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
    if(chunk.size>4){
      if(chunk.memory[0]=='G')
      im = gdImageCreateFromGifPtr(chunk.size,chunk.memory);
      else
      im = gdImageCreateFromPngPtr(chunk.size,chunk.memory);
    }

    if(chunk.memory)
      free(chunk.memory);
  
    /* we're done with libcurl, so clean it up */ 
    curl_global_cleanup();
  
    return 0;
  }

};
#endif
#endif
