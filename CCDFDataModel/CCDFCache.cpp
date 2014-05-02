/******************************************************************************
 * 
 * Project:  Generic common data format
 * Purpose:  Generic Data model to read netcdf and hdf5
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

#include "CCDFCache.h"
#include "CCDFObject.h"
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"
#include "CCDFStore.h"
const char *CDFCache::className="CDFCache";


//#define CCDFCACHE_DEBUG
//#define CCDFCACHE_DEBUG_LOW

#ifdef CCDFCACHE_DEBUG
#define CCDFCACHE_DEBUG_LOW
#endif



CCache * CDFCache::getCCache(const char * directory, const char *fileName){
  if(cache == NULL){
    cache = new CCache();
  }
  return cache;
}



int CDFCache::readBinaryData(const char * filename,void **data, CDFType type, size_t &varSize){
#ifdef CCDFCACHE_DEBUG_LOW
  CDBDebug("OPEN BINARY CACHE : {%s}",filename);
#endif
  size_t fileSize;
  size_t bytesRead;

  
  FILE * pFile = fopen ( filename , "rb" );
  if (pFile==NULL) {CDBError ("Unable to open %s",filename); return 1;}

  // obtain file size:
  fseek (pFile , 0 , SEEK_END);
  fileSize = ftell (pFile);
  rewind (pFile);

  // copy the file into the buffer:
  bytesRead = fread (&varSize,sizeof(size_t),1,pFile);
 
  if(bytesRead != 1){
    CDBError ("Unable to read file %s",filename);
    return 3;
  }
  CDF::allocateData(type,data,varSize);
  if(type!= CDF_STRING){
    
    size_t dataSize = fileSize-sizeof(size_t);
    bytesRead = fread (*data,1,dataSize,pFile);
    if (bytesRead != dataSize) {
      CDBError ("Unable to read file %s",filename);
      return 4;      
    }
  }else{
    for(size_t j=0;j<varSize;j++){
     
      size_t stringLength = 0;
      fread (&stringLength,sizeof(size_t),1,pFile);
      ((char**)(*data))[j] = (char*)malloc(stringLength+1);
      fread (((char**)(*data))[j],1,stringLength,pFile);
      (((char**)(*data))[j])[stringLength]=0;
      
      //CDBDebug("%d = %s",j,(((char**)(*data))[j]));
      
    }
    
  }

  fclose (pFile);
  
  return 0;
}


int CDFCache::writeBinaryData(const char * filename,void **data, CDFType type, size_t varSize){
#ifdef CCDFCACHE_DEBUG_LOW
  CDBDebug("WRITING BINARY CACHE: {%s} of size %d",filename,varSize);
#endif  
  
    FILE *pFile = fopen ( filename , "wb" );
    if(pFile==NULL){
      CDBError("Unable to open cachefile %s",filename);
      return 1;
    }
    size_t bytesWritten = fwrite (&varSize ,sizeof(size_t),1 , pFile );
    
    if(type!= CDF_STRING){
      bytesWritten += fwrite (*data , CDF::getTypeSize(type),varSize , pFile );
  
      if(bytesWritten!=(varSize+1)){
        CDBError("Unable to write to cachefile %s",filename);
        return 2;
      }
    }else{
      CT::string newData ="";
      //const char newLine = '\n';
      for(size_t j=0;j<varSize;j++){
        const char *string = ((const char**)(*data))[j];
        size_t stringLength = strlen(string);
        fwrite (&stringLength ,sizeof(size_t),1 , pFile );
        fwrite (string ,sizeof(char),stringLength , pFile );
      }
    
    }
    fflush (pFile);   
    fclose (pFile);
  
    
  
  return 0;
}


int CDFCache::open(const char *fileName,CDFObject *cdfObject,bool readOrWrite){
  CT::string key = "header_";
  key.concat(fileName);
  CCache * cache = getCCache(cacheDir.c_str(),key.c_str());
 
  //Try to read from cache
  if(readOrWrite == false){
    cache->checkCacheSystemReady(cacheDir.c_str(),key.c_str());
    bool cacheIsAvailable = cache->cacheIsAvailable();
    if(cacheIsAvailable){
      CT::string cacheFilename = cache->getCacheFileNameToRead();
      CDFReader *orgCDFReader = (CDFReader*)cdfObject->getCDFReader();
      if(orgCDFReader->cdfCache == NULL){
        CDBError("NC_OPEN ERR [%s]",fileName);
        return 1;
      }
      CDFReader *cacheReader = CDFStore::getCDFReader(cacheFilename.c_str());
      CDFObject *cacheReadCDFObject = cdfObject;
      cacheReadCDFObject->attachCDFReader(cacheReader);
      #ifdef CCDFCACHE_DEBUG_LOW
      CDBDebug("OPEN HEADER CACHE %s",cacheFilename.c_str());
      #endif
      int status = cacheReadCDFObject->open(cacheFilename.c_str());
      if(status == 0){
        for(size_t j=0;j<cacheReadCDFObject->variables.size();j++){
          CDF::Variable  *ov = cacheReadCDFObject->variables[j];
          ov->setCDFReaderPointer(orgCDFReader);
        }
        cdfObject->attachCDFReader(orgCDFReader);
      }
      
      if(status != 0){
        #ifdef CCDFCACHE_DEBUG
        CDBDebug("Unable to open %s",cacheFilename.c_str());
        #endif
        return 1; 
      }
   
      #ifdef CCDFCACHE_DEBUG
      CDBDebug("Finished reading cache");
      #endif
      return 0;
    }else{
      //We will write the cache.
      bool saveCacheFile = cache->saveCacheFile();
      if(saveCacheFile){
        cache->claimCacheFile();
      }
      return 1;
    }
  }
  
  
  if(readOrWrite == true){
    
    bool saveCacheFile = cache->saveCacheFile();
    if(saveCacheFile == true){
      CT::string cacheFilename = cache->getCacheFileNameToWrite();
      #ifdef CCDFCACHE_DEBUG
      CDBDebug("WRITING HEADER CACHE : {%s}",cacheFilename.c_str());
      #endif
      CT::string directory = cacheFilename.substring(0,cacheFilename.lastIndexOf("/"));
      //CDirReader::makePublicDirectory(directory.c_str());
      CDFNetCDFWriter *writer = new CDFNetCDFWriter(cdfObject);
      writer->setNetCDFMode(4);
      writer->disableVariableWrite();
      writer->write(cacheFilename.c_str());
      delete writer;
      cache->releaseCacheFile();
    }
    return 0;
  }
  
  return 1;
}

int CDFCache::readVariableData(CDF::Variable *var, CDFType type,size_t *start,size_t *count,ptrdiff_t *stride,bool readOrWrite){
  
  CT::string key = "variables";
  
  bool hasFileName = false;
  CDFObject * cdfObject = (CDFObject*)var->getCDFObjectPointer(start,count);
  
  if(cdfObject != NULL){
    CDFReader * cdfReader = (CDFReader*)cdfObject->getCDFReader();
    if(cdfReader != NULL){

      
      if(cdfReader->fileName.length()>0){
#ifdef CCDFCACHE_DEBUG
        CDBDebug("readVariableData %s %s",var->name.c_str(),cdfReader->fileName.c_str());
#endif
        key.concat(cdfReader->fileName.c_str());
        key.concat("_");
        hasFileName = true;
      }
    }
  }
  if(hasFileName == false){
    key.concat("nofile");
    key.concat("_");
  }
  
  key.concat(var->name.c_str());
  key.concat("_");
  key.concat(CDF::getCDFDataTypeName(type));
  key.concat("_");
  //CDBDebug("Start count stride");
  for(size_t j=0;j<var->dimensionlinks.size();j++){
    //CDBDebug("Dim %d",j);
    if(start == NULL && count == NULL && stride == NULL){
      key.printconcat("[-:-:-]");
    }else{
      size_t sta = 0;if(start  != NULL)sta = start[j];
      size_t cnt = 0;if(count  != NULL)cnt = count[j];
      size_t str = 0;if(stride != NULL)str = stride[j];
      key.printconcat("[%d:%d:%d]",sta,cnt,str);
    }
    
    
    
  }
  key.concat(".bin");
  
  CCache * cache = getCCache(cacheDir.c_str(),key.c_str());
  
  if(readOrWrite == false){
    //Read dataobject
    cache->checkCacheSystemReady(cacheDir.c_str(),key.c_str());
    bool cacheIsAvailable = cache->cacheIsAvailable();  
    if(cacheIsAvailable){
      CT::string cacheFilename = cache->getCacheFileNameToRead();
      size_t varSize = 0;
      int status  = readBinaryData(cacheFilename.c_str(),&var->data,type,varSize);
      if(status != 0){
        return 1;
      }
      var->setSize(varSize);
      var->currentType = type;
      return 0;
    }else{
      //We will write the cache.
      bool saveCacheFile = cache->saveCacheFile();
      if(saveCacheFile){
        cache->claimCacheFile();
      }
      return 1;
    }
  }
  
  if(readOrWrite == true){
    bool saveCacheFile = cache->saveCacheFile();
    //CDBDebug("saveCacheFile = %d",saveCacheFile);;  
    try{
      if(saveCacheFile == true){
        size_t varSize = var->getSize();
        CT::string cacheFilename = cache->getCacheFileNameToWrite();
        CT::string directory = cacheFilename.substring(0,cacheFilename.lastIndexOf("/"));
        //CDirReader::makePublicDirectory(directory.c_str());
        //Write dataobject
        
        int status = writeBinaryData(cacheFilename.c_str(),&var->data,type,varSize);
        if(status!=0)throw(status);
      }
    }catch(int e){
      cache->releaseCacheFile();
      return e;
    }
    cache->releaseCacheFile();
  }
  
  return 0;
};
