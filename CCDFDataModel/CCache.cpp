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

//#define CCACHE_DEBUG

CCache::CCache(){ 
  saveFieldFile = false;
  cacheAvailable = false;
  cacheFileClaimed = false;
  cacheSystemIsBusy = false;
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

int CCache::_readPidFile(const char *claimedLockFile){
  FILE *pFile = fopen ( claimedLockFile , "r" );
  if(pFile != NULL){
    //Read by which process the file was locked
    int thePIDThatIsLocking = -1;
    fseek (pFile , 0 , SEEK_END);
    size_t fileSize = ftell (pFile);
    rewind (pFile);
    char *buffer = (char*) malloc (sizeof(char)*fileSize);
    if (buffer == NULL) {
      CDBError ("Memory error",stderr); exit (2);
    }
    size_t result = fread (buffer,1,fileSize,pFile);
    if (result != fileSize) {
      CDBError ("Reading error",stderr);
    }else{
      CT::string s = buffer;
      thePIDThatIsLocking = s.toInt();
    }
    free(buffer);
    fclose(pFile);
    return thePIDThatIsLocking;
  }
  return -1;
}

int CCache::_writePidFile(const char *claimedLockFile){

  char buffer[20];
  int procID = int(getpid());
  
  
//   int i = _readPidFile(claimedLockFile);
//   if(i>0){
//     CDBError("PIDFILE IS THERE! PID:%d != FILE:%d",procID,i);
//     exit(4);
//   }
  
  snprintf(buffer,19,"%d",procID);
  size_t length = strlen(buffer);
  if(length>19)length=19;
  
  FILE *pFile = fopen ( claimedLockFile , "wb" );
  if(pFile==NULL){
    CDBError("LOCK: Unable to create lockfile %s",claimedLockFile);
    return 1;
  }
  size_t bytesWritten = fwrite (buffer , sizeof(char),length , pFile );
  fflush (pFile);   
  fclose (pFile);
  
  #ifdef CCACHE_DEBUG
  CDBDebug("Written LOCK for PID %s, key %s",buffer,claimedLockFile);
  #endif
  if(bytesWritten!=length){
    CDBError("LOCK: Unable to write to lockfile %s",claimedLockFile);
    return 2;
  }
  if(chmod(claimedLockFile,0777)<0){
    CDBError("LOCK: Unable to change permissions of lockfile %s",claimedLockFile);
    return 3;
  }
  return 0;
}

bool CCache::saveCacheFile(){
// #ifdef CCACHE_DEBUG
//   if(saveFieldFile){
//     CDBDebug("Cache not available from %s",this->fileName.c_str());
//   }
// #endif
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

void CCache::checkCacheSystemReady(const char *directory, const char *szfileName,const char * resource,const char *reason){
  if(cacheFileClaimed)return;
  
  if(szfileName == NULL)return;
  if(directory == NULL)return;
  if(cacheSystemIsBusy == true){
    CDBDebug("------------- ERROR cacheSystemIsBusy -------------");
    cacheAvailable = false;
    return;
  }
  saveFieldFile = false;
  cacheAvailable = false;
  //cacheFileClaimed = false;
  cacheSystemIsBusy = true;
  
  claimedCacheFileName = szfileName;
  
  CT::string myid = "";
  myid.concat(szfileName);
  myid.replaceSelf(":","_");
  //myid.replaceSelf("/","_"); 
  
  int maxlength = CCACHE_MAXFILELENGTH;
  size_t nrStrings = (myid.length()/maxlength)+1;
  CT::string myidinparts = "adaguc/cache";
  for(size_t j=0;j<nrStrings;j++){
    if(myidinparts.length()>0){
      myidinparts+="/";
    }
    myidinparts+= myid.substring(j*maxlength,j*maxlength+maxlength);
  }
  
  
//  CDBDebug("DIRECTORY %s",directory);
  
  claimedCacheFileName = directory;
  claimedCacheFileName.concat("/");
  claimedCacheFileName.concat(&myidinparts);
  this->fileName = claimedCacheFileName.c_str();
  
  claimedCacheProcIDFileName = claimedCacheFileName.c_str();
  claimedCacheProcIDFileName.concat("_pid");
  claimedCacheFileName.concat("_tmp");


  
    //Check if cache is busy
  if(isCacheFileBusyBlocking(reason)){
    saveFieldFile = false;
    cacheAvailable=false;
  }else{
    dateFromFile = CDirReader::getFileDate(resource);
    #ifdef CCACHE_DEBUG
    CDBDebug("Cache not busy for %s",this->fileName.c_str());
    #endif
    //Cache is not busy: Check if the cachefile is available
    if(CCache::isCacheFileAvailable(this->fileName.c_str())){
      //Cache is available
      
      bool cacheIsOutOfDate = true;
      
      
      CT::string dateFile = this->fileName.c_str();
      dateFile.concat("date");
      try{
        CT::string dateFromCache = CReadFile::open(dateFile.c_str());
        if(dateFromCache.equals(dateFromFile)){
           cacheIsOutOfDate = false;
           cacheAvailable = true;
           #ifdef CCACHE_DEBUG
          CDBDebug("Cache is available");
          #endif
        }
      }catch(int e){
      }
      //CDBDebug("claimedCacheFileName = [%s] for [%s]",claimedCacheFileName.c_str(),resource);
      
      if(cacheIsOutOfDate == true){
        //CDBDebug("Cache is available but needs to be refreshed");
        saveFieldFile = true;
        //TODO MAKE SURE WE DO NOT REMOVE WRONG FILES!
        CDBDebug("Removing outdated cachefile %s",fileName.c_str());
        remove(fileName.c_str());
      }
  
      
      //cacheAvailable = true;
     
    
    }else{
      //Cache is not available, but can be created
      #ifdef CCACHE_DEBUG
      CDBDebug("Cache is not available for %s at location %s",szfileName,this->fileName.c_str());
      #endif
      saveFieldFile = true;
    }
  }
  cacheSystemIsBusy = false;
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
  /*#ifdef CCACHE_DEBUG
  CDBDebug("Cache not busy \"%s\"",claimedCacheFileName.c_str());
  #endif
  */
  return false;
}

bool CCache::isCacheFileBusyBlocking(const char * reason){
 
  if(fileName.length()==0)return false;
  int maxTries = 100;
  int currentTries=maxTries;
  bool cacheWasLocked = false;
  int logEveryNChecks = 10;
  int logChecks = logEveryNChecks;
  //Is some kind of process working on any of the cache files?
  do{
    if(isCacheFileBusy()){
      cacheWasLocked = true;
      if(currentTries<=0){
        CDBDebug("CCache::LOCK !!! A process is working already 60 seconds on %s for %s, skipping wait",fileName.c_str(),reason);
        return true;
      }else{
        size_t thePIDThatIsLocking = _readPidFile(claimedCacheProcIDFileName.c_str());
        if(thePIDThatIsLocking>0){
          //Check if the process locking this file is still running
          int status = kill(thePIDThatIsLocking,0);
          if(status == -1){
            CDBDebug("WARNING: LOCKED by procID %d [%s] for key %s which is not running, continuing..." ,thePIDThatIsLocking, reason,claimedCacheFileName.c_str());
            currentTries = 0;
          }else{
            if(logChecks <= 0){
              CDBDebug("CCache::LOCKED by procID %d [%s], waiting %f seconds" ,thePIDThatIsLocking, reason,float(currentTries)/10.0);
              logChecks = logEveryNChecks;
            }else{
              logChecks--;
            }
          }
        }else{
           currentTries = 0;
           break;
        }
        if(currentTries>0){
          usleep(100000);
        }
      }
    }else{
      if(cacheWasLocked){
        CDBDebug("CCache::LOCK Another process has finished working on %s",fileName.c_str());
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
  rename(claimedCacheFileName.c_str(),fileName.c_str());
  if(claimedCacheProcIDFileName.length()>0){
    remove(claimedCacheProcIDFileName.c_str());
  }
  #ifdef CCACHE_DEBUG
  CDBDebug("CCache::LOCK Claimed cachefile released");
  #endif
  cacheFileClaimed = false;
  cacheSystemIsBusy = false;
  return 0;
}

const char *CCache::getCacheFileNameToWrite(){
  return claimedCacheFileName.c_str();
}

const char *CCache::getCacheFileNameToRead(){
  return fileName.c_str();
}


int CCache::claimCacheFile (){
  if(cacheFileClaimed){
    CDBDebug("CCache::LOCK Warning: Cache is already claimed by this process");
    return 0;
  }
  #ifdef CCACHE_DEBUG
  CDBDebug("CCache::LOCK Start claiming cache");
  #endif
  saveFieldFile = false; 
  if(isCacheFileAvailable(fileName.c_str())){
    CDBDebug("CCache::LOCK claimCacheFile: Cache is already available for %s", fileName.c_str());
    return 4;
  }
  if(isCacheFileBusy()){
    CDBDebug("CCache::LOCK claimCacheFile: Cache is already working on %s", claimedCacheFileName.c_str());
    return 3;
  }
  //const char buffer[] = { "temp_data\n" };
  
  

  CT::string directory = claimedCacheFileName.substring(0,claimedCacheFileName.lastIndexOf("/"));
  CDirReader::makePublicDirectory(directory.c_str());
  
  
   int status = _writePidFile(claimedCacheFileName.c_str());
   if(status != 0){
     CDBDebug("Unable to write pid file");
     return 1;
   }
   status = _writePidFile(claimedCacheProcIDFileName.c_str());
   if(status != 0){
     CDBDebug("Unable to write pid file");
     return 1;
   }
  
  //Write datefile
  CT::string dateFile = this->fileName.c_str();
  dateFile.concat("date");
  #ifdef CCACHE_DEBUG
  CDBDebug("WRITE CACHEDATEFILE %s %s" ,dateFile.c_str(),dateFromFile.c_str());
  #endif
  try{
    CReadFile::write(dateFile.c_str(),dateFromFile.c_str(),dateFromFile.length());
  }catch(int e){
    CDBDebug("Unable to write date file");
    return 1;
  }
  
  saveFieldFile = true;
  cacheFileClaimed = true;
  #ifdef CCACHE_DEBUG
  CDBDebug("CCache::LOCK Cache claimed \"%s\"",claimedCacheFileName.c_str());
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
    if(claimedCacheProcIDFileName.length()>0){
      remove(claimedCacheProcIDFileName.c_str());
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
int CCache::Lock::claim(const char *cacheDir, const char *identifier,const char *reason,bool enable){
  isEnabled = enable;
  if(isEnabled == false)return 0;
 
  if(cacheDir == NULL || identifier == NULL)return 0;
  
  

  if(claimedLockFile.length()>0){
    CDBDebug("Already claimed! %s",claimedLockID.c_str());
    
  }else{
    
    
    
  
    
    claimedLockID = identifier;
    
    CT::string myid = "";
    myid.concat(identifier);
    myid.replaceSelf(":","_");
    //myid.replaceSelf("/","_"); 
    
    int maxlength = CCACHE_MAXFILELENGTH;
    size_t nrStrings = (myid.length()/maxlength)+1;
    CT::string myidinparts = "adaguc/locks";
    for(size_t j=0;j<nrStrings;j++){
      if(myidinparts.length()>0){
        myidinparts+="/";
      }
      myidinparts+= myid.substring(j*maxlength,j*maxlength+maxlength);
    }
    
  
    
    claimedLockFile = cacheDir;
    claimedLockFile.concat("/");
    claimedLockFile.concat(&myidinparts);
    claimedLockFile.concat("lock.lock");
    
    
    CT::string directory = claimedLockFile.substring(0,claimedLockFile.lastIndexOf("/"));
    CDirReader::makePublicDirectory(directory.c_str());
  }

  
  //Check if the file exists...
  
  int maxTries = 100;//Wait 120 seconds.
  int logEveryNChecks = 10;
  int logChecks = logEveryNChecks;
  bool wasLocked = false;
  do{
    //Read by which process the file was locked
    int thePIDThatIsLocking = CCache::_readPidFile(claimedLockFile.c_str());
    if(thePIDThatIsLocking>0){ 
      wasLocked = true;
      //Check if the process locking this file is still running
      int status = kill(thePIDThatIsLocking,0);
      if(status == -1){
        CDBDebug("WARNING: LOCKED by procID %d for %s which is not running, for key [%s] continuing...", thePIDThatIsLocking,reason,claimedLockID.c_str() );
        maxTries = 0;
      }else{
        if(logChecks <=0){
          CDBDebug("LOCKED by procID %d for %s, waiting %f seconds",thePIDThatIsLocking,reason,float(maxTries)/10.0);
          logChecks = logEveryNChecks;
        }else{
          logChecks--;
        }
      }
      if(maxTries>0){
        usleep(100000);
      }
    
    }else{
      maxTries = 0;
      break;
    }
    maxTries--;
  }while(maxTries>0);
  if(wasLocked){
    #ifdef CCACHE_DEBUG
    CDBDebug("LOCK FREED %s,", claimedLockFile.c_str() );
    #endif
    
  }

 // const char buffer[] = { "temp_data\n" };
  CCache::_writePidFile(claimedLockFile.c_str());
    
    
  
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
