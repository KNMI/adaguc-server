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

#include "ZarrCodecs.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <zlib.h>
#ifdef ADAGUC_HAVE_ZSTD
#include <zstd.h>
#endif
#ifdef ADAGUC_HAVE_BLOSC
#include <blosc.h>
#endif

namespace Zarr {

  const char *VLEN_UTF8 = "vlen-utf8";

  static std::string getId(const nlohmann::ordered_json &codec) {
    if (!codec.is_object() || !codec.contains("id") || !codec["id"].is_string()) return "";
    return codec["id"].get<std::string>();
  }

  static std::string checkCompressor(const nlohmann::ordered_json &compressor) {
    if (compressor.is_null()) return "";
    const std::string id = getId(compressor);
    if (id == "zlib" || id == "gzip") return "";
#ifdef ADAGUC_HAVE_ZSTD
    if (id == "zstd") return "";
#endif
#ifdef ADAGUC_HAVE_BLOSC
    if (id == "blosc") return "";
#endif
    return "compressor [" + compressor.dump() + "] is not supported";
  }

  std::string checkCodecs(const nlohmann::ordered_json &compressor, const nlohmann::ordered_json &filters) {
    std::string problem = checkCompressor(compressor);
    if (!problem.empty()) return problem;
    if (filters.is_null()) return "";
    if (!filters.is_array()) return "filters [" + filters.dump() + "] is not a list";
    for (const auto &filter: filters) {
      const std::string id = getId(filter);
      // bitround only zeroes bits when encoding: decoding does nothing
      if (id != "shuffle" && id != "bitround" && id != VLEN_UTF8) return "filter [" + filter.dump() + "] is not supported";
    }
    return "";
  }

  /* Inflates zlib (windowBits 15) or gzip (windowBits 15 + 16) data */
  static std::string inflateData(const std::string &input, int windowBits, size_t expectedSize, std::string &output) {
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    if (inflateInit2(&stream, windowBits) != Z_OK) return "unable to initialize zlib";
    output.resize(expectedSize > 0 ? expectedSize : std::max<size_t>(input.size() * 4, 1024));
    stream.next_in = (Bytef *)input.data();
    stream.avail_in = (uInt)input.size();
    std::string error;
    while (true) {
      if (stream.total_out == output.size()) output.resize(output.size() * 2);
      stream.next_out = (Bytef *)output.data() + stream.total_out;
      stream.avail_out = (uInt)(output.size() - stream.total_out);
      const int result = inflate(&stream, Z_NO_FLUSH);
      if (result == Z_STREAM_END) break;
      if (result != Z_OK && result != Z_BUF_ERROR) {
        error = std::string("zlib: ") + (stream.msg != nullptr ? stream.msg : "unable to inflate");
        break;
      }
      if (stream.avail_in == 0 && stream.avail_out > 0) {
        error = "zlib: the compressed data ends too early";
        break;
      }
    }
    output.resize(stream.total_out);
    inflateEnd(&stream);
    return error;
  }

#ifdef ADAGUC_HAVE_ZSTD
  static std::string decompressZstd(const std::string &input, size_t expectedSize, std::string &output) {
    const unsigned long long frameSize = ZSTD_getFrameContentSize(input.data(), input.size());
    if (frameSize != ZSTD_CONTENTSIZE_UNKNOWN && frameSize != ZSTD_CONTENTSIZE_ERROR) {
      output.resize(frameSize);
      const size_t result = ZSTD_decompress(output.data(), output.size(), input.data(), input.size());
      if (ZSTD_isError(result)) return std::string("zstd: ") + ZSTD_getErrorName(result);
      output.resize(result);
      return "";
    }
    // The size is not in the frame: decompress as a stream
    ZSTD_DCtx *context = ZSTD_createDCtx();
    if (context == nullptr) return "unable to initialize zstd";
    output.resize(expectedSize > 0 ? expectedSize : std::max<size_t>(input.size() * 4, 1024));
    ZSTD_inBuffer in = {input.data(), input.size(), 0};
    ZSTD_outBuffer out = {output.data(), output.size(), 0};
    std::string error;
    while (in.pos < in.size) {
      if (out.pos == out.size) {
        output.resize(output.size() * 2);
        out.dst = output.data();
        out.size = output.size();
      }
      const size_t result = ZSTD_decompressStream(context, &out, &in);
      if (ZSTD_isError(result)) {
        error = std::string("zstd: ") + ZSTD_getErrorName(result);
        break;
      }
    }
    output.resize(out.pos);
    ZSTD_freeDCtx(context);
    return error;
  }
#endif

#ifdef ADAGUC_HAVE_BLOSC
  static std::string decompressBlosc(const std::string &input, std::string &output) {
    size_t numBytes = 0, compressedBytes = 0, blockSize = 0;
    if (input.size() < BLOSC_MIN_HEADER_LENGTH) return "blosc: the compressed data is too short";
    blosc_cbuffer_sizes(input.data(), &numBytes, &compressedBytes, &blockSize);
    if (compressedBytes > input.size()) return "blosc: the compressed data ends too early";
    output.resize(numBytes);
    // The _ctx variant is thread safe, unlike blosc_decompress
    if (numBytes > 0 && blosc_decompress_ctx(input.data(), output.data(), output.size(), 1) < 0) return "blosc: unable to decompress";
    return "";
  }
#endif

  /* Undoes the byte shuffle of numcodecs: the first bytes of all elements come first, then the second bytes, and so on */
  static void unshuffle(std::string &data, size_t elementSize) {
    if (elementSize <= 1) return;
    const size_t numElements = data.size() / elementSize;
    std::string output(data.size(), 0);
    for (size_t byte = 0; byte < elementSize; byte++) {
      const char *source = data.data() + byte * numElements;
      char *destination = output.data() + byte;
      for (size_t element = 0; element < numElements; element++) {
        destination[element * elementSize] = source[element];
      }
    }
    // Bytes that do not make up a whole element are not shuffled
    const size_t shuffled = numElements * elementSize;
    memcpy(output.data() + shuffled, data.data() + shuffled, data.size() - shuffled);
    data.swap(output);
  }

  std::string decodeChunk(const nlohmann::ordered_json &compressor, const nlohmann::ordered_json &filters, std::string &data, size_t expectedSize) {
    const std::string id = getId(compressor);
    if (!compressor.is_null()) {
      std::string decompressed, error;
      if (id == "zlib") {
        error = inflateData(data, 15, expectedSize, decompressed);
      } else if (id == "gzip") {
        error = inflateData(data, 15 + 16, expectedSize, decompressed);
#ifdef ADAGUC_HAVE_ZSTD
      } else if (id == "zstd") {
        error = decompressZstd(data, expectedSize, decompressed);
#endif
#ifdef ADAGUC_HAVE_BLOSC
      } else if (id == "blosc") {
        error = decompressBlosc(data, decompressed);
#endif
      } else {
        error = "compressor [" + id + "] is not supported";
      }
      if (!error.empty()) return error;
      data.swap(decompressed);
    }
    if (filters.is_array()) {
      for (auto filter = filters.rbegin(); filter != filters.rend(); ++filter) {
        const std::string filterId = getId(*filter);
        if (filterId == "shuffle") {
          size_t elementSize = 4;
          if (filter->contains("elementsize") && (*filter)["elementsize"].is_number_integer()) elementSize = (*filter)["elementsize"].get<size_t>();
          unshuffle(data, elementSize);
        } else if (filterId == "bitround" || filterId == VLEN_UTF8) {
          // Nothing to undo here
        } else {
          return "filter [" + filterId + "] is not supported";
        }
      }
    }
    return "";
  }

  std::string decodeVlenUtf8(const std::string &data, std::vector<std::string> &strings) {
    auto readUint32 = [&data](size_t position) {
      const unsigned char *bytes = (const unsigned char *)data.data() + position;
      return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) | ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
    };
    if (data.size() < 4) return "vlen-utf8: the chunk is too short";
    const uint32_t count = readUint32(0);
    strings.clear();
    strings.reserve(count);
    size_t position = 4;
    for (uint32_t index = 0; index < count; index++) {
      if (position + 4 > data.size()) return "vlen-utf8: the chunk ends too early";
      const uint32_t length = readUint32(position);
      position += 4;
      if (position + length > data.size()) return "vlen-utf8: the chunk ends too early";
      strings.emplace_back(data, position, length);
      position += length;
    }
    return "";
  }

} // namespace Zarr
