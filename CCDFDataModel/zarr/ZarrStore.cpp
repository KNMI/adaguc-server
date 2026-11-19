/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Access to the keys of a Zarr store, on disk or over HTTP(S) / S3
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

#include "ZarrStore.h"
#include "CDebugger.h"
#include "CTString.h"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <curl/curl.h>
#include <dirent.h>
#include <map>
#include <sys/stat.h>
#include <unistd.h>

namespace Zarr {

  static const long CONNECT_TIMEOUT_SECONDS = 10;
  static const long REQUEST_TIMEOUT_SECONDS = 120;
  static const int MAX_ATTEMPTS = 3;
  static const size_t DEFAULT_MAX_CONNECTIONS = 16;

  static std::string getEnvironment(const char *name) {
    const char *value = getenv(name);
    return value == nullptr ? "" : value;
  }

  size_t getMaxConnections() {
    static const size_t maxConnections = [] {
      const std::string value = getEnvironment("ADAGUC_ZARR_MAX_CONNECTIONS");
      if (value.empty()) return DEFAULT_MAX_CONNECTIONS;
      char *end = nullptr;
      const long long parsed = strtoll(value.c_str(), &end, 10);
      if (end == value.c_str() || *end != 0 || parsed < 1) {
        CDBWarning("Ignoring ADAGUC_ZARR_MAX_CONNECTIONS=[%s], expected a whole number above 0", value.c_str());
        return DEFAULT_MAX_CONNECTIONS;
      }
      return (size_t)parsed;
    }();
    return maxConnections;
  }

  /** A location split in the part before the # and the parameters of the fragment, like #mode=zarr,s3&aws.region=eu-west-1 */
  struct Location {
    std::string url;
    bool hasFragment = false;
    std::map<std::string, std::string> parameters;
    std::vector<std::string> modes;
  };

  static Location parseLocation(const std::string &location) {
    Location result;
    const auto hash = location.find('#');
    result.url = location.substr(0, hash);
    if (hash == std::string::npos) return result;
    result.hasFragment = true;
    for (const auto &parameter: CT::split(location.substr(hash + 1), "&")) {
      const auto equals = parameter.find('=');
      const std::string key = parameter.substr(0, equals);
      const std::string value = equals == std::string::npos ? "" : parameter.substr(equals + 1);
      result.parameters[key] = value;
      if (key == "mode") {
        for (const auto &mode: CT::split(value, ",")) result.modes.push_back(mode);
      }
    }
    return result;
  }

  static bool hasMode(const Location &location, const char *mode) { return std::find(location.modes.begin(), location.modes.end(), mode) != location.modes.end(); }

  bool isZarrLocation(const std::string &location) {
    const Location parsed = parseLocation(location);
    if (parsed.hasFragment) {
      // Zarr stores in a zip file are left to the NetCDF library
      if (hasMode(parsed, "zip")) return false;
      return hasMode(parsed, "zarr") || hasMode(parsed, "nczarr") || hasMode(parsed, "xarray");
    }
    std::string path = parsed.url;
    while (path.size() > 1 && path.back() == '/') path.pop_back();
    return CT::endsWith(CT::toLowerCase(path), ".zarr");
  }

  /* Percent-encodes everything except the unreserved characters, and the slash when keepSlash is set */
  static std::string percentEncode(const std::string &value, bool keepSlash) {
    static const char *hex = "0123456789ABCDEF";
    std::string result;
    for (const unsigned char c: value) {
      if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || (keepSlash && c == '/')) {
        result += (char)c;
      } else {
        result += '%';
        result += hex[c >> 4];
        result += hex[c & 15];
      }
    }
    return result;
  }

  /*******************************************************************************************************************
   * Directory on disk
   *******************************************************************************************************************/

  class LocalStore : public Store {
    std::string root;

  public:
    explicit LocalStore(const std::string &root) : root(root) {}

    std::string describe(const std::string &key) const override { return key.empty() ? root : root + "/" + key; }

    int fetch(const std::vector<std::string> &keys, const FetchCallback &callback) override {
      for (size_t index = 0; index < keys.size(); index++) {
        const std::string path = describe(keys[index]);
        std::string data;
        FILE *file = fopen(path.c_str(), "rb");
        if (file == nullptr) {
          if (errno == ENOENT || errno == ENOTDIR) {
            callback(index, false, data);
            continue;
          }
          CDBError("Unable to read Zarr key [%s]: %s", path.c_str(), strerror(errno));
          return 1;
        }
        struct stat fileStatus;
        bool ok = fstat(fileno(file), &fileStatus) == 0 && S_ISREG(fileStatus.st_mode);
        if (ok) {
          data.resize(fileStatus.st_size);
          ok = data.empty() || fread(data.data(), 1, data.size(), file) == data.size();
        }
        fclose(file);
        if (!ok) {
          CDBError("Unable to read Zarr key [%s]", path.c_str());
          return 1;
        }
        callback(index, true, data);
      }
      return 0;
    }

    int listChildren(const std::string &prefix, std::vector<std::string> &children) override {
      const std::string path = describe(prefix);
      DIR *directory = opendir(path.c_str());
      if (directory == nullptr) {
        CDBError("Unable to list Zarr directory [%s]: %s", path.c_str(), strerror(errno));
        return 1;
      }
      while (struct dirent *entry = readdir(directory)) {
        const std::string name = entry->d_name;
        if (name != "." && name != "..") children.push_back(name);
      }
      closedir(directory);
      std::sort(children.begin(), children.end());
      return 0;
    }
  };

  /*******************************************************************************************************************
   * HTTP(S), optionally signed for S3
   *******************************************************************************************************************/

  /**
   * One multi handle for the whole process: its connection cache keeps connections open between requests and between reads,
   * when the server allows that, and libcurl keeps the parsed CA certificates in it.
   */
  static CURLM *getMultiHandle() {
    static CURLM *multi = nullptr;
    static pid_t owner = 0;
    if (multi != nullptr && owner != getpid()) {
      // This is a forked child: the connections in the handle belong to the parent. Leave them alone and start over.
      multi = nullptr;
    }
    if (multi == nullptr) {
      multi = curl_multi_init();
      if (multi == nullptr) return nullptr;
      curl_multi_setopt(multi, CURLMOPT_MAX_HOST_CONNECTIONS, (long)getMaxConnections());
      curl_multi_setopt(multi, CURLMOPT_MAX_TOTAL_CONNECTIONS, (long)getMaxConnections());
      owner = getpid();
    }
    return multi;
  }

  /**
   * libcurl only keeps the parsed CA certificates in memory between connections when they come from a single CA file. When a CA
   * directory is configured as well, which is the default on Debian and Ubuntu, the CA file is parsed again for every new
   * connection, which takes about 10 ms. With an S3 server that closes the connection after each response, that is most of
   * the time of a request. Clearing the CA directory when there is a CA file avoids this; the CA file holds the same certificates.
   */
  static bool shouldClearDefaultCAPath() {
    static const bool clear = [] {
      bool result = false;
#if LIBCURL_VERSION_NUM >= 0x075400 /* CURLINFO_CAINFO and CURLINFO_CAPATH are available since 7.84.0 */
      CURL *curl = curl_easy_init();
      if (curl != nullptr) {
        char *caInfo = nullptr, *caPath = nullptr;
        if (curl_easy_getinfo(curl, CURLINFO_CAINFO, &caInfo) == CURLE_OK && curl_easy_getinfo(curl, CURLINFO_CAPATH, &caPath) == CURLE_OK) {
          result = caInfo != nullptr && caPath != nullptr && *caPath != 0 && access(caInfo, R_OK) == 0;
        }
        curl_easy_cleanup(curl);
      }
#endif
      return result;
    }();
    return clear;
  }

  static size_t appendToString(char *data, size_t size, size_t count, void *destination) {
    static_cast<std::string *>(destination)->append(data, size * count);
    return size * count;
  }

  static bool isRetryable(CURLcode result, long httpStatus) {
    if (result == CURLE_OK) return httpStatus >= 500 || httpStatus == 429;
    return result == CURLE_COULDNT_CONNECT || result == CURLE_SEND_ERROR || result == CURLE_RECV_ERROR || result == CURLE_GOT_NOTHING || result == CURLE_PARTIAL_FILE ||
           result == CURLE_OPERATION_TIMEDOUT || result == CURLE_SSL_CONNECT_ERROR;
  }

  /* The text of every <tag>...</tag> in xml, with the XML entities decoded */
  static std::vector<std::string> getElementTexts(const std::string &xml, const std::string &tag) {
    static const std::pair<const char *, const char *> entities[] = {{"&lt;", "<"}, {"&gt;", ">"}, {"&quot;", "\""}, {"&apos;", "'"}, {"&amp;", "&"}};
    std::vector<std::string> texts;
    const std::string open = "<" + tag + ">", close = "</" + tag + ">";
    size_t position = 0;
    while ((position = xml.find(open, position)) != std::string::npos) {
      const auto end = xml.find(close, position + open.size());
      if (end == std::string::npos) break;
      std::string text = xml.substr(position + open.size(), end - position - open.size());
      for (const auto &entity: entities) text = CT::replace(text, entity.first, entity.second);
      texts.push_back(text);
      position = end + close.size();
    }
    return texts;
  }

  class HttpStore : public Store {
    std::string baseUrl; // Without trailing slash, like https://host:7443/bucket/prefix/store.zarr
    bool isS3;
    std::string signature, credentials;
    struct curl_slist *headers = nullptr;

    struct Transfer {
      size_t index = 0;
      std::string url, body;
      int attempts = 0;
      bool added = false;
      CURL *curl = nullptr;
      char errorBuffer[CURL_ERROR_SIZE] = "";
    };

    void setOptions(CURL *curl, const std::string &url, std::string *body, char *errorBuffer) const {
      curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
      curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
      curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT_SECONDS);
      curl_easy_setopt(curl, CURLOPT_TIMEOUT, REQUEST_TIMEOUT_SECONDS);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, appendToString);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, body);
      curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errorBuffer);
      if (shouldClearDefaultCAPath()) curl_easy_setopt(curl, CURLOPT_CAPATH, nullptr);
      if (!signature.empty()) {
        curl_easy_setopt(curl, CURLOPT_AWS_SIGV4, signature.c_str());
        curl_easy_setopt(curl, CURLOPT_USERPWD, credentials.c_str());
      }
      if (headers != nullptr) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    }

    /* Endpoint and bucket of an S3 store, and the key of the store in the bucket. Path-style addressing: endpoint/bucket/key */
    bool splitS3Url(std::string &endpoint, std::string &bucket, std::string &storeKey) const {
      const auto schemeEnd = baseUrl.find("://");
      if (schemeEnd == std::string::npos) return false;
      const auto bucketStart = baseUrl.find('/', schemeEnd + 3);
      if (bucketStart == std::string::npos) return false;
      const auto bucketEnd = baseUrl.find('/', bucketStart + 1);
      endpoint = baseUrl.substr(0, bucketStart);
      bucket = baseUrl.substr(bucketStart + 1, bucketEnd == std::string::npos ? std::string::npos : bucketEnd - bucketStart - 1);
      storeKey = bucketEnd == std::string::npos ? "" : baseUrl.substr(bucketEnd + 1);
      return !bucket.empty();
    }

  public:
    HttpStore(const std::string &baseUrl, bool isS3, const std::string &region) : baseUrl(baseUrl), isS3(isS3) {
      if (!isS3) return;
      const std::string accessKey = getEnvironment("AWS_ACCESS_KEY_ID"), secretKey = getEnvironment("AWS_SECRET_ACCESS_KEY");
      // Without credentials the requests are not signed, which works for public buckets.
      if (accessKey.empty() || secretKey.empty()) return;
      signature = "aws:amz:" + region + ":s3";
      credentials = accessKey + ":" + secretKey;
      const std::string sessionToken = getEnvironment("AWS_SESSION_TOKEN");
      if (!sessionToken.empty()) headers = curl_slist_append(headers, ("x-amz-security-token: " + sessionToken).c_str());
    }

    ~HttpStore() { curl_slist_free_all(headers); }

    std::string describe(const std::string &key) const override { return key.empty() ? baseUrl : baseUrl + "/" + percentEncode(key, true); }

    int fetch(const std::vector<std::string> &keys, const FetchCallback &callback) override {
      if (keys.empty()) return 0;
      CURLM *multi = getMultiHandle();
      if (multi == nullptr) {
        CDBError("Unable to initialize curl");
        return 1;
      }
      const size_t maxActive = getMaxConnections();
      std::vector<std::unique_ptr<Transfer>> transfers(keys.size());
      size_t next = 0, active = 0;
      int status = 0;

      auto start = [&](Transfer &transfer) {
        transfer.body.clear();
        transfer.errorBuffer[0] = 0;
        transfer.attempts++;
        if (transfer.curl == nullptr) {
          transfer.curl = curl_easy_init();
          if (transfer.curl == nullptr) return false;
          setOptions(transfer.curl, transfer.url, &transfer.body, transfer.errorBuffer);
          curl_easy_setopt(transfer.curl, CURLOPT_PRIVATE, &transfer);
        }
        if (curl_multi_add_handle(multi, transfer.curl) != CURLM_OK) return false;
        transfer.added = true;
        active++;
        return true;
      };
      auto finish = [&](Transfer &transfer) {
        curl_easy_cleanup(transfer.curl);
        transfer.curl = nullptr;
        transfers[transfer.index].reset();
      };

      while (status == 0 && (next < keys.size() || active > 0)) {
        while (active < maxActive && next < keys.size()) {
          transfers[next] = std::make_unique<Transfer>();
          Transfer &transfer = *transfers[next];
          transfer.index = next;
          transfer.url = describe(keys[next]);
          next++;
          if (!start(transfer)) {
            CDBError("Unable to start a request for [%s]", transfer.url.c_str());
            status = 1;
            break;
          }
        }
        if (status != 0) break;

        int running = 0;
        CURLMcode multiResult = curl_multi_perform(multi, &running);
        if (multiResult != CURLM_OK) {
          CDBError("Unable to do requests: %s", curl_multi_strerror(multiResult));
          status = 1;
          break;
        }
        int queued = 0;
        while (CURLMsg *message = curl_multi_info_read(multi, &queued)) {
          if (message->msg != CURLMSG_DONE) continue;
          CURL *curl = message->easy_handle;
          const CURLcode result = message->data.result;
          Transfer *transfer = nullptr;
          curl_easy_getinfo(curl, CURLINFO_PRIVATE, (char **)&transfer);
          long httpStatus = 0;
          curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);
          curl_multi_remove_handle(multi, curl);
          transfer->added = false;
          active--;
          if (status != 0) continue;
          if (result == CURLE_OK && (httpStatus == 200 || httpStatus == 206)) {
            callback(transfer->index, true, transfer->body);
            finish(*transfer);
          } else if (result == CURLE_OK && httpStatus == 404) {
            callback(transfer->index, false, transfer->body);
            finish(*transfer);
          } else if (transfer->attempts < MAX_ATTEMPTS && isRetryable(result, httpStatus)) {
            if (!start(*transfer)) {
              CDBError("Unable to start a request for [%s]", transfer->url.c_str());
              status = 1;
            }
          } else if (result != CURLE_OK) {
            CDBError("Unable to read [%s]: %s", transfer->url.c_str(), transfer->errorBuffer[0] != 0 ? transfer->errorBuffer : curl_easy_strerror(result));
            status = 1;
          } else {
            CDBError("Unable to read [%s]: HTTP status %ld: %s", transfer->url.c_str(), httpStatus, transfer->body.substr(0, 300).c_str());
            status = 1;
          }
        }
        if (status == 0 && active > 0) {
          multiResult = curl_multi_poll(multi, nullptr, 0, 1000, nullptr);
          if (multiResult != CURLM_OK) {
            CDBError("Unable to wait for requests: %s", curl_multi_strerror(multiResult));
            status = 1;
          }
        }
      }

      // After an error, stop the requests that are still running
      for (auto &transfer: transfers) {
        if (transfer == nullptr || transfer->curl == nullptr) continue;
        if (transfer->added) curl_multi_remove_handle(multi, transfer->curl);
        curl_easy_cleanup(transfer->curl);
      }
      return status;
    }

    int listChildren(const std::string &prefix, std::vector<std::string> &children) override {
      std::string endpoint, bucket, storeKey;
      if (!isS3 || !splitS3Url(endpoint, bucket, storeKey)) {
        CDBError("Unable to list [%s]: this is only possible on S3. Consolidate the metadata of the store (.zmetadata) to read it over HTTP.", describe(prefix).c_str());
        return 1;
      }
      std::string listPrefix = storeKey.empty() ? "" : storeKey + "/";
      if (!prefix.empty()) listPrefix += prefix + "/";

      CURL *curl = curl_easy_init();
      if (curl == nullptr) {
        CDBError("Unable to initialize curl");
        return 1;
      }
      std::string continuationToken, body, url;
      char errorBuffer[CURL_ERROR_SIZE] = "";
      int status = 0;
      do {
        // The query parameters are in alphabetical order, as the request signature requires.
        url = endpoint + "/" + bucket + "?";
        if (!continuationToken.empty()) url += "continuation-token=" + percentEncode(continuationToken, false) + "&";
        url += "delimiter=%2F&list-type=2&prefix=" + percentEncode(listPrefix, false);
        body.clear();
        setOptions(curl, url, &body, errorBuffer);
        const CURLcode result = curl_easy_perform(curl);
        long httpStatus = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatus);
        if (result != CURLE_OK || httpStatus != 200) {
          CDBError("Unable to list [%s]: %s", url.c_str(), result != CURLE_OK ? curl_easy_strerror(result) : ("HTTP status " + std::to_string(httpStatus) + ": " + body.substr(0, 300)).c_str());
          status = 1;
          break;
        }
        std::vector<std::string> keys = getElementTexts(body, "Prefix"), contents = getElementTexts(body, "Key");
        keys.insert(keys.end(), contents.begin(), contents.end());
        for (std::string key: keys) {
          if (!CT::startsWith(key, listPrefix) || key.size() == listPrefix.size()) continue;
          key = key.substr(listPrefix.size());
          if (!key.empty() && key.back() == '/') key.pop_back();
          if (!key.empty() && key.find('/') == std::string::npos) children.push_back(key);
        }
        continuationToken.clear();
        const auto truncated = getElementTexts(body, "IsTruncated");
        const auto tokens = getElementTexts(body, "NextContinuationToken");
        if (!truncated.empty() && truncated[0] == "true" && !tokens.empty()) continuationToken = tokens[0];
      } while (!continuationToken.empty());
      curl_easy_cleanup(curl);
      std::sort(children.begin(), children.end());
      children.erase(std::unique(children.begin(), children.end()), children.end());
      return status;
    }
  };

  std::unique_ptr<Store> openStore(const std::string &location) {
    Location parsed = parseLocation(location);
    std::string url = parsed.url;
    while (url.size() > 1 && url.back() == '/') url.pop_back();

    std::string region = parsed.parameters["aws.region"];
    if (region.empty()) region = getEnvironment("AWS_REGION");
    if (region.empty()) region = getEnvironment("AWS_DEFAULT_REGION");
    if (region.empty()) region = "us-east-1";

    if (CT::startsWith(url, "s3://")) {
      const std::string bucketAndKey = url.substr(5);
      if (bucketAndKey.empty() || bucketAndKey[0] == '/') {
        CDBError("Unable to open Zarr store [%s]: expected s3://<bucket>/<key>", location.c_str());
        return nullptr;
      }
      return std::make_unique<HttpStore>("https://s3." + region + ".amazonaws.com/" + bucketAndKey, true, region);
    }
    if (CT::startsWith(url, "http://") || CT::startsWith(url, "https://")) {
      return std::make_unique<HttpStore>(url, hasMode(parsed, "s3"), region);
    }
    if (CT::startsWith(url, "file://")) url = url.substr(7);
    return std::make_unique<LocalStore>(url);
  }

} // namespace Zarr
