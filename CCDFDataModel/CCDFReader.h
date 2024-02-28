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

#ifndef CCDFREADER_H
#define CCDFREADER_H

#include "CCDFDataModel.h"
#include "CCDFVariable.h"
#include "CCDFObject.h"

class CDFReader {
public:
  CT::string fileName;
  CDFReader() { cdfObject = NULL; }
  virtual ~CDFReader() {}
  CDFObject *cdfObject;
  virtual int open(const char *fileName) = 0;
  virtual int close() = 0;

  // These two function may only be used by the variable class itself (TODO create friend class, protected?).
  virtual int _readVariableData(CDF::Variable *var, CDFType type) = 0;
  // Allocates and reads the variable data
  virtual int _readVariableData(CDF::Variable *var, CDFType type, size_t *start, size_t *count, ptrdiff_t *stride) = 0;
};

#endif
