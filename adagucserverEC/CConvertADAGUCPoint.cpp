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

#include "CConvertADAGUCPoint.h"
#include <algorithm>
#include "CFillTriangle.h"
#include "CImageWarper.h"
#include "CConvertADAGUCPoint_convert_BIRA_IASB_NETCDF.cpp"
#include "utils/minMax.h"
#include "CStyleConfiguration.h"
#include "CTString.h"
bool measureTime = false;

void CConvertADAGUCPoint::drawDot(int px, int py, int v, int W, int H, float *grid) {
  for (int x = -4; x < 6; x++) {
    for (int y = -4; y < 6; y++) {
      int pointX = px + x;
      int pointY = py + y;
      if (pointX >= 0 && pointY >= 0 && pointX < W && pointY < H) grid[pointX + pointY * W] = v;
    }
  }
}

void CConvertADAGUCPoint::lineInterpolated(float *grid, int W, int H, int startX, int startY, int stopX, int stopY, float startVal, float stopVal) {
  int dX = stopX - startX;
  int dY = stopY - startY;
  if (abs(dX) > 5000) return;
  if (abs(dY) > 5000) return;
  float rc = 0;
  int numPoints = 0;
  if (!(startVal == startVal) || !(stopVal == stopVal)) {
    startVal = 100;
    stopVal = 100;
  }
  float valD = stopVal - startVal;
  float angle = atan2(dY, dX) + (M_PI / 2);
  float dist = 10;
  if (abs(dX) < abs(dY)) {
    rc = float(dX) / float(dY);
    int sx = startX;
    int sy = startY;
    float myVal = startVal;
    if (dY > 0)
      numPoints = dY;
    else {
      numPoints = -dY;
      sx = stopX, sy = stopY;
      myVal = stopVal;
      valD = -valD;
    }
    float valRc = valD / numPoints;
    for (int p = 0; p < numPoints; p++) {
      float v = valRc * float(p) + myVal;

      float px = float(p) * rc + sx + cos(angle) * dist;
      float py = p + sy + sin(angle) * dist;
      drawDot(px, py, v, W, H, grid);
    }
  } else {
    int sx = startX;
    int sy = startY;
    float myVal = startVal;
    rc = float(dY) / float(dX);
    if (dX > 0)
      numPoints = dX;
    else {
      numPoints = -dX;
      sx = stopX, sy = stopY;
      myVal = stopVal;
      valD = -valD;
    }
    float valRc = valD / numPoints;
    for (int p = 0; p < numPoints; p++) {
      float v = valRc * float(p) + myVal;

      float px = p + sx + cos(angle) * dist;
      float py = float(p) * rc + sy + sin(angle) * dist;
      drawDot(px, py, v, W, H, grid);
    }
  }
}

int CConvertADAGUCPoint::checkIfADAGUCPointFormat(CDFObject *cdfObject) {
  /* Some conversions for non ADAGUC point data */
  CConvertADAGUCPoint_convert_BIRA_IASB_NETCDF(cdfObject);

  try {
    if (cdfObject->getAttributeThrows("featureType")->toString().equals("timeSeries") == false && cdfObject->getAttributeThrows("featureType")->toString().equals("point") == false) return 1;
  } catch (int e) {
    return 1;
  }
  return 0;
}

/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertADAGUCPoint::convertADAGUCPointHeader(CDFObject *cdfObject) {
  // Check whether this is really an adaguc file
  if (CConvertADAGUCPoint::checkIfADAGUCPointFormat(cdfObject) == 1) return 1;

  // Standard bounding box of adaguc data is worldwide
  CDF::Variable *pointLon;
  CDF::Variable *pointLat;

  try {
    pointLon = cdfObject->getVariableThrows("lon");
    pointLat = cdfObject->getVariableThrows("lat");
  } catch (int e) {
    CDBError("lat or lon variables not found");
    return 1;
  }

  if (measureTime) {
    StopWatch_Stop("ADAGUC POINT DATA");
  }
  if (pointLon->readData(CDF_FLOAT, true) != 0) {
    CDBError("Unable to read lon data");
    return 1;
  }
  if (pointLat->readData(CDF_FLOAT, true) != 0) {
    CDBError("Unable to read lat data");
    return 1;
  }

  if (measureTime) {
    StopWatch_Stop("DATA READ");
  }
  MinMax lonMinMax;
  MinMax latMinMax;
  lonMinMax.min = -180; // Initialize to whole world
  latMinMax.min = -90;
  lonMinMax.max = 180;
  latMinMax.max = 90;
  if (pointLon->getSize() > 0) {
    lonMinMax = getMinMax(pointLon);
    latMinMax = getMinMax(pointLat);
  }
  if (measureTime) {
    StopWatch_Stop("MIN/MAX Calculated");
  }
  double dfBBOX[] = {lonMinMax.min, latMinMax.min, lonMinMax.max, latMinMax.max};

  // Default size of adaguc 2dField is 2x2
  int width = 2;
  int height = 2;

  double cellSizeX = (dfBBOX[2] - dfBBOX[0]) / double(width);
  double cellSizeY = (dfBBOX[3] - dfBBOX[1]) / double(height);
  double offsetX = dfBBOX[0];
  double offsetY = dfBBOX[1];

  // Add geo variables, only if they are not there already
  CDF::Dimension *dimX = cdfObject->getDimensionNE("x");
  CDF::Dimension *dimY = cdfObject->getDimensionNE("y");

  CDF::Variable *varX = cdfObject->getVariableNE("x");
  CDF::Variable *varY = cdfObject->getVariableNE("y");
  if (dimX == NULL || dimY == NULL || varX == NULL || varY == NULL) {
    // If not available, create new dimensions and variables (X,Y,T)
    // For x
    dimX = new CDF::Dimension();
    dimX->name = "x";
    dimX->setSize(width);
    cdfObject->addDimension(dimX);
    varX = new CDF::Variable();
    varX->setType(CDF_DOUBLE);
    varX->name = ("x");
    varX->isDimension = true;
    varX->dimensionlinks.push_back(dimX);
    cdfObject->addVariable(varX);
    varX->allocateData(dimX->length);

    // For y
    dimY = new CDF::Dimension();
    dimY->name = "y";
    dimY->setSize(height);
    cdfObject->addDimension(dimY);
    varY = new CDF::Variable();
    varY->setType(CDF_DOUBLE);
    varY->name = ("y");
    varY->isDimension = true;
    varY->dimensionlinks.push_back(dimY);
    cdfObject->addVariable(varY);
    varY->allocateData(dimY->length);

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

  if (measureTime) {
    StopWatch_Stop("2D Coordinate dimensions created");
  }

  // Make a list of variables which will be available as 2D fields
  std::vector<std::string> varsToConvert;
  for (auto *var: cdfObject->variables) {
    if (var->isDimension == false) {
      {
        if (!var->name.equals("time2D") && !var->name.equals("time") && !var->name.equals("lon") && !var->name.equals("lat") && !var->name.equals("x") && !var->name.equals("y") &&
            !var->name.equals("lat_bnds") && !var->name.equals("lon_bnds") && !var->name.equals("custom") && !var->name.equals("projection") && !var->name.equals("product") &&
            !var->name.equals("iso_dataset") && !var->name.equals("tile_properties") && !var->name.equals("forecast_reference_time")) {
          varsToConvert.push_back(var->name);
        }
        if (var->name.equals("projection")) {
          var->setAttributeText("ADAGUC_SKIP", "true");
        }
      }
    }
  }

  // Create the new 2D field variables based on the swath variables
  for (const auto &varToConvert: varsToConvert) {
    CDF::Variable *pointVar = cdfObject->getVariableThrows(varToConvert);

    CDF::Variable *new2DVar = new CDF::Variable();
    cdfObject->addVariable(new2DVar);

    // Assign X,Y,T dims

    for (auto *dimensionLink: pointVar->dimensionlinks) {
      bool skip = dimensionLink->name.equals("station");
      if (!skip) {
        new2DVar->dimensionlinks.push_back(dimensionLink);
      }
    }

    new2DVar->dimensionlinks.push_back(dimY);
    new2DVar->dimensionlinks.push_back(dimX);

    new2DVar->setType(CDF_FLOAT);

    new2DVar->name = pointVar->name.c_str();
    pointVar->name.concat("_backup");

    // Copy variable attributes
    for (auto *a: pointVar->attributes) {
      if (a->name.equals("_FillValue")) {
        float scaleFactor = 1, addOffset = 0, fillValue = 0;
        ;
        try {
          pointVar->getAttributeThrows("scale_factor")->getData(&scaleFactor, 1);
        } catch (int e) {
        }
        try {
          pointVar->getAttributeThrows("add_offset")->getData(&addOffset, 1);
        } catch (int e) {
        }
        try {
          pointVar->getAttributeThrows("_FillValue")->getData(&fillValue, 1);
        } catch (int e) {
        }
        fillValue *= scaleFactor + addOffset;
        new2DVar->setAttribute("_FillValue", CDF_FLOAT, &fillValue, 1);
      } else {
        new2DVar->setAttribute(a->name.c_str(), a->getType(), a->data, a->length);
      }
    }
    new2DVar->setAttributeText("ADAGUC_POINT", "true");

    int typeId = pointVar->getType();
    new2DVar->setAttribute("ADAGUC_ORGPOINT_TYPE", CDF_INT, &typeId, 1);
    new2DVar->setAttributeText("ADAGUC_ORGPOINT_VARNAME", pointVar->name);

    // The swath variable is not directly plotable, so skip it
    pointVar->setAttributeText("ADAGUC_SKIP", "true");
    pointVar->setAttributeText("ADAGUC_ORGPOINT", "true");

    if (new2DVar->getAttributeNE("_FillValue") == NULL) {
      float f = -9999999;
      new2DVar->setAttribute("_FillValue", CDF_FLOAT, &f, 1);
    }
    // Scale and offset are already applied
    new2DVar->removeAttribute("scale_factor");
    new2DVar->removeAttribute("add_offset");
  }

  if (measureTime) {
    StopWatch_Stop("Header done");
  }

  return 0;
}

/**
 * This function draws the virtual 2D variable into a new 2D field
 */
int CConvertADAGUCPoint::convertADAGUCPointData(CDataSource *dataSource, int mode) {

  CDFObject *cdfObject0 = dataSource->getDataObject(0)->cdfObject;
  if (CConvertADAGUCPoint::checkIfADAGUCPointFormat(cdfObject0) == 1) return 1;

  if (measureTime) {
    StopWatch_Stop("Reading data");
  }

  CDF::Variable *pointLon;
  CDF::Variable *pointLat;

  try {
    pointLon = cdfObject0->getVariableThrows("lon");
    pointLat = cdfObject0->getVariableThrows("lat");
  } catch (int e) {
    CDBError("lat or lon variables not found");
    return 1;
  }

  size_t nrDataObjects = dataSource->getNumDataObjects();

  std::vector<CDF::Variable *> new2DVar(nrDataObjects, nullptr);
  std::vector<CDF::Variable *> pointVar(nrDataObjects, nullptr);

  for (size_t d = 0; d < nrDataObjects; d++) {
    new2DVar[d] = dataSource->getDataObject(d)->cdfVariable;
    std::string origSwathName = new2DVar[d]->name;
    origSwathName += "_backup";
    pointVar[d] = dataSource->getDataObject(d)->cdfObject->getVariableNE(origSwathName);
    if (pointVar[d] == NULL) {
      CDBWarning("Unable to find orignal swath variable with name %s", origSwathName.c_str());

      return 1;
    }
  }

  /* Find out which variables are point data variables */
  std::vector<CDF::Variable *> pointVariables;
  for (auto *variable: cdfObject0->variables) {
    if (variable->getAttributeNE("ADAGUC_ORGPOINT") != NULL) {
      pointVariables.push_back(variable);
    }
  }

  // Read original data first

  size_t numDims = pointVar[0]->dimensionlinks.size();
  std::vector<size_t> start(numDims);
  std::vector<size_t> count(numDims);
  std::vector<ptrdiff_t> stride(numDims);

  /*
   * There is always a station dimension, we wish to read all stations and for the other dimensions just one: count=1;
   *
   * Therefore we need to know what the station dimension index is
   */

  /*First read LAT and LON*/

  /*Find which index is the station dim*/
  int stationDimIndexInCoord = -1;
  int numStations = 1;
  for (size_t j = 0; j < pointLon->dimensionlinks.size(); j++) {
    if (pointLon->dimensionlinks[j]->name.equals("station") || cdfObject0->getVariableNE(pointLon->dimensionlinks[j]->name.c_str()) == NULL) {
      stationDimIndexInCoord = j;
      break;
    }
  }
  /*Set dimension indices*/
  for (size_t j = 0; j < pointLon->dimensionlinks.size(); j++) {
    if (stationDimIndexInCoord == int(j)) {
      start[j] = 0;
      numStations = pointLon->dimensionlinks[j]->getSize();
      count[j] = numStations;
      stride[j] = 1;
    } else {
      start[j] = dataSource->getDimensionIndex(pointLon->dimensionlinks[j]->name.c_str());
      count[j] = 1;
      stride[j] = 1;
    }
  }

  pointLon->freeData();
  pointLat->freeData();
  if (pointLon->readData(CDF_FLOAT, start.data(), count.data(), stride.data(), true) != 0) {
    CDBError("Unable to read pointLon data");
    return 1;
  }
  if (pointLat->readData(CDF_FLOAT, start.data(), count.data(), stride.data(), true) != 0) {
    CDBError("Unable to read pointLat data");
    return 1;
  }

  if (measureTime) {
    StopWatch_Stop("Lat and lon read");
  }

  for (size_t d = 0; d < nrDataObjects; d++) {
    if (pointVar[d] != NULL) {
      /*Second read actual variables*/
      int stationDimIndexInVariable = -1;

      for (size_t j = 0; j < pointVar[d]->dimensionlinks.size(); j++) {
        if (pointVar[d]->dimensionlinks[j]->name.equals("station") || dataSource->getDataObject(d)->cdfObject->getVariableNE(pointVar[d]->dimensionlinks[j]->name.c_str()) == NULL) {
          stationDimIndexInVariable = j;
          break;
        }
      }

      /*Set dimension indices*/
      for (size_t j = 0; j < pointVar[d]->dimensionlinks.size(); j++) {
        if (stationDimIndexInVariable == int(j)) {
          start[j] = 0;
          count[j] = pointVar[d]->dimensionlinks[j]->getSize();
          stride[j] = 1;
        } else {
          start[j] = dataSource->getDimensionIndex(pointVar[d]->dimensionlinks[j]->name.c_str());
          count[j] = 1;
          stride[j] = 1;
        }
      }

      if (pointVar[d]->nativeType != CDF_STRING && pointVar[d]->nativeType != CDF_CHAR) {
        pointVar[d]->freeData();

        if (pointVar[d]->readData(CDF_FLOAT, start.data(), count.data(), stride.data(), true) != 0) {
          CDBError("Unable to read pointVar[d] data");
          return 1;
        }
      } else {

        if (pointVar[d]->nativeType == CDF_CHAR) {
          // Compatibilty function for LCW data, reading strings stored as fixed arrays of CDF_CHAR
          start[1] = 0;
          count[1] = pointVar[d]->dimensionlinks[1]->getSize();
          std::vector<std::string> data(count[0]);

          if (pointVar[d]->readData(CDF_CHAR, start.data(), count.data(), stride.data(), false) != 0) {
            CDBError("Unable to read pointVar[d] data");
            return 1;
          }
          for (size_t j = 0; j < count[0]; j++) {
            data[j].copy(((char *)pointVar[d]->data + j * count[1]), count[1] - 1);
          }
          pointVar[d]->freeData();

          pointVar[d]->data = malloc(count[0] * sizeof(size_t));
          for (size_t j = 0; j < count[0]; j++) {
            (((char **)pointVar[d]->data)[j]) = ((char *)malloc((count[1] + 1) * sizeof(char)));
            strncpy((((char **)pointVar[d]->data)[j]), data[j].c_str(), count[1]);
          }
        }
        if (pointVar[d]->nativeType == CDF_STRING) {
          pointVar[d]->freeData();
          if (pointVar[d]->readData(CDF_STRING, start.data(), count.data(), stride.data(), false) != 0) {
            CDBError("Unable to read pointVar[d] data");
            return 1;
          }
        }
      }
    }
  }

  if (measureTime) {
    StopWatch_Stop("Variables read");
  }

  for (size_t d = 0; d < nrDataObjects; d++) {
    if (pointVar[d] != NULL) {
      CDF::Attribute *fillValue = new2DVar[d]->getAttributeNE("_FillValue");
      if (fillValue != NULL) {
        dataSource->getDataObject(d)->hasNodataValue = true;
        fillValue->getData(&dataSource->getDataObject(d)->dfNodataValue, 1);
      }
    }
  }

  if (measureTime) {
    StopWatch_Stop("FillValues set");
  }

  // Set statistics
  if (dataSource->stretchMinMax) {

    if (dataSource->statistics == NULL) {
      dataSource->statistics = new Statistics();
      dataSource->statistics->calculate(pointVar[0]->getSize(), ((float *)pointVar[0]->data), CDF_FLOAT, dataSource->getDataObject(0)->dfNodataValue, dataSource->getDataObject(0)->hasNodataValue);
    }
  }

  if (measureTime) {
    StopWatch_Stop("Statistics set");
  }

  // Make the width and height of the new 2D adaguc field the same as the viewing window
  dataSource->dWidth = dataSource->srvParams->geoParams.width;
  dataSource->dHeight = dataSource->srvParams->geoParams.height;

  if (dataSource->dWidth == 1 && dataSource->dHeight == 1) {
    dataSource->srvParams->geoParams.bbox = dataSource->srvParams->geoParams.bbox;
  }
  // Width needs to be at least 2 in this case.
  if (dataSource->dWidth == 1) dataSource->dWidth = 2;
  if (dataSource->dHeight == 1) dataSource->dHeight = 2;
  double cellSizeX = dataSource->srvParams->geoParams.bbox.span().x / dataSource->dWidth;
  double cellSizeY = dataSource->srvParams->geoParams.bbox.span().y / dataSource->dHeight;
  double offsetX = dataSource->srvParams->geoParams.bbox.left;
  double offsetY = dataSource->srvParams->geoParams.bbox.bottom;

  if (mode == CNETCDFREADER_MODE_OPEN_ALL) {

    CDF::Dimension *dimX;
    CDF::Dimension *dimY;
    CDF::Variable *varX;
    CDF::Variable *varY;

    // Create new dimensions and variables (X,Y,T)
    dimX = cdfObject0->getDimensionThrows("x");
    dimX->setSize(dataSource->dWidth);

    dimY = cdfObject0->getDimensionThrows("y");
    dimY->setSize(dataSource->dHeight);

    varX = cdfObject0->getVariableThrows("x");
    varY = cdfObject0->getVariableThrows("y");

    varX->allocateData(dimX->length);
    varY->allocateData(dimY->length);

    // Fill in the X and Y dimensions with the array of coordinates
    for (size_t j = 0; j < dimX->length; j++) {
      double x = offsetX + double(j) * cellSizeX + cellSizeX / 2;
      ((double *)varX->data)[j] = x;
    }
    for (size_t j = 0; j < dimY->length; j++) {
      double y = offsetY + double(j) * cellSizeY + cellSizeY / 2;
      ((double *)varY->data)[j] = y;
    }

    if (measureTime) {
      StopWatch_Stop("Dimensions set");
    }

    // Allocate 2D field
    for (size_t d = 0; d < nrDataObjects; d++) {
      if (pointVar[d] != NULL) {
        size_t fieldSize = dataSource->dWidth * dataSource->dHeight;
        new2DVar[d]->setSize(fieldSize);
        new2DVar[d]->allocateData(fieldSize);

        // Fill in nodata
        auto *dataObject = dataSource->getDataObject(d);
        float *fieldData = (float *)dataObject->cdfVariable->data;
        float fillValue = dataObject->hasNodataValue ? (float)dataObject->dfNodataValue : NAN;
        std::fill_n(fieldData, fieldSize, fillValue);
      }
    }

    if (measureTime) {
      StopWatch_Stop("2D Field allocated");
    }

    float *lonData = (float *)pointLon->data;
    float *latData = (float *)pointLat->data;

    CImageWarper imageWarper;
    bool projectionRequired = false;
    if (dataSource->srvParams->geoParams.crs.length() > 0) {
      projectionRequired = true;
      for (size_t d = 0; d < nrDataObjects; d++) {
        if (pointVar[d] != NULL) {
          new2DVar[d]->setAttributeText("grid_mapping", "customgridprojection");
        }
      }
      if (cdfObject0->getVariableNE("customgridprojection") == NULL) {
        CDF::Variable *projectionVar = new CDF::Variable();
        projectionVar->name = ("customgridprojection");
        cdfObject0->addVariable(projectionVar);
        dataSource->nativeEPSG = dataSource->srvParams->geoParams.crs;
        imageWarper.decodeCRS(&dataSource->nativeProj4, &dataSource->nativeEPSG, &dataSource->srvParams->cfg->Projection);
        if (dataSource->nativeProj4.length() == 0) {
          dataSource->nativeProj4 = LATLONPROJECTION;
          dataSource->nativeEPSG = "EPSG:4326";
          projectionRequired = false;
        }
        projectionVar->setAttributeText("proj4_params", dataSource->nativeProj4);
      }
    }

    if (projectionRequired) {
      int status = imageWarper.initreproj(dataSource, dataSource->srvParams->geoParams, &dataSource->srvParams->cfg->Projection);
      if (status != 0) {
        CDBError("Unable to init projection");
        return 1;
      }
    }

    // Read ID's
    CDF::Variable *pointID = cdfObject0->getVariableNE("station");
    if (pointID == NULL) {
      pointID = cdfObject0->getVariableNE("location_backup");
      if (pointID == NULL) {
        pointID = cdfObject0->getVariableNE("id_backup");
      }
    }
    if (pointID != NULL) {
      if (pointID->getType() == CDF_STRING) {
        pointID->readData(CDF_STRING);
        // Set the original datatype as attribute in the variable, point reader will find this.
        pointID->setAttribute("ADAGUC_ORGPOINT_TYPE", CDF_INT, &pointID->currentType, 1);
      } else {
        pointID = NULL;
      }
    }

    CDF::Variable *roadIdVar = cdfObject0->getVariableNE("roadId_backup");
    float *roadIdData = NULL;
    float prevRoadId = -1;
    if (roadIdVar != NULL) {
      {
        CDBDebug("Start reading data");
        roadIdVar->readData(CDF_FLOAT);
        CDBDebug("Done reading data");
        roadIdData = (float *)roadIdVar->data;
        prevRoadId = roadIdData[0];
      }
    }

    // Read dates for obs
    bool hasTimeValuePerObs = false;
    CTime *obsTime = nullptr;
    double *obsTimeData = NULL;
    CDF::Variable *timeVarPerObs = cdfObject0->getVariableNE("time");
    if (timeVarPerObs != NULL) {
      if (timeVarPerObs->isDimension == false) {
        if (timeVarPerObs->dimensionlinks[0]->getSize() == pointVar[0]->dimensionlinks[0]->getSize()) {
          CDF::Attribute *timeStringAttr = timeVarPerObs->getAttributeNE("units");
          if (timeStringAttr != NULL) {
            if (timeStringAttr->data != NULL) {
              obsTime = CTime::GetCTimeInstance(timeVarPerObs);
              if (obsTime != nullptr) {
                hasTimeValuePerObs = true;
                timeVarPerObs->readData(CDF_DOUBLE, start.data(), count.data(), stride.data(), true);
                obsTimeData = (double *)timeVarPerObs->data;
              }
            }
          }
        }
      }
    }

    if (measureTime) {
      CDBDebug("Date numStations = %d", numStations);
    }

    /**
     * The following code is for
     * https://github.com/KNMI/adaguc-server/blob/master/doc/configuration/Point.md#pointstyle-zoomablepoint
     *
     * TODO: https://github.com/KNMI/adaguc-server/issues/586
     */
    float discRadius = 8;
    float discRadiusX = discRadius;
    float discRadiusY = discRadius;
    bool hasZoomableDiscRadius = false;
    float discSize = 1;
    if (dataSource != NULL) {
      CStyleConfiguration *styleConfiguration = dataSource->getStyle();
      if (styleConfiguration != NULL) {
        for (auto pointInterval: styleConfiguration->pointIntervals) {
          if (!pointInterval->attr.discradius.empty()) {
            hasZoomableDiscRadius = true;
            discSize = atof(pointInterval->attr.discradius.c_str());
          }
        }
      }
    }

    int prevLon = 0, prevLat = 0;
    float prevVal = 0;
    bool hasPrevLatLon = false;

    bool doDrawLinearInterpolated = dataSource->getStyle()->renderMethod & RM_POINT_LINEARINTERPOLATION;

    auto getKeyAndDescription = [](const CDF::Variable *variable) -> CKeyValueDescriptionPair {
      CDF::Attribute *longName = variable->getAttributeNE("long_name");
      return {.key = variable->name, .description = longName != nullptr ? (const char *)longName->data : variable->name, .value = ""};
    };

    std::vector<CKeyValueDescriptionPair> pointVarKeyDesc;
    pointVarKeyDesc.reserve(nrDataObjects);
    for (const auto &pointV: pointVar) pointVarKeyDesc.push_back(getKeyAndDescription(pointV));

    const auto &pointIdKeyAndDescription = pointID != nullptr ? getKeyAndDescription(pointID) : CKeyValueDescriptionPair();

    const auto &timeVarPerObsKeyAndDescription = hasTimeValuePerObs ? getKeyAndDescription(timeVarPerObs) : CKeyValueDescriptionPair();

    for (int stationNr = 0; stationNr < numStations; stationNr++) {
      int pPoint = stationNr;
      int pGeo = stationNr;

      double lon = (double)lonData[pGeo];
      double lat = (double)latData[pGeo];

      double rotation = 0;
      double projectedX = lon;
      double projectedY = lat;
      double projectedXOffsetY = lon;
      double projectedYOffsetY = lat + 0.1;
      double latCorrection = 1;
      if (hasZoomableDiscRadius) {
        latCorrection = cos((lat / 90) * (M_PI / 2));
        ;
        if (latCorrection < 0.01) latCorrection = 0.01;
      }
      double projectedXOffsetX = lon + 0.1 / latCorrection;
      double projectedYOffsetX = lat;
      bool projectionError = false;
      if (projectionRequired) {
        if (imageWarper.reprojfromLatLon(projectedX, projectedY) != 0) {
          projectionError = true;
          hasPrevLatLon = false;
        }
        if (hasZoomableDiscRadius) {
          if (imageWarper.reprojfromLatLon(projectedXOffsetY, projectedYOffsetY) != 0) {
            projectionError = true;
          }
          if (imageWarper.reprojfromLatLon(projectedXOffsetX, projectedYOffsetX) != 0) {
            projectionError = true;
          }
          float dX = projectedXOffsetY - projectedX;
          float dY = projectedYOffsetY - projectedY;
          rotation = -atan2(dY, dX) + (M_PI / 2);
        }
      }

      if (hasZoomableDiscRadius) {
        float yDistanceProjected = projectedYOffsetY - projectedY;
        discRadius = ((dataSource->srvParams->geoParams.bbox.top - dataSource->srvParams->geoParams.bbox.bottom) / yDistanceProjected) / float(dataSource->srvParams->geoParams.width);
        discRadius = (discSize / 5) / discRadius;
        if (discRadius < 0.1) discRadius = 0.1;
        discRadiusY = discRadius;

        float xDistanceProjected = projectedXOffsetX - projectedX;
        discRadiusX = ((dataSource->srvParams->geoParams.bbox.right - dataSource->srvParams->geoParams.bbox.left) / xDistanceProjected) / float(dataSource->srvParams->geoParams.width);
        discRadiusX = (discSize / 5) / discRadiusX;
        if (discRadiusX < 0.1) discRadiusX = 0.1;
      }

      if (projectionError == false && cellSizeX != 0 && cellSizeY != 0) {
        int dlon = int((projectedX - offsetX) / cellSizeX);
        int dlat = int((projectedY - offsetY) / cellSizeY);
        for (size_t d = 0; d < nrDataObjects; d++) {
          if (pointVar[d] != NULL) {
            auto *dataObject = dataSource->getDataObject(d);
            float *sdata = (float *)dataObject->cdfVariable->data;
            PointDVWithLatLon *lastPoint = NULL;

            /**
             * With string and char (text) data, the float value is set to NAN and character data is put in the paramlist
             */
            if (pointVar[d]->currentType == CDF_STRING) {
              float v = NAN;
              dataObject->points.push_back(PointDVWithLatLon(dlon, dlat, lon, lat, v));
              lastPoint = &dataObject->points.back();
              /* Get the last pushed point from the array and push the character text data in the paramlist */
              const char *b = ((const char **)pointVar[d]->data)[pPoint];
              lastPoint->paramList.push_back({.key = pointVarKeyDesc[d].key, .description = pointVarKeyDesc[d].description, .value = CT::fromCharPointer(b)});
            }

            if (pointVar[d]->currentType == CDF_CHAR) {
              float v = NAN;
              dataObject->points.push_back(PointDVWithLatLon(dlon, dlat, lon, lat, v));
              lastPoint = &dataObject->points.back();
              /* Get the last pushed point from the array and push the character text data in the paramlist */
              lastPoint->paramList.push_back({.key = pointVarKeyDesc[d].key, .description = pointVarKeyDesc[d].description, .value = CT::fromCharPointer(((const char **)pointVar[d]->data)[pPoint])});
            }

            if (pointVar[d]->currentType == CDF_FLOAT) {
              float val = ((float *)pointVar[d]->data)[pPoint];
              dataObject->points.push_back(PointDVWithLatLon(dlon, dlat, lon, lat, val, rotation, discRadiusX, discRadiusY));
              lastPoint = &dataObject->points.back();
              if (pointID != NULL) {
                lastPoint->paramList.push_back(
                    {.key = pointIdKeyAndDescription.key, .description = pointIdKeyAndDescription.description, .value = CT::fromCharPointer(((const char **)pointID->data)[pGeo])});
              }
              if (doDrawLinearInterpolated && roadIdData != NULL) {
                if (hasPrevLatLon) {
                  float roadId = roadIdData[pPoint];
                  if (roadId == prevRoadId && val == val && prevVal == prevVal) {
                    lineInterpolated(sdata, dataSource->dWidth, dataSource->dHeight, prevLon, prevLat, dlon, dlat, prevVal, val);
                  }
                  prevRoadId = roadId;
                }
              }
              prevLon = dlon;
              prevLat = dlat;
              prevVal = val;
              hasPrevLatLon = true;
            }

            if (hasTimeValuePerObs && lastPoint != NULL) {
              lastPoint->paramList.push_back(
                  {.key = timeVarPerObsKeyAndDescription.key, .description = timeVarPerObsKeyAndDescription.description, .value = obsTime->dateToISOString(obsTime->getDate(obsTimeData[pPoint]))});
            }
          }
        }
      }
    }

    if (measureTime) {
      StopWatch_Stop("Points added");
    }
    imageWarper.closereproj();
  }

  return 0;
}
