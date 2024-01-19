/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
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

#include "CConvertLatLonGrid.h"
#include "CFillTriangle.h"
#include "CImageWarper.h"

#define CConvertLatLonGrid_DEBUG

const char *CConvertLatLonGrid::className = "CConvertLatLonGrid";

static const char *const lonNamesToCheck[] = {"lon", "longitude"};
static const char *const latNamesToCheck[] = {"lat", "latitude"};

CDF::Variable *CConvertLatLonGrid::getLon1D(CDFObject *cdfObject) {
  for (auto lonName : lonNamesToCheck) {
    CDF::Variable *lon1DVar = cdfObject->getVariableNE(lonName);
    if (lon1DVar != nullptr && lon1DVar->dimensionlinks.size() == 1) {
      return lon1DVar;
    }
  }
  return nullptr;
}

CDF::Variable *CConvertLatLonGrid::getLat1D(CDFObject *cdfObject) {
  for (auto latName : latNamesToCheck) {
    CDF::Variable *lon1DVar = cdfObject->getVariableNE(latName);
    if (lon1DVar != nullptr && lon1DVar->dimensionlinks.size() == 1) {
      return lon1DVar;
    }
  }
  return nullptr;
}
CDF::Variable *CConvertLatLonGrid::getLon2D(CDFObject *cdfObject) {
  for (auto lonName : lonNamesToCheck) {
    CDF::Variable *lon1DVar = cdfObject->getVariableNE(lonName);
    if (lon1DVar != nullptr && lon1DVar->dimensionlinks.size() == 2) {
      return lon1DVar;
    }
  }
  return nullptr;
}
CDF::Variable *CConvertLatLonGrid::getLat2D(CDFObject *cdfObject) {
  for (auto latName : latNamesToCheck) {
    CDF::Variable *lon1DVar = cdfObject->getVariableNE(latName);
    if (lon1DVar != nullptr && lon1DVar->dimensionlinks.size() == 2) {
      return lon1DVar;
    }
  }
  return nullptr;
}

bool CConvertLatLonGrid::checkIfIrregularLatLon(CDFObject *cdfObject) {
  CDF::Variable *lon1DVar = getLon1D(cdfObject);
  CDF::Variable *lat1DVar = getLat1D(cdfObject);

  if (lon1DVar != nullptr && lat1DVar != nullptr) {
    if (lat1DVar->dimensionlinks.size() == 1 && lon1DVar->dimensionlinks.size() == 1) {
      lon1DVar->readData(CDF_DOUBLE);
      lat1DVar->readData(CDF_DOUBLE);
      size_t width = lon1DVar->dimensionlinks[0]->getSize();
      size_t height = lat1DVar->dimensionlinks[0]->getSize();

      double *dfdim_X = (double *)lon1DVar->data;
      double *dfdim_Y = (double *)lat1DVar->data;

      double cellSizeXBorder = fabs(dfdim_X[0] - dfdim_X[0 + 1]);
      double cellSizeXCenter = fabs(dfdim_X[width / 2] - dfdim_X[width / 2 + 1]);
      double deviationX = ((cellSizeXBorder - cellSizeXCenter) / cellSizeXBorder);

      double cellSizeYBorder = fabs(dfdim_Y[0] - dfdim_Y[0 + 1]);
      double cellSizeYCenter = fabs(dfdim_Y[height / 2] - dfdim_Y[height / 2 + 1]);
      double deviationY = ((cellSizeYBorder - cellSizeYCenter) / cellSizeYBorder);

      CDBDebug("CellsizesY %f %f %f", cellSizeYBorder, cellSizeYCenter, deviationY);
      // When the cellsize deviates more than 1% in the center than in the border, we call this grid irregular lat/lon
      if (deviationY > 0.01 || deviationX > 0.01) {
        CDBDebug("IT is IRRREGULAR!");
        return true;
      }
    }
  }
  return false;
}

bool CConvertLatLonGrid::isLatLonGrid(CDFObject *cdfObject) {

  if (cdfObject->getAttributeNE("ConvertLatLonGridActive") != nullptr) {
    return true;
  }

  bool hasXYDimensions = cdfObject->getDimensionNE("x") != NULL && cdfObject->getDimensionNE("y") != NULL;
  bool hasXYVariables = cdfObject->getVariableNE("x") != NULL && cdfObject->getVariableNE("y") != NULL;

  bool fixIrregular = false;

  if (checkIfIrregularLatLon(cdfObject)) {
    CDBDebug("Found ADAGUCIRREGULARGRID attribute in cdfObject");
    fixIrregular = true;
  }
  if (fixIrregular == true) {
    // Convert
    hasXYDimensions = true;
    hasXYVariables = false;

    // When the latitude or longitude grids are not present in the cdfObject, add them to the cdfObject
    CDF::Variable *lonGridVar = getLon2D(cdfObject);
    if (lonGridVar == nullptr) {
      CDBDebug("Adding 2D latlon grids");

      // Rename 1D lat/lon variables to x and y
      CDF::Variable *lon1DVar = getLon1D(cdfObject);
      CDF::Variable *lat1DVar = getLat1D(cdfObject);
      CDF::Dimension *lon1DDim = lon1DVar->dimensionlinks[0];
      CDF::Dimension *lat1DDim = lat1DVar->dimensionlinks[0];
      lon1DDim->setName("x");
      lat1DDim->setName("y");

      lon1DVar->setName("unusedlon");
      lat1DVar->setName("unusedlat");
      lon1DVar->readData(CDF_DOUBLE);
      lat1DVar->readData(CDF_DOUBLE);

      //  Define the 2D latitude longitude grids
      CDF::Variable *longitude = new CDF::Variable();
      longitude->setType(CDF_DOUBLE);
      longitude->name.copy("longitude");
      longitude->dimensionlinks.push_back(lat1DDim);
      longitude->dimensionlinks.push_back(lon1DDim);
      longitude->setAttributeText("ADAGUCConvertLatLonGridConverter", "DONE");
      // longitude->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);
      cdfObject->addVariable(longitude);
      CDF::allocateData(CDF_DOUBLE, &longitude->data, lon1DDim->length * lat1DDim->length);

      CDF::Variable *latitude = new CDF::Variable();
      latitude->setType(CDF_DOUBLE);
      latitude->name.copy("latitude");
      latitude->dimensionlinks.push_back(lat1DDim);
      latitude->dimensionlinks.push_back(lon1DDim);
      latitude->setAttributeText("ADAGUCConvertLatLonGridConverter", "DONE");
      // latitude->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);

      cdfObject->addVariable(latitude);
      CDF::allocateData(CDF_DOUBLE, &latitude->data, lon1DDim->length * lat1DDim->length);
      CDBDebug("Making latitude and longitude grids");
      for (size_t latIndex = 0; latIndex < lat1DDim->length; latIndex += 1) {
        for (size_t lonIndex = 0; lonIndex < lon1DDim->length; lonIndex += 1) {
          size_t p = lonIndex + latIndex * lon1DDim->length;
          ((double *)longitude->data)[p] = ((double *)lon1DVar->data)[lonIndex];
          ((double *)latitude->data)[p] = ((double *)lat1DVar->data)[latIndex];
        }
      }
    }
  }

  CDF::Variable *latVar = getLat2D(cdfObject);
  CDF::Variable *lonVar = getLon2D(cdfObject);
  bool hasLatLonVariables = (latVar != NULL && lonVar != NULL);

  if (hasXYDimensions && !hasXYVariables && hasLatLonVariables) {
    if (latVar->dimensionlinks.size() == 2 && lonVar->dimensionlinks.size() == 2) {
      if (latVar->dimensionlinks[0]->name.equals("y") && lonVar->dimensionlinks[0]->name.equals("y") && latVar->dimensionlinks[1]->name.equals("x") && lonVar->dimensionlinks[1]->name.equals("x")) {
        cdfObject->setAttributeText("ConvertLatLonGridActive", "TRUE");
        return true;
      }
    }
  }
  return false;
}

/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertLatLonGrid::convertLatLonGridHeader(CDFObject *cdfObject, CServerParams *) {
#ifdef CConvertLatLonGrid_DEBUG
  CDBDebug("CHECKING convertLatLonGridHeader");
#endif
  if (!isLatLonGrid(cdfObject)) return 1;

  try {
    CDF::Dimension *timeDim = cdfObject->getDimensionNE("time");
    if (timeDim == NULL) {

      CDF::Variable *timeVar = cdfObject->getVariable("acquisition_time");
      timeVar->name = "time";
      timeDim = new CDF::Dimension("time", 1);
      cdfObject->addDimension(timeDim);
      timeVar->dimensionlinks.push_back(timeDim);
    }

  } catch (int e) {
  }
#ifdef CConvertLatLonGrid_DEBUG
  CDBDebug("Using CConvertLatLonGrid.h");
#endif
  bool hasTimeData = false;

  // Is there a time variable
  CDF::Variable *origT = cdfObject->getVariableNE("time");
  if (origT != NULL) {
    hasTimeData = true;

    // Create a new time dimension for the new 2D fields.
    CDF::Dimension *dimT = new CDF::Dimension();
    dimT->name = "time2D";
    dimT->setSize(1);
    cdfObject->addDimension(dimT);

    // Create a new time variable for the new 2D fields.
    CDF::Variable *varT = new CDF::Variable();
    varT->setType(CDF_DOUBLE);
    varT->name.copy(dimT->name.c_str());
    varT->setAttributeText("standard_name", "time");
    varT->setAttributeText("long_name", "time");
    varT->dimensionlinks.push_back(dimT);
    CDF::allocateData(CDF_DOUBLE, &varT->data, dimT->length);
    cdfObject->addVariable(varT);

    // Detect time from the netcdf data and copy the same units from the original time variable
    if (origT != NULL) {
      try {
        varT->setAttributeText("units", origT->getAttribute("units")->toString().c_str());
        if (origT->readData(CDF_DOUBLE) != 0) {
          CDBError("Unable to read time variable");
        } else {
          // Loop through the time variable and detect the earliest time
          double tfill;
          bool hastfill = false;
          try {
            origT->getAttribute("_FillValue")->getData(&tfill, 1);
            hastfill = true;
          } catch (int e) {
          }
          double *tdata = ((double *)origT->data);
          double firstTimeValue = tdata[0];
          size_t tsize = origT->getSize();
          if (hastfill == true) {
            for (size_t j = 0; j < tsize; j++) {
              if (tdata[j] != tfill) {
                firstTimeValue = tdata[j];
              }
            }
          }
#ifdef CConvertLatLonGrid_DEBUG
          CDBDebug("firstTimeValue  = %f", firstTimeValue);
#endif
          // Set the time data
          varT->setData(CDF_DOUBLE, &firstTimeValue, 1);
        }
      } catch (int e) {
      }
    }
  }

  // Standard bounding box of ascat data is worldwide
  double dfBBOX[] = {-180, -90, 180, 90};

  // Default size of ascat 2dField is 2x2
  int width = 2;
  int height = 2;

  double cellSizeX = (dfBBOX[2] - dfBBOX[0]) / double(width);
  double cellSizeY = (dfBBOX[3] - dfBBOX[1]) / double(height);
  double offsetX = dfBBOX[0];
  double offsetY = dfBBOX[1];

  // Add geo variables, only if they are not there already
  CDF::Dimension *dimX = cdfObject->getDimensionNE("adx");
  CDF::Dimension *dimY = cdfObject->getDimensionNE("ady");
  CDF::Variable *varX = cdfObject->getVariableNE("adx");
  CDF::Variable *varY = cdfObject->getVariableNE("ady");
  if (dimX == NULL || dimY == NULL || varX == NULL || varY == NULL) {
    // If not available, create new dimensions and variables (X,Y,T)
    // For x
    dimX = new CDF::Dimension();
    dimX->name = "adx";
    dimX->setSize(width);
    cdfObject->addDimension(dimX);
    varX = new CDF::Variable();
    varX->setType(CDF_DOUBLE);
    varX->name.copy("adx");
    varX->isDimension = true;
    varX->dimensionlinks.push_back(dimX);
    cdfObject->addVariable(varX);
    CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);

    // For y
    dimY = new CDF::Dimension();
    dimY->name = "ady";
    dimY->setSize(height);
    cdfObject->addDimension(dimY);
    varY = new CDF::Variable();
    varY->setType(CDF_DOUBLE);
    varY->name.copy("ady");
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

  // Make a list of variables which will be available as 2D fields
  CT::StackList<CT::string> varsToConvert;
  for (size_t v = 0; v < cdfObject->variables.size(); v++) {
    CDF::Variable *var = cdfObject->variables[v];
    if (var->isDimension == false) {
      if (!var->name.equals("time2D") && !var->name.equals("time") && !var->name.equals("lon") && !var->name.equals("lat") && !var->name.equals("longitude") && !var->name.equals("latitude")) {
        varsToConvert.add(CT::string(var->name.c_str()));
      }
    }
  }

  // Create the new regular grid field variiables based on the irregular grid variables
  for (size_t v = 0; v < varsToConvert.size(); v++) {
    CDF::Variable *irregularGridVar = cdfObject->getVariable(varsToConvert[v].c_str());
    if (irregularGridVar->dimensionlinks.size() >= 2) {
#ifdef CConvertLatLonGrid_DEBUG
      CDBDebug("Converting %s", irregularGridVar->name.c_str());
#endif

      CDF::Variable *destRegularGrid = new CDF::Variable();
      cdfObject->addVariable(destRegularGrid);

      // Assign X,Y,T dims
      if (hasTimeData) {
        CDF::Variable *newTimeVar = cdfObject->getVariableNE("time2D");
        if (newTimeVar != NULL) {
          destRegularGrid->dimensionlinks.push_back(newTimeVar->dimensionlinks[0]);
        }
      }
      destRegularGrid->dimensionlinks.push_back(dimY);
      destRegularGrid->dimensionlinks.push_back(dimX);

      destRegularGrid->setType(irregularGridVar->getType());
      destRegularGrid->name = irregularGridVar->name.c_str();
      irregularGridVar->name.concat("_backup");

      // Copy variable attributes
      for (size_t j = 0; j < irregularGridVar->attributes.size(); j++) {
        CDF::Attribute *a = irregularGridVar->attributes[j];
        destRegularGrid->setAttribute(a->name.c_str(), a->getType(), a->data, a->length);
        destRegularGrid->setAttributeText("ADAGUC_VECTOR", "true");
      }

      // The irregularGridVar variable is not directly plotable, so skip it
      irregularGridVar->setAttributeText("ADAGUC_SKIP", "true");

      // Scale and offset are already applied
      destRegularGrid->removeAttribute("scale_factor");
      destRegularGrid->removeAttribute("add_offset");
      destRegularGrid->setType(CDF_FLOAT);
    }
  }
  return 0;
}

/**
 * This function draws the virtual 2D variable into a new 2D field
 */
int CConvertLatLonGrid::convertLatLonGridData(CDataSource *dataSource, int mode) {

  CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
  if (!isLatLonGrid(cdfObject)) return 1;
  size_t nrDataObjects = dataSource->getNumDataObjects();
  if (nrDataObjects <= 0) return 1;

  CDataSource::DataObject *dataObjects[nrDataObjects];
  for (size_t d = 0; d < nrDataObjects; d++) {
    dataObjects[d] = dataSource->getDataObject(d);
  }
#ifdef CConvertLatLonGrid_DEBUG

  CDBDebug("convertLatLonGridData %s", dataObjects[0]->cdfVariable->name.c_str());
#endif
  CDF::Variable *destRegularGrid[nrDataObjects];
  CDF::Variable *irregularGridVar[nrDataObjects];

  // Make references destRegularGrid and irregularGridVar
  for (size_t d = 0; d < nrDataObjects; d++) {
    destRegularGrid[d] = dataObjects[d]->cdfVariable;
    CT::string orgName = destRegularGrid[d]->name.c_str();
    orgName.concat("_backup");
    irregularGridVar[d] = cdfObject->getVariableNE(orgName.c_str());
    if (irregularGridVar[d] == NULL) {
      CDBError("Unable to find orignal variable with name %s", orgName.c_str());
      return 1;
    }
  }

  CDF::Variable *longitudeGrid = getLon2D(cdfObject);
  CDF::Variable *latitudeGrid = getLat2D(cdfObject);

  // Read original data first
  for (size_t d = 0; d < nrDataObjects; d++) {
    irregularGridVar[d]->readData(CDF_FLOAT, true);
    CDF::Attribute *fillValue = irregularGridVar[d]->getAttributeNE("_FillValue");
    if (fillValue != NULL) {
      dataObjects[d]->hasNodataValue = true;
      fillValue->getData(&dataObjects[d]->dfNodataValue, 1);
#ifdef CConvertLatLonGrid_DEBUG
      CDBDebug("_FillValue = %f", dataObjects[d]->dfNodataValue);
#endif
      float f = dataObjects[d]->dfNodataValue;
      destRegularGrid[d]->getAttribute("_FillValue")->setData(CDF_FLOAT, &f, 1);
    } else
      dataObjects[d]->hasNodataValue = false;
  }

  // If the data was not populated in the code above, try to read it from the file
  if (longitudeGrid->data == nullptr) {
    longitudeGrid->readData(CDF_DOUBLE, true);
  }
  if (latitudeGrid->data == nullptr) {
    latitudeGrid->readData(CDF_DOUBLE, true);
  }

  float fill = (float)dataObjects[0]->dfNodataValue;
  float min = fill;
  float max = fill;

  // Detect minimum and maximum values
  size_t l = irregularGridVar[0]->getSize();
  for (size_t j = 0; j < l; j++) {
    float v = ((float *)irregularGridVar[0]->data)[j];
    if (v != fill) {
      if (min == fill) min = v;
      if (max == fill) max = v;
      if (v < min) min = v;
      if (v > max) max = v;
    }
  }
#ifdef CConvertLatLonGrid_DEBUG
  CDBDebug("Calculated min/max : %f %f", min, max);
#endif

  // Set statistics
  if (dataSource->stretchMinMax) {
#ifdef CConvertLatLonGrid_DEBUG
    CDBDebug("dataSource->stretchMinMax");
#endif
    if (dataSource->statistics == NULL) {
#ifdef CConvertLatLonGrid_DEBUG
      CDBDebug("Setting statistics: min/max : %f %f", min, max);
#endif
      dataSource->statistics = new CDataSource::Statistics();
      dataSource->statistics->setMaximum(max);
      dataSource->statistics->setMinimum(min);
    }
  }

  // Make the width and height of the new regular grid field the same as the viewing window
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

#ifdef CConvertLatLonGrid_DEBUG
  CDBDebug("Datasource bbox:%f %f %f %f", dataSource->srvParams->Geo->dfBBOX[0], dataSource->srvParams->Geo->dfBBOX[1], dataSource->srvParams->Geo->dfBBOX[2], dataSource->srvParams->Geo->dfBBOX[3]);
  CDBDebug("Datasource width height %d %d", dataSource->dWidth, dataSource->dHeight);
  CDBDebug("L2 %d %d", dataSource->dWidth, dataSource->dHeight);
#endif

  if (mode == CNETCDFREADER_MODE_OPEN_ALL) {
#ifdef CConvertLatLonGrid_DEBUG
    CDBDebug("Drawing %s", destRegularGrid[0]->name.c_str());
#endif

    CDF::Dimension *dimX;
    CDF::Dimension *dimY;
    CDF::Variable *varX;
    CDF::Variable *varY;

    // Create new dimensions and variables (X,Y,T)
    dimX = cdfObject->getDimension("adx");
    dimX->setSize(dataSource->dWidth);

    dimY = cdfObject->getDimension("ady");
    dimY->setSize(dataSource->dHeight);

    varX = cdfObject->getVariable("adx");
    varY = cdfObject->getVariable("ady");

    CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);
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

    size_t fieldSize = dataSource->dWidth * dataSource->dHeight;

    // Allocate and clear data
    for (size_t d = 0; d < nrDataObjects; d++) {
      destRegularGrid[d]->setSize(fieldSize);
      CDF::allocateData(destRegularGrid[d]->getType(), &(destRegularGrid[d]->data), fieldSize);
      for (size_t j = 0; j < fieldSize; j++) {
        ((float *)dataObjects[d]->cdfVariable->data)[j] = NAN;
      }
    }

    double *lonData = (double *)longitudeGrid->data;
    double *latData = (double *)latitudeGrid->data;

    int numY = longitudeGrid->dimensionlinks[0]->getSize();
    int numX = longitudeGrid->dimensionlinks[1]->getSize();
#ifdef CConvertLatLonGrid_DEBUG
    CDBDebug("numRows %d numCells %d", numY, numX);
#endif

    CImageWarper imageWarper;
    bool projectionRequired = false;
    if (dataSource->srvParams->Geo->CRS.length() > 0) {
      projectionRequired = true;
      for (size_t d = 0; d < nrDataObjects; d++) {
        destRegularGrid[d]->setAttributeText("grid_mapping", "customgridprojection");
      }
      if (cdfObject->getVariableNE("customgridprojection") == NULL) {
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
        projectionVar->setAttributeText("proj4_params", dataSource->nativeProj4.c_str());
      }
    }
    if (projectionRequired) {
      int status = imageWarper.initreproj(dataSource, dataSource->srvParams->Geo, &dataSource->srvParams->cfg->Projection);
      if (status != 0) {
        CDBError("Unable to init projection");
        return 1;
      }
    }

    bool drawBilinear = false;
    CStyleConfiguration *styleConfiguration = dataSource->getStyle();
    if (styleConfiguration->styleCompositionName.indexOf("bilinear") >= 0) {
      drawBilinear = true;
    }

#ifdef CConvertLatLonGrid_DEBUG
    CDBDebug("Start projecting numRows %d numCells %d", numY, numX);
#endif
    size_t num = numY * numX;

    proj_trans_generic(imageWarper.projLatlonToDest, PJ_FWD, lonData, sizeof(double), num, latData, sizeof(double), num, nullptr, 0, 0, nullptr, 0, 0);
#ifdef CConvertLatLonGrid_DEBUG
    CDBDebug("Done projecting numRows %d numCells %d, now start drawing", numY, numX);
#endif

    for (int indexY = 0; indexY < numY - 1; indexY++) {
      for (int indexX = 0; indexX < numX - 1; indexX++) {
        int gridPointer = indexX + indexY * numX;
        int bottom = 1 * numX; //(indexY != 0 ? -numX : numX);
        int right = 1;         //(indexX != 0 ? -1 : 1);
        double lons[4], lats[4];
        lons[0] = lonData[gridPointer];                  // topleft
        lons[1] = lonData[gridPointer + right];          // topright
        lons[2] = lonData[gridPointer + bottom];         // bottomleft
        lons[3] = lonData[gridPointer + bottom + right]; // bottomright
        lats[0] = latData[gridPointer];
        lats[1] = latData[gridPointer + right];
        lats[2] = latData[gridPointer + bottom];
        lats[3] = latData[gridPointer + bottom + right];

        int dlons[4], dlats[4];
        for (size_t dataObjectIndex = 0; dataObjectIndex < nrDataObjects; dataObjectIndex++) {
          float *destinationGrid = ((float *)dataObjects[dataObjectIndex]->cdfVariable->data);
          float *sourceIrregularGrid = (float *)irregularGridVar[dataObjectIndex]->data;
          float irregularGridValues[4];
          irregularGridValues[0] = sourceIrregularGrid[gridPointer];

          if (drawBilinear) {
            // Bilinear mode will use the four corner values to draw a quad with those values interpolated
            irregularGridValues[1] = sourceIrregularGrid[gridPointer + right];
            irregularGridValues[2] = sourceIrregularGrid[gridPointer + bottom];
            irregularGridValues[3] = sourceIrregularGrid[gridPointer + bottom + right];
          } else {
            // Nearest mode will use the topleft value for all values in the quad
            irregularGridValues[1] = irregularGridValues[0];
            irregularGridValues[2] = irregularGridValues[0];
            irregularGridValues[3] = irregularGridValues[0];
          }
          bool irregularGridCellHasNoData = false;
          // Check if this is no data (irregularGridCellHasNoData)
          if (irregularGridValues[0] == fill || irregularGridValues[1] == fill || irregularGridValues[2] == fill || irregularGridValues[3] == fill) irregularGridCellHasNoData = true;

          if (irregularGridCellHasNoData == false) {
            if (dataObjectIndex == 0) {
              for (int j = 0; j < 4; j++) {
                dlons[j] = int((lons[j] - offsetX) / cellSizeX);
                dlats[j] = int((lats[j] - offsetY) / cellSizeY);
              }
            }
            // Draw the data into the new regular grid variable
            fillQuadGouraud(destinationGrid, irregularGridValues, dataSource->dWidth, dataSource->dHeight, dlons, dlats);
          }
        }
      }
    }
    imageWarper.closereproj();
  }
#ifdef CConvertLatLonGrid_DEBUG
  CDBDebug("/convertLatLonGridData");
#endif
  return 0;
}
