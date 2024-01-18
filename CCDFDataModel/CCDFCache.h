/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Generic Data model to read netcdf and hdf5
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2013-06-01
 *
 ******************************************************************************
 *
 * Copyright 2013, Royal Netherlands Meteorological Institute (KNMI)
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

#ifndef CCDFCACHE_H
#define CCDFCACHE_H

#include <stdio.h>
#include <vector>
#include <iostream>
#include <sys/stat.h>
#include "CTypes.h"
#include "CDirReader.h"
#include "CCDFObject.h"
#include "CCache.h"
//#define CCDFCACHE_DEBUG

#include "CDebugger.h"

class CDFCache {
private:
  CCache *cache;
  CT::string cacheDir;

  CCache *getCCache(const char *directory, const char *fileName);

  int writeBinaryData(const char *filename, void **data, CDFType type, size_t varSize);
  int readBinaryData(const char *filename, void **data, CDFType type, size_t &varSize);

public:
  DEF_ERRORFUNCTION();

  CDFCache() { cache = NULL; }
  CDFCache(CT::string cacheDir) {
    // CDBDebug("DIRECTORY %s",cacheDir.c_str());
    this->cacheDir = cacheDir;
    cache = NULL;
  }
  ~CDFCache() { delete cache; }
  // Saves or returns size and data.
  int readVariableData(CDF::Variable *var, CDFType type, size_t *start, size_t *count, ptrdiff_t *stride, bool readOrWrite);

  // Saves or returns the netcdf header
  int open(const char *fileName, CDFObject *cdfObject, bool readOrWrite);
};

#endif
