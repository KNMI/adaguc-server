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

#ifndef CCDFNETCDFIOWRITER_H
#define CCDFNETCDFIOWRITER_H

#include <string>
#include <vector>
#include <netcdf.h>
#include "CCDFDataModel.h"
#include "CDebugger.h"

class CDFNetCDFWriter {
private:
  bool writeData;
  bool readData;
  bool listNCCommands;
  std::string NCCommands;
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
  static std::string NCtypeConversionToString(CDFType type);
  std::string getNCCommands();
  void setNetCDFMode(int mode);
  void disableVariableWrite();
  void disableReadData();
  void setDeflateShuffle(int deflate, int deflate_level, int shuffle);
  void recordNCCommands(bool enable);
  int write(const char *fileName);
  int write(const char *fileName, void (*progress)(const char *message, float percentage));
};

#endif
