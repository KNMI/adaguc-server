/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Generic Data model to read netcdf and hdf5
 * Author:   Maarten Plieger, plieger "at" knmi.nl, GST - GeoSpatialTeam KNMI
 * Date:     2026-09-10
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

#include "CCDFReader.h"
#include "CDebugger.h"
#include <cerrno>
#include <cstdint>
#include <cstdlib>

CDFReader::CDFReader() { cdfObject = NULL; }
CDFReader::~CDFReader() {}

size_t CDFReader::getChunkCacheSizeFromEnvironment() {
  const size_t megabyte = 1024 * 1024;
  // The default chunk cache of the NetCDF library (4.9): small, for deployments with little memory
  const size_t defaultSize = 16 * megabyte;
  const char *value = getenv("ADAGUC_CHUNK_CACHE_MB");
  if (value == nullptr || value[0] == '\0') return defaultSize;
  char *end = nullptr;
  errno = 0;
  unsigned long long megabytes = strtoull(value, &end, 10);
  // strtoull accepts a sign and wraps a negative number around, so only digits are allowed
  bool isNumber = value[0] >= '0' && value[0] <= '9' && *end == '\0' && errno == 0;
  if (!isNumber || megabytes > SIZE_MAX / megabyte) {
    static bool warned = false;
    if (!warned) {
      CDBWarning("ADAGUC_CHUNK_CACHE_MB=%s is not a whole number of megabytes, using the default of 16", value);
      warned = true;
    }
    return defaultSize;
  }
  return (size_t)megabytes * megabyte;
}
