/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Reader for Zarr v2 stores, on disk or over HTTP(S) / S3
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

#ifndef CCDFZARRIO_H
#define CCDFZARRIO_H

#include <map>
#include <memory>
#include <string>
#include "CCDFReader.h"

namespace Zarr {
  class Store;
  struct Array;
  /** Chunks as stored, by key: whether the chunk exists, and its data */
  typedef std::map<std::string, std::pair<bool, std::string>> PrefetchedChunks;
} // namespace Zarr

/** The global attribute that the reader adds to the data model, to tell that the data was read by this reader */
#define ZARR_READER_ATTRIBUTE_NAME "adaguc_data_reader"
#define ZARR_READER_ATTRIBUTE_VALUE "ADAGUC-ZARR"

/**
 * Reads Zarr v2 stores into the CDF data model, like the NetCDF library presents them:
 * - Arrays become variables, named by their path in the store (group/name). The dimensions come from the _ARRAY_DIMENSIONS
 *   attribute that xarray writes, or from the dimension references of NCZarr.
 * - The metadata is read with one request from the consolidated metadata (.zmetadata), or else from the .zgroup, .zarray and
 *   .zattrs of every group and array.
 * - The coordinate variables and single values in one small chunk are fetched at the same time when the store is opened.
 * - Reading data only fetches the chunks that hold the requested part. They are fetched in parallel, and decoded on a pool
 *   of threads. A chunk that does not exist holds only the fill value.
 * - When an array has a fill_value but no _FillValue attribute, the fill_value becomes the _FillValue, like xarray does.
 * - The global attribute adaguc_data_reader = "ADAGUC-ZARR" tells that the data was read by this reader.
 *
 * Supported: numeric and boolean data types in both byte orders, fixed length (S, U) and variable length (vlen-utf8) strings,
 * C and F order, the zlib, gzip, zstd and blosc compressors and the shuffle filter.
 *
 * Settings: ADAGUC_ZARR_MAX_CONNECTIONS (default 16) limits the requests at the same time, ADAGUC_ZARR_THREADS (default the
 * number of processors, at most 8) the threads that decode chunks.
 */
class CDFZarrReader : public CDFReader {
public:
  CDFZarrReader();
  ~CDFZarrReader();
  int open(const char *fileName);
  int close();
  int _readVariableData(CDF::Variable *var, CDFType type);
  int _readVariableData(CDF::Variable *var, CDFType type, size_t *start, size_t *count, ptrdiff_t *stride);

  /** True when the location is a Zarr store that this reader opens, see Zarr::isZarrLocation */
  static bool isZarrLocation(const std::string &location);

private:
  std::unique_ptr<Zarr::Store> store;
  std::map<std::string, std::shared_ptr<Zarr::Array>> arrays; // By variable name
  Zarr::PrefetchedChunks prefetchedChunks;                     // Fetched when the store was opened, see prefetchSmallArrays

  /** Fetches the coordinate variables and single values that are stored in one small chunk, all at the same time */
  void prefetchSmallArrays();
};

#endif
