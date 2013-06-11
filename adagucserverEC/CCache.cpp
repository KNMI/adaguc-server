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

#include "CCache.h"

const char *CCache::className="CCache";

CCache::CCache(){
  saveFieldFile = false;
  cacheAvailable = false;
  cacheFileClaimed = false;
}

CCache::~CCache(){
  if(fileName.length()!=0){
    if(isCacheFileBusy()){
      if(cacheFileClaimed){
        CDBDebug("!!!!! CACHE FILE NOT RELEASED !!!!");
        removeClaimedCachefile();
      }
    }
  }
}

bool CCache::saveCacheFile(){
#ifdef CCACHE_DEBUG
  if(saveFieldFile){
    CDBDebug("Cache not available from %s",this->fileName.c_str());
  }
#endif
  return saveFieldFile;
}
bool CCache::cacheIsAvailable(){
#ifdef CCACHE_DEBUG
  if(cacheAvailable){
    CDBDebug("Cache available from %s",this->fileName.c_str());
  }
#endif
  return cacheAvailable;
}

void CCache::checkCacheSystemReady(const char *szfileName){
  if(szfileName == NULL)return;
  this->fileName = szfileName;
  claimedCacheFileName = szfileName;
  claimedCacheFileName.concat("_tmp");
    //Check if cache is busy
  if(isCacheFileBusyBlocking()){
    saveFieldFile = false;
    cacheAvailable=false;
  }else{
    //Cache is not busy: Check if the cachefile is available
    if(CCache::isCacheFileAvailable(szfileName)){
      //Cache is available
      cacheAvailable = true;
    }else{
      //Cache is not available, but can be created
      saveFieldFile = true;
    }
  }
}

bool CCache::isCacheFileBusy(){
  if(claimedCacheFileName.length()==0)return false;
  FILE *pFile = fopen ( claimedCacheFileName.c_str() , "r" );
  if(pFile != NULL){
    fclose (pFile);
    #ifdef CCACHE_DEBUG
    CDBDebug("Cache busy \"%s\"",claimedCacheFileName.c_str());
    #endif
    return true;
  }
  #ifdef CCACHE_DEBUG
  CDBDebug("Cache not busy \"%s\"",claimedCacheFileName.c_str());
  #endif
  return false;
}

bool CCache::isCacheFileBusyBlocking(){
  if(fileName.length()==0)return false;
  int maxTries = 60;//Wait 60 seconds.
  int currentTries=maxTries;
  
  //Is some kind of process working on any of the cache files?
  do{
    if(isCacheFileBusy()){
      if(currentTries<=0){
        CDBDebug("!!! A process is working already 60 seconds on %s, skipping wait",fileName.c_str());
        return true;
      }else{
        CDBDebug("Another process is working on %s, waiting... %d",fileName.c_str(),maxTries-currentTries+1);
        //Wait 1 second and try again
        usleep(1000000);
      }
    }else return false;
    currentTries--;
  }while(currentTries>0);
  return false;        
}


bool CCache::isCacheFileAvailable(const char *fileName){
  FILE *pFile = fopen ( fileName , "r" );
  if(pFile != NULL){
    fclose (pFile);
    return true;
  }
  return false;
}


int CCache::releaseCacheFile(){
  CT::string f = fileName.c_str();f.concat("_tmp");
  if(chmod(f.c_str(),0777)<0){
    return 1;
  }
  rename(f.c_str(),fileName.c_str());
  #ifdef CCACHE_DEBUG
  CDBDebug("Claimed cachefile released");
  #endif
  return 0;
}

const char *CCache::getCacheFileNameToWrite(){
  return claimedCacheFileName.c_str();
}

int CCache::claimCacheFile (){
  #ifdef CCACHE_DEBUG
  CDBDebug("Start claiming cache");
  #endif
  saveFieldFile = false; 
  if(isCacheFileAvailable(fileName.c_str())){
    CDBDebug("claimCacheFile: Cache is already available for %s", fileName.c_str());
    return 4;
  }
  if(isCacheFileBusy()){
    CDBDebug("claimCacheFile: Cache is already working on %s", claimedCacheFileName.c_str());
    return 3;
  }
  const char buffer[] = { "temp_data\n" };
  FILE *pFile = fopen ( claimedCacheFileName.c_str() , "wb" );
  if(pFile==NULL){
    CDBError("claimCacheFile: Unable to open cachefile %s",claimedCacheFileName.c_str());
    return 1;
  }
  size_t bytesWritten = fwrite (buffer , sizeof(char),10 , pFile );
  fflush (pFile);   
  fclose (pFile);
  if(bytesWritten!=10){
    CDBError("claimCacheFile: Unable to write to cachefile %s",claimedCacheFileName.c_str());
    return 2;
  }
  saveFieldFile = true;
  cacheFileClaimed = true;
  #ifdef CCACHE_DEBUG
  CDBDebug("Cache claimed \"%s\"",claimedCacheFileName.c_str());
  #endif
  return 0;
}

void CCache::removeClaimedCachefile(){
  if(cacheFileClaimed){
    if(claimedCacheFileName.length()>0){
      //TODO MAKE SURE WE DO NOT REMOVE WRONG FILES!
      CDBDebug("Removing %s",claimedCacheFileName.c_str());
      remove(claimedCacheFileName.c_str());
    }
  }
  #ifdef CCACHE_DEBUG
  CDBDebug("Claimed cachefile removed");
  #endif
  cacheFileClaimed = false;
  saveFieldFile = false;
}

