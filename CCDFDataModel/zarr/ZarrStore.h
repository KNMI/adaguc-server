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

#ifndef ZARRSTORE_H
#define ZARRSTORE_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Zarr {

  /**
   * Called once for every key that was found or is missing, on the thread that called Store::fetch, in the order in which the
   * keys arrive. The data may be moved away by the callback.
   * @param index The index of the key in the list given to Store::fetch
   * @param found False when the key does not exist; a missing chunk means that the chunk only contains the fill value.
   */
  typedef std::function<void(size_t index, bool found, std::string &data)> FetchCallback;

  class Store {
  public:
    virtual ~Store() {}

    /**
     * Fetches the keys, as many at the same time as the store allows.
     * @return 0 when every key was found or is missing, otherwise nonzero; the error is logged.
     */
    virtual int fetch(const std::vector<std::string> &keys, const FetchCallback &callback) = 0;

    /**
     * Lists the names of the keys and "directories" directly below a prefix, for stores without consolidated metadata.
     * @param prefix Empty for the root of the store, otherwise a path like "group/array" without a trailing slash
     * @return 0 on success, otherwise nonzero; the error is logged.
     */
    virtual int listChildren(const std::string &prefix, std::vector<std::string> &children) = 0;

    /** The location of a key, for messages */
    virtual std::string describe(const std::string &key) const = 0;

    /** True when every fetch is a request over the network, as for HTTP(S) and S3; false for a directory on disk */
    virtual bool isRemote() const { return false; }
  };

  /**
   * True when the location is a Zarr store:
   * - its fragment has a mode list with zarr or nczarr, like the NetCDF library uses: https://host/bucket/store.zarr#mode=zarr,s3
   * - or, without a fragment, its path ends in .zarr: /data/store.zarr
   */
  bool isZarrLocation(const std::string &location);

  /**
   * Opens a store. Supported locations:
   * - A directory on disk: /data/store.zarr or file:///data/store.zarr#mode=zarr,file
   * - HTTP(S): https://host/path/store.zarr#mode=zarr. With s3 in the mode the requests are signed (AWS signature version 4)
   *   with AWS_ACCESS_KEY_ID / AWS_SECRET_ACCESS_KEY (and AWS_SESSION_TOKEN), for the region in the fragment (aws.region=...),
   *   AWS_REGION, AWS_DEFAULT_REGION or us-east-1.
   * - s3://bucket/path/store.zarr, which is read from https://s3.<region>.amazonaws.com/bucket/path/store.zarr
   * @return The store, or nullptr with the error logged.
   */
  std::unique_ptr<Store> openStore(const std::string &location);

  /**
   * Loads the CA certificates for HTTPS once per process. This happens with the first request anyway; the fork server calls it
   * before it forks, so that its children do not parse the certificates for every request.
   */
  void loadCACertificates();

  /** The number of HTTP requests that may run at the same time, ADAGUC_ZARR_MAX_CONNECTIONS (default 16). */
  size_t getMaxConnections();

} // namespace Zarr

#endif
