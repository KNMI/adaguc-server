/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Decoding of Zarr v2 chunks: compressors and filters
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

#ifndef ZARRCODECS_H
#define ZARRCODECS_H

#include <string>
#include <vector>
#include "json.hpp"

namespace Zarr {

  /** The id of the filter that stores strings of variable length, which the reader decodes itself */
  extern const char *VLEN_UTF8;

  /**
   * Checks if the compressor (null or an object with "id") and the filters (null or a list of objects with "id") of a .zarray
   * can be decoded.
   * @return Empty when they can, otherwise the reason why not.
   */
  std::string checkCodecs(const nlohmann::ordered_json &compressor, const nlohmann::ordered_json &filters);

  /**
   * Decodes a chunk as it is stored to the bytes of the array: decompresses it, then undoes the filters, the last one first.
   * The vlen-utf8 filter is left to the caller. zlib and gzip are inflated with libdeflate when adaguc-server was built with
   * it and the size of the chunk is known, which is about 2.5 times faster than zlib; otherwise with zlib.
   * Thread safe.
   * @param data The stored chunk, replaced by the decoded chunk
   * @param expectedSize The size of the decoded chunk in bytes, or 0 when not known in advance
   * @return Empty on success, otherwise the error.
   */
  std::string decodeChunk(const nlohmann::ordered_json &compressor, const nlohmann::ordered_json &filters, std::string &data, size_t expectedSize);

  /**
   * Decodes a chunk that was encoded with the vlen-utf8 filter: a little endian uint32 with the number of strings, followed
   * by a little endian uint32 length and the bytes of every string.
   * @return Empty on success, otherwise the error.
   */
  std::string decodeVlenUtf8(const std::string &data, std::vector<std::string> &strings);

} // namespace Zarr

#endif
