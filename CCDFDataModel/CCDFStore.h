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

#ifndef CCDFSTORE_H
#define CCDFSTORE_H

#include <cstdio>
#include <vector>
#include <iostream>
#include <sys/stat.h>
#include "CTString.h"
#include "CDirReader.h"
#include "CCDFObject.h"
#include "CCDFReader.h"
#include "CDebugger.h"
// #define CCDFSTORE_DEBUG
typedef std::map<std::string, CDFReader *> CDFStore_CDFReadersMap;
typedef std::map<std::string, CDFReader *>::iterator CDFStore_CDFReadersIterator;

class CDFStore {
private:
  static CDFStore_CDFReadersMap cdfReaders;

public:
  DEF_ERRORFUNCTION();
  static CDFReader *getCDFReader(const char *fileName);
  static void clear();
};

#endif
