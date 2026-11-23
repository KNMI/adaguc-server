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

#include "ZarrMetadataCache.h"
#include "CDebugger.h"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

namespace Zarr {

  namespace {

    /* The first bytes of an entry. Another layout of the file gets another number: old entries are then not used. */
    const std::string MAGIC = "ADAGUC-ZARR-OPENED-STORE-1\n";
    const char *ENTRY_EXTENSION = ".zarrmeta";
    const char *CLEANED_MARKER = "cleaned";
    /* Metadata and coordinate variables are kilobytes. Anything much larger is not kept, and not read. */
    const size_t MAX_ENTRY_BYTES = 32 * 1024 * 1024;
    const time_t MIN_SECONDS_BETWEEN_CLEANING = 60;

    /* FNV-1a: the same file name for a location in every process and every build. The entry holds the location itself. */
    std::string entryPath(const std::string &directory, const std::string &location) {
      uint64_t hash = 14695981039346656037ULL;
      for (const unsigned char c: location) {
        hash ^= c;
        hash *= 1099511628211ULL;
      }
      char name[32];
      snprintf(name, sizeof(name), "%016llx", (unsigned long long)hash);
      return directory + "/" + name + ENTRY_EXTENSION;
    }

    void appendNumber(std::string &output, uint64_t value) {
      for (int byte = 0; byte < 8; byte++) output += (char)((value >> (8 * byte)) & 0xff);
    }

    void appendText(std::string &output, const std::string &text) {
      appendNumber(output, text.size());
      output += text;
    }

    bool readNumber(const std::string &input, size_t &position, uint64_t &value) {
      if (input.size() - position < 8) return false;
      value = 0;
      for (int byte = 0; byte < 8; byte++) value |= (uint64_t)(unsigned char)input[position + byte] << (8 * byte);
      position += 8;
      return true;
    }

    bool readText(const std::string &input, size_t &position, std::string &text) {
      uint64_t size = 0;
      if (!readNumber(input, position, size) || size > input.size() - position) return false;
      text.assign(input, position, size);
      position += size;
      return true;
    }

    /* Seconds since the file was written, or -1 when it does not exist or is from the future */
    time_t getAge(const std::string &path) {
      struct stat status;
      if (stat(path.c_str(), &status) != 0) return -1;
      const time_t now = time(nullptr);
      return status.st_mtime > now ? -1 : now - status.st_mtime;
    }

    bool isCacheFile(const std::string &name) {
      const bool isEntry = name.size() > strlen(ENTRY_EXTENSION) && name.compare(name.size() - strlen(ENTRY_EXTENSION), std::string::npos, ENTRY_EXTENSION) == 0;
      // A temporary file, see writeMetadataCache: left when a process stopped while writing
      const bool isTemporary = name.find(std::string(ENTRY_EXTENSION) + ".") != std::string::npos;
      return isEntry || isTemporary;
    }

    /* Removes the entries and temporary files older than maxAgeSeconds, or all of them when maxAgeSeconds is negative */
    void removeEntries(const std::string &directory, time_t maxAgeSeconds) {
      DIR *entries = opendir(directory.c_str());
      if (entries == nullptr) return;
      while (struct dirent *entry = readdir(entries)) {
        const std::string name = entry->d_name;
        if (!isCacheFile(name)) continue;
        const std::string path = directory + "/" + name;
        const time_t age = getAge(path);
        if (maxAgeSeconds < 0 || age < 0 || age > maxAgeSeconds) unlink(path.c_str());
      }
      closedir(entries);
    }

    /* Removes the entries that are too old, at most once per maxAgeSeconds for all processes together */
    void removeOldEntries(const std::string &directory, size_t maxAgeSeconds) {
      const time_t period = std::max<time_t>((time_t)maxAgeSeconds, MIN_SECONDS_BETWEEN_CLEANING);
      const std::string marker = directory + "/" + CLEANED_MARKER;
      const time_t markerAge = getAge(marker);
      if (markerAge >= 0 && markerAge < period) return;
      // First the marker, so that other processes do not clean at the same time
      const int markerFile = open(marker.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
      if (markerFile == -1) return;
      close(markerFile);
      utimes(marker.c_str(), nullptr);
      removeEntries(directory, period);
    }

    bool metadataCacheUsed = true;
  } // namespace

  size_t getMetadataCacheSeconds() {
    const char *value = getenv("ADAGUC_ZARR_METADATA_CACHE_SECONDS");
    if (value == nullptr || value[0] == '\0') return 0;
    char *end = nullptr;
    errno = 0;
    const unsigned long long seconds = strtoull(value, &end, 10);
    // strtoull accepts a sign and wraps a negative number around, so only digits are allowed
    const bool isNumber = value[0] >= '0' && value[0] <= '9' && *end == '\0' && errno == 0 && seconds <= 366ULL * 24 * 3600;
    if (!isNumber) {
      static bool warned = false;
      if (!warned) {
        CDBWarning("ADAGUC_ZARR_METADATA_CACHE_SECONDS=%s is not a whole number of seconds of at most a year, the metadata of Zarr stores is not kept", value);
        warned = true;
      }
      return 0;
    }
    return (size_t)seconds;
  }

  std::string getMetadataCacheDirectory() {
    const char *temporaryDirectory = getenv("ADAGUC_TMP");
    if (temporaryDirectory == nullptr || temporaryDirectory[0] == '\0') return "";
    return std::string(temporaryDirectory) + "/zarr-metadata-cache";
  }

  void setMetadataCacheUsed(bool used) { metadataCacheUsed = used; }

  bool isMetadataCacheUsed() { return metadataCacheUsed; }

  void cleanMetadataCache() {
    const std::string directory = getMetadataCacheDirectory();
    if (directory.empty()) return;
    const size_t seconds = getMetadataCacheSeconds();
    if (seconds > 0) {
      removeEntries(directory, std::max<time_t>((time_t)seconds, MIN_SECONDS_BETWEEN_CLEANING));
      return;
    }
    // The cache is off: nothing in the directory is used any more
    removeEntries(directory, -1);
    unlink((directory + "/" + CLEANED_MARKER).c_str());
    rmdir(directory.c_str());
  }

  bool readMetadataCache(const std::string &directory, const std::string &location, size_t maxAgeSeconds, OpenedStore &opened) {
    opened = OpenedStore();
    if (directory.empty() || maxAgeSeconds == 0) return false;
    const std::string path = entryPath(directory, location);
    const time_t age = getAge(path);
    if (age < 0 || age > (time_t)maxAgeSeconds) return false;

    FILE *file = fopen(path.c_str(), "rb");
    if (file == nullptr) return false;
    std::string content;
    char buffer[65536];
    size_t numRead = 0;
    while ((numRead = fread(buffer, 1, sizeof(buffer), file)) > 0 && content.size() <= MAX_ENTRY_BYTES) content.append(buffer, numRead);
    const bool readFailed = ferror(file) != 0;
    fclose(file);
    if (readFailed || content.size() > MAX_ENTRY_BYTES) return false;

    // An entry that is not complete, or of another location with the same file name, is not used
    if (content.compare(0, MAGIC.size(), MAGIC) != 0) return false;
    size_t position = MAGIC.size();
    std::string entryLocation;
    uint64_t numChunks = 0;
    if (!readText(content, position, entryLocation) || entryLocation != location) return false;
    if (!readText(content, position, opened.consolidatedMetadata) || !readNumber(content, position, numChunks)) return false;
    for (uint64_t chunk = 0; chunk < numChunks; chunk++) {
      std::string key, data;
      uint64_t found = 0;
      if (!readText(content, position, key) || !readNumber(content, position, found) || found > 1 || !readText(content, position, data)) {
        opened = OpenedStore();
        return false;
      }
      opened.prefetched[key] = {found == 1, std::move(data)};
    }
    if (position != content.size() || opened.consolidatedMetadata.empty()) {
      opened = OpenedStore();
      return false;
    }
    return true;
  }

  bool writeMetadataCache(const std::string &directory, const std::string &location, size_t maxAgeSeconds, const OpenedStore &opened) {
    if (directory.empty() || maxAgeSeconds == 0 || opened.consolidatedMetadata.empty()) return false;
    std::string content = MAGIC;
    appendText(content, location);
    appendText(content, opened.consolidatedMetadata);
    appendNumber(content, opened.prefetched.size());
    for (const auto &chunk: opened.prefetched) {
      appendText(content, chunk.first);
      appendNumber(content, chunk.second.first ? 1 : 0);
      appendText(content, chunk.second.second);
    }
    if (content.size() > MAX_ENTRY_BYTES) return false;

    if (mkdir(directory.c_str(), 0700) != 0 && errno != EEXIST) return false;
    // Written next to the entry and then renamed over it, so that a reader never sees a part of an entry. mkstemp makes a
    // file that no other process uses, also when processes of more machines or containers share the directory.
    const std::string path = entryPath(directory, location);
    std::string temporaryPath = path + ".XXXXXX";
    const int descriptor = mkstemp(temporaryPath.data());
    if (descriptor == -1) return false;
    FILE *file = fdopen(descriptor, "wb");
    if (file == nullptr) {
      close(descriptor);
      unlink(temporaryPath.c_str());
      return false;
    }
    const bool written = fwrite(content.data(), 1, content.size(), file) == content.size();
    const bool closed = fclose(file) == 0;
    if (!written || !closed || rename(temporaryPath.c_str(), path.c_str()) != 0) {
      unlink(temporaryPath.c_str());
      return false;
    }
    removeOldEntries(directory, maxAgeSeconds);
    return true;
  }

} // namespace Zarr
