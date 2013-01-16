#include <sys/stat.h>
#include "CTypes.h"
#ifndef CCACHE_H
#define CCACHE_H

class CCache{
  private:
  bool saveFieldFile;
  bool cacheAvailable;
  bool cacheFileClaimed;
   CT::string fileName;
  CT::string claimedCacheFileName;
  DEF_ERRORFUNCTION();
 
   /**
  * Check if cachefile is working on a file. Tests _tmp file. When returns true, a new cache file should not be generated
  * @param fileName
  * @return true when cache is busy
  */
  bool isCacheFileBusy();
  
   /**
  * Check if cachefile is working on a file. Tests _tmp file. When returns true, a new cache file should not be generated
  * @param fileName
  * @return true when cache is busy
  */
  bool isCacheFileBusyBlocking();

  public:
    
  /**
    * Constructor
    */
  CCache();
  
  /**
   * Destructor
   */
  ~CCache();
  
  /**
   * Returns true if cachefile can be saved
   */
  bool saveCacheFile();
  
  /**
   * Returns true if valid cachefile is available
   */
  bool cacheIsAvailable();
  
  /**
   * Checks availability of cachefile and determines if cache is busy. Cache methods saveCacheFile() and cacheIsAvailable() are set
   * If cache is noy busy and cache is not available, immidiately a cachefile is claimed and should be released with releaseCacheFile(const char*) when ready. 
   * @param fileName;
   */
  void check(const char *fileName);


  /**
  * Check if cachefile is available
  * @param fileName
  * @return true if is available
  */
  static bool isCacheFileAvailable(const char *fileName);

  
  /**
  * Move temporary cachefile to permanent and make it public for everyone in the system, in order to have it deletable
  * @return Zero on success
  */
  int releaseCacheFile();
  
 
  
  
  /**
   * Get the filename of the claimed cachefile
   * @return The claimed cache filename
   */
  const char *getCacheFileNameToWrite();
  
  /**
   * Claimes the cachefile
   */
  int claimCacheFile ();
  
   void removeClaimedCachefile();

  
  
};
#endif
