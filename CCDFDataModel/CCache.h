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

#include <sys/stat.h>
#include "CTypes.h"
#include "CReadFile.h"
#include "CDirReader.h"
#ifndef CCACHE_H
#define CCACHE_H
#define CCACHE_MAXFILELENGTH 220
#include <unistd.h>
/**
 * Helper class for cache files.
 * The cache system uses a temporary file to write intermediate results.
 * It takes care of multiple processes trying to read save and modify the same cache file.
 * This set of methods take care of both the temporary and the non temporary files, moves files when ready and sets permissions
 *
 * Methods should be called in the following order:
 * 1) cache.checkCacheSystemReady($cachefile)        : Initialization of cache system.
 * 2) cache.cacheIsAvailable()       :  Returns true if valid cachefile is available, cached data is available for usage
 * 3) cache.saveCacheFile()          :  Returns true if cachesystem is ready to save a cachefile
 * 4) cache.getCacheFileNameToWrite(): Filename to write (same as $cachefile, but with _tmp appended)
 * 5) cache.releaseCacheFile()       : Temporary cache file is moved by CCCACHE and is immediately available for waiting other processes.
 */

class CCache {
private:
  bool saveFieldFile;
  bool cacheAvailable;
  bool cacheFileClaimed;
  bool cacheSystemIsBusy;
  CT::string fileName;
  CT::string dateFromFile;
  CT::string claimedCacheFileName;
  CT::string claimedCacheProcIDFileName;

  static int _writePidFile(const char *file);
  static int _readPidFile(const char *file);

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
  bool isCacheFileBusyBlocking(const char *reason);

  /**
   * Check if cachefile is available
   * @param fileName
   * @return true if is available
   */
  static bool isCacheFileAvailable(const char *fileName);

public:
  class Lock {
  private:
    CT::string claimedLockFile;
    CT::string claimedLockID;
    bool isEnabled;

  public:
    Lock();
    ~Lock();
    int claim(const char *cacheDir, const char *identifier, const char *reason, bool enable);
    void release();
  };
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
  void checkCacheSystemReady(const char *directory, const char *fileName, const char *resource, const char *reason);

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

  const char *getCacheFileNameToRead();

  /**
   * Claimes the cachefile, must be released with releaseCacheFile or removeClaimedCachefile.
   */
  int claimCacheFile();

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
