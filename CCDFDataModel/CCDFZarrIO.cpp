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

#include "CCDFZarrIO.h"
#include "CDFCopyData.h"
#include "CDebugger.h"
#include "CTString.h"
#include "traceTimings/traceTimings.h"
#include "zarr/ZarrChunkCache.h"
#include "zarr/ZarrCodecs.h"
#include "zarr/ZarrStore.h"
#include "zarr/ZarrWorkerPool.h"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <thread>
#include <type_traits>

using ordered_json = nlohmann::ordered_json;

static const bool CCDFZARRIO_DEBUG = false;

namespace Zarr {
  enum class Kind {
    Numeric,      // Numbers and booleans
    FixedBytes,   // Strings of a fixed number of bytes (dtype S)
    FixedUnicode, // Strings of a fixed number of UTF-32 characters (dtype U)
    VlenUtf8      // Strings of any length (dtype O with the vlen-utf8 filter)
  };

  /** How an array is stored, from its .zarray */
  struct Array {
    std::string key; // The path of the array in the store, like group/temperature
    std::vector<size_t> shape, chunks;
    CDFType type = CDF_NONE;
    Kind kind = Kind::Numeric;
    size_t elementSize = 0; // Bytes of one element in a chunk; 0 for strings of any length
    bool swapBytes = false;
    bool fortranOrder = false;
    std::string separator = ".";
    ordered_json compressor, filters;
    std::string fillBytes; // One element with the fill value, in the byte order of this machine
    bool hasFillValue = false;
    std::string unsupported; // Why the data cannot be read; empty when it can
  };
} // namespace Zarr

namespace {

  size_t getNumThreads() {
    static const size_t numThreads = [] {
      const char *value = getenv("ADAGUC_ZARR_THREADS");
      if (value != nullptr && *value != 0) {
        char *end = nullptr;
        const long long parsed = strtoll(value, &end, 10);
        if (end != value && *end == 0 && parsed >= 1) return (size_t)parsed;
        CDBWarning("Ignoring ADAGUC_ZARR_THREADS=[%s], expected a whole number above 0", value);
      }
      return std::max<size_t>(1, std::min<size_t>(std::thread::hardware_concurrency(), 8));
    }();
    return numThreads;
  }

  const ordered_json &getOrNull(const ordered_json &object, const char *key) {
    static const ordered_json null;
    if (!object.is_object()) return null;
    const auto found = object.find(key);
    return found == object.end() ? null : *found;
  }

  /* Puts NaN, Infinity and -Infinity outside strings between quotes. Python writes these in JSON, but they are not valid JSON. */
  std::string quoteNonFiniteNumbers(const std::string &text) {
    std::string result;
    result.reserve(text.size());
    bool inString = false;
    for (size_t position = 0; position < text.size(); position++) {
      const char c = text[position];
      if (inString) {
        result += c;
        if (c == '\\' && position + 1 < text.size()) {
          result += text[++position];
        } else if (c == '"') {
          inString = false;
        }
        continue;
      }
      if (c == '"') inString = true;
      bool replaced = false;
      for (const char *token: {"-Infinity", "Infinity", "NaN"}) {
        if (text.compare(position, strlen(token), token) == 0) {
          result += std::string("\"") + token + "\"";
          position += strlen(token) - 1;
          replaced = true;
          break;
        }
      }
      if (!replaced) result += c;
    }
    return result;
  }

  bool parseJson(const std::string &text, ordered_json &result, const std::string &location) {
    try {
      result = ordered_json::parse(text);
      return true;
    } catch (const std::exception &) {
    }
    try {
      result = ordered_json::parse(quoteNonFiniteNumbers(text));
      return true;
    } catch (const std::exception &exception) {
      CDBError("Unable to parse [%s] as JSON: %s", location.c_str(), exception.what());
      return false;
    }
  }

  /* Fetches keys; results[i] tells if keys[i] was found, and its data */
  int fetchAll(Zarr::Store &store, const std::vector<std::string> &keys, std::vector<std::pair<bool, std::string>> &results) {
    results.assign(keys.size(), {false, ""});
    return store.fetch(keys, [&results](size_t index, bool found, std::string &data) {
      results[index].first = found;
      results[index].second.swap(data);
    });
  }

  /* A JSON number, boolean or one of the strings NaN, Infinity and -Infinity as double */
  bool jsonToDouble(const ordered_json &value, double &result) {
    if (value.is_number()) {
      result = value.get<double>();
      return true;
    }
    if (value.is_boolean()) {
      result = value.get<bool>() ? 1 : 0;
      return true;
    }
    if (value.is_string()) {
      const std::string text = value.get<std::string>();
      if (text == "NaN") {
        result = std::numeric_limits<double>::quiet_NaN();
      } else if (text == "Infinity") {
        result = std::numeric_limits<double>::infinity();
      } else if (text == "-Infinity") {
        result = -std::numeric_limits<double>::infinity();
      } else {
        return false;
      }
      return true;
    }
    return false;
  }

  template <typename T> bool jsonToNumber(const ordered_json &value, T &result) {
    if constexpr (std::is_integral_v<T>) {
      if (value.is_number_unsigned()) {
        result = (T)value.get<uint64_t>();
        return true;
      }
      if (value.is_number_integer()) {
        result = (T)value.get<int64_t>();
        return true;
      }
    }
    double number;
    if (!jsonToDouble(value, number)) return false;
    if constexpr (std::is_integral_v<T>) {
      if (!std::isfinite(number)) return false;
    }
    result = (T)number;
    return true;
  }

  template <typename T> std::string toBytes(T value) {
    std::string bytes(sizeof(T), 0);
    memcpy(bytes.data(), &value, sizeof(T));
    return bytes;
  }

  /* A JSON value as one element of a CDF type, in the byte order of this machine */
  bool jsonToElement(const ordered_json &value, CDFType type, std::string &bytes) {
#define ZARR_JSON_TO_ELEMENT(CDFTYPE, CTYPE)                                                                                                                                                           \
  case CDFTYPE: {                                                                                                                                                                                      \
    CTYPE number;                                                                                                                                                                                      \
    if (!jsonToNumber(value, number)) return false;                                                                                                                                                    \
    bytes = toBytes(number);                                                                                                                                                                           \
    return true;                                                                                                                                                                                       \
  }
    switch (type) {
      ZARR_JSON_TO_ELEMENT(CDF_BYTE, int8_t)
      ZARR_JSON_TO_ELEMENT(CDF_UBYTE, uint8_t)
      ZARR_JSON_TO_ELEMENT(CDF_SHORT, int16_t)
      ZARR_JSON_TO_ELEMENT(CDF_USHORT, uint16_t)
      ZARR_JSON_TO_ELEMENT(CDF_INT, int32_t)
      ZARR_JSON_TO_ELEMENT(CDF_UINT, uint32_t)
      ZARR_JSON_TO_ELEMENT(CDF_INT64, int64_t)
      ZARR_JSON_TO_ELEMENT(CDF_UINT64, uint64_t)
      ZARR_JSON_TO_ELEMENT(CDF_FLOAT, float)
      ZARR_JSON_TO_ELEMENT(CDF_DOUBLE, double)
    default:
      return false;
    }
#undef ZARR_JSON_TO_ELEMENT
  }

  /* Numbers become int when they all fit, else long when they are all integers, else double */
  CDF::Attribute *jsonNumbersToAttribute(const std::string &name, const ordered_json &numbers) {
    bool anyFloat = false, allFitInt = true, allFitLong = true;
    for (const auto &number: numbers) {
      if (number.is_number_float()) {
        anyFloat = true;
      } else if (number.is_number_unsigned()) {
        const uint64_t value = number.get<uint64_t>();
        if (value > (uint64_t)std::numeric_limits<int32_t>::max()) allFitInt = false;
        if (value > (uint64_t)std::numeric_limits<int64_t>::max()) allFitLong = false;
      } else {
        const int64_t value = number.get<int64_t>();
        if (value < std::numeric_limits<int32_t>::min() || value > std::numeric_limits<int32_t>::max()) allFitInt = false;
      }
    }
    if (anyFloat || !allFitLong) {
      std::vector<double> values;
      for (const auto &number: numbers) values.push_back(number.get<double>());
      return new CDF::Attribute(name.c_str(), CDF_DOUBLE, values.data(), values.size());
    }
    if (allFitInt) {
      std::vector<int> values;
      for (const auto &number: numbers) values.push_back((int)number.get<int64_t>());
      return new CDF::Attribute(name.c_str(), CDF_INT, values.data(), values.size());
    }
    std::vector<long> values;
    for (const auto &number: numbers) values.push_back((long)number.get<int64_t>());
    return new CDF::Attribute(name.c_str(), CDF_INT64, values.data(), values.size());
  }

  /* Returns nullptr for values that do not make an attribute: null and empty lists */
  CDF::Attribute *jsonToAttribute(const std::string &name, const ordered_json &value) {
    if (value.is_null()) return nullptr;
    if (value.is_string()) return new CDF::Attribute(name.c_str(), value.get<std::string>().c_str());
    if (value.is_boolean()) {
      const int number = value.get<bool>() ? 1 : 0;
      return new CDF::Attribute(name.c_str(), CDF_INT, &number, 1);
    }
    if (value.is_number()) return jsonNumbersToAttribute(name, ordered_json::array({value}));
    if (value.is_array()) {
      if (value.empty()) return nullptr;
      const bool allNumbers = std::all_of(value.begin(), value.end(), [](const ordered_json &item) { return item.is_number(); });
      if (allNumbers) return jsonNumbersToAttribute(name, value);
      const bool allStrings = std::all_of(value.begin(), value.end(), [](const ordered_json &item) { return item.is_string(); });
      if (allStrings) {
        std::vector<std::string> strings;
        for (const auto &item: value) strings.push_back(item.get<std::string>());
        std::vector<const char *> pointers;
        for (const auto &text: strings) pointers.push_back(text.c_str());
        return new CDF::Attribute(name.c_str(), CDF_STRING, pointers.data(), pointers.size());
      }
    }
    // Objects and mixed lists are kept as their JSON text
    return new CDF::Attribute(name.c_str(), value.dump().c_str());
  }

  bool isInternalAttribute(const std::string &name) { return name == "_ARRAY_DIMENSIONS" || name == "_NCProperties" || CT::startsWith(name, "_nczarr"); }

  void addAttributes(std::vector<CDF::Attribute *> &target, const ordered_json &attributes, const Zarr::Array *array) {
    if (!attributes.is_object()) return;
    for (const auto &item: attributes.items()) {
      const std::string &name = item.key();
      if (isInternalAttribute(name)) continue;
      if (std::any_of(target.begin(), target.end(), [&name](CDF::Attribute *existing) { return existing->name == name; })) continue;
      CDF::Attribute *attribute = nullptr;
      // Like in NetCDF, the _FillValue has the type of the variable
      if (array != nullptr && name == "_FillValue" && array->kind == Zarr::Kind::Numeric) {
        std::string bytes;
        if (jsonToElement(item.value(), array->type, bytes)) attribute = new CDF::Attribute(name.c_str(), array->type, bytes.data(), 1);
      }
      if (attribute == nullptr) attribute = jsonToAttribute(name, item.value());
      if (attribute != nullptr) target.push_back(attribute);
    }
  }

  std::string parseDataType(const ordered_json &dtype, Zarr::Array &array) {
    if (!dtype.is_string()) return "data type [" + dtype.dump() + "] is not supported";
    const std::string text = dtype.get<std::string>();
    if (text.size() < 2) return "data type [" + text + "] is not supported";
    const char byteOrder = text[0], kind = text[1];
    const size_t size = strtoul(text.c_str() + 2, nullptr, 10);
    const bool littleEndianMachine = std::endian::native == std::endian::little;
    array.swapBytes = (byteOrder == '<' && !littleEndianMachine) || (byteOrder == '>' && littleEndianMachine);
    array.kind = Zarr::Kind::Numeric;
    array.elementSize = size;
    array.type = CDF_NONE;
    switch (kind) {
    case 'b':
      if (size == 1) array.type = CDF_UBYTE;
      break;
    case 'i':
      array.type = size == 1 ? CDF_BYTE : size == 2 ? CDF_SHORT : size == 4 ? CDF_INT : size == 8 ? CDF_INT64 : CDF_NONE;
      break;
    case 'u':
      array.type = size == 1 ? CDF_UBYTE : size == 2 ? CDF_USHORT : size == 4 ? CDF_UINT : size == 8 ? CDF_UINT64 : CDF_NONE;
      break;
    case 'f':
      array.type = size == 4 ? CDF_FLOAT : size == 8 ? CDF_DOUBLE : CDF_NONE;
      break;
    case 'M': // datetime64 and timedelta64, as their integer values
    case 'm':
      if (size == 8) array.type = CDF_INT64;
      break;
    case 'S':
      if (size > 0) {
        array.kind = Zarr::Kind::FixedBytes;
        array.type = CDF_STRING;
      }
      break;
    case 'U':
      if (size > 0) {
        array.kind = Zarr::Kind::FixedUnicode;
        array.elementSize = size * 4;
        array.type = CDF_STRING;
      }
      break;
    case 'O':
      array.kind = Zarr::Kind::VlenUtf8;
      array.elementSize = 0;
      array.type = CDF_STRING;
      break;
    }
    if (array.type == CDF_NONE) return "data type [" + text + "] is not supported";
    return "";
  }

  bool readSizes(const ordered_json &value, std::vector<size_t> &sizes) {
    if (!value.is_array()) return false;
    for (const auto &size: value) {
      if (!size.is_number_unsigned()) return false;
      sizes.push_back(size.get<size_t>());
    }
    return true;
  }

  bool hasFilter(const ordered_json &filters, const char *id) {
    if (!filters.is_array()) return false;
    return std::any_of(filters.begin(), filters.end(), [id](const ordered_json &filter) { return getOrNull(filter, "id") == id; });
  }

  /* Returns nullptr with the problem set when the array cannot be added as a variable */
  std::shared_ptr<Zarr::Array> parseArray(const std::string &key, const ordered_json &zarray, const ordered_json &attributes, std::string &problem) {
    auto array = std::make_shared<Zarr::Array>();
    array->key = key;
    if (!zarray.is_object()) {
      problem = ".zarray is not an object";
      return nullptr;
    }
    const auto &format = getOrNull(zarray, "zarr_format");
    if (!format.is_null() && format != 2) {
      problem = "zarr_format " + format.dump() + " is not supported";
      return nullptr;
    }
    if (!readSizes(getOrNull(zarray, "shape"), array->shape)) {
      problem = "shape is missing or invalid";
      return nullptr;
    }
    if (!readSizes(getOrNull(zarray, "chunks"), array->chunks) || array->chunks.size() != array->shape.size() ||
        std::any_of(array->chunks.begin(), array->chunks.end(), [](size_t size) { return size == 0; })) {
      problem = "chunks is missing or invalid";
      return nullptr;
    }
    const auto &order = getOrNull(zarray, "order");
    if (order == "F") {
      array->fortranOrder = true;
    } else if (!order.is_null() && order != "C") {
      problem = "order " + order.dump() + " is not supported";
      return nullptr;
    }
    const auto &separator = getOrNull(zarray, "dimension_separator");
    if (separator == "/") {
      array->separator = "/";
    } else if (!separator.is_null() && separator != ".") {
      problem = "dimension_separator " + separator.dump() + " is not supported";
      return nullptr;
    }
    array->compressor = getOrNull(zarray, "compressor");
    array->filters = getOrNull(zarray, "filters");
    problem = parseDataType(getOrNull(zarray, "dtype"), *array);
    if (!problem.empty()) return nullptr;
    if (array->kind == Zarr::Kind::VlenUtf8 && !hasFilter(array->filters, Zarr::VLEN_UTF8)) {
      problem = "data type O is only supported with the vlen-utf8 filter";
      return nullptr;
    }

    // Signed integers marked with _Unsigned are read as unsigned, like the NetCDF reader does
    const auto &isUnsigned = getOrNull(attributes, "_Unsigned");
    if (array->kind == Zarr::Kind::Numeric && (isUnsigned == "true" || isUnsigned == true)) {
      if (array->type == CDF_BYTE) array->type = CDF_UBYTE;
      if (array->type == CDF_SHORT) array->type = CDF_USHORT;
      if (array->type == CDF_INT) array->type = CDF_UINT;
      if (array->type == CDF_INT64) array->type = CDF_UINT64;
    }

    array->unsupported = Zarr::checkCodecs(array->compressor, array->filters);
    if (array->unsupported.empty() && array->kind != Zarr::Kind::VlenUtf8 && hasFilter(array->filters, Zarr::VLEN_UTF8)) {
      array->unsupported = "the vlen-utf8 filter is only supported with data type O";
    }

    array->fillBytes = std::string(array->elementSize, 0);
    const auto &fillValue = getOrNull(zarray, "fill_value");
    if (array->kind == Zarr::Kind::Numeric && !fillValue.is_null()) {
      std::string bytes;
      if (jsonToElement(fillValue, array->type, bytes)) {
        array->fillBytes = bytes;
        array->hasFillValue = true;
      }
    }
    return array;
  }

  /* The group of an array, like "group/subgroup/", or empty for the root group */
  std::string getGroupPrefix(const std::string &key) {
    const auto slash = key.rfind('/');
    return slash == std::string::npos ? "" : key.substr(0, slash + 1);
  }

  struct DimensionName {
    std::string name;
    bool inGroup; // The name is relative to the group of the array, otherwise it is a full name
  };

  /**
   * The names of the dimensions of an array. Arrays without dimension names get dimensions named like the NetCDF library
   * names them.
   */
  std::vector<DimensionName> getDimensionNames(const Zarr::Array &array, const ordered_json &zarray, const ordered_json &attributes) {
    const size_t numDims = array.shape.size();
    std::vector<DimensionName> names;

    // Written by xarray: names in the group of the array
    const auto &arrayDimensions = getOrNull(attributes, "_ARRAY_DIMENSIONS");
    if (arrayDimensions.is_array() && arrayDimensions.size() == numDims) {
      for (const auto &name: arrayDimensions) {
        if (!name.is_string()) break;
        names.push_back({name.get<std::string>(), true});
      }
      if (names.size() == numDims) return names;
      names.clear();
    }

    // Written by NCZarr: full names like /group/time
    for (const auto *source: {&attributes, &zarray}) {
      const auto &dimensionReferences = getOrNull(getOrNull(*source, "_nczarr_array"), "dimrefs");
      if (dimensionReferences.is_array() && dimensionReferences.size() == numDims) {
        for (const auto &reference: dimensionReferences) {
          if (!reference.is_string()) break;
          std::string name = reference.get<std::string>();
          while (!name.empty() && name[0] == '/') name.erase(0, 1);
          names.push_back({name, false});
        }
        if (names.size() == numDims) return names;
        names.clear();
      }
    }

    for (size_t dim = 0; dim < numDims; dim++) {
      names.push_back({numDims == 1 && array.shape[0] == 1 ? "_scalar_" : "_Anonymous_Dim_" + std::to_string(array.shape[dim]), false});
    }
    return names;
  }

  /**
   * Finds or makes the dimension for a name. Like in NetCDF, a name in a group refers to the dimension of the group, or else
   * to the dimension with that name and length of the nearest parent group. Dimensions in groups are named with the path of
   * the group, like the NetCDF reader names them.
   * @return nullptr when the dimension of the group has another length
   */
  CDF::Dimension *getDimension(CDFObject *cdfObject, const std::string &arrayKey, const DimensionName &dimensionName, size_t length) {
    const std::string groupPrefix = dimensionName.inGroup ? getGroupPrefix(arrayKey) : "";
    const std::string ownName = groupPrefix + dimensionName.name;
    CDF::Dimension *dimension = cdfObject->getDimensionNE(ownName.c_str());
    if (dimension != nullptr) return dimension->getSize() == length ? dimension : nullptr;
    if (dimensionName.inGroup) {
      std::string prefix = groupPrefix;
      while (!prefix.empty()) {
        prefix.pop_back();
        prefix = getGroupPrefix(prefix);
        CDF::Dimension *parentDimension = cdfObject->getDimensionNE((prefix + dimensionName.name).c_str());
        if (parentDimension != nullptr && parentDimension->getSize() == length) return parentDimension;
      }
    }
    dimension = new CDF::Dimension(ownName, length);
    dimension->id = (int)cdfObject->dimensions.size();
    cdfObject->dimensions.push_back(dimension);
    return dimension;
  }

  /* Reads the .zgroup, .zarray and .zattrs of the groups and arrays below a group, for stores without consolidated metadata */
  int walkGroup(Zarr::Store &store, const std::string &prefix, ordered_json &metadata, int depth) {
    if (depth > 32) {
      CDBError("Groups in [%s] are nested too deep", store.describe(prefix).c_str());
      return 1;
    }
    std::vector<std::string> children;
    if (store.listChildren(prefix, children) != 0) return 1;
    std::vector<std::string> keys, paths;
    for (const auto &child: children) {
      if (child.empty() || child[0] == '.') continue;
      const std::string path = prefix.empty() ? child : prefix + "/" + child;
      paths.push_back(path);
      for (const char *name: {"/.zarray", "/.zgroup", "/.zattrs"}) keys.push_back(path + name);
    }
    std::vector<std::pair<bool, std::string>> results;
    if (fetchAll(store, keys, results) != 0) return 1;
    std::vector<std::string> groups;
    for (size_t index = 0; index < keys.size(); index++) {
      if (!results[index].first) continue;
      ordered_json parsed;
      if (!parseJson(results[index].second, parsed, store.describe(keys[index]))) return 1;
      metadata[keys[index]] = parsed;
      if (CT::endsWith(keys[index], "/.zgroup")) groups.push_back(paths[index / 3]);
    }
    for (const auto &group: groups) {
      if (walkGroup(store, group, metadata, depth + 1) != 0) return 1;
    }
    return 0;
  }

  /* Reads the metadata of all groups and arrays, as in consolidated metadata: a map from key (like group/name/.zarray) to JSON */
  int loadMetadata(Zarr::Store &store, ordered_json &metadata) {
    std::vector<std::pair<bool, std::string>> results;
    if (fetchAll(store, {".zmetadata"}, results) != 0) return 1;
    if (results[0].first) {
      ordered_json consolidated;
      if (!parseJson(results[0].second, consolidated, store.describe(".zmetadata"))) return 1;
      metadata = getOrNull(consolidated, "metadata");
      if (!metadata.is_object()) {
        CDBError("[%s] has no metadata object", store.describe(".zmetadata").c_str());
        return 1;
      }
      return 0;
    }

    // Without consolidated metadata, walk the groups
    if (fetchAll(store, {".zgroup", ".zattrs", "zarr.json"}, results) != 0) return 1;
    if (!results[0].first) {
      if (results[2].first) {
        CDBError("[%s] is a Zarr v3 store, only Zarr v2 is supported", store.describe("").c_str());
      } else {
        CDBError("[%s] is not a Zarr v2 group: it has no .zmetadata and no .zgroup", store.describe("").c_str());
      }
      return 1;
    }
    metadata = ordered_json::object();
    for (size_t index = 0; index < 2; index++) {
      if (!results[index].first) continue;
      ordered_json parsed;
      const std::string key = index == 0 ? ".zgroup" : ".zattrs";
      if (!parseJson(results[index].second, parsed, store.describe(key))) return 1;
      metadata[key] = parsed;
    }
    return walkGroup(store, "", metadata, 0);
  }

  /** The part of one chunk that is read: per dimension, the selected indices that fall in this chunk */
  struct ChunkPart {
    std::string key;
    std::vector<size_t> firstOut;     // The index in the output of the first selected index in this chunk
    std::vector<size_t> numOut;       // The number of selected indices in this chunk
    std::vector<size_t> firstInChunk; // The position in the chunk of the first selected index
  };

  /**
   * Calls copy(chunkElement, outElement, count) for every run of selected elements of a chunk part that lies contiguous both
   * in the chunk and in the output.
   */
  template <typename Copy>
  void forEachRun(const ChunkPart &part, const std::vector<size_t> &outStrides, const std::vector<size_t> &chunkStrides, const std::vector<ptrdiff_t> &stride, Copy copy) {
    const size_t numDims = part.numOut.size();
    if (numDims == 0) {
      copy(0, 0, 1);
      return;
    }
    const size_t last = numDims - 1;
    const bool contiguous = stride[last] == 1 && chunkStrides[last] == 1;
    std::vector<size_t> counter(numDims, 0);
    while (true) {
      size_t chunkElement = part.firstInChunk[last] * chunkStrides[last], outElement = part.firstOut[last];
      for (size_t dim = 0; dim < last; dim++) {
        chunkElement += (part.firstInChunk[dim] + counter[dim] * stride[dim]) * chunkStrides[dim];
        outElement += (part.firstOut[dim] + counter[dim]) * outStrides[dim];
      }
      if (contiguous) {
        copy(chunkElement, outElement, part.numOut[last]);
      } else {
        for (size_t index = 0; index < part.numOut[last]; index++) {
          copy(chunkElement + index * stride[last] * chunkStrides[last], outElement + index, 1);
        }
      }
      ptrdiff_t dim = (ptrdiff_t)last - 1;
      while (dim >= 0) {
        if (++counter[dim] < part.numOut[dim]) break;
        counter[dim] = 0;
        dim--;
      }
      if (dim < 0) break;
    }
  }

  void swapBytes(std::string &data, size_t elementSize) {
    if (elementSize <= 1) return;
    for (size_t position = 0; position + elementSize <= data.size(); position += elementSize) {
      std::reverse(data.begin() + position, data.begin() + position + elementSize);
    }
  }

  void appendUtf8(std::string &text, uint32_t codePoint) {
    if (codePoint < 0x80) {
      text += (char)codePoint;
    } else if (codePoint < 0x800) {
      text += (char)(0xC0 | (codePoint >> 6));
      text += (char)(0x80 | (codePoint & 0x3F));
    } else if (codePoint < 0x10000) {
      text += (char)(0xE0 | (codePoint >> 12));
      text += (char)(0x80 | ((codePoint >> 6) & 0x3F));
      text += (char)(0x80 | (codePoint & 0x3F));
    } else {
      text += (char)(0xF0 | (codePoint >> 18));
      text += (char)(0x80 | ((codePoint >> 12) & 0x3F));
      text += (char)(0x80 | ((codePoint >> 6) & 0x3F));
      text += (char)(0x80 | (codePoint & 0x3F));
    }
  }

  /* A string element of fixed size (dtype S or U) as UTF-8, without the zero padding */
  std::string fixedElementToString(const Zarr::Array &array, const char *element) {
    if (array.kind == Zarr::Kind::FixedBytes) return std::string(element, strnlen(element, array.elementSize));
    std::string text;
    for (size_t position = 0; position < array.elementSize; position += 4) {
      unsigned char bytes[4];
      memcpy(bytes, element + position, 4);
      if (array.swapBytes) std::reverse(bytes, bytes + 4);
      const uint32_t codePoint = std::endian::native == std::endian::little ? (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) | ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24)
                                                                            : (uint32_t)bytes[3] | ((uint32_t)bytes[2] << 8) | ((uint32_t)bytes[1] << 16) | ((uint32_t)bytes[0] << 24);
      if (codePoint == 0) break;
      appendUtf8(text, codePoint);
    }
    return text;
  }

  /**
   * Reads the selection of an array: numbers into numbers (elementSize bytes each, in the byte order of this machine), or
   * strings into strings. Fetches the chunks in parallel and decodes them on a pool of threads.
   * @return 0 on success, otherwise nonzero with the error logged
   */
  int readSelection(Zarr::Store &store, const Zarr::Array &array, const std::vector<size_t> &start, const std::vector<size_t> &count, const std::vector<ptrdiff_t> &stride, char *numbers,
                    std::vector<std::string> *strings, const Zarr::PrefetchedChunks &prefetched, Zarr::ChunkCache *cache) {
    const size_t numDims = array.shape.size();

    // Per dimension, the chunks that hold selected indices
    struct Span {
      size_t chunk, firstOut, numOut, firstInChunk;
    };
    std::vector<std::vector<Span>> spans(numDims);
    for (size_t dim = 0; dim < numDims; dim++) {
      const size_t chunkSize = array.chunks[dim];
      size_t index = 0;
      while (index < count[dim]) {
        const size_t position = start[dim] + index * stride[dim];
        const size_t chunk = position / chunkSize;
        const size_t chunkEnd = std::min((chunk + 1) * chunkSize, array.shape[dim]);
        const size_t lastIndex = std::min(count[dim] - 1, (chunkEnd - 1 - start[dim]) / stride[dim]);
        spans[dim].push_back({chunk, index, lastIndex - index + 1, position - chunk * chunkSize});
        index = lastIndex + 1;
      }
    }

    // All combinations of the spans of the dimensions are the chunk parts to read
    std::vector<ChunkPart> parts;
    std::vector<size_t> combination(numDims, 0);
    while (true) {
      ChunkPart part;
      part.key = array.key + "/";
      for (size_t dim = 0; dim < numDims; dim++) {
        const Span &span = spans[dim][combination[dim]];
        if (dim > 0) part.key += array.separator;
        part.key += std::to_string(span.chunk);
        part.firstOut.push_back(span.firstOut);
        part.numOut.push_back(span.numOut);
        part.firstInChunk.push_back(span.firstInChunk);
      }
      if (numDims == 0) part.key += "0";
      parts.push_back(std::move(part));
      ptrdiff_t dim = (ptrdiff_t)numDims - 1;
      while (dim >= 0) {
        if (++combination[dim] < spans[dim].size()) break;
        combination[dim] = 0;
        dim--;
      }
      if (dim < 0) break;
    }

    std::vector<size_t> outStrides(numDims, 1), chunkStrides(numDims, 1);
    for (ptrdiff_t dim = (ptrdiff_t)numDims - 2; dim >= 0; dim--) outStrides[dim] = outStrides[dim + 1] * count[dim + 1];
    if (array.fortranOrder) {
      for (size_t dim = 1; dim < numDims; dim++) chunkStrides[dim] = chunkStrides[dim - 1] * array.chunks[dim - 1];
    } else {
      for (ptrdiff_t dim = (ptrdiff_t)numDims - 2; dim >= 0; dim--) chunkStrides[dim] = chunkStrides[dim + 1] * array.chunks[dim + 1];
    }
    size_t chunkElements = 1;
    for (size_t size: array.chunks) chunkElements *= size;
    const size_t chunkBytes = chunkElements * array.elementSize;
    const size_t elementSize = array.elementSize;
    // Fixed size strings are first read as their bytes
    std::string fixedStrings;
    char *output = numbers;
    if (array.kind == Zarr::Kind::FixedBytes || array.kind == Zarr::Kind::FixedUnicode) {
      size_t total = 1;
      for (size_t size: count) total *= size;
      fixedStrings.resize(total * elementSize);
      output = fixedStrings.data();
    }

    std::string fillChunk; // A chunk with only the fill value, for chunks that do not exist
    std::atomic<bool> failed(false);
    std::mutex errorMutex;
    std::string error;
    auto setError = [&](const std::string &message) {
      std::lock_guard<std::mutex> lock(errorMutex);
      if (error.empty()) error = message;
      failed = true;
    };

    // Decodes one chunk and copies its selected elements to the output. Runs on the pool: chunks write to different elements.
    auto copyFromChunk = [&](const ChunkPart &part, const char *chunk) {
      forEachRun(part, outStrides, chunkStrides, stride, [&](size_t chunkElement, size_t outElement, size_t numElements) {
        memcpy(output + outElement * elementSize, chunk + chunkElement * elementSize, numElements * elementSize);
      });
    };

    auto process = [&](size_t partIndex, bool found, std::string data) {
      if (failed) return;
      const ChunkPart &part = parts[partIndex];
      if (found) {
        const std::string decodeError = Zarr::decodeChunk(array.compressor, array.filters, data, chunkBytes);
        if (!decodeError.empty()) {
          setError(store.describe(part.key) + ": " + decodeError);
          return;
        }
      }
      if (array.kind == Zarr::Kind::VlenUtf8) {
        std::vector<std::string> elements;
        if (found) {
          const std::string decodeError = Zarr::decodeVlenUtf8(data, elements);
          if (!decodeError.empty() || elements.size() != chunkElements) {
            setError(store.describe(part.key) + ": " + (decodeError.empty() ? "the chunk does not have one string per element" : decodeError));
            return;
          }
        } else {
          elements.assign(chunkElements, "");
        }
        forEachRun(part, outStrides, chunkStrides, stride, [&](size_t chunkElement, size_t outElement, size_t numElements) {
          for (size_t index = 0; index < numElements; index++) (*strings)[outElement + index] = elements[chunkElement + index];
        });
        return;
      }
      // fillChunk is only read for missing chunks: it is complete before the first missing chunk is submitted
      if (!found) {
        copyFromChunk(part, fillChunk.data());
        return;
      }
      if (data.size() != chunkBytes) {
        setError(store.describe(part.key) + ": the chunk has " + std::to_string(data.size()) + " bytes instead of " + std::to_string(chunkBytes));
        return;
      }
      if (array.swapBytes && array.kind == Zarr::Kind::Numeric) swapBytes(data, elementSize);
      // Kept for the next reads of this store, which often need the same chunk, for example for the next level
      auto decoded = std::make_shared<const std::string>(std::move(data));
      if (cache != nullptr) cache->put(part.key, decoded);
      copyFromChunk(part, decoded->data());
    };

    // Decoded chunks in the cache, and chunks that were fetched when the store was opened, are not fetched again
    const bool useCache = cache != nullptr && array.kind != Zarr::Kind::VlenUtf8;
    std::vector<std::shared_ptr<const std::string>> cachedChunks(parts.size());
    std::vector<std::string> keysToFetch;
    std::vector<size_t> partsToFetch;
    for (size_t index = 0; index < parts.size(); index++) {
      if (useCache) cachedChunks[index] = cache->get(parts[index].key);
      if (cachedChunks[index] == nullptr && prefetched.find(parts[index].key) == prefetched.end()) {
        keysToFetch.push_back(parts[index].key);
        partsToFetch.push_back(index);
      }
    }
    const size_t numThreads = std::min(getNumThreads(), parts.size());

    int status = 0;
    {
      // One chunk is decoded on this thread; more chunks on a pool, while the next chunks are being fetched
      std::unique_ptr<Zarr::WorkerPool> pool;
      if (numThreads > 1) pool = std::make_unique<Zarr::WorkerPool>(numThreads, numThreads * 2);
      auto handleChunk = [&](size_t index, bool found, std::string &data) {
        if (!found && fillChunk.empty() && array.kind != Zarr::Kind::VlenUtf8) {
          fillChunk.reserve(chunkBytes);
          for (size_t element = 0; element < chunkElements; element++) fillChunk += array.fillBytes;
        }
        if (pool == nullptr) {
          process(index, found, std::move(data));
          return;
        }
        pool->submit([&process, &setError, index, found, chunkData = std::move(data)]() mutable {
          try {
            process(index, found, std::move(chunkData));
          } catch (const std::exception &exception) {
            setError(exception.what());
          }
        });
      };
      for (size_t index = 0; index < parts.size(); index++) {
        if (cachedChunks[index] != nullptr) {
          copyFromChunk(parts[index], cachedChunks[index]->data());
          continue;
        }
        const auto cached = prefetched.find(parts[index].key);
        if (cached == prefetched.end()) continue;
        std::string data = cached->second.second; // A copy: the chunk may be read again
        handleChunk(index, cached->second.first, data);
      }
      if (!keysToFetch.empty()) {
        status = store.fetch(keysToFetch, [&](size_t index, bool found, std::string &data) { handleChunk(partsToFetch[index], found, data); });
      }
      if (pool != nullptr) pool->wait();
    }
    if (status != 0) return 1;
    if (failed) {
      CDBError("Unable to read Zarr array [%s]: %s", array.key.c_str(), error.c_str());
      return 1;
    }

    if (!fixedStrings.empty()) {
      for (size_t index = 0; index < strings->size(); index++) (*strings)[index] = fixedElementToString(array, fixedStrings.data() + index * elementSize);
    }
    if (CCDFZARRIO_DEBUG) {
      CDBDebug("Read [%s]: %lu chunks on %lu threads", array.key.c_str(), parts.size(), numThreads);
    }
    return 0;
  }

} // namespace

bool CDFZarrReader::isZarrLocation(const std::string &location) { return Zarr::isZarrLocation(location); }

CDFZarrReader::CDFZarrReader() : CDFReader() {}

CDFZarrReader::~CDFZarrReader() { close(); }

int CDFZarrReader::close() { return 0; }

int CDFZarrReader::open(const char *fileName) {
  if (cdfObject == nullptr) {
    CDBError("No CDFObject defined, use CDFObject::attachCDFReader(CDFZarrReader*)");
    return 1;
  }
  this->fileName = fileName;
  arrays.clear();
  prefetchedChunks.clear();
  const size_t chunkCacheSize = CDFReader::getChunkCacheSizeFromEnvironment();
  chunkCache = chunkCacheSize > 0 ? std::make_unique<Zarr::ChunkCache>(chunkCacheSize) : nullptr;
  store = Zarr::openStore(fileName);
  if (store == nullptr) return 1;

  ordered_json metadata;
  traceTimingsSpanStart(TraceTimingType::FSZARROPEN);
  const int status = loadMetadata(*store, metadata);
  traceTimingsSpanEnd(TraceTimingType::FSZARROPEN);
  if (status != 0) {
    CDBError("Unable to open Zarr store [%s]", fileName);
    return 1;
  }

  addAttributes(cdfObject->attributes, getOrNull(metadata, ".zattrs"), nullptr);
  // Tells which reader made this data model, replacing an attribute with this name from the store
  cdfObject->removeAttribute(ZARR_READER_ATTRIBUTE_NAME);
  cdfObject->attributes.push_back(new CDF::Attribute(ZARR_READER_ATTRIBUTE_NAME, ZARR_READER_ATTRIBUTE_VALUE));

  // First the arrays and their dimensions, so that it is known which variables are dimensions
  struct Pending {
    std::shared_ptr<Zarr::Array> array;
    std::vector<CDF::Dimension *> dimensions;
    const ordered_json *attributes;
  };
  // The arrays of a group before those of its subgroups, so that the dimensions of parent groups are known
  std::vector<std::string> keys;
  for (const auto &item: metadata.items()) {
    if (CT::endsWith(item.key(), "/.zarray")) keys.push_back(item.key());
  }
  std::stable_sort(keys.begin(), keys.end(), [](const std::string &a, const std::string &b) { return std::count(a.begin(), a.end(), '/') < std::count(b.begin(), b.end(), '/'); });

  std::vector<Pending> pending;
  for (const auto &key: keys) {
    const ordered_json &zarray = metadata[key];
    const std::string path = key.substr(0, key.size() - strlen("/.zarray"));
    const ordered_json &attributes = getOrNull(metadata, (path + "/.zattrs").c_str());
    std::string problem;
    auto array = parseArray(path, zarray, attributes, problem);
    if (array == nullptr) {
      CDBWarning("Skipping Zarr array [%s] in [%s]: %s", path.c_str(), fileName, problem.c_str());
      continue;
    }
    const auto names = getDimensionNames(*array, zarray, attributes);
    Pending entry = {array, {}, &attributes};
    for (size_t dim = 0; dim < names.size(); dim++) {
      CDF::Dimension *dimension = getDimension(cdfObject, path, names[dim], array->shape[dim]);
      if (dimension == nullptr) {
        CDBError("In [%s], dimension [%s] of array [%s] has length %lu, but it has another length elsewhere", fileName, names[dim].name.c_str(), path.c_str(), array->shape[dim]);
        return 1;
      }
      entry.dimensions.push_back(dimension);
    }
    pending.push_back(entry);
  }

  for (auto &entry: pending) {
    const std::string &name = entry.array->key;
    if (cdfObject->getVariableNE(name.c_str()) != nullptr) continue;
    CDF::Variable *variable = new CDF::Variable();
    variable->setName(name);
    variable->dimensionlinks = entry.dimensions;
    addAttributes(variable->attributes, *entry.attributes, entry.array.get());
    variable->isDimension = cdfObject->getDimensionNE(name.c_str()) != nullptr;
    // Like xarray, the fill_value is the _FillValue when there is no _FillValue attribute. Not for coordinate variables, which
    // xarray writes with fill_value NaN: the NetCDF view of the same data has no _FillValue there.
    if (entry.array->hasFillValue && !variable->isDimension && variable->getAttributeNE("_FillValue") == nullptr) {
      variable->attributes.push_back(new CDF::Attribute("_FillValue", entry.array->type, entry.array->fillBytes.data(), 1));
    }
    variable->setType(entry.array->type);
    variable->nativeType = entry.array->type;
    variable->id = (int)cdfObject->variables.size();
    variable->setParentCDFObject(cdfObject);
    // It is essential that the variable knows which reader can be used to read the data
    variable->setCDFReaderPointer((void *)this);
    cdfObject->variables.push_back(variable);
    arrays[name] = entry.array;
  }

  prefetchSmallArrays();

  if (CCDFZARRIO_DEBUG) {
    CDBDebug("Opened Zarr store [%s]: %lu variables, %lu dimensions, %lu chunks prefetched", fileName, cdfObject->variables.size(), cdfObject->dimensions.size(), prefetchedChunks.size());
  }
  return 0;
}

void CDFZarrReader::prefetchSmallArrays() {
  // Coordinate variables and single values in one chunk, like x, y, time and projection. Nearly every request reads these, and
  // fetching them together here takes one round trip instead of one per variable.
  static const size_t MAX_PREFETCH_BYTES = 1024 * 1024;
  std::vector<std::string> keys;
  for (auto variable: cdfObject->variables) {
    const auto found = arrays.find(variable->name);
    if (found == arrays.end()) continue;
    const Zarr::Array &array = *found->second;
    size_t numElements = 1;
    bool oneChunk = true;
    for (size_t dim = 0; dim < array.shape.size(); dim++) {
      numElements *= array.shape[dim];
      if (array.chunks[dim] < array.shape[dim]) oneChunk = false;
    }
    const bool isSmall = array.kind != Zarr::Kind::VlenUtf8 && numElements * array.elementSize <= MAX_PREFETCH_BYTES;
    if (!array.unsupported.empty() || !oneChunk || !isSmall || (!variable->isDimension && numElements > 1)) continue;
    std::string key = array.key + "/";
    for (size_t dim = 0; dim < array.shape.size(); dim++) key += dim == 0 ? "0" : array.separator + "0";
    if (array.shape.empty()) key += "0";
    keys.push_back(key);
  }
  if (keys.empty()) return;
  std::vector<std::pair<bool, std::string>> results;
  if (fetchAll(*store, keys, results) != 0) {
    // Not fatal: the variables are fetched again when they are read
    prefetchedChunks.clear();
    return;
  }
  for (size_t index = 0; index < keys.size(); index++) prefetchedChunks[keys[index]] = std::move(results[index]);
}

int CDFZarrReader::_readVariableData(CDF::Variable *var, CDFType type) { return _readVariableData(var, type, nullptr, nullptr, nullptr); }

int CDFZarrReader::_readVariableData(CDF::Variable *var, CDFType type, size_t *start, size_t *count, ptrdiff_t *stride) {
  const auto found = arrays.find(var->name);
  if (found == arrays.end() || store == nullptr) {
    CDBError("Variable [%s] is not an array in Zarr store [%s]", var->name.c_str(), fileName.c_str());
    return 1;
  }
  const Zarr::Array &array = *found->second;
  if (!array.unsupported.empty()) {
    CDBError("Unable to read variable [%s] from Zarr store [%s]: %s", var->name.c_str(), fileName.c_str(), array.unsupported.c_str());
    return 1;
  }
  if ((array.type == CDF_STRING) != (type == CDF_STRING)) {
    CDBError("Unable to read variable [%s] of type %s as %s", var->name.c_str(), CDF::getCDFDataTypeName(array.type).c_str(), CDF::getCDFDataTypeName(type).c_str());
    return 1;
  }

  const size_t numDims = array.shape.size();
  std::vector<size_t> selectionStart(numDims, 0), selectionCount(array.shape);
  std::vector<ptrdiff_t> selectionStride(numDims, 1);
  if (start != nullptr && count != nullptr && stride != nullptr) {
    for (size_t dim = 0; dim < numDims; dim++) {
      selectionStart[dim] = start[dim];
      selectionCount[dim] = count[dim];
      selectionStride[dim] = stride[dim];
      const bool valid = stride[dim] >= 1 && (count[dim] == 0 || start[dim] + (count[dim] - 1) * stride[dim] < array.shape[dim]);
      if (!valid) {
        CDBError("Start, count and stride are out of bounds for dim nr [%lu] of variable [%s]: start[%lu], count[%lu], stride[%td], size[%lu]", dim, var->name.c_str(), start[dim], count[dim],
                 stride[dim], array.shape[dim]);
        return 1;
      }
    }
  }
  size_t total = 1;
  for (size_t size: selectionCount) total *= size;

  var->freeData();
  var->setSize(total);
  var->setType(type);
  if (total == 0) return 0;

  traceTimingsSpanStart(TraceTimingType::FSZARRREAD);
  int status = 0;
  if (array.type == CDF_STRING) {
    std::vector<std::string> strings(total);
    status = readSelection(*store, array, selectionStart, selectionCount, selectionStride, nullptr, &strings, prefetchedChunks, chunkCache.get());
    if (status == 0) {
      CDF::allocateData(CDF_STRING, &var->data, total);
      for (size_t index = 0; index < total; index++) {
        char *text = (char *)malloc(strings[index].size() + 1);
        memcpy(text, strings[index].c_str(), strings[index].size() + 1);
        ((char **)var->data)[index] = text;
      }
    }
  } else {
    void *data = nullptr;
    CDF::allocateData(array.type, &data, total);
    status = readSelection(*store, array, selectionStart, selectionCount, selectionStride, (char *)data, nullptr, prefetchedChunks, chunkCache.get());
    if (status == 0 && type != array.type) {
      CDF::allocateData(type, &var->data, total);
      CDFCopyData(var->data, type, data, array.type, 0, 0, total);
      CDF::freeData(&data);
    } else if (status == 0) {
      var->data = data;
    } else {
      CDF::freeData(&data);
    }
  }
  traceTimingsSpanEnd(TraceTimingType::FSZARRREAD);
  if (status != 0) {
    var->setSize(0);
    return 1;
  }
  return 0;
}
