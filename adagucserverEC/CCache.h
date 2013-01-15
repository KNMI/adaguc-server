#include <sys/stat.h>
#include "CTypes.h"
#ifndef CCACHE_H
#define CCACHE_H

class Cache{
  private:
  bool saveFieldFile;
  bool cacheAvailable;
  const char *fileName;
  CT::string claimedCacheFileName;
  
  public:
  Cache(){
    saveFieldFile = false;
    cacheAvailable = false;
    fileName = NULL;
  }
  
  bool saveCacheFile(){
    return saveFieldFile;
  }
  bool cacheIsAvailable(){
    return cacheAvailable;
  }
  
  /**
   * Checks availability of cachefile and determines if cache is busy. Cache methods saveCacheFile() and cacheIsAvailable() are set
   * If cache is noy busy and cache is not available, immidiately a cachefile is claimed and should be released with releaseCacheFile(const char*) when ready. 
   * @param fileName;
   */
  void check(const char *fileName){
    if(fileName == NULL)return;
    this->fileName = fileName;
    claimedCacheFileName = fileName;
    claimedCacheFileName.concat("_tmp");
      //Check if cache is busy
    if(Cache::isCacheFileBusy(fileName)){
      saveFieldFile = false;
      cacheAvailable=false;
    }else{
      //Cache is not busy: Check if the cachefile is available
      if(Cache::isCacheFileAvailable(fileName)){
        cacheAvailable = true;
      }else{
        //Cache is not available: Claim cachefile
        if(Cache::claimCacheFile(fileName)==0){
          saveFieldFile = true;  
        }
      }
    }
  }

  /**
  * Check if cachefile is working on a file. Tests _tmp file. When returns true, a new cache file should not be generated
  * @param fileName
  * @return true when cache is busy
  */
  static bool isCacheFileBusy(const char *fileName){
    //Is some kind of process working on any of the cache files?
    CT::string f = fileName;f.concat("_tmp");
    FILE *pFile = fopen ( f.c_str() , "r" );
    if(pFile != NULL){
      fclose (pFile);
      return true;
    }
    return false;        
  }

  /**
  * Check if cachefile is available
  * @param fileName
  * @return true if is available
  */
  static bool isCacheFileAvailable(const char *fileName){
    FILE *pFile = fopen ( fileName , "r" );
    if(pFile != NULL){
      fclose (pFile);
      return true;
    }
    return false;
  }

  
  /**
  * Move temporary cachefile to permanent and make it public for everyone in the system, in order to have it deletable
  * @return Zero on success
  */
  int releaseCacheFile(){
    CT::string f = fileName;f.concat("_tmp");
    if(chmod(f.c_str(),0777)<0){
      return 1;
    }
    rename(f.c_str(),fileName);
    return 0;
  }
  
  /**
   * Get the filename of the claimed cachefile
   * @return The claimed cache filename
   */
  const char *getCacheFileNameToWrite(){
    return claimedCacheFileName.c_str();
  }
  
  
  /**
   * Claims cachefile
   * @param fileName;
   * @return Zero on success, One if file could not be created, Two if file is not writable
   */
  static int claimCacheFile (const char *fileName){
    CT::string f = fileName;f.concat("_tmp");
    const char buffer[] = { "temp_data\n" };
    FILE *pFile = fopen ( f.c_str() , "wb" );
    if(pFile==NULL){
      return 1;
    }
    size_t bytesWritten = fwrite (buffer , sizeof(char),10 , pFile );
    fflush (pFile);   
    fclose (pFile);
    if(bytesWritten!=10){
      return 2;
    }
    return 0;
  }
  
};
#endif
