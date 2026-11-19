/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Cache of decoded Zarr chunks
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2026-11-19
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

#ifndef ZARRCHUNKCACHE_H
#define ZARRCHUNKCACHE_H

#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace Zarr {

  /**
   * Decoded chunks by key, at most maxBytes; the chunks used longest ago are dropped first. A chunk that spans more than
   * the requested part, like a chunk of all levels when one level is read, is then fetched and decoded once for all reads
   * of a request instead of once per read. Thread safe.
   */
  class ChunkCache {
  public:
    explicit ChunkCache(size_t maxBytes) : maxBytes(maxBytes) {}

    /** The chunk, or nullptr when it is not in the cache */
    std::shared_ptr<const std::string> get(const std::string &key) {
      std::lock_guard<std::mutex> lock(mutex);
      const auto found = entries.find(key);
      if (found == entries.end()) return nullptr;
      order.splice(order.begin(), order, found->second.second);
      return found->second.first;
    }

    void put(const std::string &key, std::shared_ptr<const std::string> chunk) {
      if (chunk == nullptr || chunk->size() > maxBytes) return;
      std::lock_guard<std::mutex> lock(mutex);
      if (entries.find(key) != entries.end()) return;
      order.push_front(key);
      entries[key] = {chunk, order.begin()};
      bytes += chunk->size();
      while (bytes > maxBytes && !order.empty()) {
        const auto oldest = entries.find(order.back());
        bytes -= oldest->second.first->size();
        entries.erase(oldest);
        order.pop_back();
      }
    }

    void clear() {
      std::lock_guard<std::mutex> lock(mutex);
      entries.clear();
      order.clear();
      bytes = 0;
    }

  private:
    const size_t maxBytes;
    size_t bytes = 0;
    std::list<std::string> order; // The most recently used first
    std::unordered_map<std::string, std::pair<std::shared_ptr<const std::string>, std::list<std::string>::iterator>> entries;
    std::mutex mutex;
  };

} // namespace Zarr

#endif
