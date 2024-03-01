/******************************************************************************
 *
 * Project:  Generic common data format
 * Purpose:  Generic Data model to read netcdf and hdf5
 * Author:   Ernst de Vreede, ernst.de.vreede "at" knmi.nl
 * Date:     2016-03-05
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

#ifndef CCDFGEOJSONIO_H
#define CCDFGEOJSONIO_H

#include <stdio.h>
#include <vector>
#include <iostream>
#include <netcdf.h>
#include <math.h>
#include <strings.h>
#include "CCDFDataModel.h"
#include "CCDFReader.h"
#include "CDebugger.h"

// #define CCDFGEOJSONIO_DEBUG
// #define CCDFGEOJSONIO_DEBUG_OPEN

class CDFGeoJSONReader : public CDFReader {
private:
  static void ncError(int line, const char *className, const char *msg, int e);

  // CCDFWarper warper;
  static CDFType typeConversion(nc_type type);
  DEF_ERRORFUNCTION();
  int status = 0, root_id = 0;
  int nDims = 0, nVars = 0, nRootAttributes = 0, unlimDimIdP = 0;
  bool keepFileOpen = -1;
  int readDimensions();
  int readAttributes(std::vector<CDF::Attribute *> &attributes, int varID, int natt);
  int readVariables();

public:
  CDFGeoJSONReader();
  ~CDFGeoJSONReader();

  // void enableLonWarp(bool enableLonWarp);

  int open(const char *fileName);

  int close();

  int _readVariableData(CDF::Variable *var, CDFType type);

  int _readVariableData(CDF::Variable *var, CDFType type, size_t *start, size_t *count, ptrdiff_t *stride);
};
#endif
