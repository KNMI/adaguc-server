/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Geo Spatial Team gstf@knmi.nl
 * Date:     2022-01-12
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
#include "CFillTriangle.h"
#include "CImageWarper.h"
#include "CConvertADAGUCPoint.h"
#include "CCDFHDF5IO.h"
#define CConvertKNMIH5VolScan_DEBUG

const char *CConvertKNMIH5VolScan::className = "CConvertKNMIH5VolScan";

int CConvertKNMIH5VolScan::checkIfKNMIH5VolScanFormat(CDFObject *cdfObject) {
  try {
    /* Check if the image1.statistics variable and stat_cell_number is set */
    CDF::Attribute *attr = cdfObject->getVariable("overview")->getAttribute("number_scan_groups");
    int number_scan_groups;
    attr->getData(&number_scan_groups, 1);
    if (number_scan_groups>0) return 0;
    /* TODO: Return 1 in case it is not a point renderer*/
  } catch (int e) {
    return 1;
  }
  return 0;
}

/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertKNMIH5VolScan::convertKNMIH5VolScanHeader(CDFObject *cdfObject) {
  // Check whether this is really an KNMIH5VolScanFormat file
  if (CConvertKNMIH5VolScan::checkIfKNMIH5VolScanFormat(cdfObject) == 1) return 1;

  CDBDebug("Starting convertKNMIH5VolScanHeader");

  CDF::Attribute *attr = cdfObject->getVariable("overview")->getAttribute("number_scan_groups");
  int number_scan_groups;
  attr->getData(&number_scan_groups, 1);

  int scan_elevations[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  CT::string scan_params[] = {"KDP", "PhiDP"};

  for (size_t v = 0; v < cdfObject->variables.size(); v++) {
    CDF::Variable *var = cdfObject->variables[v];
    CT::string *terms = var->name.splitToArray(".");
    if (terms->count>1) {
      if (terms[0].startsWith("scan")&&terms[1].startsWith("scan_")&&terms[1].endsWith("_data")) {
        var->setAttributeText("ADAGUC_SKIP", "TRUE");
      }
    }
    if (var->name.startsWith("visualisation")){
      var->setAttributeText("ADAGUC_SKIP", "TRUE");
    }
  }

  size_t width=700;
  size_t height=765;

  //For x
  CDF::Dimension *dimX = new CDF::Dimension();
  dimX->name = "x";
  dimX->setSize(width);
  cdfObject->addDimension(dimX);
  CDF::Variable *varX = new CDF::Variable();
  varX->setType(CDF_DOUBLE);
  varX->name.copy("x");
  varX->isDimension = true;
  varX->dimensionlinks.push_back(dimX);
  cdfObject->addVariable(varX);
  CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);

  // For y
  CDF::Dimension *dimY = new CDF::Dimension();
  dimY->name = "y";
  dimY->setSize(height);
  cdfObject->addDimension(dimY);
  CDF::Variable *varY = new CDF::Variable();
  varY->setType(CDF_DOUBLE);
  varY->name.copy("y");
  varY->isDimension = true;
  varY->dimensionlinks.push_back(dimY);
  cdfObject->addVariable(varY);
  CDF::allocateData(CDF_DOUBLE, &varY->data, dimY->length);

  // Create a new time dimension for the new 2D fields.
  CDF::Dimension *dimT = new CDF::Dimension();
  dimT->name = "time";
  dimT->setSize(1);
  cdfObject->addDimension(dimT);
  CT::string time = cdfObject->getVariable("overview")->getAttribute("product_datetime_start")->getDataAsString();
  CDF::Variable *timeVar = new CDF::Variable();
  timeVar->setType(CDF_DOUBLE);
  timeVar->name.copy("time");
  timeVar->setAttributeText("standard_name", "time");
  timeVar->setAttributeText("long_name", "time");
  timeVar->isDimension=true;
  char szStartTime[100];
  CT::string time_units="minutes since 2000-01-01 00:00:00\0";
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
  // CDBDebug("offset from %s = %f", szStartTime, offset);

  timeVar->setAttributeText("units", time_units.c_str());
  timeVar->dimensionlinks.push_back(dimT);
  CDF::allocateData(CDF_DOUBLE, &timeVar->data, 1);
  ((double *)timeVar->data)[0] = offset;
  cdfObject->addVariable(timeVar);

  // Create a new elevation dimension for the new 2D fields.
  CDF::Dimension *dimElevation = new CDF::Dimension();
  dimElevation->name = "scan_elevation";
  dimElevation->setSize(16);
  cdfObject->addDimension(dimElevation);
  CDF::Variable *varElevation = new CDF::Variable();
  varElevation->setType(CDF_DOUBLE);
  varElevation->name.copy("scan_elevation");
  varElevation->isDimension = true;
  varElevation->dimensionlinks.push_back(dimElevation);
  cdfObject->addVariable(varElevation);
  CDF::allocateData(CDF_DOUBLE, &varElevation->data, dimElevation->length);
  for (int i=0; i<number_scan_groups; i++) {
     ((double*)varElevation->data)[i]=scan_elevations[i];
  }

  CDF::Variable *projection = new CDF::Variable();
  projection->name = "projection";
  projection->setType(CDF_CHAR);
  projection->setAttributeText("projection_name", "STEREOGRAPHIC");
  projection->setAttributeText("grid_mapping_name", "polar_stereograpic");
  projection->setAttributeText("projection_proj4_params", "+proj=stere +lat_0=90 +lon_0=0 +lat_ts=60 +a=6378.14 +b=6356.75 +x_0=0 y_0=0");
  cdfObject->addVariable(projection);

  for (CT::string s: scan_params) {
    CDBDebug("Adding variable %s", s.c_str());
    CDF::Variable *var = new CDF::Variable();
    var->setType(CDF_DOUBLE);
    var->name.copy(s);
    var->setAttributeText("standard_name", s.c_str());
    var->setAttributeText("long_name", s.c_str());
    var->setAttributeText("grid_mapping_name", "projection");

    var->dimensionlinks.push_back(dimT);
    var->dimensionlinks.push_back(dimElevation);
    var->dimensionlinks.push_back(dimY);
    var->dimensionlinks.push_back(dimX);
    cdfObject->addVariable(var);
  }
#if FALSE
  // Default size of adaguc 2d Field is 2x2
  int width = 2;
  int height = 2;

  // Deterimine product corners based on file metadata:

  CT::string geo_product_corners = cdfObject->getVariable("geographic")->getAttribute("geo_product_corners")->getDataAsString();
  CT::StackList<CT::stringref> cell_max = geo_product_corners.splitToStackReferences(" ");

  double minX = 1000, maxX = -1000, minY = 1000, maxY = -1000;
  for (size_t j = 0; j < 4; j++) {
    double x = (CT::string(cell_max[j * 2].c_str())).toDouble();
    double y = (CT::string(cell_max[j * 2 + 1].c_str())).toDouble();
    if (minX > x) minX = x;
    if (minY > y) minY = y;
    if (maxX < x) maxX = x;
    if (maxY < y) maxY = y;
  }
  double dfBBOX[] = {minX, minY, maxX, maxY};

  double cellSizeX = (dfBBOX[2] - dfBBOX[0]) / double(width);
  double cellSizeY = (dfBBOX[3] - dfBBOX[1]) / double(height);
  double offsetX = dfBBOX[0];
  double offsetY = dfBBOX[1];
  // Add geo variables, only if they are not there already
  CDF::Dimension *dimX = cdfObject->getDimensionNE("xet");
  CDF::Dimension *dimY = cdfObject->getDimensionNE("yet");

  CDF::Variable *varX = cdfObject->getVariableNE("xet");
  CDF::Variable *varY = cdfObject->getVariableNE("yet");
  if (dimX == NULL || dimY == NULL || varX == NULL || varY == NULL) {
    // If not available, create new dimensions and variables (X,Y,T)
    // For x
    dimX = new CDF::Dimension();
    dimX->name = "xet";
    dimX->setSize(width);
    cdfObject->addDimension(dimX);
    varX = new CDF::Variable();
    varX->setType(CDF_DOUBLE);
    varX->name.copy("xet");
    varX->isDimension = true;
    varX->dimensionlinks.push_back(dimX);
    cdfObject->addVariable(varX);
    CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);

    // For y
    dimY = new CDF::Dimension();
    dimY->name = "yet";
    dimY->setSize(height);
    cdfObject->addDimension(dimY);
    varY = new CDF::Variable();
    varY->setType(CDF_DOUBLE);
    varY->name.copy("yet");
    varY->isDimension = true;
    varY->dimensionlinks.push_back(dimY);
    cdfObject->addVariable(varY);
    CDF::allocateData(CDF_DOUBLE, &varY->data, dimY->length);

    // Fill in the X and Y dimensions with the array of coordinates
    for (size_t j = 0; j < dimX->length; j++) {
      double x = offsetX + double(j) * cellSizeX + cellSizeX / 2;
      ((double *)varX->data)[j] = x;
    }
    for (size_t j = 0; j < dimY->length; j++) {
      double y = offsetY + double(j) * cellSizeY + cellSizeY / 2;
      ((double *)varY->data)[j] = y;
    }
  }

  CDF::Dimension *dimT = cdfObject->getDimensionNE("time");

  CDF::Variable *echoToppenVar = new CDF::Variable();
  cdfObject->addVariable(echoToppenVar);
  echoToppenVar->setType(CDF_FLOAT);
  float fillValue[] = {-1};
  echoToppenVar->setAttribute("_FillValue", echoToppenVar->getType(), fillValue, 1);
  echoToppenVar->setAttributeText("ADAGUC_POINT", "true");

  echoToppenVar->dimensionlinks.push_back(dimT);
  echoToppenVar->dimensionlinks.push_back(dimY);
  echoToppenVar->dimensionlinks.push_back(dimX);
  echoToppenVar->setType(CDF_FLOAT);

  echoToppenVar->name = "echotoppen";
  echoToppenVar->addAttribute(new CDF::Attribute("units", "FL (ft*100)"));
  echoToppenVar->setAttributeText("grid_mapping", "projection");
#endif
  return 0;
}

/**
 * This function draws the virtual 2D variable into a new 2D field
 */
int CConvertKNMIH5VolScan::convertKNMIH5VolScanData(CDataSource *dataSource, int mode) {
  CDFObject *cdfObject0 = dataSource->getDataObject(0)->cdfObject;
  if (CConvertKNMIH5VolScan::checkIfKNMIH5VolScanFormat(cdfObject0) == 1) return 1;

#ifdef CConvertKNMIH5VolScan_DEBUG
  CDBDebug("ConvertKNMIH5VolScanData");
#endif

#if FALSE
  // CDF::Variable *echoToppenVar = dataSource->getDataObject(0)->cdfVariable;

  CImageWarper imageWarper;
  imageWarper.decodeCRS(&dataSource->nativeProj4, &dataSource->nativeEPSG, &dataSource->srvParams->cfg->Projection);

  // Make the width and height of the new 2D adaguc field the same as the viewing window
  dataSource->dWidth = dataSource->srvParams->Geo->dWidth;
  dataSource->dHeight = dataSource->srvParams->Geo->dHeight;

  if (dataSource->dWidth == 1 && dataSource->dHeight == 1) {
    dataSource->srvParams->Geo->dfBBOX[0] = dataSource->srvParams->Geo->dfBBOX[0];
    dataSource->srvParams->Geo->dfBBOX[1] = dataSource->srvParams->Geo->dfBBOX[1];
    dataSource->srvParams->Geo->dfBBOX[2] = dataSource->srvParams->Geo->dfBBOX[2];
    dataSource->srvParams->Geo->dfBBOX[3] = dataSource->srvParams->Geo->dfBBOX[3];
  }

  // Width needs to be at least 2 in this case.
  if (dataSource->dWidth == 1) dataSource->dWidth = 2;
  if (dataSource->dHeight == 1) dataSource->dHeight = 2;
  double cellSizeX = (dataSource->srvParams->Geo->dfBBOX[2] - dataSource->srvParams->Geo->dfBBOX[0]) / double(dataSource->dWidth);
  double cellSizeY = (dataSource->srvParams->Geo->dfBBOX[3] - dataSource->srvParams->Geo->dfBBOX[1]) / double(dataSource->dHeight);
  double offsetX = dataSource->srvParams->Geo->dfBBOX[0];
  double offsetY = dataSource->srvParams->Geo->dfBBOX[1];

  if (mode == CNETCDFREADER_MODE_OPEN_ALL) {
    CDF::Dimension *dimX;
    CDF::Dimension *dimY;
    CDF::Variable *varX;
    CDF::Variable *varY;

    // Create new dimensions and variables (X,Y,T)
    dimX = cdfObject0->getDimension("xet");
    dimX->setSize(dataSource->dWidth);

    dimY = cdfObject0->getDimension("yet");
    dimY->setSize(dataSource->dHeight);

    varX = cdfObject0->getVariable("xet");
    varY = cdfObject0->getVariable("yet");

    CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);
    CDF::allocateData(CDF_DOUBLE, &varY->data, dimY->length);
    CDF::Variable *echoToppenVar = dataSource->getDataObject(0)->cdfVariable;
    size_t fieldSize = dimX->length * dimY->length;
    echoToppenVar->setSize(fieldSize);
    CDF::allocateData(echoToppenVar->getType(), &(echoToppenVar->data), fieldSize);
    float fillValue = -1;
    CDF::fill(echoToppenVar->data, echoToppenVar->getType(), fillValue, fieldSize);

    echoToppenVar->setAttributeText("grid_mapping", "projection");
    // Fill in the X and Y dimensions with the array of coordinates
    for (size_t j = 0; j < dimX->length; j++) {
      double x = offsetX + double(j) * cellSizeX + cellSizeX / 2;
      ((double *)varX->data)[j] = x;
    }
    for (size_t j = 0; j < dimY->length; j++) {
      double y = offsetY + double(j) * cellSizeY + cellSizeY / 2;
      ((double *)varY->data)[j] = y;
    }

    double cellSizeX = (dataSource->srvParams->Geo->dfBBOX[2] - dataSource->srvParams->Geo->dfBBOX[0]) / double(dataSource->dWidth);
    double cellSizeY = (dataSource->srvParams->Geo->dfBBOX[3] - dataSource->srvParams->Geo->dfBBOX[1]) / double(dataSource->dHeight);
    double offsetX = dataSource->srvParams->Geo->dfBBOX[0];
    double offsetY = dataSource->srvParams->Geo->dfBBOX[1];
    CImageWarper imageWarper;

    imageWarper.initreproj(dataSource, dataSource->srvParams->Geo, &dataSource->srvParams->cfg->Projection);

    CImageWarper imageWarperEchoToppen;
    CT::string projectionString = cdfObject0->getVariable("projection")->getAttribute("proj4_params")->toString();
    imageWarperEchoToppen.initreproj(projectionString.c_str(), dataSource->srvParams->Geo, &dataSource->srvParams->cfg->Projection);

    float left = cdfObject0->getVariable("x")->getDataAt<float>(0);
    float right = cdfObject0->getVariable("x")->getDataAt<float>(cdfObject0->getVariable("x")->getSize() - 1);
    float bottom = cdfObject0->getVariable("y")->getDataAt<float>(0);
    float top = cdfObject0->getVariable("y")->getDataAt<float>(cdfObject0->getVariable("y")->getSize() - 1);
    float cellsizeX = (right - left) / float(cdfObject0->getVariable("x")->getSize());
    float cellsizeY = (bottom - top) / float(cdfObject0->getVariable("y")->getSize());

    int stat_cell_number;
    cdfObject0->getVariable("image1.statistics")->getAttribute("stat_cell_number")->getData(&stat_cell_number, 1);
    CT::string stat_cell_column = cdfObject0->getVariable("image1.statistics")->getAttribute("stat_cell_column")->getDataAsString();
    CT::string stat_cell_row = cdfObject0->getVariable("image1.statistics")->getAttribute("stat_cell_row")->getDataAsString();
    CT::string stat_cell_max = cdfObject0->getVariable("image1.statistics")->getAttribute("stat_cell_max")->getDataAsString();
    CT::StackList<CT::stringref> cell_max = stat_cell_max.splitToStackReferences(" ");
    CT::StackList<CT::stringref> cell_column = stat_cell_column.splitToStackReferences(" ");
    CT::StackList<CT::stringref> cell_row = stat_cell_row.splitToStackReferences(" ");

    for (size_t k = 0; k < (size_t)stat_cell_number; k++) {
      /* Echotoppen grid coordinates / row and col */
      double col = CT::string(cell_column[k].c_str()).toDouble();
      double row = cdfObject0->getVariable("y")->getSize() - CT::string(cell_row[k].c_str()).toDouble();
      float v = calcFlightLevel(CT::string(cell_max[k].c_str()).toFloat());

      double h5X = col / cellsizeX + left;
      double h5Y = row / cellsizeY + top;

      /* Convert from HDF5 projection to requested coordinate system (in the WMS request)  */
      imageWarperEchoToppen.reprojpoint_inv(h5X, h5Y);

      /* Convert from requested coordinate system (in the WMS request) to latlon */
      double lat = h5Y;
      double lon = h5X;
      imageWarperEchoToppen.reprojToLatLon(lon, lat);

      int dlon = int((h5X - offsetX) / cellSizeX);
      int dlat = int((h5Y - offsetY) / cellSizeY);

      dataSource->dataObjects[0]->points.push_back(PointDVWithLatLon(dlon, dlat, lon, lat, v));
      // lastPoint = &(dataSource->dataObjects[0]->points.back());
      // lastPoint->paramList.push_back(CKeyValue("echotoppen", "echotoppen", "0.1"));

      //CConvertADAGUCPoint::drawDot(dlon, dlat, v, dimX->length, dimY->length, (float *)echoToppenVar->data);
    }
  }
  #endif
  return 0;
}
