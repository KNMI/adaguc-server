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

#ifndef S3LISTER_H
#define S3LISTER_H

#include <string>
#include <vector>

/**
 * A directory of Zarr stores on S3 is written like the NetCDF library writes the URL of a single store, but ending in a slash:
 *
 *   single store: https://<host>[:<port>]/<bucket>/[<prefix>/]<name>.zarr#mode=zarr,s3
 *   directory:    https://<host>[:<port>]/<bucket>/[<prefix>/]#mode=zarr,s3
 *
 * Listing a directory gives the URLs of the stores below it. These are what the NetCDF library opens,
 * and what is kept as "file" in the database.
 */
struct S3Directory {
  std::string endpoint; // https://<host>[:<port>]
  std::string bucket;
  std::string prefix;   // Empty, or ending in a slash
  std::string fragment; // Including the #, for example #mode=zarr,s3
};

/** One page of an S3 ListObjectsV2 response that was requested with a delimiter. */
struct S3ListPage {
  std::vector<std::string> prefixes; // The "subdirectories": keys up to and including the next slash
  std::string nextContinuationToken; // Set when there are more pages
};

/** True when the location is an https URL with "s3" in its mode, of which the path ends in a slash. */
bool isS3Directory(const std::string &location);

/** Splits the location of a directory into its parts. Returns false if it is not a S3 directory. */
bool parseS3Directory(const std::string &location, S3Directory &directory);

/** Extracts the prefixes and the continuation token from the XML of a ListObjectsV2 response. */
S3ListPage parseS3ListPage(const std::string &xml);

/**
 * The name of a store as the FilePath filter sees it: the last part of the path, without the fragment.
 * For https://host/bucket/a/b.zarr#mode=zarr,s3 that is b.zarr.
 */
std::string getS3StoreName(const std::string &storeLocation);

/** True when the store lies in the directory, at any depth, and its name matches the filter. */
bool isS3StoreInDirectory(const std::string &storeLocation, const std::string &directoryLocation, const std::string &filter);

/**
 * True when the store at the given location (a single store, as listed by listS3Stores) is complete: its
 * .zmetadata exists. Writers create that object last, so a store that is still being written is not complete.
 * Costs one request per store, so it is meant for stores that are new to the index only.
 */
bool isS3StoreComplete(const std::string &storeLocation);

/**
 * Lists the Zarr stores in a S3 directory, including those in subdirectories.
 *
 * A store is a key prefix ending in ".zarr/". Stores are not descended into.
 * Credentials and region come from AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY and AWS_REGION, the same variables
 * the NetCDF library uses to open the stores. Without credentials the request is not signed.
 *
 * @param location The directory, see S3Directory
 * @param filter POSIX regexp that the name of a store must match, for example ^radar_.*\.zarr$. Empty matches all stores.
 * @param subDirectory Optional: only list this subdirectory of the location
 * @returns The sorted locations of the stores, each with the fragment of the directory
 * @throws int when the bucket cannot be listed
 */
std::vector<std::string> listS3Stores(const std::string &location, const std::string &filter, const std::string &subDirectory = "");

#endif
