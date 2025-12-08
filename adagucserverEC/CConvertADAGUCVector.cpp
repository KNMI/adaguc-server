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

#include "CConvertADAGUCVector.h"
#include "CFillTriangle.h"
#include "CImageWarper.h"
// #define CCONVERTADAGUCVECTOR_DEBUG

const char *CConvertADAGUCVector::className = "CConvertADAGUCVector";

/**
 * Checks if the format of this file corresponds to the ADAGUC Vector format.
 */
bool isADAGUCVectorFormat(CDFObject *cdfObject) {
  try {

    // Check if necessary variables and dimensions are present.
    cdfObject->getDimension("time");
    cdfObject->getVariable("time");
    cdfObject->getVariable("lat_bnds");
    cdfObject->getVariable("lon_bnds");

    // Check if there are vertices defined and if there are exactly four.
    CDF::Dimension *dimNv = cdfObject->getDimension("nv");
    if (dimNv->getSize() != 4) {
      return false;
    }
  } catch (int e) {
    return false;
  }

  return true;
}

/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertADAGUCVector::convertADAGUCVectorHeader(CDFObject *cdfObject) {
  // Check whether this is really an adaguc file
  if (!isADAGUCVectorFormat(cdfObject)) {
    return 1;
  }

  CDBDebug("Using CConvertADAGUCVector.h");

  // Create virtual X, Y and T variables.
  createVirtualTimeVariable(cdfObject);
  createVirtualGeoVariables(cdfObject);

  // Make a list of variables which will be available as 2D fields
  std::vector<CT::string> varsToConvert;
  for (size_t v = 0; v < cdfObject->variables.size(); v++) {
    CDF::Variable *var = cdfObject->variables[v];
    if (var->isDimension == false) {
      if (!var->name.equals("time2D") && !var->name.equals("time") && !var->name.equals("lon") && !var->name.equals("lat") && !var->name.equals("lat_bnds") && !var->name.equals("lon_bnds") &&
          !var->name.equals("custom") && !var->name.equals("projection") && !var->name.equals("product") && !var->name.equals("iso_dataset") && !var->name.equals("tile_properties")) {
        varsToConvert.push_back(CT::string(var->name.c_str()));
      }
      if (var->name.equals("projection")) {
        var->setAttributeText("ADAGUC_SKIP", "true");
      }
    }
  }

  // Create the new 2D field variables based on the swath variables.
  for (size_t v = 0; v < varsToConvert.size(); v++) {
    CDF::Variable *swathVar = cdfObject->getVariable(varsToConvert[v].c_str());

#ifdef CCONVERTADAGUCVECTOR_DEBUG
    CDBDebug("Converting %s", swathVar->name.c_str());
#endif

    CDF::Variable *new2DVar = new CDF::Variable();
    cdfObject->addVariable(new2DVar);

    // Assign X,Y,T dims
    CDF::Variable *newTimeVar = cdfObject->getVariableNE("time2D");
    new2DVar->dimensionlinks.push_back(newTimeVar->dimensionlinks[0]);
    new2DVar->dimensionlinks.push_back(cdfObject->getDimension("y"));
    new2DVar->dimensionlinks.push_back(cdfObject->getDimension("x"));

    new2DVar->setType(swathVar->getType());
    new2DVar->name = swathVar->name.c_str();
    swathVar->name.concat("_backup");

    // Copy variable attributes
    for (size_t j = 0; j < swathVar->attributes.size(); j++) {
      CDF::Attribute *a = swathVar->attributes[j];
      new2DVar->setAttribute(a->name.c_str(), a->getType(), a->data, a->length);
      new2DVar->setAttributeText("ADAGUC_VECTOR", "true");
    }

    // The swath variable is not directly plotable, so skip it
    swathVar->setAttributeText("ADAGUC_SKIP", "true");

    // Scale and offset are already applied
    new2DVar->removeAttribute("scale_factor");
    new2DVar->removeAttribute("add_offset");

    new2DVar->setType(CDF_FLOAT);
  }

  return 0;
}

/**
 * This function draws the virtual 2D variable into a new 2D field
 */
int CConvertADAGUCVector::convertADAGUCVectorData(CDataSource *dataSource, int mode) {
#ifdef CCONVERTADAGUCVECTOR_DEBUG
  CDBDebug("convertADAGUCVectorData");
#endif
  CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;

  if (!isADAGUCVectorFormat(cdfObject)) {
    return 1;
  }

  size_t nrDataObjects = dataSource->getNumDataObjects();
  CDataSource::DataObject *dataObjects[nrDataObjects];
  for (size_t d = 0; d < nrDataObjects; d++) {
    dataObjects[d] = dataSource->getDataObject(d);
  }
  CDF::Variable *new2DVar;
  new2DVar = dataObjects[0]->cdfVariable;

  CDF::Variable *swathVar;
  CT::string origSwathName = new2DVar->name.c_str();
  origSwathName.concat("_backup");
  swathVar = cdfObject->getVariableNE(origSwathName.c_str());
  if (swathVar == NULL) {
    CDBError("Unable to find orignal swath variable with name %s", origSwathName.c_str());
    return 1;
  }
  CDF::Variable *swathLon;
  CDF::Variable *swathLat;

  try {
    swathLon = cdfObject->getVariable("lon_bnds");
    swathLat = cdfObject->getVariable("lat_bnds");
  } catch (int e) {
    CDBError("lat or lon variables not found");
    return 1;
  }

  // Read original data first
  swathVar->readData(CDF_FLOAT, true);
  swathLon->readData(CDF_FLOAT, true);
  swathLat->readData(CDF_FLOAT, true);

  CDF::Attribute *fillValue = swathVar->getAttributeNE("_FillValue");
  if (fillValue != NULL) {
    dataObjects[0]->hasNodataValue = true;
    fillValue->getData(&dataObjects[0]->dfNodataValue, 1);
#ifdef CCONVERTADAGUCVECTOR_DEBUG
    CDBDebug("_FillValue = %f", dataObjects[0]->dfNodataValue);
#endif
    float f = dataObjects[0]->dfNodataValue;
    new2DVar->getAttribute("_FillValue")->setData(CDF_FLOAT, &f, 1);
  } else {
    dataObjects[0]->hasNodataValue = true;
    dataObjects[0]->dfNodataValue = -9999999;
    float f = dataObjects[0]->dfNodataValue;
    new2DVar->setAttribute("_FillValue", CDF_FLOAT, &f, 1);
  }

  // Detect minimum and maximum values
  float fill = (float)dataObjects[0]->dfNodataValue;
  float min = fill;
  float max = fill;
  for (size_t j = 0; j < swathVar->getSize(); j++) {
    float v = ((float *)swathVar->data)[j];
    if (v != fill) {
      if (min == fill) min = v;
      if (max == fill) max = v;
      if (v < min) min = v;
      if (v > max) max = v;
    }
  }

#ifdef CCONVERTADAGUCVECTOR_DEBUG
  CDBDebug("Calculated min/max : %f %f", min, max);
#endif

  // Set statistics
  if (dataSource->stretchMinMax) {
#ifdef CCONVERTADAGUCVECTOR_DEBUG
    CDBDebug("dataSource->stretchMinMax");
#endif
    if (dataSource->statistics == NULL) {
#ifdef CCONVERTADAGUCVECTOR_DEBUG
      CDBDebug("Setting statistics: min/max : %f %f", min, max);
#endif
      dataSource->statistics = new CDataSource::Statistics();
      dataSource->statistics->setMaximum(max);
      dataSource->statistics->setMinimum(min);
    }
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

#ifdef CCONVERTADAGUCVECTOR_DEBUG
    CDBDebug("Drawing %s", new2DVar->name.c_str());
#endif

    CDF::Dimension *dimX;
    CDF::Dimension *dimY;
    CDF::Variable *varX;
    CDF::Variable *varY;

    // Create new dimensions and variables (X,Y,T)
    dimX = cdfObject->getDimension("x");
    dimX->setSize(dataSource->dWidth);

    dimY = cdfObject->getDimension("y");
    dimY->setSize(dataSource->dHeight);

    varX = cdfObject->getVariable("x");
    varY = cdfObject->getVariable("y");

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
    new2DVar->setSize(fieldSize);
    CDF::allocateData(new2DVar->getType(), &(new2DVar->data), fieldSize);

    // Draw data!
    if (dataObjects[0]->hasNodataValue) {
      for (size_t j = 0; j < fieldSize; j++) {
        ((float *)dataObjects[0]->cdfVariable->data)[j] = (float)dataObjects[0]->dfNodataValue;
      }
    } else {
      for (size_t j = 0; j < fieldSize; j++) {
        ((float *)dataObjects[0]->cdfVariable->data)[j] = NAN;
      }
    }

    float *sdata = ((float *)dataObjects[0]->cdfVariable->data);

    float *lonData = (float *)swathLon->data;
    float *latData = (float *)swathLat->data;

    int numTimes = swathVar->dimensionlinks[0]->getSize();

#ifdef CCONVERTADAGUCVECTOR_DEBUG
    CDBDebug("numTimes %d ", numTimes);
#endif

    CImageWarper imageWarper;
    bool projectionRequired = false;
    if (dataSource->srvParams->geoParams.crs.length() > 0) {
      projectionRequired = true;
      new2DVar->setAttributeText("grid_mapping", "customgridprojection");
      if (cdfObject->getVariableNE("customgridprojection") == NULL) {
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
        projectionVar->setAttributeText("proj4_params", dataSource->nativeProj4.c_str());
      }
    }

#ifdef CCONVERTADAGUCVECTOR_DEBUG
    CDBDebug("Datasource CRS = %s nativeproj4 = %s", dataSource->nativeEPSG.c_str(), dataSource->nativeProj4.c_str());
    CDBDebug("Datasource bbox:%f %f %f %f", dataSource->srvParams->geoParams.bbox.left, dataSource->srvParams->geoParams.bbox.bottom, dataSource->srvParams->geoParams.bbox.right,
             dataSource->srvParams->geoParams.bbox.top);
    CDBDebug("Datasource width height %d %d", dataSource->dWidth, dataSource->dHeight);
#endif

    if (projectionRequired) {
      int status = imageWarper.initreproj(dataSource, dataSource->srvParams->geoParams, &dataSource->srvParams->cfg->Projection);
      if (status != 0) {
        CDBError("Unable to init projection");
        return 1;
      }
    }

    float *swathData = (float *)swathVar->data;
    float fillValueLat = fill;
    float fillValueLon = fill;

    /* Get time variable */
    double *timeData = NULL;
    double timeNotLowerThan = -1, timeNotMoreThan = -1;
    ;

    CDF::Variable *origTimeVar = cdfObject->getVariableNE("time");

    /* Read time units and create ctime object */
    CTime *obsTime = CTime::GetCTimeInstance(origTimeVar);
    if (obsTime == nullptr) {
      CDBError("Unable to initialize time");
    } else {
      /* Read time data */
      double tfill = -1;
      try {
        origTimeVar->getAttribute("_FillValue")->getData(&tfill, 1);
      } catch (int) {
      }
      if (origTimeVar->readData(CDF_DOUBLE) != 0) {
        CDBError("Unable to read time variable");

      } else {
        timeData = (double *)origTimeVar->data;

        /* Find timerange */
        CT::string timeStringFromURL = "";
        for (size_t j = 0; j < dataSource->requiredDims.size(); j++) {
          CDBDebug("%s", dataSource->requiredDims[j]->name.c_str());
          if (dataSource->requiredDims[j]->name.equals("time")) {
            timeStringFromURL = dataSource->requiredDims[j]->value.c_str();
          }
        }
        CDBDebug("timeStringFromURL = %s", timeStringFromURL.c_str());
        std::vector<CT::string> timeStrings = timeStringFromURL.splitToStack("/");
        if (timeStrings.size() == 2) {
          timeNotLowerThan = obsTime->dateToOffset(obsTime->freeDateStringToDate(timeStrings[0].c_str()));
          timeNotMoreThan = obsTime->dateToOffset(obsTime->freeDateStringToDate(timeStrings[1].c_str()));
        }
      }
    }

    for (int timeNr = 0; timeNr < numTimes; timeNr++) {
      if (timeData != NULL && timeNotLowerThan != timeNotMoreThan) {
        double currentTimeValue = timeData[timeNr];
        if (currentTimeValue < timeNotLowerThan || currentTimeValue > timeNotMoreThan) continue;
      }

      int pSwath = timeNr;

      double lons[4], lats[4];
      float vals[4];
      lons[0] = (float)lonData[pSwath * 4 + 0];
      lons[1] = (float)lonData[pSwath * 4 + 1];
      lons[2] = (float)lonData[pSwath * 4 + 3];
      lons[3] = (float)lonData[pSwath * 4 + 2];

      lats[0] = (float)latData[pSwath * 4 + 0];
      lats[1] = (float)latData[pSwath * 4 + 1];
      lats[2] = (float)latData[pSwath * 4 + 3];
      lats[3] = (float)latData[pSwath * 4 + 2];

      vals[0] = swathData[pSwath];
      vals[1] = swathData[pSwath];
      vals[2] = swathData[pSwath];
      vals[3] = swathData[pSwath];

      bool tileHasNoData = false;

      float lonMin, lonMax, lonMiddle = 0;
      for (int j = 0; j < 4; j++) {
        float lon = lons[j];
        if (j == 0) {
          lonMin = lon;
          lonMax = lon;
        } else {
          if (lon < lonMin) lonMin = lon;
          if (lon > lonMax) lonMax = lon;
        }
        lonMiddle += lon;
        float lat = lats[j];
        float val = vals[j];
        if (val == fill || val == INFINITY || val == NAN || val == -INFINITY || !(val == val)) {
          tileHasNoData = true;
          break;
        }
        if (lat == fillValueLat || lat == INFINITY || lat == -INFINITY || !(lat == lat)) {
          tileHasNoData = true;
          break;
        }
        if (lon == fillValueLon || lon == INFINITY || lon == -INFINITY || !(lon == lon)) {
          tileHasNoData = true;
          break;
        }
      }
      lonMiddle /= 4;
      if (lonMax - lonMin >= 350) {
        if (lonMiddle > 0) {
          for (int j = 0; j < 4; j++)
            if (lons[j] < lonMiddle) lons[j] += 360;
        } else {
          for (int j = 0; j < 4; j++)
            if (lons[j] > lonMiddle) lons[j] -= 360;
        }
      }

      vals[1] = vals[0];
      vals[2] = vals[0];
      vals[3] = vals[0];
      int dlons[4], dlats[4];
      bool projectionIsOk = true;
      if (tileHasNoData == false) {
        int dlonMin, dlonMax, dlatMin, dlatMax;

        for (int j = 0; j < 4; j++) {
          if (projectionRequired) {
            if (imageWarper.reprojfromLatLon(lons[j], lats[j]) != 0) projectionIsOk = false;
          }
          dlons[j] = int((lons[j] - offsetX) / cellSizeX);
          dlats[j] = int((lats[j] - offsetY) / cellSizeY);
          int lon = dlons[j];
          int lat = dlats[j];
          if (j == 0) {
            dlonMin = lon;
            dlonMax = lon;
            dlatMin = lat;
            dlatMax = lat;
          } else {
            if (lon < dlonMin) dlonMin = lon;
            if (lon > dlonMax) dlonMax = lon;
            if (lat < dlatMin) dlatMin = lat;
            if (lat > dlatMax) dlatMax = lat;
          }
        }
        if (dlonMax - dlonMin < 256 && dlatMax - dlatMin < 256) {
          if (projectionIsOk) {
            fillQuadGouraud(sdata, vals, dataSource->dWidth, dataSource->dHeight, dlons, dlats);
          }
        }
      }
    }

    imageWarper.closereproj();
  }
#ifdef CCONVERTADAGUCVECTOR_DEBUG
  CDBDebug("/convertADAGUCVectorData");
#endif
  return 0;
}

/**
 * Create virtual time variable.
 * TODO: Currently mostly duplicate code with CConvertASCAT --> Refactor to general method.
 */
bool CConvertADAGUCVector::createVirtualTimeVariable(CDFObject *cdfObject) {

  // Is there a time variable
  CDF::Variable *origT = cdfObject->getVariableNE("time");
  if (origT != NULL) {

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
    try {
#ifdef CCONVERTADAGUCVECTOR_DEBUG
      CDBDebug("Start reading time dim");
#endif
      varT->setAttributeText("units", origT->getAttribute("units")->toString().c_str());
      if (origT->readData(CDF_DOUBLE) != 0) {
        CDBError("Unable to read time variable");
      } else {
#ifdef CCONVERTADAGUCVECTOR_DEBUG
        CDBDebug("Done reading time dim");
#endif

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
#ifdef CCONVERTADAGUCVECTOR_DEBUG
        CDBDebug("firstTimeValue  = %f", firstTimeValue);
#endif
        // Set the time data
        varT->setData(CDF_DOUBLE, &firstTimeValue, 1);
      }
    } catch (int e) {
    }
  }
  return true;
}

/**
 * Creates the virtual X and Y variables for the headers.
 */
bool CConvertADAGUCVector::createVirtualGeoVariables(CDFObject *cdfObject) {

  // Standard bounding box of adaguc data is worldwide.
  double dfBBOX[] = {-180, -90, 180, 90};

  // Default size of adaguc 2dField is 2x2
  int width = 2;
  int height = 2;

  double cellSizeX = (dfBBOX[2] - dfBBOX[0]) / double(width);
  double cellSizeY = (dfBBOX[3] - dfBBOX[1]) / double(height);
  double offsetX = dfBBOX[0];
  double offsetY = dfBBOX[1];

  // Create the x and y variables.
  // For x
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

  // Fill in the X and Y dimensions with the array of coordinates
  for (size_t j = 0; j < dimX->length; j++) {
    double x = offsetX + double(j) * cellSizeX + cellSizeX / 2;
    ((double *)varX->data)[j] = x;
  }
  for (size_t j = 0; j < dimY->length; j++) {
    double y = offsetY + double(j) * cellSizeY + cellSizeY / 2;
    ((double *)varY->data)[j] = y;
  }
  return true;
}
