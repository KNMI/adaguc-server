/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
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

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <netcdf.h>
#include "CDebugger.h"
#include "CCDFDataModel.h"
#include "CCDFNetCDFIO.h"
#include "CCDFZarrIO.h"
#include "CTime.h"
#include "CCDFDataModel.h"
#include "utils.h"

int testCTimeInit(CDF::Variable *testVar, const char *testDate) {
  CTime *testTime = CTime::GetCTimeInstance(testVar);
  if (testTime == NULL) {
    CDBError("[FAILED] testTime is NULL");
    return 1;
  } else {
    CDBDebug("[OK] testTime is returned");
  }

  CTime::Date date;
  try {
    date = testTime->getDate(6600000000);
    CDBDebug("[OK] testTime->getDate(6600000000)");
  } catch (int e) {
    CDBError("[FAILED] testTime->getDate(6600000000)");
    return 1;
  }
  std::string dateString = testTime->dateToString(date);
  CDBDebug("dateString = [%s]", dateString.c_str());
  if (dateString == testDate) {
    CDBDebug("[OK] dateToString");
  } else {
    CDBError("[FAILED] dateToString expected [%s]: [%s]", testDate, dateString.c_str());
    return 1;
  }
  return 0;
}

int testCTime2Init(CDF::Variable *testVar, double valueToCheck) {
  CTime *adagucTime = CTime::GetCTimeInstance(testVar);

  try {
    double startGraphTime = adagucTime->dateToOffset(adagucTime->freeDateStringToDate("2014-08-08T11:22:33Z"));
    if (startGraphTime != valueToCheck) {
      CDBError("[FAILED] startGraphTime != %f, %f", valueToCheck, startGraphTime);
      return 1;
    } else {
      CDBDebug("[OK] startGraphTime == %f", valueToCheck);
    }
    adagucTime->getDate(0);
    CDBDebug("[OK] adagucTime->getDate(0) %f", startGraphTime);
  } catch (int e) {
    CDBError("[FAILED] testTime->getDate(0)");
    return 1;
  }
  return 0;
}

int testCTimeEpochTimeConversion() {
  bool failed = false;
  try {
    if (CTime::getEpochTimeFromDateString("Sun Sep 22 13:23:18 2019") != 1569158598) {
      CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"Sun Sep 22 13:23:18 2019\") != 1569158598");
      failed = true;
    } else {
      CDBDebug("[OK] CTime::getEpochTimeFromDateString(\"Sun Sep 22 13:23:18 2019\") == 1569158598");
    }
  } catch (int e) {
    failed = true;
    CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"Sun Sep 22 13:23:18 2019\") throws an exception");
  }

  try {
    if (CTime::getEpochTimeFromDateString("2019-09-22T13:23:18") != 1569158598) {
      CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18\") != 1569158598");
      failed = true;
    } else {
      CDBDebug("[OK] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18\") == 1569158598");
    }
  } catch (int e) {
    failed = true;
    CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18\") throws an exception");
  }

  try {
    if (CTime::getEpochTimeFromDateString("2019-09-22T13:23:18Z") != 1569158598) {
      CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18Z\") != 1569158598");
      failed = true;
    } else {
      CDBDebug("[OK] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18Z\") == 1569158598");
    }
  } catch (int e) {
    failed = true;
    CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"2019-09-22T13:23:18Z\") throws an exception");
  }

  try {
    CTime::getEpochTimeFromDateString("SHOULDTHROWEXCEPTION");
    failed = true;
    CDBError("[FAILED] CTime::getEpochTimeFromDateString(\"SHOULDTHROWEXCEPTION\") did not throw an exception");
  } catch (int e) {
    CDBDebug("[OK] CTime::getEpochTimeFromDateString(\"SHOULDTHROWEXCEPTION\") throws an exception");
  }

  try {
    CTime::Date date = CTime::periodToDate("P3Y4M5DT6H7M13S");
    if (CTime::dateToPeriod(date) != "P3Y4M5DT6H7M13S") {
      CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"P3Y4M5DT6H7M13S\") is %s", CTime::dateToPeriod(date).c_str());
      failed = true;
    } else {
      CDBDebug("[OK] CTime::dateToPeriod(date).equals(\"P3Y4M5DT6H7M13S\") is %s", CTime::dateToPeriod(date).c_str());
    }
  } catch (int e) {
    CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"P3Y4M5DT6H7M13S\") throws an exception");
    failed = true;
  }

  try {
    CTime::Date date = CTime::periodToDate("P1234Y");
    if (CTime::dateToPeriod(date) != "P1234Y") {
      CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"P1234Y\") is %s", CTime::dateToPeriod(date).c_str());
      failed = true;
    } else {
      CDBDebug("[OK] CTime::dateToPeriod(date).equals(\"P1234Y\") is %s", CTime::dateToPeriod(date).c_str());
    }
  } catch (int e) {
    CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"P1234Y\") throws an exception");
    failed = true;
  }

  try {
    CTime::Date date = CTime::periodToDate("P15M");
    if (CTime::dateToPeriod(date) != "P15M") {
      CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"P15M\") is %s", CTime::dateToPeriod(date).c_str());
      failed = true;
    } else {
      CDBDebug("[OK] CTime::dateToPeriod(date).equals(\"P15M\") is %s", CTime::dateToPeriod(date).c_str());
    }
  } catch (int e) {
    CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"P15M\") throws an exception");
    failed = true;
  }

  try {
    CTime::Date date = CTime::periodToDate("P7D");
    if (CTime::dateToPeriod(date) != "P7D") {
      CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"P7D\") is %s", CTime::dateToPeriod(date).c_str());
      failed = true;
    } else {
      CDBDebug("[OK] CTime::dateToPeriod(date).equals(\"P7D\") is %s", CTime::dateToPeriod(date).c_str());
    }
  } catch (int e) {
    CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"P7D\") throws an exception");
    failed = true;
  }

  try {
    CTime::Date date = CTime::periodToDate("PT12H");
    if (CTime::dateToPeriod(date) != "PT12H") {
      CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"PT12H\") is %s", CTime::dateToPeriod(date).c_str());
      failed = true;
    } else {
      CDBDebug("[OK] CTime::dateToPeriod(date).equals(\"PT12H\") is %s", CTime::dateToPeriod(date).c_str());
    }
  } catch (int e) {
    CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"PT12H\") throws an exception");
    failed = true;
  }

  try {
    CTime::Date date = CTime::periodToDate("PT1M");
    if (CTime::dateToPeriod(date) != "PT1M") {
      CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"PT1M\") is %s", CTime::dateToPeriod(date).c_str());
      failed = true;
    } else {
      CDBDebug("[OK] CTime::dateToPeriod(date).equals(\"PT1M\") is %s", CTime::dateToPeriod(date).c_str());
    }
  } catch (int e) {
    CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"PT1M\") throws an exception");
    failed = true;
  }

  try {
    CTime::Date date = CTime::periodToDate("PT30S");
    if (CTime::dateToPeriod(date) != "PT30S") {
      CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"PT30S\") is %s", CTime::dateToPeriod(date).c_str());
      failed = true;
    } else {
      CDBDebug("[OK] CTime::dateToPeriod(date).equals(\"PT30S\") is %s", CTime::dateToPeriod(date).c_str());
    }
  } catch (int e) {
    CDBError("[FAILED] CTime::dateToPeriod(date).equals(\"PT30S\") throws an exception");
    failed = true;
  }

  std::string f[] = {// 0                                                                                 //
                    "2019-09-22T13:23:18Z", "PT1H", "2019-09-22T12:23:18Z",
                    // 1                                                                                 //
                    "2019-09-22T13:23:18Z", "PT60M", "2019-09-22T12:23:18Z",
                    // 2                                                                                 //
                    "2019-09-22T13:23:18Z", "P1M", "2019-08-22T13:23:18Z",
                    // 3                                                                                 //
                    "2019-09-22T13:23:18Z", "P19Y08M21DT13H23M18S", "2000-01-01T00:00:00Z"};

  for (int j = 0; j < 4; j++) {
    std::string in = f[j * 3 + 0];
    std::string op = f[j * 3 + 1];
    std::string out = f[j * 3 + 2];
    try {
      CTime ctime;
      ctime.init("seconds since 1970-01-01", "none");
      CTime::Date date = ctime.ISOStringToDate(in);
      if (ctime.dateToISOString(ctime.subtractPeriodFromDate(date, op.c_str())) != out) {
        CDBError("[FAILED]!ctime.dateToISOString(ctime.subtractPeriodFromDate(date, \"%s\")) returns %s and not %s", op.c_str(),
                 ctime.dateToISOString(ctime.subtractPeriodFromDate(date, op.c_str())).c_str(), out.c_str());
        failed = true;
        break;
      } else {
        CDBDebug("[OK] CTime::subtractPeriodFromDate(\"%s\",(\"%s\" ) == (\"%s\")", in.c_str(), op.c_str(), out.c_str());
      }
    } catch (int e) {
      failed = true;
      CDBError("[FAILED]  CTime::subtractPeriodFromDate(\"%s\",(\"%s\" ) throws an exception", in.c_str(), op.c_str());
      break;
    }
  }

  return failed;
}

int testHDF5Reader() {
  CDBDebug("testHDF5Reader");
  std::string testFile = "./testdata/variable_string.h5";
  CDFReader *cdfReader = findReaderByFileName(testFile.c_str());
  CDFObject *cdfObject = new CDFObject();
  cdfObject->attachCDFReader(cdfReader);
  if (cdfReader->open(testFile.c_str()) != 0) {
    CDBError("[FAILED] testHDF5Reader was unable to open file %s", testFile.c_str());
    return 1;
  }
  std::string dumpString = CDF::dump(cdfObject);

  std::string expectedString = cdfObject->getVariableThrows("overview")->getAttributeThrows("product_datetime_start")->toString();

  if (expectedString != "22-NOV-2021;08:00:00.000") {
    CDBDebug("[FAILED] testHDF5Reader: expectedString 22-NOV-2021;08:00:00.000 is different then ");
    return 1;
  } else {
    CDBDebug("[OK] testHDF5Reader: expectedString is equal to dumpstring");
  }

  delete cdfObject;
  delete cdfReader;
  CTime::cleanInstances();

  return 0;
}

int testNetCDFChunkCacheFromEnvironment() {
  CDBDebug("testNetCDFChunkCacheFromEnvironment");
  const size_t megabyte = 1024 * 1024;
  bool failed = false;

  // ADAGUC_CHUNK_CACHE_MB is for the NetCDF and the Zarr reader. Not set, empty or not a whole number: 16 megabytes.
  unsetenv("ADAGUC_CHUNK_CACHE_MB");
  if (CDFReader::getChunkCacheSizeFromEnvironment() != 16 * megabyte) {
    CDBError("[FAILED] chunk cache size without ADAGUC_CHUNK_CACHE_MB should be 16 megabytes");
    failed = true;
  }
  for (const char *invalid: {"", "-128", "+128", "12.5", "128MB", " 128", "abc", "99999999999999999999999999"}) {
    setenv("ADAGUC_CHUNK_CACHE_MB", invalid, 1);
    if (CDFReader::getChunkCacheSizeFromEnvironment() != 16 * megabyte) {
      CDBError("[FAILED] chunk cache size for ADAGUC_CHUNK_CACHE_MB=[%s] should be 16 megabytes", invalid);
      failed = true;
    }
  }
  setenv("ADAGUC_CHUNK_CACHE_MB", "0", 1);
  if (CDFReader::getChunkCacheSizeFromEnvironment() != 0) {
    CDBError("[FAILED] chunk cache size for ADAGUC_CHUNK_CACHE_MB=0 should be 0");
    failed = true;
  }
  setenv("ADAGUC_CHUNK_CACHE_MB", "96", 1);
  if (CDFReader::getChunkCacheSizeFromEnvironment() != 96 * megabyte) {
    CDBError("[FAILED] chunk cache size for ADAGUC_CHUNK_CACHE_MB=96 should be 96 megabytes");
    failed = true;
  }

  size_t defaultSize = 0, defaultNumElements = 0, size = 0, numElements = 0;
  float defaultPreemption = 0, preemption = 0;
  nc_get_chunk_cache(&defaultSize, &defaultNumElements, &defaultPreemption);

  // The test writes its own NetCDF file, so it needs no files from outside this directory: the Docker build runs these
  // tests without the data directory.
  std::string testFile = "/tmp/adaguc_testccdfdatamodel_chunkcache_XXXXXX";
  int fileDescriptor = mkstemp(testFile.data());
  if (fileDescriptor == -1) {
    CDBError("[FAILED] unable to make a temporary file %s", testFile.c_str());
    return 1;
  }
  close(fileDescriptor);
  int ncId = 0, dimId = 0, varId = 0;
  if (nc_create(testFile.c_str(), NC_NETCDF4 | NC_CLOBBER, &ncId) != NC_NOERR || nc_def_dim(ncId, "x", 2, &dimId) != NC_NOERR || nc_def_var(ncId, "data", NC_FLOAT, 1, &dimId, &varId) != NC_NOERR ||
      nc_close(ncId) != NC_NOERR) {
    CDBError("[FAILED] unable to write NetCDF file %s", testFile.c_str());
    remove(testFile.c_str());
    return 1;
  }

  auto openTestFile = [&testFile]() {
    CDFNetCDFReader cdfReader;
    CDFObject cdfObject;
    cdfObject.attachCDFReader(&cdfReader);
    int status = cdfReader.open(testFile.c_str());
    if (status != 0) {
      CDBError("[FAILED] unable to open file %s", testFile.c_str());
    }
    return status;
  };

  // Opening a file sets the size of the chunk cache of the NetCDF library, and keeps the number of slots and the preemption
  for (const auto &[value, expectedSize]: std::vector<std::pair<const char *, size_t>>{{"abc", 16 * megabyte}, {"96", 96 * megabyte}, {"0", 0}}) {
    setenv("ADAGUC_CHUNK_CACHE_MB", value, 1);
    if (openTestFile() != 0) failed = true;
    nc_get_chunk_cache(&size, &numElements, &preemption);
    if (size != expectedSize || numElements != defaultNumElements || preemption != defaultPreemption) {
      CDBError("[FAILED] with ADAGUC_CHUNK_CACHE_MB=%s the chunk cache is %zu bytes with %zu slots, expected %zu bytes with %zu slots", value, size, numElements, expectedSize,
               defaultNumElements);
      failed = true;
    }
  }

  remove(testFile.c_str());
  unsetenv("ADAGUC_CHUNK_CACHE_MB");
  nc_set_chunk_cache(defaultSize, defaultNumElements, defaultPreemption);
  CTime::cleanInstances();
  if (!failed) {
    CDBDebug("[OK] testNetCDFChunkCacheFromEnvironment");
  }
  return failed;
}

/* True when the variables hold the same values: bitwise for numbers (also NaN), by text for strings */
static bool haveSameData(CDF::Variable *a, CDF::Variable *b) {
  if (a->getSize() != b->getSize() || a->currentType != b->currentType) return false;
  if (a->currentType == CDF_STRING) {
    for (size_t index = 0; index < a->getSize(); index++) {
      if (strcmp(((char **)a->data)[index], ((char **)b->data)[index]) != 0) return false;
    }
    return true;
  }
  return memcmp(a->data, b->data, a->getSize() * CDF::getTypeSize(a->currentType)) == 0;
}

/* Reads a selection from both variables and compares the data */
static bool readAndCompare(CDF::Variable *expected, CDF::Variable *actual, CDFType type, size_t *start, size_t *count, ptrdiff_t *stride, const std::string &what) {
  expected->freeData();
  actual->freeData();
  if (expected->readData(type, start, count, stride) != 0) {
    CDBError("[FAILED] Zarr: unable to read %s from the NetCDF file", what.c_str());
    return false;
  }
  if (actual->readData(type, start, count, stride) != 0) {
    CDBError("[FAILED] Zarr: unable to read %s", what.c_str());
    return false;
  }
  if (!haveSameData(expected, actual)) {
    CDBError("[FAILED] Zarr: %s differs from the NetCDF file: %lu %s values, expected %lu %s values", what.c_str(), actual->getSize(), CDF::getCDFDataTypeName(actual->currentType).c_str(),
             expected->getSize(), CDF::getCDFDataTypeName(expected->currentType).c_str());
    if (actual->currentType != CDF_STRING && actual->getSize() > 0 && expected->getSize() > 0) {
      CDBError("first values: %f, expected %f", actual->getDataAt<double>(0), expected->getDataAt<double>(0));
    }
    return false;
  }
  return true;
}

static std::string attributesAsText(std::vector<CDF::Attribute *> &attributes) {
  std::vector<std::string> lines;
  for (auto attribute: attributes) lines.push_back(attribute->name + "=" + attribute->toString());
  std::sort(lines.begin(), lines.end());
  return CT::join(lines, ";");
}

/**
 * Reads the Zarr test stores (written by data/datasets/zarr/create_zarr_test_stores.py) and the NetCDF file with the same
 * data, and expects the same dimensions, variables, attributes and data, for the whole variables and for selections.
 */
int testZarrReader() {
  const std::string directory = "../data/datasets/zarr/";
  int failed = 0;
  CDFObject netcdf;
  CDFNetCDFReader netcdfReader;
  netcdf.attachCDFReader(&netcdfReader);
  if (netcdf.open((directory + "zarrtest.nc").c_str()) != 0) {
    CDBError("[FAILED] Zarr: unable to open the NetCDF file");
    return 1;
  }
  for (const char *storeName: {"consolidated_zlib_shuffle.zarr", "unconsolidated_gzip_forder.zarr", "consolidated_zstd.zarr", "consolidated_blosc.zarr", "consolidated_uncompressed.zarr"}) {
    const std::string store = directory + storeName;
    if (!CDFZarrReader::isZarrLocation(store)) {
      CDBError("[FAILED] Zarr: %s is not recognized as a Zarr store", storeName);
      failed++;
    }
    CDFObject zarr;
    CDFZarrReader zarrReader;
    zarr.attachCDFReader(&zarrReader);
    if (zarr.open(store.c_str()) != 0) {
      CDBError("[FAILED] Zarr: unable to open %s", storeName);
      failed++;
      continue;
    }
    const int failuresBefore = failed;
    // The reader tells that it made the data model; the other global attributes are those of the NetCDF file
    CDF::Attribute *readerAttribute = zarr.getAttributeNE(ZARR_READER_ATTRIBUTE_NAME);
    if (readerAttribute == nullptr || readerAttribute->toString() != ZARR_READER_ATTRIBUTE_VALUE) {
      CDBError("[FAILED] Zarr: %s has no global attribute %s = %s", storeName, ZARR_READER_ATTRIBUTE_NAME, ZARR_READER_ATTRIBUTE_VALUE);
      failed++;
    }
    zarr.removeAttribute(ZARR_READER_ATTRIBUTE_NAME);
    if (attributesAsText(netcdf.attributes) != attributesAsText(zarr.attributes)) {
      CDBError("[FAILED] Zarr: global attributes of %s: [%s], expected [%s]", storeName, attributesAsText(zarr.attributes).c_str(), attributesAsText(netcdf.attributes).c_str());
      failed++;
    }
    if (netcdf.dimensions.size() != zarr.dimensions.size() || netcdf.variables.size() != zarr.variables.size()) {
      CDBError("[FAILED] Zarr: %s has %lu dimensions and %lu variables, expected %lu and %lu", storeName, zarr.dimensions.size(), zarr.variables.size(), netcdf.dimensions.size(),
               netcdf.variables.size());
      failed++;
    }
    for (auto dimension: netcdf.dimensions) {
      auto zarrDimension = zarr.getDimensionNE(dimension->name);
      if (zarrDimension == nullptr || zarrDimension->getSize() != dimension->getSize()) {
        CDBError("[FAILED] Zarr: dimension %s of %s is missing or has another length", dimension->name.c_str(), storeName);
        failed++;
      }
    }
    for (auto variable: netcdf.variables) {
      const std::string what = std::string(storeName) + ":" + variable->name;
      CDF::Variable *zarrVariable = zarr.getVariableNE(variable->name);
      if (zarrVariable == nullptr) {
        CDBError("[FAILED] Zarr: variable %s is missing", what.c_str());
        failed++;
        continue;
      }
      std::string dimensions, zarrDimensions;
      for (auto dimension: variable->dimensionlinks) dimensions += dimension->name + ",";
      for (auto dimension: zarrVariable->dimensionlinks) zarrDimensions += dimension->name + ",";
      if (variable->getNativeType() != zarrVariable->getNativeType() || dimensions != zarrDimensions || variable->isDimension != zarrVariable->isDimension) {
        CDBError("[FAILED] Zarr: %s is %s(%s), expected %s(%s)", what.c_str(), CDF::getCDFDataTypeName(zarrVariable->getNativeType()).c_str(), zarrDimensions.c_str(),
                 CDF::getCDFDataTypeName(variable->getNativeType()).c_str(), dimensions.c_str());
        failed++;
        continue;
      }
      if (attributesAsText(variable->attributes) != attributesAsText(zarrVariable->attributes)) {
        CDBError("[FAILED] Zarr: attributes of %s: [%s], expected [%s]", what.c_str(), attributesAsText(zarrVariable->attributes).c_str(), attributesAsText(variable->attributes).c_str());
        failed++;
      }

      // The whole variable, converted and in its own type. Converted first: reading in the own type must give the own type again.
      const CDFType type = variable->getNativeType();
      if (type != CDF_STRING && !readAndCompare(variable, zarrVariable, CDF_DOUBLE, nullptr, nullptr, nullptr, what + " as double")) failed++;
      if (!readAndCompare(variable, zarrVariable, type, nullptr, nullptr, nullptr, what)) failed++;

      // Selections: the same pattern on every dimension, and a different pattern per dimension
      const size_t numDims = variable->dimensionlinks.size();
      for (int pattern = 0; pattern < 10 && numDims > 0; pattern++) {
        std::vector<size_t> start(numDims), count(numDims);
        std::vector<ptrdiff_t> stride(numDims);
        std::string selection;
        for (size_t dim = 0; dim < numDims; dim++) {
          const size_t length = variable->dimensionlinks[dim]->getSize();
          switch (pattern < 5 ? pattern : (pattern + dim) % 5) {
          case 0: // Everything
            start[dim] = 0, count[dim] = length, stride[dim] = 1;
            break;
          case 1: // Every second element
            start[dim] = std::min<size_t>(1, length - 1), count[dim] = std::max<size_t>(1, (length - start[dim] + 1) / 2), stride[dim] = 2;
            break;
          case 2: // A range in the middle
            start[dim] = length / 3, count[dim] = std::max<size_t>(1, length / 2), stride[dim] = 1;
            break;
          case 3: // The last element
            start[dim] = length - 1, count[dim] = 1, stride[dim] = 1;
            break;
          default: // Every third element
            start[dim] = 0, count[dim] = (length + 2) / 3, stride[dim] = 3;
            break;
          }
          selection += CT::printf("[%lu:%lu:%td]", start[dim], count[dim], stride[dim]);
        }
        if (!readAndCompare(variable, zarrVariable, type, start.data(), count.data(), stride.data(), what + selection)) failed++;
      }
      variable->freeData();
      zarrVariable->freeData();
    }
    if (failed == failuresBefore) {
      CDBDebug("[OK] Zarr: %s is the same as the NetCDF file", storeName);
    }
  }

  // Locations
  const std::vector<std::pair<std::string, bool>> locations = {
      {"https://host:7443/bucket/store.zarr#mode=zarr,s3", true},
      {"https://host/bucket/store#mode=nczarr,s3&aws.region=eu-west-1", true},
      {"file:///data/store.zarr#mode=zarr,file", true},
      {"/data/store.zarr", true},
      {"/data/store.zarr/", true},
      {"s3://bucket/store.zarr", true},
      {"/data/store.zip#mode=zarr,zip", false},
      {"/data/file.nc", false},
      {"https://host/thredds/dodsC/file.nc", false},
  };
  for (const auto &location: locations) {
    if (CDFZarrReader::isZarrLocation(location.first) != location.second) {
      CDBError("[FAILED] Zarr: isZarrLocation(%s) should be %d", location.first.c_str(), location.second);
      failed++;
    }
  }

  // Opening a store that does not exist fails
  {
    CDFObject zarr;
    CDFZarrReader zarrReader;
    zarr.attachCDFReader(&zarrReader);
    if (zarr.open((directory + "does_not_exist.zarr").c_str()) == 0) {
      CDBError("[FAILED] Zarr: opening a store that does not exist succeeded");
      failed++;
    }
  }
  if (!failed) {
    CDBDebug("[OK] testZarrReader");
  }
  return failed > 0;
}

int main(int, char **) {
  bool failed = false;
  CDBDebug("Testing CTime");
  CDF::Variable *testVarA = new CDF::Variable();
  testVarA->setAttributeText("units", "seconds since 1904-01-01 00:00:00.000 00:00");
  CDF::Variable *testVarB = new CDF::Variable();
  testVarB->setAttributeText("units", "seconds since 2004-01-01 00:00:00.000 00:00");

  if (testCTimeInit(testVarA, "21130221T212000") != 0) failed = true;
  if (testCTime2Init(testVarA, 3490341753.000000) != 0) failed = true;
  if (testCTimeInit(testVarB, "22130222T212000") != 0) failed = true;
  if (testCTime2Init(testVarB, 0334581753.000000) != 0) failed = true;
  if (testCTimeInit(testVarA, "21130221T212000") != 0) failed = true;
  if (testCTimeInit(testVarB, "22130222T212000") != 0) failed = true;

  if (testCTimeEpochTimeConversion() != 0) failed = true;

  if (testHDF5Reader() != 0) failed = true;

  if (testNetCDFChunkCacheFromEnvironment() != 0) failed = true;

  if (testZarrReader() != 0) failed = true;

  CTime::cleanInstances();
  delete testVarA;
  delete testVarB;
  if (failed) {
    CDBError("[FAILED] Some tests failed.");
  }
  return failed;
}
