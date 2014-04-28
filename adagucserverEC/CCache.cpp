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
#include <signal.h>
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
  bool cacheWasLocked = false;
  //Is some kind of process working on any of the cache files?
  do{
    if(isCacheFileBusy()){
      cacheWasLocked = true;
      if(currentTries<=0){
        CDBDebug("!!! A process is working already 60 seconds on %s, skipping wait",fileName.c_str());
        return true;
      }else{
        CDBDebug("Another process is working on %s, waiting... %d",fileName.c_str(),maxTries-currentTries+1);
        
        //Wait 1 second and try again
        usleep(1000000);
      }
    }else {
      if(cacheWasLocked){
        CDBDebug("Another process has finished working on %s",fileName.c_str());
      }
      return false;
    }
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
  //const char buffer[] = { "temp_data\n" };
  
  char buffer[20];
  int procID = int(getpid());
  snprintf(buffer,19,"%d",procID);
  size_t length = strlen(buffer);
  if(length>19)length=19;
  
  FILE *pFile = fopen ( claimedCacheFileName.c_str() , "wb" );
  if(pFile==NULL){
    CDBError("claimCacheFile: Unable to open cachefile %s",claimedCacheFileName.c_str());
    return 1;
  }
  size_t bytesWritten = fwrite (buffer , sizeof(char),length , pFile );
  fflush (pFile);   
  fclose (pFile);
  if(bytesWritten!=length){
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


CCache::Lock::Lock(){
  claimedLockFile = "";
  claimedLockID = "";
  isEnabled = false;
}
CCache::Lock::~Lock(){
  release();
}
int CCache::Lock::claim(const char *cacheDir, const char *identifier,bool enable){
  isEnabled = enable;
  if(isEnabled == false)return 0;
 
  if(cacheDir == NULL || identifier == NULL)return 0;
  
  CDirReader::makePublicDirectory(cacheDir);

  if(claimedLockFile.length()>0){
    CDBError("Already claimed! %s",claimedLockID.c_str());
    
  }else{
    CT::string myid = "lock_";
    myid.concat(identifier);
    claimedLockID = identifier;
    myid.replaceSelf(":",""); myid.replaceSelf("/",""); 
    claimedLockFile = cacheDir;
    claimedLockFile.concat("/");
    claimedLockFile.concat(&myid);
  }
  FILE *pFile = NULL;
  
  //Check if the file exists...
  
  int maxTries = 1200;//Wait 120 seconds.
  bool wasLocked = false;
  do{
    pFile = fopen ( claimedLockFile.c_str() , "r" );
    if(pFile != NULL){ 
      wasLocked = true;
      
      //Read by which process the file was locked
      size_t thePIDThatIsLocking = 0;
      fseek (pFile , 0 , SEEK_END);
      size_t fileSize = ftell (pFile);
      rewind (pFile);
      char *buffer = (char*) malloc (sizeof(char)*fileSize);
      if (buffer == NULL) {CDBError ("Memory error",stderr); exit (2);}
      size_t result = fread (buffer,1,fileSize,pFile);
      if (result != fileSize) {CDBError ("Reading error",stderr);}else{
        CT::string s = buffer;
        thePIDThatIsLocking = s.toInt();
      }
      free(buffer);
      
      //Check if the process locking this file is still running
      int status = kill(thePIDThatIsLocking,0);
      if(status == -1){
        CDBError("WARNING: Locked %s by procID %d which is not running, continuing...", claimedLockID.c_str() ,thePIDThatIsLocking);
        maxTries = 0;
      }else{
        CDBDebug("LOCKED %s by procID %d, waiting %f seconds", claimedLockID.c_str() ,thePIDThatIsLocking,float(maxTries)/10.0);
      }
      fclose (pFile);
      usleep(100000);
      maxTries--;
    }
  }while(pFile != NULL&&maxTries>0);
  if(wasLocked){CDBDebug("LOCK FREED %s,", claimedLockID.c_str() );}

 // const char buffer[] = { "temp_data\n" };
  
  char buffer[20];
  int procID = int(getpid());
  snprintf(buffer,19,"%d",procID);
  size_t length = strlen(buffer);
  if(length>19)length=19;
  
  pFile = fopen ( claimedLockFile.c_str() , "wb" );
  if(pFile==NULL){
    CDBError("LOCK: Unable to create lockfile %s",claimedLockFile.c_str());
    return 1;
  }
  size_t bytesWritten = fwrite (buffer , sizeof(char),length , pFile );
  fflush (pFile);   
  fclose (pFile);
  if(bytesWritten!=length){
    CDBError("LOCK: Unable to write to lockfile %s",claimedLockFile.c_str());
    return 2;
  }
  if(chmod(claimedLockFile.c_str(),0777)<0){
    CDBError("LOCK: Unable to change permissions of lockfile %s",claimedLockFile.c_str());
    return 3;
  }
  
  //CDBDebug("LOCK Claimed: %s", claimedLockFile.c_str() );
  return 0;
}

void CCache::Lock::release(){
  if(isEnabled == false)return;
  if( claimedLockFile.length()==0)return;
  //CDBDebug("LOCK release %s",claimedLockID.c_str());
  remove(claimedLockFile.c_str());
  claimedLockFile = "";
}
