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

#ifndef CCDFPNGIO_H
#define CCDFPNGIO_H
#include "CDebugger.h"
#include "CCDFDataModel.h"
#include <stdio.h>
#include <vector>
#include <iostream>
#include "CDebugger.h"
#include "CTime.h"
#include "CProj4ToCF.h"
#include "CReadPNG.h"
// #define CCDFPNGIO_DEBUG

class CDFPNGReader : public CDFReader {
private:
  DEF_ERRORFUNCTION();
  bool isSlippyMapFormat = false;
  size_t rasterWidth = 0;
  size_t rasterHeight = 0;
  CPNGRaster *pngRaster = nullptr;

public:
  ~CDFPNGReader();

  int open(const char *fileName);

  int close();

  // These two function may only be used by the variable class itself (TODO create friend class, protected?).
  int _readVariableData(CDF::Variable *var, CDFType type);

  // Allocates and reads the variable data
  int _readVariableData(CDF::Variable *var, CDFType type, size_t *start, size_t *count, ptrdiff_t *stride);
};

#endif
