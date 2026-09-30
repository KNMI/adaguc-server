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

#ifndef CCDFNETCDFIO_H
#define CCDFNETCDFIO_H

#include <cstdio>
#include <vector>
#include <iostream>
#include <netcdf.h>
#include <cmath>
#include "CCDFDataModel.h"
#include "CCDFReader.h"
#include "CDebugger.h"

class CDFNetCDFReader : public CDFReader {
private:
  static CDFType _typeConversionVar(nc_type type, bool isUnsigned);
  static CDFType _typeConversionAtt(nc_type type);

  int status, root_id;
  bool keepFileOpen;
  int _readDimensions(int groupId, std::string &groupName);
  int _readAttributes(int root_id, std::vector<CDF::Attribute *> &attributes, int varID, int natt);
  /**
   * @param mode, mode = 0: read dims, 1: read variables
   */
  int _readVariables(int groupId, std::string &groupName, int mode);
  int _netcdfReOpen(CDF::Variable *var);
  int _readStringVariableData(CDF::Variable *var, size_t *start, size_t *count, ptrdiff_t *stride, int varGroupId, bool useStartCount, bool useStriding);
  int _readVariableRequestedType(CDF::Variable *var, CDFType type, size_t *start, size_t *count, ptrdiff_t *stride, int varGroupId, bool useStartCount, bool useStriding);
  int _readVariableNativeType(CDF::Variable *var, CDFType type, size_t *start, size_t *count, ptrdiff_t *stride, int varGroupId, bool useStartCount, bool useStriding);
  int cdfReadVariableData(CDF::Variable *var, CDFType type);
  int cdfReadVariableData(CDF::Variable *var, CDFType type, size_t *start, size_t *count, ptrdiff_t *stride);

  int _findNCGroupIdForCDFVariable(const std::string &varName);

public:
  CDFNetCDFReader();
  ~CDFNetCDFReader();
  int open(const char *fileName);
  int close();
};

#endif
