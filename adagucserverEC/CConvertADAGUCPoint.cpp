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

#include "CImageWarper.h"
#include "CConvertADAGUCPoint_convert_BIRA_IASB_NETCDF.cpp"
#include "CStyleConfiguration.h"
#include "CTString.h"
#include "utils/minMax.h"

static bool measureTime = false;

static std::vector<std::string> variablesNotToConvert = {
    "time2D", "time", "lon", "lat", "x", "y", "lat_bnds", "lon_bnds", "custom", "projection", "product", "iso_dataset", "tile_properties", "forecast_reference_time"};

static bool checkIfADAGUCPointFormat(CDFObject *cdfObject) {
  /* Some conversions for non ADAGUC point data */
  CConvertADAGUCPoint_convert_BIRA_IASB_NETCDF(cdfObject);
  const auto &featureType = cdfObject->getAttrText("featureType");
  return featureType == "timeSeries" || featureType == "point";
}

// Creates (or reuses/updates) a coordinate dimension variable of the given size, filling it with evenly
// spaced coordinates [offset + cellSize/2, offset + 0.5*cellSize, ...]
static CDF::Variable *createOrUpdateCoordinateVariable(CDFObject *cdfObject, const char *name, int size, double offset, double cellSize) {
  CDF::Variable *var = cdfObject->getDimVarOrCreate(name, size, CDF_DOUBLE);
  if (var->data == nullptr) {
    size_t allocatedElements = var->allocateData();
    for (size_t j = 0; j < allocatedElements; j++) {
      ((double *)var->data)[j] = offset + double(j) * cellSize + cellSize / 2;
    }
  }
  return var;
}

// Creates the new 2D field variable for a swath (point) variable: assigns X,Y dims, copies
// attributes (applying scale/offset to _FillValue), and marks the original variable as a backup.
static void createTwoDVariableFromPointVariable(CDFObject *cdfObject, CDF::Variable *pointVar, CDF::Variable *varY, CDF::Variable *varX) {
  CDF::Variable *new2DVar = cdfObject->addVariable(new CDF::Variable(pointVar->name.c_str(), CDF_FLOAT));
  // Assign dims but skip station.
  for (auto *dimensionLink: pointVar->dimensionlinks) {
    if (!dimensionLink->name.equals("station")) {
      new2DVar->dimensionlinks.push_back(dimensionLink);
    }
  }
  // Assign X,Y dims too.
  new2DVar->dimensionlinks.push_back(varX->dimensionlinks[0]);
  new2DVar->dimensionlinks.push_back(varY->dimensionlinks[0]);
  // Rename the point data variable.
  pointVar->name.concat("_backup");

  // Copy variable attributes
  for (auto *a: pointVar->attributes) {
    if (a->name.equals("_FillValue")) {
      float scaleFactor = pointVar->getAttrDataAt0("scale_factor", 1);
      float addOffset = pointVar->getAttrDataAt0("addOffset", 0);
      float fillValue = pointVar->getAttrDataAt0("_FillValue", 0);
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
};

/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertADAGUCPoint::convertADAGUCPointHeader(CDFObject *cdfObject) {
  // Check whether this is really an adaguc file
  if (!checkIfADAGUCPointFormat(cdfObject)) {
    return 1;
  }

  // Standard bounding box of adaguc data is worldwide
  CDF::Variable *pointLon = cdfObject->getVar("lon");
  CDF::Variable *pointLat = cdfObject->getVar("lat");
  if (pointLon == nullptr || pointLat == nullptr) {
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
  // Default size of adaguc 2dField is 2x2
  int width = 2;
  int height = 2;

  double cellSizeX = (lonMinMax.max - lonMinMax.min) / double(width);
  double cellSizeY = (latMinMax.max - latMinMax.min) / double(height);
  double offsetX = lonMinMax.min;
  double offsetY = latMinMax.min;

  CDF::Variable *varX = createOrUpdateCoordinateVariable(cdfObject, "x", width, offsetX, cellSizeX);
  CDF::Variable *varY = createOrUpdateCoordinateVariable(cdfObject, "y", height, offsetY, cellSizeY);

  if (measureTime) {
    StopWatch_Stop("2D Coordinate dimensions created");
  }

  // Make a list of variables which will be available as 2D fields
  std::vector<std::string> varsToConvert;
  for (auto *var: cdfObject->variables) {
    std::string varName = var->name;
    if (var->isDimension == false) {
      if (std::find(variablesNotToConvert.begin(), variablesNotToConvert.end(), varName) == variablesNotToConvert.end()) {
        varsToConvert.push_back(varName);
      } else {
        var->setAttributeText("ADAGUC_SKIP", "true");
      }
    }
  }

  // Create the new 2D field variables based on the swath variables
  for (const auto &varToConvert: varsToConvert) {
    createTwoDVariableFromPointVariable(cdfObject, cdfObject->getVar(varToConvert), varX, varY);
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
  if (!checkIfADAGUCPointFormat(cdfObject0)) {
    return 1;
  }

  if (measureTime) {
    StopWatch_Stop("Reading data");
  }

  CDF::Variable *pointLon = cdfObject0->getVar("lon");
  CDF::Variable *pointLat = cdfObject0->getVar("lat");
  if (pointLon == nullptr || pointLat == nullptr) {
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

  // Read original data first

  size_t numDims = pointVar[0]->dimensionlinks.size();
  std::vector<size_t> start(numDims);
  std::vector<size_t> count(numDims);
  std::vector<ptrdiff_t> stride(numDims);

  /*
   * There is always a station dimension, we wish to read all stations and for the other dimensions just one: count=1;
   *
   * Therefore we need to know what the station dimension index is. Finds it for `variable` (owned by
   * `ownerCdfObject`), fills start/count/stride accordingly, and returns the station dimension's size.
   */
  auto setStationDimensionIndices = [&](const CDF::Variable *variable, CDFObject *ownerCdfObject) -> size_t {
    int stationDimIndex = -1;
    for (size_t j = 0; j < variable->dimensionlinks.size(); j++) {
      if (variable->dimensionlinks[j]->name.equals("station") || ownerCdfObject->getVariableNE(variable->dimensionlinks[j]->name.c_str()) == NULL) {
        stationDimIndex = j;
        break;
      }
    }
    size_t stationSize = 1;
    for (size_t j = 0; j < variable->dimensionlinks.size(); j++) {
      if (stationDimIndex == int(j)) {
        start[j] = 0;
        stationSize = variable->dimensionlinks[j]->getSize();
        count[j] = stationSize;
        stride[j] = 1;
      } else {
        start[j] = dataSource->getDimensionIndex(variable->dimensionlinks[j]->name.c_str());
        count[j] = 1;
        stride[j] = 1;
      }
    }
    return stationSize;
  };

  int numStations = setStationDimensionIndices(pointLon, cdfObject0);

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

  // Reads the actual data for data object d's point variable: figures out the station dimension
  // index, sets up start/count/stride for it, and reads FLOAT, CDF_CHAR (fixed-width strings, LCW
  // compatibility) or CDF_STRING data accordingly. Returns false on read failure.
  auto readPointVariableData = [&](size_t d) -> bool {
    CDF::Variable *variable = pointVar[d];
    setStationDimensionIndices(variable, dataSource->getDataObject(d)->cdfObject);

    if (variable->nativeType != CDF_STRING && variable->nativeType != CDF_CHAR) {
      variable->freeData();

      if (variable->readData(CDF_FLOAT, start.data(), count.data(), stride.data(), true) != 0) {
        CDBError("Unable to read pointVar[d] data");
        return false;
      }
    } else {
      if (variable->nativeType == CDF_CHAR) {
        // Compatibilty function for LCW data, reading strings stored as fixed arrays of CDF_CHAR
        start[1] = 0;
        count[1] = variable->dimensionlinks[1]->getSize();
        std::vector<std::string> data(count[0]);

        if (variable->readData(CDF_CHAR, start.data(), count.data(), stride.data(), false) != 0) {
          CDBError("Unable to read pointVar[d] data");
          return false;
        }
        for (size_t j = 0; j < count[0]; j++) {
          data[j].copy(((char *)variable->data + j * count[1]), count[1] - 1);
        }
        variable->freeData();

        variable->data = malloc(count[0] * sizeof(size_t));
        for (size_t j = 0; j < count[0]; j++) {
          (((char **)variable->data)[j]) = ((char *)malloc((count[1] + 1) * sizeof(char)));
          strncpy((((char **)variable->data)[j]), data[j].c_str(), count[1]);
        }
      }
      if (variable->nativeType == CDF_STRING) {
        variable->freeData();
        if (variable->readData(CDF_STRING, start.data(), count.data(), stride.data(), false) != 0) {
          CDBError("Unable to read pointVar[d] data");
          return false;
        }
      }
    }
    return true;
  };

  for (size_t d = 0; d < nrDataObjects; d++) {
    if (pointVar[d] != NULL) {
      if (!readPointVariableData(d)) {
        return 1;
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

  // Width needs to be at least 2 in this case.
  if (dataSource->dWidth == 1) dataSource->dWidth = 2;
  if (dataSource->dHeight == 1) dataSource->dHeight = 2;
  double cellSizeX = dataSource->srvParams->geoParams.bbox.span().x / dataSource->dWidth;
  double cellSizeY = dataSource->srvParams->geoParams.bbox.span().y / dataSource->dHeight;
  double offsetX = dataSource->srvParams->geoParams.bbox.left;
  double offsetY = dataSource->srvParams->geoParams.bbox.bottom;

  if (mode == CNETCDFREADER_MODE_OPEN_ALL) {

    createOrUpdateCoordinateVariable(cdfObject0, "x", dataSource->dWidth, offsetX, cellSizeX);
    createOrUpdateCoordinateVariable(cdfObject0, "y", dataSource->dHeight, offsetY, cellSizeY);

    if (measureTime) {
      StopWatch_Stop("Dimensions set");
    }
    dataSource->hasFieldData = false;
    // // Allocate 2D field
    // for (size_t d = 0; d < nrDataObjects; d++) {
    //   if (pointVar[d] != NULL) {
    //     size_t fieldSize = dataSource->dWidth * dataSource->dHeight;
    //     new2DVar[d]->setSize(fieldSize);
    //     new2DVar[d]->allocateData(fieldSize);

    //     // Fill in nodata
    //     auto *dataObject = dataSource->getDataObject(d);
    //     float *fieldData = (float *)dataObject->cdfVariable->data;
    //     float fillValue = dataObject->hasNodataValue ? (float)dataObject->dfNodataValue : NAN;
    //     std::fill_n(fieldData, fieldSize, fillValue);
    //   }
    // }

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

        dataSource->nativeEPSG = dataSource->srvParams->geoParams.crs;
        imageWarper.decodeCRS(&dataSource->nativeProj4, &dataSource->nativeEPSG, &dataSource->srvParams->cfg->Projection);
        if (dataSource->nativeProj4.length() == 0) {
          dataSource->nativeProj4 = LATLONPROJECTION;
          dataSource->nativeEPSG = "EPSG:4326";
          projectionRequired = false;
        }
        CDF::Variable *projectionVar = cdfObject0->addVariable(new CDF::Variable("customgridprojection", CDF_NONE));
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

    auto getKeyAndDescription = [](const CDF::Variable *variable) -> CKeyValueDescriptionPair {
      CDF::Attribute *longName = variable->getAttributeNE("long_name");
      return {.key = variable->name, .description = longName != nullptr ? (const char *)longName->data : variable->name, .value = ""};
    };

    std::vector<CKeyValueDescriptionPair> pointVarKeyDesc;
    pointVarKeyDesc.reserve(nrDataObjects);
    for (const auto &pointV: pointVar) pointVarKeyDesc.push_back(getKeyAndDescription(pointV));

    const auto &pointIdKeyAndDescription = pointID != nullptr ? getKeyAndDescription(pointID) : CKeyValueDescriptionPair();

    const auto &timeVarPerObsKeyAndDescription = hasTimeValuePerObs ? getKeyAndDescription(timeVarPerObs) : CKeyValueDescriptionPair();

    // Pushes a point (plus its paramlist entries: text/char value, station id, obs time) for data object d
    // at station index pPoint/pGeo. With string and char (text) data, the float value is set to NAN and the
    // character data is put in the paramlist. Mutates the prevLon/prevLat/prevVal/hasPrevLatLon/prevRoadId
    // state used for linear interpolation between successive road points.
    auto addPointForDataObject = [&](size_t d, int pPoint, int pGeo, int dlon, int dlat, double lon, double lat, double rotation) {
      if (pointVar[d] == NULL) return;

      auto *dataObject = dataSource->getDataObject(d);
      PointDVWithLatLon *lastPoint = NULL;

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
      }

      if (hasTimeValuePerObs && lastPoint != NULL) {
        lastPoint->paramList.push_back(
            {.key = timeVarPerObsKeyAndDescription.key, .description = timeVarPerObsKeyAndDescription.description, .value = obsTime->dateToISOString(obsTime->getDate(obsTimeData[pPoint]))});
      }
    };

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
        if (latCorrection < 0.01) latCorrection = 0.01;
      }
      double projectedXOffsetX = lon + 0.1 / latCorrection;
      double projectedYOffsetX = lat;
      bool projectionError = false;
      if (projectionRequired) {
        if (imageWarper.reprojfromLatLon(projectedX, projectedY) != 0) {
          projectionError = true;
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
          addPointForDataObject(d, pPoint, pGeo, dlon, dlat, lon, lat, rotation);
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
