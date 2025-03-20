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

#include <tuple>
#include <vector>
#include "CConvertH5VolScanUtils.h"
#include "CCDFHDF5IO.h"

const CT::string scan_params_odim[] = {"CCORH", "CCORV", "CPAH", "CPAV", "KDP", "PHIDP", "RHOHV", "SQIH", "VRADH", "VRADV", "WRADH", "WRADV", "DBZH", "DBZV", "TH", "TV", "ZDR", "Height"};
const CT::string units_odim[] = {"dB", "dB", "-", "-", "deg/km", "deg", "-", "-", "m/s", "m/s", "m/s", "m/s", "dBZ", "dBZ", "dBZ", "dBZ", "dB", "km"};

const CT::string scan_params_knmi[] = {"CCOR", "CCORv", "CPA", "CPAv", "KDP", "PhiDP", "RhoHV", "SQI", "V", "Vv", "W", "Wv", "Z", "Zv", "uPhiDP", "uZ", "uZv", "ZDR", "Height"};
const CT::string units_knmi[] = {"dB", "dB", "-", "-", "deg/km", "deg", "-", "-", "m/s", "m/s", "m/s", "m/s", "dBZ", "dBZ", "deg", "dBZ", "dBZ", "dB", "km"};

enum FileType { ODIM_H5, KNMI_H5 };

FileType checkH5VolScanType(CDFObject *cdfObject) {
  /* If ODIM file then don't handle as KNMI hdf5, for support of hybrid format */
  CDF::Attribute *conventionsAttr = cdfObject->getAttributeNE("Conventions");
  if (conventionsAttr != nullptr) {
    CT::string conventionsString = conventionsAttr->getDataAsString();
    if (conventionsString.startsWith("ODIM_H5")) {
      return ODIM_H5;
    }
  }
  return KNMI_H5;
}

int checkIfIsH5VolScan(CDFObject *cdfObject) {
  try {
    if (checkH5VolScanType(cdfObject) == ODIM_H5) {
      CDF::Variable *whatVar = cdfObject->getVariable("what");
      CDF::Attribute *whatObjectAttr = whatVar->getAttribute("object");
      CT::string whatObjectString = whatObjectAttr->getDataAsString();
      if (not whatObjectString.equals("SCAN") && not whatObjectString.equals("PVOL")) {
        CDBDebug("Is not a volume or scan dataset, skipping parsing as ODIM volume dataset");
        return 1;
      }
      return 0;
    } else {
      /* Check if the overview variable and number_scan_groups is set and not 0 */
      CDF::Attribute *attr = cdfObject->getVariable("overview")->getAttribute("number_scan_groups");
      int number_scan_groups;
      attr->getData(&number_scan_groups, 1);
      if (number_scan_groups == 0) return 1;
      return 0;
    }
  } catch (int e) {
    return 1;
  }
}

std::tuple<double, int, double, int, double> getScanMetadata(CDFObject *cdfObject, int scan) {
  if (checkH5VolScanType(cdfObject) == ODIM_H5) {
    CT::string scanVarWhereName;
    scanVarWhereName.print("dataset%1d.where", scan);
    CDF::Variable *scanVarWhere = cdfObject->getVariableNE(scanVarWhereName.c_str());
    if (scanVarWhere == nullptr) return std::make_tuple(-1.0, -1, -1.0, -1, -1.0);
    double scan_elevation;
    scanVarWhere->getAttribute("elangle")->getData<double>(&scan_elevation, 1);
    int scan_nrang;
    scanVarWhere->getAttribute("nbins")->getData<int>(&scan_nrang, 1);
    double scan_rscale;
    scanVarWhere->getAttribute("rscale")->getData<double>(&scan_rscale, 1);
    int scan_nazim;
    scanVarWhere->getAttribute("nrays")->getData<int>(&scan_nazim, 1);
    double scan_ascale = 360.0 / scan_nazim;
    return std::make_tuple(scan_elevation, scan_nrang, scan_rscale * 0.001, scan_nazim, scan_ascale);
  } else {
    CT::string scanVarName;
    scanVarName.print("scan%1d", scan);
    CDF::Variable *scanVar = cdfObject->getVariableNE(scanVarName.c_str());
    if (scanVar == nullptr) return std::make_tuple(-1.0, -1, -1.0, -1, -1.0);
    double scan_elevation;
    scanVar->getAttribute("scan_elevation")->getData<double>(&scan_elevation, 1);
    int scan_nrang;
    scanVar->getAttribute("scan_number_range")->getData<int>(&scan_nrang, 1);
    double scan_rscale;
    scanVar->getAttribute("scan_range_bin")->getData<double>(&scan_rscale, 1);
    int scan_nazim;
    scanVar->getAttribute("scan_number_azim")->getData<int>(&scan_nazim, 1);
    double scan_ascale;
    scanVar->getAttribute("scan_azim_bin")->getData<double>(&scan_ascale, 1);
    return std::make_tuple(scan_elevation, scan_nrang, scan_rscale, scan_nazim, scan_ascale);
  }
}

std::tuple<double, double, double> getRadarLocation(CDFObject *cdfObject) {
  if (checkH5VolScanType(cdfObject) == ODIM_H5) {
    double radarLon;
    double radarLat;
    double radarHeight;
    cdfObject->getVariable("where")->getAttribute("lon")->getData<double>(&radarLon, 1);
    cdfObject->getVariable("where")->getAttribute("lat")->getData<double>(&radarLat, 1);
    cdfObject->getVariable("where")->getAttribute("height")->getData<double>(&radarHeight, 1);
    return std::make_tuple(radarLon, radarLat, radarHeight * 0.001);
  } else {
    double radarLonLat[2];
    cdfObject->getVariable("radar1")->getAttribute("radar_location")->getData<double>(radarLonLat, 2);
    double radarLon = radarLonLat[0];
    double radarLat = radarLonLat[1];
    return std::make_tuple(radarLon, radarLat, 0.0);
  }
}

CT::string getRadarStartTime(CDFObject *cdfObject) {
  if (checkH5VolScanType(cdfObject) == ODIM_H5) {
    CT::string h5Date = cdfObject->getVariable("what")->getAttribute("date")->getDataAsString();
    CT::string h5Time = cdfObject->getVariable("what")->getAttribute("time")->getDataAsString();
    CT::string timeString;
    timeString.print("%sT%s00Z", h5Date.c_str(), h5Time.substring(0, 4).c_str());
    return timeString;
  } else {
    char szStartTime[100];
    CT::string h5Time = cdfObject->getVariable("overview")->getAttribute("product_datetime_start")->getDataAsString();
    CDFHDF5Reader::HDF5ToADAGUCTime(szStartTime, h5Time.c_str());
    return CT::string(szStartTime);
  }
}

std::vector<CT::string> getScanParams(CDFObject *cdfObject) {
  if (checkH5VolScanType(cdfObject) == ODIM_H5) {
    return std::vector<CT::string>(std::begin(scan_params_odim), std::end(scan_params_odim));
  } else {
    return std::vector<CT::string>(std::begin(scan_params_knmi), std::end(scan_params_knmi));
  }
}

std::vector<CT::string> getUnits(CDFObject *cdfObject) {
  if (checkH5VolScanType(cdfObject) == ODIM_H5) {
    return std::vector<CT::string>(std::begin(units_odim), std::end(units_odim));
  } else {
    return std::vector<CT::string>(std::begin(units_knmi), std::end(units_knmi));
  }
}

int findOdimParamNum(CDFObject *cdfObject, int scan, CT::string param) {
  /* Assume no more than 99 params */
  for (int paramNum = 1; paramNum < 100; paramNum++) {
    CT::string dataWhatVarName;
    dataWhatVarName.print("dataset%1d.data%1d.what", scan, paramNum);
    CDF::Variable *dataWhatVar = cdfObject->getVariableNE(dataWhatVarName.c_str());
    if (dataWhatVar == nullptr) break;
    CT::string quantity = dataWhatVar->getAttribute("quantity")->getDataAsString();
    if (quantity.equals(param)) return paramNum;
  }
  return -1;
}

bool hasParam(CDFObject *cdfObject, std::vector<int> sorted_scans, CT::string param) {
  if (checkH5VolScanType(cdfObject) == ODIM_H5) {
    int paramNum = findOdimParamNum(cdfObject, sorted_scans[0], param);
    if (paramNum == -1 && !param.equals("Height")) {
      if (!param.equals("ZDR")) return false;
      int paramNumDBZV = findOdimParamNum(cdfObject, sorted_scans[0], CT::string("DBZV"));
      int paramNumDBZH = findOdimParamNum(cdfObject, sorted_scans[0], CT::string("DBZH"));
      if (paramNumDBZV == -1 || paramNumDBZH == -1) return false;
    }
    return true;
  } else {
    CT::string dataVarName;
    dataVarName.print("scan%1d.scan_%s_data", sorted_scans[0], param.c_str());
    CDF::Variable *dataVar = cdfObject->getVariableNE(dataVarName.c_str());
    if (dataVar == nullptr && !param.equals("Height")) {
      if (!param.equals("ZDR")) return false;
      CT::string dataDBZVName;
      dataDBZVName.print("scan%1d.scan_Zv_data", sorted_scans[0]);
      CDF::Variable *dataDBZV = cdfObject->getVariableNE(dataDBZVName.c_str());
      CT::string dataDBZHName;
      dataDBZHName.print("scan%1d.scan_Z_data", sorted_scans[0]);
      CDF::Variable *dataDBZH = cdfObject->getVariableNE(dataDBZHName.c_str());
      if (dataDBZV == nullptr || dataDBZH == nullptr) return false;
    }
    return true;
  }
}

CDF::Variable *getDataVarForParam(CDFObject *cdfObject, int scan, CT::string param) {
  if (checkH5VolScanType(cdfObject) == ODIM_H5) {
    int paramNum = findOdimParamNum(cdfObject, scan, param);
    if (paramNum == -1) return nullptr;
    CT::string dataVarName;
    dataVarName.print("dataset%1d.data%1d.data", scan, paramNum);
    CDF::Variable *dataVar = cdfObject->getVariableNE(dataVarName.c_str());
    return dataVar;
  } else {
    if (param.equals("DBZH")) param = CT::string("Z");
    if (param.equals("DBZV")) param = CT::string("Zv");
    CT::string dataVarName;
    dataVarName.print("scan%1d.scan_%s_data", scan, param.c_str());
    CDF::Variable *dataVar = cdfObject->getVariableNE(dataVarName.c_str());
    return dataVar;
  }
}

std::tuple<double, double, double, double> getCalibrationParameters(CDFObject *cdfObject, int scan, CT::string param) {
  /* Use doubles here so that the most integer values can be represented exactly */
  if (checkH5VolScanType(cdfObject) == ODIM_H5) {
    int paramNum = findOdimParamNum(cdfObject, scan, param);
    CT::string dataWhatVarName;
    dataWhatVarName.print("dataset%1d.data%1d.what", scan, paramNum);
    CDF::Variable *dataWhatVar = cdfObject->getVariable(dataWhatVarName.c_str());
    double gain;
    double offset;
    CDF::Attribute *gainAttr = dataWhatVar->getAttribute("gain");
    CDF::Attribute *offsetAttr = dataWhatVar->getAttribute("offset");
    if (gainAttr != nullptr && offsetAttr != nullptr) {
      gainAttr->getData<double>(&gain, 1);
      offsetAttr->getData<double>(&offset, 1);
    } else {
      CDBDebug("Using default gain/offset");
      gain = 1.0;
      offset = 0.0;
    }
    double undetect;
    dataWhatVar->getAttribute("undetect")->getData<double>(&undetect, 1);

    double nodata;
    CDF::Attribute *nodataAttr = dataWhatVar->getAttributeNE("nodata");
    if (nodataAttr == nullptr) {
      nodata = undetect;
    } else {
      nodataAttr->getData<double>(&nodata, 1);
    }
    return std::make_tuple(gain, offset, undetect, nodata);
  } else {
    if (param.equals("DBZH")) param = CT::string("Z");
    if (param.equals("DBZV")) param = CT::string("Zv");
    CT::string scanCalibrationVarName;
    scanCalibrationVarName.print("scan%1d.calibration", scan);
    CDF::Variable *scanCalibrationVar = cdfObject->getVariable(scanCalibrationVarName);
    CT::string componentCalibrationStringName;
    componentCalibrationStringName.print("calibration_%s_formulas", param.c_str());
    CT::string formula = scanCalibrationVar->getAttribute(componentCalibrationStringName.c_str())->getDataAsString();
    int rightPartFormulaPos = formula.indexOf("=");
    int multiplicationSignPos = formula.indexOf("*");
    int additionSignPos = formula.indexOf("+");
    double gain;
    double offset;
    if (rightPartFormulaPos != -1 && multiplicationSignPos != -1 && additionSignPos != -1) {
      gain = formula.substring(rightPartFormulaPos + 1, multiplicationSignPos).trim().toDouble();
      offset = formula.substring(additionSignPos + 1, formula.length()).trim().toDouble();
    } else {
      CDBDebug("Using default gain/offset");
      gain = 1.0;
      offset = 0.0;
    }
    double undetect;
    scanCalibrationVar->getAttribute("calibration_missing_data")->getData<double>(&undetect, 1);

    double nodata;
    CDF::Attribute *nodataAttr = scanCalibrationVar->getAttributeNE("calibration_out_of_image");
    if (nodataAttr == nullptr) {
      nodata = undetect;
    } else {
      nodataAttr->getData<double>(&nodata, 1);
    }
    return std::make_tuple(gain, offset, undetect, nodata);
  }
}
