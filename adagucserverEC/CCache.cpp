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
  return saveFieldFile;
}
bool CCache::cacheIsAvailable(){
  return cacheAvailable;
}

void CCache::check(const char *szfileName){
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
    return true;
  }
  return false;
}

bool CCache::isCacheFileBusyBlocking(){
  if(fileName.length()==0)return false;
  int maxTries = 40;
  int currentTries=maxTries;
  
  //Is some kind of process working on any of the cache files?
  do{
    if(isCacheFileBusy()){
      if(currentTries<=0){
        CDBDebug("!!! A process is working already 10 seconds on %s, skipping wait",fileName.c_str());
        return true;
      }else{
        CDBDebug("Another process is working on %s, waiting... %d",fileName.c_str(),maxTries-currentTries+1);
        //Wait 0.5 second and try again
        usleep(250000);
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
  return 0;
}

const char *CCache::getCacheFileNameToWrite(){
  return claimedCacheFileName.c_str();
}

int CCache::claimCacheFile (){
  saveFieldFile = false; 
  if(isCacheFileBusy())return 3;
  if(isCacheFileAvailable(fileName.c_str()))return 4;
  const char buffer[] = { "temp_data\n" };
  FILE *pFile = fopen ( claimedCacheFileName.c_str() , "wb" );
  if(pFile==NULL){
    return 1;
  }
  size_t bytesWritten = fwrite (buffer , sizeof(char),10 , pFile );
  fflush (pFile);   
  fclose (pFile);
  if(bytesWritten!=10){
    return 2;
  }
  saveFieldFile = true;
  cacheFileClaimed = true;
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
  cacheFileClaimed = false;
  saveFieldFile = false;
}

