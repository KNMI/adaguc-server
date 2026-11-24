/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  Lists the Zarr stores in an S3 bucket, like files in a directory
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

#include "S3Lister.h"
#include "CDebugger.h"
#include "CTString.h"

#include <algorithm>
#include <cstdlib>
#include <curl/curl.h>
#include <deque>
#include <map>

static const char *S3_STORE_EXTENSION = ".zarr";
static const long S3_CONNECT_TIMEOUT_SECONDS = 10;
static const long S3_REQUEST_TIMEOUT_SECONDS = 120;

/* A scan asks for the same directory once per layer; within one process the bucket is listed once. */
static std::map<std::string, std::vector<std::string>> listedDirectories;

/* A scan asks per dimension whether a new store is complete; it is checked once per process. */
static std::map<std::string, bool> checkedStores;

/* The marker a writer creates last, so that a store counts as complete once it exists. */
static const char *S3_STORE_COMPLETION_MARKER = ".zmetadata";

static std::string getEnvironment(const char *name) {
  const char *value = getenv(name);
  return value == nullptr ? "" : value;
}

/* Splits a location in the part before the # and the fragment, which includes the #. */
static void splitFragment(const std::string &location, std::string &url, std::string &fragment) {
  const auto hash = location.find('#');
  url = location.substr(0, hash);
  fragment = hash == std::string::npos ? "" : location.substr(hash);
}

/* True when the fragment has a mode list that contains s3, like #mode=zarr,s3 */
static bool fragmentHasS3Mode(const std::string &fragment) {
  if (fragment.size() < 2) return false;
  for (const auto &parameter: CT::split(fragment.substr(1), "&")) {
    if (!CT::startsWith(parameter, "mode=")) continue;
    for (const auto &mode: CT::split(parameter.substr(5), ",")) {
      if (mode == "s3") return true;
    }
  }
  return false;
}

bool parseS3Directory(const std::string &location, S3Directory &directory) {
  std::string url;
  splitFragment(location, url, directory.fragment);
  // The NetCDF library only speaks https to S3
  static const std::string scheme = "https://";
  if (!CT::startsWith(url, scheme) || !CT::endsWith(url, "/") || !fragmentHasS3Mode(directory.fragment)) {
    return false;
  }
  const auto bucketStart = url.find('/', scheme.size());
  if (bucketStart == std::string::npos || bucketStart == scheme.size()) {
    return false;
  }
  const auto bucketEnd = url.find('/', bucketStart + 1);
  if (bucketEnd == std::string::npos || bucketEnd == bucketStart + 1) {
    return false;
  }
  directory.endpoint = url.substr(0, bucketStart);
  directory.bucket = url.substr(bucketStart + 1, bucketEnd - bucketStart - 1);
  directory.prefix = url.substr(bucketEnd + 1);
  return true;
}

bool isS3Directory(const std::string &location) {
  S3Directory directory;
  return parseS3Directory(location, directory);
}

std::string getS3StoreName(const std::string &storeLocation) {
  std::string url, fragment;
  splitFragment(storeLocation, url, fragment);
  while (CT::endsWith(url, "/")) {
    url.pop_back();
  }
  return url.substr(url.find_last_of('/') + 1);
}

bool isS3StoreInDirectory(const std::string &storeLocation, const std::string &directoryLocation, const std::string &filter) {
  S3Directory directory;
  if (!parseS3Directory(directoryLocation, directory)) {
    return false;
  }
  std::string storeUrl, storeFragment;
  splitFragment(storeLocation, storeUrl, storeFragment);
  const std::string directoryUrl = directory.endpoint + "/" + directory.bucket + "/" + directory.prefix;
  if (storeFragment != directory.fragment || !CT::startsWith(storeUrl, directoryUrl) || !CT::endsWith(storeUrl, S3_STORE_EXTENSION)) {
    return false;
  }
  return filter.empty() || CT::testRegEx(getS3StoreName(storeLocation), filter.c_str());
}

static std::string decodeXMLEntities(std::string text) {
  static const std::pair<const char *, const char *> entities[] = {{"&lt;", "<"}, {"&gt;", ">"}, {"&quot;", "\""}, {"&apos;", "'"}, {"&amp;", "&"}};
  for (const auto &entity: entities) {
    text = CT::replace(text, entity.first, entity.second);
  }
  return text;
}

/* Returns the text of the first <tag>...</tag> at or after position from, and moves from behind it. Returns false if there is none. */
static bool nextElementText(const std::string &xml, const std::string &tag, size_t &from, std::string &text) {
  const std::string open = "<" + tag + ">", close = "</" + tag + ">";
  const auto start = xml.find(open, from);
  if (start == std::string::npos) return false;
  const auto end = xml.find(close, start + open.size());
  if (end == std::string::npos) return false;
  text = xml.substr(start + open.size(), end - start - open.size());
  from = end + close.size();
  return true;
}

S3ListPage parseS3ListPage(const std::string &xml) {
  S3ListPage page;
  size_t position = 0;
  std::string commonPrefixes;
  while (nextElementText(xml, "CommonPrefixes", position, commonPrefixes)) {
    size_t prefixPosition = 0;
    std::string prefix;
    while (nextElementText(commonPrefixes, "Prefix", prefixPosition, prefix)) {
      page.prefixes.push_back(decodeXMLEntities(prefix));
    }
  }
  position = 0;
  std::string truncated, token;
  if (nextElementText(xml, "IsTruncated", position, truncated) && truncated == "true") {
    position = 0;
    if (nextElementText(xml, "NextContinuationToken", position, token)) {
      page.nextContinuationToken = decodeXMLEntities(token);
    }
  }
  return page;
}

static size_t appendToString(char *data, size_t size, size_t count, void *destination) {
  static_cast<std::string *>(destination)->append(data, size * count);
  return size * count;
}

/* Does one ListObjectsV2 request for the keys directly below prefix, and returns the body. Throws int on failure. */
static std::string requestS3ListPage(CURL *curl, const S3Directory &directory, const std::string &prefix, const std::string &continuationToken) {
  auto encode = [curl](const std::string &value) {
    char *escaped = curl_easy_escape(curl, value.c_str(), (int)value.size());
    std::string result = escaped == nullptr ? "" : escaped;
    curl_free(escaped);
    return result;
  };
  // The query parameters are in alphabetical order, as the request signature requires.
  std::string url = directory.endpoint + "/" + directory.bucket + "?";
  if (!continuationToken.empty()) {
    url += "continuation-token=" + encode(continuationToken) + "&";
  }
  url += "delimiter=%2F&list-type=2&max-keys=1000&prefix=" + encode(prefix);

  std::string body;
  char errorBuffer[CURL_ERROR_SIZE] = "";
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, appendToString);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
  const CURLcode result = curl_easy_perform(curl);
  long httpStatus = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, nullptr);
  if (result != CURLE_OK) {
    CDBError("Unable to list S3 bucket [%s] at [%s]: %s", directory.bucket.c_str(), directory.endpoint.c_str(), errorBuffer[0] != 0 ? errorBuffer : curl_easy_strerror(result));
    throw(__LINE__);
  }
  if (httpStatus != 200) {
    CDBError("Unable to list S3 bucket [%s] at [%s]: HTTP status %ld: %s", directory.bucket.c_str(), directory.endpoint.c_str(), httpStatus, body.substr(0, 300).c_str());
    throw(__LINE__);
  }
  return body;
}

/*
 * A curl handle that signs its requests with the same credentials the Zarr reader uses to open the stores:
 * AWS_ACCESS_KEY_ID and AWS_SECRET_ACCESS_KEY, and AWS_SESSION_TOKEN for the temporary credentials of a role.
 */
struct S3Curl {
  CURL *curl = nullptr;
  curl_slist *headers = nullptr;

  S3Curl() {
    curl = curl_easy_init();
    if (curl == nullptr) {
      CDBError("Unable to initialize curl");
      throw(__LINE__);
    }
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, S3_CONNECT_TIMEOUT_SECONDS);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, S3_REQUEST_TIMEOUT_SECONDS);

    const std::string accessKey = getEnvironment("AWS_ACCESS_KEY_ID"), secretKey = getEnvironment("AWS_SECRET_ACCESS_KEY");
    std::string region = getEnvironment("AWS_REGION");
    if (region.empty()) region = getEnvironment("AWS_DEFAULT_REGION");
    if (region.empty()) region = "us-east-1";
    if (!accessKey.empty() && !secretKey.empty()) {
      // libcurl copies these strings
      curl_easy_setopt(curl, CURLOPT_AWS_SIGV4, ("aws:amz:" + region + ":s3").c_str());
      curl_easy_setopt(curl, CURLOPT_USERPWD, (accessKey + ":" + secretKey).c_str());
      // Temporary credentials are only accepted with their session token. libcurl does not copy the list of headers.
      const std::string sessionToken = getEnvironment("AWS_SESSION_TOKEN");
      if (!sessionToken.empty()) {
        headers = curl_slist_append(headers, ("x-amz-security-token: " + sessionToken).c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
      }
    }
  }
  ~S3Curl() {
    curl_easy_cleanup(curl);
    curl_slist_free_all(headers);
  }
  S3Curl(const S3Curl &) = delete;
  S3Curl &operator=(const S3Curl &) = delete;
};

bool isS3StoreComplete(const std::string &storeLocation) {
  const auto cached = checkedStores.find(storeLocation);
  if (cached != checkedStores.end()) {
    return cached->second;
  }
  std::string url, fragment;
  splitFragment(storeLocation, url, fragment);
  url += std::string("/") + S3_STORE_COMPLETION_MARKER;

  char errorBuffer[CURL_ERROR_SIZE] = "";
  S3Curl s3;
  CURL *curl = s3.curl;
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
  const CURLcode result = curl_easy_perform(curl);
  long httpStatus = 0;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, nullptr);

  if (result != CURLE_OK) {
    CDBWarning("Unable to check whether store [%s] is complete: %s", storeLocation.c_str(), errorBuffer[0] != 0 ? errorBuffer : curl_easy_strerror(result));
    return false; // Not cached: try again next time
  }
  if (httpStatus != 200 && httpStatus != 404) {
    CDBWarning("Unable to check whether store [%s] is complete: HTTP status %ld", storeLocation.c_str(), httpStatus);
    return false;
  }
  const bool complete = httpStatus == 200;
  checkedStores[storeLocation] = complete;
  return complete;
}

std::vector<std::string> listS3Stores(const std::string &location, const std::string &filter, const std::string &subDirectory) {
  S3Directory directory;
  if (!parseS3Directory(location, directory)) {
    CDBError("[%s] is not a S3 directory, expected https://<host>/<bucket>/[<prefix>/]#mode=zarr,s3", location.c_str());
    throw(__LINE__);
  }
  std::string startPrefix = directory.prefix;
  if (!subDirectory.empty()) {
    startPrefix += subDirectory;
    if (!CT::endsWith(startPrefix, "/")) startPrefix += "/";
  }

  const std::string cacheKey = location + "\n" + filter + "\n" + startPrefix;
  const auto cached = listedDirectories.find(cacheKey);
  if (cached != listedDirectories.end()) {
    return cached->second;
  }

  // Released when this function returns, also when a request fails
  S3Curl s3;
  CURL *curl = s3.curl;

  std::vector<std::string> stores;
  size_t numberOfRequests = 0;
  std::deque<std::string> prefixesToList = {startPrefix};
  while (!prefixesToList.empty()) {
    const std::string prefix = prefixesToList.front();
    prefixesToList.pop_front();
    std::string continuationToken;
    do {
      const S3ListPage page = parseS3ListPage(requestS3ListPage(curl, directory, prefix, continuationToken));
      numberOfRequests++;
      for (const auto &key: page.prefixes) {
        // A key like prefix/name.zarr/ is a store, any other key ending in a slash is a subdirectory.
        const std::string keyWithoutSlash = key.substr(0, key.size() - 1);
        const std::string name = keyWithoutSlash.substr(keyWithoutSlash.find_last_of('/') + 1);
        if (name.empty() || name[0] == '.') continue;
        if (!CT::endsWith(name, S3_STORE_EXTENSION)) {
          prefixesToList.push_back(key);
        } else if (filter.empty() || CT::testRegEx(name, filter.c_str())) {
          stores.push_back(directory.endpoint + "/" + directory.bucket + "/" + keyWithoutSlash + directory.fragment);
        }
      }
      continuationToken = page.nextContinuationToken;
    } while (!continuationToken.empty());
  }

  std::sort(stores.begin(), stores.end());
  CDBDebug("Listed S3 directory [%s]: %lu stores match filter [%s], in %lu requests", location.c_str(), stores.size(), filter.c_str(), numberOfRequests);
  listedDirectories[cacheKey] = stores;
  return stores;
}
