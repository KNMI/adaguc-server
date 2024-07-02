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

#ifndef CCDFHDF5IO_H
#define CCDFHDF5IO_H

#include "CCDFDataModel.h"
#include <stdio.h>
#include <cfloat>
#include <vector>
#include <iostream>
#include <hdf5.h>
#include "CDebugger.h"
#include "CTime.h"
#include "CProj4ToCF.h"
#include <proj.h>
// #define CCDFHDF5IO_DEBUG

#define CCDFHDF5IO_GROUPSEPARATOR "."

#define KNMI_VOLSCAN_PROJ4 "+proj=stere +lat_0=90 +lon_0=0 +lat_ts=60 +a=6378.14 +b=6356.75 +x_0=0 y_0=0 +unit=km"

void ncError(int line, const char *className, const char *msg, int e);
class CDFHDF5Reader : public CDFReader {
private:
  CT::string fileName;
  bool fileIsOpen;
  CDFType typeConversion(hid_t type);

  hid_t cdfTypeToHDFType(CDFType type);

  DEF_ERRORFUNCTION();

  int readDimensions() { return 0; }
  int readAttributes(std::vector<CDF::Attribute *> &, int, int) { return 0; }
  int readVariables() { return 0; }
  hid_t H5F_file;
  herr_t status;
  std::vector<hid_t> opengroups;
  // HDF5 error handling
  herr_t (*old_func)(hid_t, void *);

  void *old_client_data;
  hid_t error_stack;
  bool b_EnableKNMIHDF5toCFConversion;
  bool b_EnableODIMHDF5toCFConversion;
  bool b_KNMIHDF5UseEndTime;

public:
  class CustomForecastReader : public CDF::Variable::CustomReader {
  public:
    ~CustomForecastReader() {}
    int readData(CDF::Variable *thisVar, size_t *start, size_t *count, ptrdiff_t *stride);
  };

  CDFHDF5Reader() : CDFReader() {
#ifdef CCDFHDF5IO_DEBUG
    CDBDebug("CCDFHDF5IO init");
#endif
    H5F_file = -1;
    // Get error strack
    error_stack = H5Eget_current_stack();
    /* Save old error handler */
    // H5Eget_auto2(error_stack, &old_func, &old_client_data);
    /* Turn off error handling */
    H5Eset_auto2(error_stack, NULL, NULL);
    H5Eset_auto2(H5E_DEFAULT, NULL, NULL);
    b_EnableKNMIHDF5toCFConversion = false;
    b_EnableODIMHDF5toCFConversion = false;
    b_KNMIHDF5UseEndTime = false;
    forecastReader = NULL;
    fileIsOpen = false;
  }
  ~CDFHDF5Reader();

  int readAttributes(std::vector<CDF::Attribute *> &attributes, hid_t HDF5_group);

  CDF::Dimension *makeDimension(const char *name, size_t len);

  void list(hid_t groupID, char *groupName);

  static int HDF5ToADAGUCTime(char *pszADAGUCTime, const char *pszRadarTime);

  void enableKNMIHDF5toCFConversion();

  void enableKNMIHDF5UseEndTime();

  int convertNWCSAFtoCF();

  int convertLSASAFtoCF();

  int convertKNMIH5VolScan();

  int convertKNMIHDF5toCF();

  int convertODIMHDF5toCF();

  int open(const char *fileName);
  int close();

  void closeH5GroupByName(const char *variableGroupName);
  hid_t openH5GroupByName(char *varNameOut, size_t maxVarNameLen, const char *variableGroupName);
  int _readVariableData(CDF::Variable *var, CDFType type, size_t *start, size_t *count, ptrdiff_t *);
  int _readVariableData(CDF::Variable *var, CDFType type);

private:
  CustomForecastReader *forecastReader;

  static CDF::Variable *getWhatVar(CDFObject *cdfObject, size_t datasetCounter, int dataCounter);
  static CDF::Attribute *getNestedAttribute(CDFObject *cdfObject, size_t datasetCounter, int dataCounter, const char *varName, const char *attrName);
};

#endif
