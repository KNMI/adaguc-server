#include <sys/stat.h>
#include "CTypes.h"
#ifndef CCACHE_H
#define CCACHE_H

/**
 * Helper class for cache files. 
 * The cache system uses a temporary file to write intermediate results.
 * It takes care of multiple processes trying to read save and modify the same cache file.
 * This set of methods take care of both the temporary and the non temporary files, moves files when ready and sets permissions
 * 
 * Methods should be called in the following order:
 * 1) cache.check($cachefile)        : Initialization of cache system.
 * 2) cache.cacheIsAvailable()       :  Returns true if valid cachefile is available
 * 3) cache.saveCacheFile()          :  Returns true if cachesystem is ready to save a cachefile 
 * 4) cache.getCacheFileNameToWrite(): Filename to write (same as $cachefile, but with _tmp appended)
 * 5) cache.releaseCacheFile()       : Temporary cache file is moved and ready for other processes.
 */

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

  
  /**
  * Check if cachefile is available
  * @param fileName
  * @return true if is available
  */
  static bool isCacheFileAvailable(const char *fileName);

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
   * Checks availability of cachefile and determines if cache is busy. Cache methods saveCacheFile() and cacheIsAvailable() are available to query after this has been called.
   * @param fileName;
   */
  void checkCacheSystemReady(const char *fileName);
  
  /**
   * Returns true if cachefile can be saved
   */
  bool saveCacheFile();
  
  /**
   * Returns true if valid cachefile is available
   */
  bool cacheIsAvailable();
  
  /**
   * Get the filename of the claimed cachefile
   * @return The claimed cache filename
   */
  const char *getCacheFileNameToWrite();
  
  /**
   * Claimes the cachefile, must be released with releaseCacheFile or removeClaimedCachefile.
   */
  int claimCacheFile ();
  
  /**
  * Move temporary cachefile to permanent and make it public for everyone in the system (public in order to have it deletable)
  * @return Zero on success
  */
  int releaseCacheFile();
  
  /**
   * Removes and unclaims cachefile, necessary in case an error happened and the program is unable to save a cachefile
   */
  void removeClaimedCachefile();

  
  
};
#endif
