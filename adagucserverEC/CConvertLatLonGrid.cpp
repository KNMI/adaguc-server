/******************************************************************************
 *
 * Project:  ADAGUC Server
 * Purpose:  ADAGUC OGC Server
 * Author:   Maarten Plieger, plieger "at" knmi.nl
 * Date:     2024-01-26
 *
 ******************************************************************************
 *
 * Copyright 2024, Royal Netherlands Meteorological Institute (KNMI)
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
      double deviationX = fabs((cellSizeXBorder - cellSizeXCenter) / cellSizeXBorder);

      double cellSizeYBorder = fabs(dfdim_Y[0] - dfdim_Y[0 + 1]);
      double cellSizeYCenter = fabs(dfdim_Y[height / 2] - dfdim_Y[height / 2 + 1]);
      double deviationY = fabs((cellSizeYBorder - cellSizeYCenter) / cellSizeYBorder);

      // When the cellsize deviates more than 1% in the center than in the border, we call this grid irregular lat/lon
      if (deviationY > 0.01 || deviationX > 0.01) {
        CDBDebug("Note: Irregular grid encountered");
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

  bool fixIrregular = checkIfIrregularLatLon(cdfObject);

  if (fixIrregular) {
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

      // These variables should not be used, as the 2d lat/lon variables are used instead
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
      longitude->setAttributeText("ADAGUC_SKIP", "TRUE");

      // longitude->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);
      cdfObject->addVariable(longitude);
      longitude->allocateData(lon1DDim->length * lat1DDim->length);

      CDF::Variable *latitude = new CDF::Variable();
      latitude->setType(CDF_DOUBLE);
      latitude->name.copy("latitude");
      latitude->dimensionlinks.push_back(lat1DDim);
      latitude->dimensionlinks.push_back(lon1DDim);
      latitude->setAttributeText("ADAGUCConvertLatLonGridConverter", "DONE");
      latitude->setAttributeText("ADAGUC_SKIP", "TRUE");
      // latitude->setCustomReader(CDF::Variable::CustomMemoryReaderInstance);

      cdfObject->addVariable(latitude);
      latitude->allocateData(lon1DDim->length * lat1DDim->length);

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
