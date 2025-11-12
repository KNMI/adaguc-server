/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  Convert HDF5 volume scan data to CDM
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

#include <tuple>
#include <vector>
#include "CConvertH5VolScan.h"
#include "CConvertH5VolScanUtils.h"
#include "CImageWarper.h"
#include "COGCDims.h"
#include "CCDFHDF5IO.h"

// #define CCONVERTH5VOLSCAN_DEBUG
const char *CConvertH5VolScan::className = "CConvertH5VolScan";

bool sortFunction(CT::string one, CT::string other) {
  if (one.endsWith("l")) {
    one = one.substring(0, one.lastIndexOf("l"));
    if (one.equals(other)) return true;
  }
  if (other.endsWith("l")) {
    other = other.substring(0, other.lastIndexOf("l"));
    if (one.equals(other)) return false;
  }
  return (std::atof(one) < std::atof(other));
}

int CConvertH5VolScan::convertH5VolScanHeader(CDFObject *cdfObject, CServerParams *srvParams) {
  if (checkIfIsH5VolScan(cdfObject) != 0) return 1;
  CDBDebug("convertH5VolScanHeader()");

  /* Read all scans from file and give them a name based on their elevation */
  int nrscans = 0;
  std::vector<int> scan_ranges;
  std::vector<int> scans;
  std::vector<CT::string> elevation_names;
  std::vector<CT::string> scan_params = getScanParams(cdfObject);
  std::vector<CT::string> units = getUnits(cdfObject);

  int max_range = 0;
  /* Assume no more than 99 scans */
  for (int scan = 1; scan < 100; scan++) {
    double scan_elevation;
    int scan_nrang;
    double scan_rscale;
    int scan_nazim;
    double scan_ascale;
    std::tie(scan_elevation, scan_nrang, scan_rscale, scan_nazim, scan_ascale) = getScanMetadata(cdfObject, scan);
    if (scan_nrang == -1) continue;
    int scan_range = lround(scan_nrang * scan_rscale);
    if (scan_range > max_range) max_range = scan_range;
    int scanElevationInt = lround(scan_elevation * 10.0);
    /* Skip 90 degree scan */
    if (scanElevationInt == 900) continue;
    CT::string elevation_name;
    if (scanElevationInt % 10 == 0) {
      elevation_name.print("%d", scanElevationInt / 10);
    } else {
      elevation_name.print("%d.%d", scanElevationInt / 10, scanElevationInt % 10);
    }
    /* Dutch radars contain 3 0.3 degree scans. 1st is long range, second is short range, third is long range again. */
    /* Keep the first 2 and skip the last. */
    bool longRangePresent = false;
    int scanElevationIndex = -1;
    for (int i = 0; i < nrscans; i++) {
      if (elevation_names[i].equals(elevation_name)) {
        scanElevationIndex = i;
      }
      if (elevation_names[i].equals(elevation_name + "l")) {
        longRangePresent = true;
      }
    }
    if (scanElevationIndex >= 0) {
      if (!longRangePresent || scan_ranges[scanElevationIndex] == scan_range) {
        if (scan_ranges[scanElevationIndex] < scan_range) {
          elevation_name += "l";
        } else {
          elevation_names[scanElevationIndex] = elevation_name + "l";
        }
      } else {
        continue;
      }
    }
    scan_ranges.push_back(scan_range);
    scans.push_back(scan);
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

  for (size_t v = 0; v < cdfObject->variables.size(); v++) {
    CDF::Variable *var = cdfObject->variables[v];
    auto terms = var->name.splitToStack(".");
    if (terms.size() > 1) {
      if (terms[0].startsWith("scan") && terms[1].startsWith("scan_") && terms[1].endsWith("_data")) {
        var->setAttributeText("ADAGUC_SKIP", "TRUE");
      }
    }
    if (var->name.startsWith("visualisation")) {
      var->setAttributeText("ADAGUC_SKIP", "TRUE");
    }
    /* Allow hybrid format used in IRC */
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

  double radarLon;
  double radarLat;
  double radarHeight;
  std::tie(radarLon, radarLat, radarHeight) = getRadarLocation(cdfObject);

  double dfBBOX[] = {radarLon - max_range / (111.0 * cos(radarLat * M_PI / 180.0)), radarLat - max_range / 111.0, radarLon + max_range / (111.0 * cos(radarLat * M_PI / 180.0)),
                     radarLat + max_range / 111.0};

  // Default size of adaguc 2dField is 2x2
  int width = 2;  // swathMiddleLon->dimensionlinks[1]->getSize();
  int height = 2; // swathMiddleLon->dimensionlinks[0]->getSize();

  if (srvParams == NULL) {
    CDBError("srvParams is not set");
    return 1;
  }
  if (srvParams->geoParams.width > 1 && srvParams->geoParams.height > 1) {
    width = srvParams->geoParams.width;
    height = srvParams->geoParams.height;
  }

#ifdef CCONVERTH5VOLSCAN_DEBUG
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

#ifdef CCONVERTH5VOLSCAN_DEBUG
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
    timeVar->setType(CDF_DOUBLE);
    timeVar->name.copy("time");
    timeVar->setAttributeText("standard_name", "time");
    timeVar->setAttributeText("long_name", "time");
    timeVar->isDimension = true;
    CT::string time_units = "minutes since 2000-01-01 00:00:00\0";
    CT::string szStartTime = getRadarStartTime(cdfObject);
    // Set adaguc time
    CTime ctime;
    if (ctime.init(time_units, NULL) != 0) {
      CDBError("Could not initialize CTIME: %s", time_units.c_str());
      return 1;
    }
    double offset;
    try {
      offset = ctime.dateToOffset(ctime.stringToDate(szStartTime.c_str()));
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
      ((char **)varElevation->data)[i] = strdup(elevation_names[i].c_str());
      ((unsigned int *)varScan->data)[i] = sorted_scans[i];
    }
  }
  // CDFHDF5Reader::CustomVolScanReader *volScanReader = new CDFHDF5Reader::CustomVolScanReader();
  // CDF::Variable::CustomMemoryReader *memoryReader = CDF::Variable::CustomMemoryReaderInstance;
  int cnt = -1;
  for (CT::string param : scan_params) {
    cnt++;
    if (!hasParam(cdfObject, sorted_scans, param)) continue;
    CDF::Variable *var = new CDF::Variable();
    var->setType(CDF_FLOAT);
    var->name.copy(param);
    cdfObject->addVariable(var);
    var->setAttributeText("standard_name", param.c_str());
    var->setAttributeText("long_name", param.c_str());
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
  }

  return 0;
}

int CConvertH5VolScan::convertH5VolScanData(CDataSource *dataSource, int mode) {
  CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
  if (checkIfIsH5VolScan(cdfObject) != 0) return 1;
  if (mode == CNETCDFREADER_MODE_OPEN_ALL) {
    CDF::Variable *new2DVar = dataSource->getDataObject(0)->cdfVariable;

    bool doZdr = (new2DVar->name.equals("ZDR"));
    bool doHeight = (new2DVar->name.equals("Height"));

    // Make the width and height of the new 2D adaguc field the same as the viewing window
    dataSource->dWidth = dataSource->srvParams->geoParams.width;
    dataSource->dHeight = dataSource->srvParams->geoParams.height;

    // Width needs to be at least 2, the bounding box is calculated from these.
    if (dataSource->dWidth == 1) dataSource->dWidth = 2;
    if (dataSource->dHeight == 1) dataSource->dHeight = 2;
    double cellSizeX = (dataSource->srvParams->geoParams.bbox.right - dataSource->srvParams->geoParams.bbox.left) / double(dataSource->dWidth);
    double cellSizeY = (dataSource->srvParams->geoParams.bbox.top - dataSource->srvParams->geoParams.bbox.bottom) / double(dataSource->dHeight);
    double offsetX = dataSource->srvParams->geoParams.bbox.left;
    double offsetY = dataSource->srvParams->geoParams.bbox.bottom;

#ifdef CCONVERTH5VOLSCAN_DEBUG
    CDBDebug("Drawing %s with WH = [%d,%d]", new2DVar->name.c_str(), dataSource->dWidth, dataSource->dHeight);
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

#ifdef CCONVERTH5VOLSCAN_DEBUG
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

    CImageWarper imageWarper;
    bool projectionRequired = false;
    if (dataSource->srvParams->geoParams.crs.length() > 0) {
      projectionRequired = true;
      new2DVar->setAttributeText("grid_mapping", "customgridprojection");
      // Apply once
      if (cdfObject->getVariableNE("customgridprojection") == NULL) {
        CDBDebug("Adding customgridprojection");
        CDF::Variable *projectionVar = new CDF::Variable();
        projectionVar->name.copy("customgridprojection");
        cdfObject->addVariable(projectionVar);
        dataSource->nativeEPSG = dataSource->srvParams->geoParams.crs.c_str();
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

#ifdef CCONVERTH5VOLSCAN_DEBUG
    CDBDebug("Datasource CRS = %s nativeproj4 = %s", dataSource->nativeEPSG.c_str(), dataSource->nativeProj4.c_str());
    CDBDebug("Datasource bbox:%f %f %f %f", dataSource->srvParams->geoParams.bbox.left, dataSource->srvParams->geoParams.bbox.bottom, dataSource->srvParams->geoParams.bbox.right,
             dataSource->srvParams->geoParams.bbox.top);
    CDBDebug("Datasource width height %d %d", dataSource->dWidth, dataSource->dHeight);
#endif

    int scan_index = dataSource->getDimensionIndex("scan_elevation");
    CDF::Variable *scanNumberVar = cdfObject->getVariable("scan_number");
    int scan = scanNumberVar->getDataAt<int>(scan_index);

    if (doZdr) {
      CDF::Variable *dataZdr = getDataVarForParam(cdfObject, scan, CT::string("ZDR"));
      if (dataZdr != nullptr) {
        doZdr = false;
      }
    }

    size_t fieldSize = dataSource->dWidth * dataSource->dHeight;
    new2DVar->setSize(fieldSize);

    CDF::allocateData(new2DVar->getType(), &(new2DVar->data), fieldSize);
    new2DVar->fill(FLT_MAX);

    double radarLon;
    double radarLat;
    double radarHeight;
    std::tie(radarLon, radarLat, radarHeight) = getRadarLocation(cdfObject);

    double scan_elevation;
    int scan_nrang;
    double scan_rscale;
    int scan_nazim;
    double scan_ascale;
    std::tie(scan_elevation, scan_nrang, scan_rscale, scan_nazim, scan_ascale) = getScanMetadata(cdfObject, scan);

    double gain, offset;
    double gainDBZV, offsetDBZV;
    double undetect, nodata;
    CT::string scanDataVarName;
    CDF::Variable *scanDataVar;
    CDF::Variable *scanDataVarDBZV;

    if (!doHeight) {
      CT::string componentCalibrationStringName;
      if (doZdr) {
        std::tie(gainDBZV, offsetDBZV, undetect, nodata) = getCalibrationParameters(cdfObject, scan, CT::string("DBZV"));
        scanDataVarDBZV = getDataVarForParam(cdfObject, scan, CT::string("DBZV"));
        scanDataVarDBZV->readData(CDF_DOUBLE);

        /* Assume nodata and undetect are the same between DBZV and DBZH */
        std::tie(gain, offset, undetect, nodata) = getCalibrationParameters(cdfObject, scan, CT::string("DBZH"));
        scanDataVar = getDataVarForParam(cdfObject, scan, CT::string("DBZH"));
        scanDataVar->readData(CDF_DOUBLE);
      } else {
        std::tie(gain, offset, undetect, nodata) = getCalibrationParameters(cdfObject, scan, new2DVar->name);
        scanDataVar = getDataVarForParam(cdfObject, scan, new2DVar->name);
        scanDataVar->readData(CDF_DOUBLE);
      }
    }

    std::vector<double *> pScans;
    std::vector<double> gains = {gain};
    std::vector<double> offsets = {offset};

    if (!doHeight) {
      pScans = {(double *)scanDataVar->data};
      if (doZdr) {
        pScans.push_back((double *)scanDataVarDBZV->data);
        gains.push_back(gainDBZV);
        offsets.push_back(offsetDBZV);
      }
    }
    if (!projectionRequired) {
      return 0;
    }

    /*Setting geographical projection parameters of input Cartesian grid.*/
    CT::string scanProj4;
    scanProj4.print("+proj=aeqd +a=6378.137 +b=6356.752 +R_A +lat_0=%.3f +lon_0=%.3f +x_0=0 +y_0=0", radarLat, radarLon);
    CImageWarper radarProj;
    radarProj.initreproj(scanProj4.c_str(), dataSource->srvParams->geoParams, &dataSource->srvParams->cfg->Projection);

    double x, y, ground_range;
    double range, azim, ground_height;
    int ir, ia;
    double scan_elevation_rad = scan_elevation * M_PI / 180.0;
    double four_thirds_radius = 6371.0 * 4.0 / 3.0;
    float *p = (float *)new2DVar->data; // ptr to store data

    for (int row = 0; row < height; row++) {
      for (int col = 0; col < width; col++) {
        x = ((double *)varX->data)[col];
        y = ((double *)varY->data)[row];
        if (radarProj.reprojpoint(x, y)) {
          *p++ = FLT_MAX;
          continue;
        }
        ground_range = sqrt(x * x + y * y);
        /* Formulas below are only valid when (scan_elevation_rad + ground_range / four_thirds_radius) < (M_PI / 2). */
        /* Longest range for DE/BE/NL radars is ~400 km, which is an equivalent ground range, so we cap the ground range. */
        if (ground_range > 1000.0) {
          *p++ = FLT_MAX;
          continue;
        }
        ground_height = ((cos(scan_elevation_rad) * (four_thirds_radius + radarHeight)) / cos(scan_elevation_rad + (ground_range / four_thirds_radius))) - four_thirds_radius;
        range = ((ground_height + four_thirds_radius) * sin(ground_range / four_thirds_radius)) / cos(scan_elevation_rad);
        azim = atan2(x, y) * 180.0 / M_PI;
        ir = (int)(range / scan_rscale);
        if (ir < scan_nrang) {
          if (doHeight) {
            *p++ = ground_height;
            continue;
          }
          ia = (int)(azim / scan_ascale);
          ia = (ia + scan_nazim) % scan_nazim;
          std::vector<double> vs;
          for (auto pScan : pScans) {
            vs.push_back(pScan[ir + ia * scan_nrang]);
          }
          if (vs[0] == undetect || vs[0] == nodata) {
            *p++ = FLT_MAX;
            continue;
          }
          if (doZdr) {
            if (vs[1] == undetect || vs[1] == nodata) {
              *p++ = FLT_MAX;
              continue;
            }
            *p++ = vs[0] * gains[0] + offsets[0] - (vs[1] * gains[1] + offsets[1]);
          } else {
            *p++ = vs[0] * gains[0] + offsets[0];
          }
        } else {
          *p++ = FLT_MAX;
        }
      }
    }
  }
  return 0;
}