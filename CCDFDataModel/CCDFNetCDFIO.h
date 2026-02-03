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

#ifndef CCDFNETCDFIO_H
#define CCDFNETCDFIO_H

#include <stdio.h>
#include <vector>
#include <iostream>
#include <netcdf.h>
#include <cmath>
#include "CCDFDataModel.h"
#include "CCDFReader.h"
#include "CDebugger.h"

//  #define CCDFNETCDFIO_DEBUG
// #define CCDFNETCDFIO_DEBUG_OPEN
// #define CCDFNETCDFWRITER_DEBUG

class CDFNetCDFReader : public CDFReader {
private:
  static void ncError(int line, const char *msg, int e);

  // CCDFWarper warper;
  static CDFType _typeConversionVar(nc_type type, bool isUnsigned);
  static CDFType _typeConversionAtt(nc_type type);

  int status, root_id;
  bool keepFileOpen;
  int readDimensions(int groupId, CT::string *groupName);
  int readAttributes(int root_id, std::vector<CDF::Attribute *> &attributes, int varID, int natt);
  /**
   * @param mode, mode = 0: read dims, 1: read variables
   */
  int readVariables(int groupId, CT::string *groupName, int mode);
  int _readVariableData(CDF::Variable *var, CDFType type);
  int _readVariableData(CDF::Variable *var, CDFType type, size_t *start, size_t *count, ptrdiff_t *stride);

  int _findNCGroupIdForCDFVariable(CT::string *varName);

public:
  CDFNetCDFReader();
  ~CDFNetCDFReader();
  void enableLonWarp(bool enableLonWarp);
  int open(const char *fileName);
  int close();
};

class CDFNetCDFWriter {
private:
  static void ncError(int line, const char *msg, int e);
  bool writeData;
  bool readData;
  bool listNCCommands;
  CT::string NCCommands;
  const char *fileName;
  int shuffle;
  int deflate;
  int deflate_level;
  std::vector<CDF::Dimension *> dimensions;
  CDFObject *cdfObject;

  int root_id, status;
  int netcdfMode;
  int _write(void (*progress)(const char *message, float percentage));
  int copyVar(CDF::Variable *variable, int nc_var_id, size_t *start, size_t *count);

public:
  CDFNetCDFWriter(CDFObject *cdfObject);
  ~CDFNetCDFWriter();
  static nc_type NCtypeConversion(CDFType type);
  static CT::string NCtypeConversionToString(CDFType type);
  CT::string getNCCommands();
  void setNetCDFMode(int mode);
  void disableVariableWrite();
  void disableReadData();
  void setDeflateShuffle(int deflate, int deflate_level, int shuffle);
  void recordNCCommands(bool enable);
  int write(const char *fileName);
  int write(const char *fileName, void (*progress)(const char *message, float percentage));
};

#endif
