/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Cache on disk of what is fetched when a Zarr store is opened
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2026-11-20
 *
 ******************************************************************************
 *
 * Copyright 2026, Royal Netherlands Meteorological Institute (KNMI)
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

#ifndef ZARRMETADATACACHE_H
#define ZARRMETADATACACHE_H

#include <map>
#include <string>
#include <utility>

namespace Zarr {

  /**
   * What is fetched when a store is opened: the consolidated metadata, and the chunks of the coordinate variables and single
   * values. Over HTTP(S) or S3 these are two rounds of requests for every request to adaguc-server, because every request is
   * a new process. Kept in a file, the next requests for the same store open it without any request to the store.
   */
  struct OpenedStore {
    /** The text of .zmetadata, as stored */
    std::string consolidatedMetadata;
    /** Chunks as stored, by key: whether the chunk exists, and its data. The type of Zarr::PrefetchedChunks. */
    std::map<std::string, std::pair<bool, std::string>> prefetched;
  };

  /**
   * How long an opened store is kept, in seconds: ADAGUC_ZARR_METADATA_CACHE_SECONDS, a whole number. 0, the default, turns
   * the cache off. A store that changes under the same location is read with its old metadata for at most this long.
   */
  size_t getMetadataCacheSeconds();

  /** The directory of the cache, zarr-metadata-cache in ADAGUC_TMP. Empty when ADAGUC_TMP is not set: then nothing is kept. */
  std::string getMetadataCacheDirectory();

  /**
   * Whether this process uses the cache, true unless switched off. Scans (--updatedb) switch it off: they open every store,
   * which says nothing about which stores will be requested, and they should see each store as it is now.
   */
  void setMetadataCacheUsed(bool used);
  bool isMetadataCacheUsed();

  /**
   * Cleans the cache: with the cache on, removes the entries older than ADAGUC_ZARR_METADATA_CACHE_SECONDS; with the cache off,
   * removes all entries and the directory. Called when the fork server starts, so that entries do not stay after the cache
   * is turned off, or when no store is opened any more.
   */
  void cleanMetadataCache();

  /**
   * Reads what was kept for a store.
   * @param directory The directory of the cache
   * @param location The location of the store, as given to Zarr::openStore
   * @param maxAgeSeconds Entries written longer ago are not used
   * @return False when there is no entry that can be used; nothing is logged, the store is then opened as usual.
   */
  bool readMetadataCache(const std::string &directory, const std::string &location, size_t maxAgeSeconds, OpenedStore &opened);

  /**
   * Keeps what was fetched for a store, replacing an older entry. Processes that read at the same time see the old or the
   * new entry, never a part of it. Once per maxAgeSeconds (at least a minute) the entries that are too old are removed.
   * @return False when the entry could not be written; nothing is logged.
   */
  bool writeMetadataCache(const std::string &directory, const std::string &location, size_t maxAgeSeconds, const OpenedStore &opened);

} // namespace Zarr

#endif
