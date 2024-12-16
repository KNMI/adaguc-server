/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  Convert KNMI HDF5 volume scan data to CDM
 * Author:   Ernst de Vreede, ernst.de.vreede "at" knmi.nl
 * Date:     2022-08-15
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

#include "CConvertKNMIH5VolScan.h"
#include "CImageWarper.h"
#include "COGCDims.h"
#include "CCDFHDF5IO.h"

// #define CCONVERTKNMIH5VOLSCAN_DEBUG
const char *CConvertKNMIH5VolScan::className = "CConvertKNMIH5VolScan";

int CConvertKNMIH5VolScan::checkIfIsKNMIH5VolScan(CDFObject *cdfObject, CServerParams *) {
  try {
    /* Check if the image1.statistics variable and stat_cell_number is set */
    CDF::Attribute *attr = cdfObject->getVariable("overview")->getAttribute("number_scan_groups");
    int number_scan_groups;
    attr->getData(&number_scan_groups, 1);
    if (number_scan_groups == 0) return 1;
  } catch (int e) {
    return 1;
  }
  return 0;
}

bool sortFunction(CT::string one, CT::string other) {
  if (one.endsWith("s")) return true;
  return (std::atof(one) < std::atof(other));
}

int CConvertKNMIH5VolScan::convertKNMIH5VolScanHeader(CDFObject *cdfObject, CServerParams *srvParams) {
  if (checkIfIsKNMIH5VolScan(cdfObject, srvParams) != 0) return 1;
  CDBDebug("convertKNMIH5VolScanHeader()");

  int scan_i = 0;
  int nrscans = 0;
  std::vector<int> scans;
  std::vector<CT::string> elevation_names;
  while (true) {
    scan_i++;
    CT::string scanVarName;
    scanVarName.print("scan%1d", scan_i);
    CDF::Variable *scanVar = cdfObject->getVariableNE(scanVarName.c_str());
    if (scanVar == NULL) break;
    float scanElevation;
    scanVar->getAttribute("scan_elevation")->getData(&scanElevation, 1);
    int scanElevationInt = lround(scanElevation * 10.0);
    /* Skip 90 degree scan */
    if (scanElevationInt == 900) continue;
    CT::string elevation_name;
    elevation_name.print("%d.%d", scanElevationInt / 10, scanElevationInt % 10);
    /* Dutch radars contain 3 0.3 degree scans. 1st is long range, second is short range, third is long range again. */
    /* Keep the first 2 and skip the last. */
    bool shortRangePresent = false;
    int scanElevationIndex = -1;
    for (int i = 0; i < nrscans; i++) {
      if (elevation_names[i].equals(elevation_name)) {
        scanElevationIndex = i;
      }
      if (elevation_names[i].equals(CT::string("0.3l"))) {
        shortRangePresent = true;
      }
    }
    if (scanElevationIndex >= 0) {
      if (scanElevationInt == 3 && !shortRangePresent) {
        elevation_names[scanElevationIndex] = CT::string("0.3l");
      } else {
        continue;
      }
    }
    scans.push_back(scan_i);
    elevation_names.push_back(elevation_name);
    nrscans++;
  }
  /* Sort by elevation_name */
  std::vector<CT::string> elevation_names_original(elevation_names);
  std::sort(elevation_names.begin(), elevation_names.end(), sortFunction);
  std::vector<int> sorted_scans;
  for (int i = 0; i < nrscans; i++) {
    int sort_index = -1;
    for (int j = 0; j < nrscans; j++) {
      if (elevation_names[i].equals(elevation_names_original[j])) {
        sort_index = j;
      }
    }
    if (sort_index == -1) return 1;
    sorted_scans.push_back(scans[sort_index]);
  }

  CT::string scan_params[] = {"KDP", "PhiDP", "RhoHV", "V", "W", "Z", "Zv", "ZDR"};
  CT::string units[] = {"deg/km", "deg", "-", "-", "-", "dbZ", "dbZ", "dbZ"};

  for (size_t v = 0; v < cdfObject->variables.size(); v++) {
    CDF::Variable *var = cdfObject->variables[v];
    CT::string *terms = var->name.splitToArray(".");
    if (terms->count > 1) {
      if (terms[0].startsWith("scan") && terms[1].startsWith("scan_") && terms[1].endsWith("_data")) {
        var->setAttributeText("ADAGUC_SKIP", "TRUE");
      }
    }
    if (var->name.startsWith("visualisation")) {
      var->setAttributeText("ADAGUC_SKIP", "TRUE");
    }
    if (var->name.startsWith("dataset")) {
      var->setAttributeText("ADAGUC_SKIP", "TRUE");
    }
    if (var->name.startsWith("how")) {
      var->setAttributeText("ADAGUC_SKIP", "TRUE");
    }
    if (var->name.startsWith("what")) {
      var->setAttributeText("ADAGUC_SKIP", "TRUE");
    }
    if (var->name.startsWith("where")) {
      var->setAttributeText("ADAGUC_SKIP", "TRUE");
    }
  }

  // double *dfBBOX = new double[4];
  double dfBBOX[] = {0, 48, 10, 58}; // TODO: retrieve actual bbox from HDF5
  // Default size of adaguc 2dField is 2x2
  int width = 2;  // swathMiddleLon->dimensionlinks[1]->getSize();
  int height = 2; // swathMiddleLon->dimensionlinks[0]->getSize();

  if (srvParams == NULL) {
    CDBError("srvParams is not set");
    return 1;
  }
  if (srvParams->Geo->dWidth > 1 && srvParams->Geo->dHeight > 1) {
    width = srvParams->Geo->dWidth;
    height = srvParams->Geo->dHeight;
  }

#ifdef CCONVERTKNMIH5VOLSCAN_DEBUG
  CDBDebug("Width = %d, Height = %d", width, height);
#endif
  if (width < 2 || height < 2) {
    CDBError("width and height are too small");
    return 1;
  }

  double cellSizeX = (dfBBOX[2] - dfBBOX[0]) / double(width);
  double cellSizeY = (dfBBOX[3] - dfBBOX[1]) / double(height);
  double offsetX = dfBBOX[0];
  double offsetY = dfBBOX[1];
  // delete[] dfBBOX;
  // dfBBOX = NULL;

  // Add geo variables, only if they are not there already
  CDF::Dimension *dimX = cdfObject->getDimensionNE("adaguccoordinatex");
  CDF::Dimension *dimY = cdfObject->getDimensionNE("adaguccoordinatey");
  CDF::Variable *varX = cdfObject->getVariableNE("adaguccoordinatex");
  CDF::Variable *varY = cdfObject->getVariableNE("adaguccoordinatey");
  CDF::Dimension *dimT = new CDF::Dimension();
  CDF::Variable *timeVar = new CDF::Variable();
  CDF::Dimension *dimElevation = new CDF::Dimension();
  CDF::Variable *varElevation = new CDF::Variable();

  if (dimX == NULL || dimY == NULL || varX == NULL || varY == NULL) {
    // If not available, create new dimensions and variables (X,Y,T)
    // For x
    dimX = new CDF::Dimension();
    dimX->name = "adaguccoordinatex";
    dimX->setSize(width);
    cdfObject->addDimension(dimX);
    varX = new CDF::Variable();
    varX->setType(CDF_DOUBLE);
    varX->name.copy("adaguccoordinatex");
    varX->isDimension = true;
    varX->dimensionlinks.push_back(dimX);
    cdfObject->addVariable(varX);
    CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);

    // For y
    dimY = new CDF::Dimension();
    dimY->name = "adaguccoordinatey";
    dimY->setSize(height);
    cdfObject->addDimension(dimY);
    varY = new CDF::Variable();
    varY->setType(CDF_DOUBLE);
    varY->name.copy("adaguccoordinatey");
    varY->isDimension = true;
    varY->dimensionlinks.push_back(dimY);
    cdfObject->addVariable(varY);
    CDF::allocateData(CDF_DOUBLE, &varY->data, dimY->length);

#ifdef CCONVERTKNMIH5VOLSCAN_DEBUG
    CDBDebug("Data allocated for 'x' and 'y' variables (%zu x %zu)", dimX->getSize(), dimY->getSize());
#endif

    // Fill in the X and Y dimensions with the array of coordinates
    for (size_t j = 0; j < dimX->getSize(); j++) {
      double x = offsetX + double(j) * cellSizeX + cellSizeX / 2;
      ((double *)varX->data)[j] = x;
    }
    for (size_t j = 0; j < dimY->getSize(); j++) {
      double y = offsetY + double(j) * cellSizeY + cellSizeY / 2;
      ((double *)varY->data)[j] = y;
    }

    // Create a new time dimension for the new 2D fields.
    dimT->name = "time";
    dimT->setSize(1);
    cdfObject->addDimension(dimT);
    CT::string time = cdfObject->getVariable("overview")->getAttribute("product_datetime_start")->getDataAsString();
    timeVar->setType(CDF_DOUBLE);
    timeVar->name.copy("time");
    timeVar->setAttributeText("standard_name", "time");
    timeVar->setAttributeText("long_name", "time");
    timeVar->isDimension = true;
    char szStartTime[100];
    CT::string time_units = "minutes since 2000-01-01 00:00:00\0";
    CT::string h5Time = cdfObject->getVariable("overview")->getAttribute("product_datetime_start")->getDataAsString();
    CDFHDF5Reader::HDF5ToADAGUCTime(szStartTime, h5Time.c_str());
    // Set adaguc time
    CTime ctime;
    if (ctime.init(time_units, NULL) != 0) {
      CDBError("Could not initialize CTIME: %s", time_units.c_str());
      return 1;
    }
    double offset;
    try {
      offset = ctime.dateToOffset(ctime.stringToDate(szStartTime));
    } catch (int e) {
      CT::string message = CTime::getErrorMessage(e);
      CDBError("CTime Exception %s", message.c_str());
      return 1;
    }

    timeVar->setAttributeText("units", time_units.c_str());
    timeVar->dimensionlinks.push_back(dimT);
    CDF::allocateData(CDF_DOUBLE, &timeVar->data, 1);
    ((double *)timeVar->data)[0] = offset;
    cdfObject->addVariable(timeVar);

    // Create a new elevation dimension for the new 2D fields.
    dimElevation->name = "scan_elevation";
    dimElevation->setSize(nrscans);
    cdfObject->addDimension(dimElevation);
    varElevation->setType(CDF_STRING);
    varElevation->name.copy("scan_elevation");
    CDF::Attribute *unit = new CDF::Attribute("units", "degrees");
    varElevation->addAttribute(unit);
    varElevation->isDimension = true;
    varElevation->dimensionlinks.push_back(dimElevation);
    cdfObject->addVariable(varElevation);
    CDF::allocateData(CDF_STRING, &varElevation->data, dimElevation->length);

    CDF::Variable *varScan = new CDF::Variable();
    varScan->setType(CDF_UINT);
    varScan->name.copy("scan_number");
    varScan->isDimension = false;
    varScan->dimensionlinks.push_back(dimElevation);
    cdfObject->addVariable(varScan);
    CDF::allocateData(varScan->getType(), &varScan->data, dimElevation->length);

    for (int i = 0; i < nrscans; i++) {
      CT::string scanVarName;
      scanVarName.print("scan%1d", sorted_scans[i]);
      CDF::Variable *scanVar = cdfObject->getVariable(scanVarName.c_str());
      float scanElevation;
      scanVar->getAttribute("scan_elevation")->getData(&scanElevation, 1);
      ((char **)varElevation->data)[i] = strdup(elevation_names[i].c_str());
      ((unsigned int *)varScan->data)[i] = sorted_scans[i];
    }
  }
  // CDFHDF5Reader::CustomVolScanReader *volScanReader = new CDFHDF5Reader::CustomVolScanReader();
  // CDF::Variable::CustomMemoryReader *memoryReader = CDF::Variable::CustomMemoryReaderInstance;
  int cnt = 0;
  for (CT::string s : scan_params) {
    CT::string dataVarName;
    dataVarName.print("scan%1d.scan_%s_data", sorted_scans[0], s.c_str());
    CDF::Variable *dataVar = cdfObject->getVariableNE(dataVarName.c_str());
    if (dataVar == NULL) {
      if (!s.equals("ZDR")) continue;
      CT::string dataZvName;
      dataZvName.print("scan%1d.scan_Zv_data", sorted_scans[0]);
      CDF::Variable *dataZv = cdfObject->getVariableNE(dataZvName.c_str());
      CT::string dataZName;
      dataZName.print("scan%1d.scan_Z_data", sorted_scans[0]);
      CDF::Variable *dataZ = cdfObject->getVariableNE(dataZName.c_str());
      if (dataZv == NULL || dataZ == NULL) continue;
    }
    CDF::Variable *var = new CDF::Variable();
    var->setType(CDF_FLOAT);
    var->name.copy(s);
    cdfObject->addVariable(var);
    var->setAttributeText("standard_name", s.c_str());
    var->setAttributeText("long_name", s.c_str());
    var->setAttributeText("grid_mapping", "projection");
    var->setAttributeText("ADAGUC_VOL_SCAN", "TRUE");
    var->setAttributeText("ADAGUC_VECTOR", "true"); /* Set this to true to tell adagucserverEC/CImageDataWriter.cpp, 708 to use full screenspace to retrieve GetFeatureInfo value */
    var->setAttributeText("units", units[cnt].c_str());
    float fillValue = FLT_MAX;
    var->setAttribute("_FillValue", CDF_FLOAT, &fillValue, 1);
    // var->setCustomReader(memoryReader);

    var->dimensionlinks.push_back(dimT);
    var->dimensionlinks.push_back(dimElevation);
    var->dimensionlinks.push_back(dimY);
    var->dimensionlinks.push_back(dimX);

    cnt++;
  }

  return 0;
}

int CConvertKNMIH5VolScan::getCalibrationParameters(CT::string formula, float &factor, float &offset) {
  int rightPartFormulaPos = formula.indexOf("=");
  int multiplicationSignPos = formula.indexOf("*");
  int additionSignPos = formula.indexOf("+");
  if (rightPartFormulaPos != -1 && multiplicationSignPos != -1 && additionSignPos != -1) {
    factor = formula.substring(rightPartFormulaPos + 1, multiplicationSignPos).trim().toFloat();
    offset = formula.substring(additionSignPos + 1, formula.length()).trim().toFloat();
    return 0;
  }
  CDBDebug("Using default factor/offset from %s", formula.c_str());
  factor = 1;
  offset = 0;
  return 1;
}

int CConvertKNMIH5VolScan::convertKNMIH5VolScanData(CDataSource *dataSource, int mode) {
  CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
  if (checkIfIsKNMIH5VolScan(cdfObject, dataSource->srvParams) != 0) return 1;
  if (mode == CNETCDFREADER_MODE_OPEN_ALL) {
    size_t nrDataObjects = dataSource->getNumDataObjects();
    CDataSource::DataObject *dataObjects[nrDataObjects];
    for (size_t d = 0; d < nrDataObjects; d++) {
      dataObjects[d] = dataSource->getDataObject(d);
    }
    CDF::Variable *new2DVar;
    new2DVar = dataObjects[0]->cdfVariable;

    bool doZdr = (new2DVar->name.equals("ZDR"));
    if (doZdr) {
      CT::string dataZdrName;
      dataZdrName.print("scan%1d.scan_ZDR_data", 1); // FIXME
      CDF::Variable *dataZdr = cdfObject->getVariableNE(dataZdrName.c_str());
      if (dataZdr != NULL) {
        doZdr = false;
      }
    }

    // Make the width and height of the new 2D adaguc field the same as the viewing window
    dataSource->dWidth = dataSource->srvParams->Geo->dWidth;
    dataSource->dHeight = dataSource->srvParams->Geo->dHeight;

    // Width needs to be at least 2, the bounding box is calculated from these.
    if (dataSource->dWidth == 1) dataSource->dWidth = 2;
    if (dataSource->dHeight == 1) dataSource->dHeight = 2;
    double cellSizeX = (dataSource->srvParams->Geo->dfBBOX[2] - dataSource->srvParams->Geo->dfBBOX[0]) / double(dataSource->dWidth);
    double cellSizeY = (dataSource->srvParams->Geo->dfBBOX[3] - dataSource->srvParams->Geo->dfBBOX[1]) / double(dataSource->dHeight);
    double offsetX = dataSource->srvParams->Geo->dfBBOX[0];
    double offsetY = dataSource->srvParams->Geo->dfBBOX[1];

#ifdef CCONVERTKNMIH5VOLSCAN_DEBUG
    CDBDebug("Drawing %s with WH = [%d,%d]", "new2DVar" /*new2DVar->name.c_str()*/, dataSource->dWidth, dataSource->dHeight);
    CDBDebug("  %f %f %f %f", cellSizeX, cellSizeY, offsetX, offsetY);
#endif

    CDF::Dimension *dimX;
    CDF::Dimension *dimY;
    CDF::Variable *varX;
    CDF::Variable *varY;

    // Create new dimensions and variables (X,Y,T)
    dimX = cdfObject->getDimension("adaguccoordinatex");
    dimX->setSize(dataSource->dWidth);

    dimY = cdfObject->getDimension("adaguccoordinatey");
    dimY->setSize(dataSource->dHeight);

    varX = cdfObject->getVariable("adaguccoordinatex");
    varY = cdfObject->getVariable("adaguccoordinatey");

    CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);
    CDF::allocateData(CDF_DOUBLE, &varY->data, dimY->length);

#ifdef CCONVERTKNMIH5VOLSCAN_DEBUG
    CDBDebug("Data allocated for 'x' and 'y' variables");
#endif

    // Fill in the X and Y dimensions with the array of coordinates
    for (size_t j = 0; j < dimX->length; j++) {
      double x = offsetX + double(j) * cellSizeX + cellSizeX / 2;
      ((double *)varX->data)[j] = x;
    }
    int width = dimX->length;
    for (size_t j = 0; j < dimY->length; j++) {
      double y = offsetY + double(j) * cellSizeY + cellSizeY / 2;
      ((double *)varY->data)[j] = y;
    }
    int height = dimY->length;

    CDF::Variable *scanVar;

    CImageWarper imageWarper;
    bool projectionRequired = false;
    if (dataSource->srvParams->Geo->CRS.length() > 0) {
      projectionRequired = true;
      new2DVar->setAttributeText("grid_mapping", "customgridprojection");
      // Apply once
      if (cdfObject->getVariableNE("customgridprojection") == NULL) {
        CDBDebug("Adding customgridprojection");
        CDF::Variable *projectionVar = new CDF::Variable();
        projectionVar->name.copy("customgridprojection");
        cdfObject->addVariable(projectionVar);
        dataSource->nativeEPSG = dataSource->srvParams->Geo->CRS.c_str();
        imageWarper.decodeCRS(&dataSource->nativeProj4, &dataSource->nativeEPSG, &dataSource->srvParams->cfg->Projection);
        if (dataSource->nativeProj4.length() == 0) {
          dataSource->nativeProj4 = LATLONPROJECTION;
          dataSource->nativeEPSG = "EPSG:4326";
          projectionRequired = false;
        }
        if (projectionRequired) {
          CDBDebug("Reprojection is needed");
        }

        projectionVar->setAttributeText("proj4_params", dataSource->nativeProj4.c_str());
      }
    }

#ifdef CCONVERTKNMIH5VOLSCAN_DEBUG
    CDBDebug("Datasource CRS = %s nativeproj4 = %s", dataSource->nativeEPSG.c_str(), dataSource->nativeProj4.c_str());
    CDBDebug("Datasource bbox:%f %f %f %f", dataSource->srvParams->Geo->dfBBOX[0], dataSource->srvParams->Geo->dfBBOX[1], dataSource->srvParams->Geo->dfBBOX[2], dataSource->srvParams->Geo->dfBBOX[3]);
    CDBDebug("Datasource width height %d %d", dataSource->dWidth, dataSource->dHeight);
#endif

    int scan_index = dataSource->getDimensionIndex("scan_elevation");
    CDF::Variable *scanNumberVar = cdfObject->getVariable("scan_number");
    int scan = scanNumberVar->getDataAt<int>(scan_index);

    size_t fieldSize = dataSource->dWidth * dataSource->dHeight;
    new2DVar->setSize(fieldSize);

    CDF::allocateData(new2DVar->getType(), &(new2DVar->data), fieldSize);
    // Draw data!
    if (dataObjects[0]->hasNodataValue) {
      float *fp = ((float *)dataObjects[0]->cdfVariable->data);
      for (size_t j = 0; j < fieldSize; j++) {
        *fp++ = (float)dataObjects[0]->dfNodataValue;
      }
    } else {
      float *fp = ((float *)dataObjects[0]->cdfVariable->data);
      for (size_t j = 0; j < fieldSize; j++) {
        *fp++ = NAN;
      }
    }

    float radarLonLat[2];
    cdfObject->getVariable("radar1")->getAttribute("radar_location")->getData<float>(radarLonLat, 2);
    float radarLon = radarLonLat[0];
    float radarLat = radarLonLat[1];

    CT::string scanName;
    scanName.print("scan%1d", scan);
    scanVar = cdfObject->getVariable(scanName);
    float scan_elevation;
    scanVar->getAttribute("scan_elevation")->getData<float>(&scan_elevation, 1);
    int scan_nrang;
    scanVar->getAttribute("scan_number_range")->getData<int>(&scan_nrang, 1);
    int scan_nazim;
    scanVar->getAttribute("scan_number_azim")->getData<int>(&scan_nazim, 1);
    float scan_rscale;
    scanVar->getAttribute("scan_range_bin")->getData<float>(&scan_rscale, 1);
    float scan_ascale;
    scanVar->getAttribute("scan_azim_bin")->getData<float>(&scan_ascale, 1);

    float factor = 1, offset = 0;
    float zv_factor, zv_offset;

    CT::string scanCalibrationVarName;
    scanCalibrationVarName.print("scan%1d.calibration", scan);
    CDF::Variable *scanCalibrationVar = cdfObject->getVariable(scanCalibrationVarName);
    CT::string componentCalibrationStringName;
    if (!doZdr) {
      componentCalibrationStringName.print("calibration_%s_formulas", new2DVar->name.c_str());
      CT::string calibrationFormula = scanCalibrationVar->getAttribute(componentCalibrationStringName.c_str())->getDataAsString();
      getCalibrationParameters(calibrationFormula, factor, offset);
    } else {
      componentCalibrationStringName.print("calibration_%s_formulas", "Z");
      CT::string calibrationFormula = scanCalibrationVar->getAttribute(componentCalibrationStringName.c_str())->getDataAsString();
      getCalibrationParameters(calibrationFormula, factor, offset);

      componentCalibrationStringName.print("calibration_%s_formulas", "Zv");
      calibrationFormula = scanCalibrationVar->getAttribute(componentCalibrationStringName.c_str())->getDataAsString();
      getCalibrationParameters(calibrationFormula, zv_factor, zv_offset);
    }

    CT::string scanDataVarName;
    CDF::Variable *scanDataVar;
    CDF::Variable *scanDataVar_Zv;
    if (doZdr) {
      scanDataVarName.print("scan%d.scan_%s_data", scan, "Z");
      scanDataVar = cdfObject->getVariable(scanDataVarName);
      scanDataVar->readData(scanDataVar->getType());
      scanDataVarName.print("scan%d.scan_%s_data", scan, "Zv");
      scanDataVar_Zv = cdfObject->getVariable(scanDataVarName);
      scanDataVar_Zv->readData(scanDataVar_Zv->getType());
    } else {
      scanDataVarName.print("scan%d.scan_%s_data", scan, new2DVar->name.c_str());
      scanDataVar = cdfObject->getVariable(scanDataVarName);
      scanDataVar->readData(scanDataVar->getType());
    }

    /*Setting geographical projection parameters of input Cartesian grid.*/
    CT::string scanProj4;

    scanProj4.print("+proj=aeqd +a=6378.137 +b=6356.752 +R_A +lat_0=%.3f +lon_0=%.3f +x_0=0 +y_0=0", radarLat, radarLon);
    if (!projectionRequired) {
      return 0;
    }
    CImageWarper radarProj;
    radarProj.initreproj(scanProj4.c_str(), dataSource->srvParams->Geo, &dataSource->srvParams->cfg->Projection);

    double x, y, ground_range, ground_height;
    float range, azim;
    int ir, ia;
    double scan_elevation_rad = scan_elevation * M_PI / 180.0;
    double four_thirds_radius = 6371.0 * 4.0 / 3.0;
    double radar_height = 0.0;          // Radar height is not present in KNMI HDF5 format, but it is in ODIM format so it could be used there.
    float *p = (float *)new2DVar->data; // ptr to store data
    std::vector<unsigned char *> pScansChar;
    std::vector<unsigned short *> pScans;
    if (scanDataVar->getType() == CDF_UBYTE) {
      pScansChar = {(unsigned char *)scanDataVar->data};
    } else {
      pScans = {(unsigned short *)scanDataVar->data};
    }
    std::vector<float> factors = {factor};
    std::vector<float> offsets = {offset};
    if (doZdr) {
      if (scanDataVar->getType() == CDF_UBYTE) {
        pScansChar.push_back((unsigned char *)scanDataVar_Zv->data);
      } else {
        pScans.push_back((unsigned short *)scanDataVar_Zv->data);
      }
      factors.push_back(zv_factor);
      offsets.push_back(zv_offset);
    }

    for (int row = 0; row < height; row++) {
      for (int col = 0; col < width; col++) {
        x = ((double *)varX->data)[col];
        y = ((double *)varY->data)[row];
        radarProj.reprojpoint(x, y);
        ground_range = sqrt(x * x + y * y);
        ground_height = ((cos(scan_elevation_rad) * (four_thirds_radius + radar_height)) / cos(scan_elevation_rad + (ground_range / four_thirds_radius))) - four_thirds_radius;
        range = ((ground_height + four_thirds_radius) * sin(ground_range / four_thirds_radius)) / cos(scan_elevation_rad);
        azim = atan2(x, y) * 180.0 / M_PI;
        ir = (int)(range / scan_rscale);
        if (ir < scan_nrang) {
          ia = (int)(azim / scan_ascale);
          ia = (ia + scan_nazim) % scan_nazim;
          if (scanDataVar->getType() == CDF_UBYTE) {
            std::vector<unsigned char> vs;
            for (auto pScanChar : pScansChar) {
              vs.push_back(pScanChar[ir + ia * scan_nrang]);
            }
            if (doZdr) {
              *p++ = vs[0] * factors[0] + offsets[0] - (vs[1] * factors[1] + offsets[1]);
            } else {
              *p++ = vs[0] * factors[0] + offsets[0];
            }
          } else {
            std::vector<unsigned short> vs;
            for (auto pScan : pScans) {
              vs.push_back(pScan[ir + ia * scan_nrang]);
            }
            if (doZdr) {
              *p++ = vs[0] * factors[0] + offsets[0] - (vs[1] * factors[1] + offsets[1]);
            } else {
              *p++ = vs[0] * factors[0] + offsets[0];
            }
          }
        } else {
          *p++ = FLT_MAX;
        }
      }
    }
  }
  return 0;
}