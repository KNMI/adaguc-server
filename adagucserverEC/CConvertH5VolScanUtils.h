/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  Utils for conversion HDF5 volume scan data to CDM
 * Author:   Mats Veldhuizen mats.veldhuizen "at" knmi.nl
 * Date:     2025-03-20
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

#ifndef CCONVERTH5VOLSCANUTILS_H
#define CCONVERTH5VOLSCANUTILS_H
#include <tuple>
#include <vector>
#include "CDataSource.h"
#include "CImageWarper.h"
#include "COGCDims.h"

int checkIfIsH5VolScan(CDFObject *cdfObject);
std::tuple<double, int, double, int, double> getScanMetadata(CDFObject *cdfObject, int scan);
std::tuple<double, double, double> getRadarLocation(CDFObject *cdfObject);
CT::string getRadarStartTime(CDFObject *cdfObject);
std::vector<CT::string> getScanParams(CDFObject *cdfObject);
std::vector<CT::string> getUnits(CDFObject *cdfObject);
bool hasParam(CDFObject *cdfObject, std::vector<int> sorted_scans, CT::string param);
CDF::Variable *getDataVarForParam(CDFObject *cdfObject, int scan, CT::string param);
std::tuple<double, double, double, double> getCalibrationParameters(CDFObject *cdfObject, int scan, CT::string param);
#endif
