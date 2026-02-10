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

/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertLatLonGrid::convertLatLonGridHeader(CDFObject *cdfObject, CServerParams *) {
#ifdef CConvertLatLonGrid_DEBUG
  CDBDebug("CHECKING convertLatLonGridHeader");
#endif
  if (!isLatLonGrid(cdfObject)) return 1;

  // Determine bbox based on 2D lat/lon
  double dfBBOX[] = {-180, -90, 180, 90};
  CDF::Variable *longitudeGrid = getLon2D(cdfObject);
  CDF::Variable *latitudeGrid = getLat2D(cdfObject);

  if (longitudeGrid == nullptr) {
    CDBDebug("longitudeGrid = longitudeGrid");
    longitudeGrid = getLon1D(cdfObject);
    CDBDebug("longitudeGrid = longitudeGrid");
  }
  if (latitudeGrid == nullptr) {
    latitudeGrid = getLat1D(cdfObject);
  }

  if (longitudeGrid != nullptr && latitudeGrid != nullptr) {
    // If the data was not populated in the code above, try to read it from the file
    if (longitudeGrid->data == nullptr) {
      longitudeGrid->readData(CDF_DOUBLE, true);
    }
    if (latitudeGrid->data == nullptr) {
      latitudeGrid->readData(CDF_DOUBLE, true);
    }
    if (longitudeGrid->data != nullptr && latitudeGrid->data != nullptr) {
      try {
        MinMax lonMinMax = getMinMax(longitudeGrid);
        MinMax latMinMax = getMinMax(latitudeGrid);
        dfBBOX[0] = lonMinMax.min;
        dfBBOX[1] = latMinMax.min;
        dfBBOX[2] = lonMinMax.max;
        dfBBOX[3] = latMinMax.max;
      } catch (int e) {
        CDBDebug("CALC BOX ERROR %d", e);
      }
    }
  } else {
    CDBWarning("Unable to determine BBOX from lat/lon variables");
  }
  // Default size of new virtual 2dField is 2x2
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
    varX->allocateData(dimX->length);

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

  // Make a list of variables which will be available as 2D fields
  std::vector<CT::string> varsToConvert;
  for (size_t v = 0; v < cdfObject->variables.size(); v++) {
    CDF::Variable *var = cdfObject->variables[v];
    if (var->isDimension == false) {
      if (var->dimensionlinks.size() >= 2 && !var->name.equals("acquisition_time") && !var->name.equals("time") && !var->name.equals("lon") && !var->name.equals("lat") &&
          !var->name.equals("longitude") && !var->name.equals("latitude")) {
        varsToConvert.push_back(CT::string(var->name.c_str()));
      }
    }
  }

  // Add time dimension
  bool addCustomTimeDimension = false;
  try {
    CDF::Dimension *timeDim = cdfObject->getDimensionNE("time");
    if (timeDim == nullptr) {

      CDF::Variable *timeVar = cdfObject->getVariableNE("acquisition_time");
      if (timeVar != nullptr) {
        timeVar->name = "time";
        timeDim = new CDF::Dimension("time", 1);
        cdfObject->addDimension(timeDim);
        timeVar->dimensionlinks.push_back(timeDim);
        addCustomTimeDimension = true;
      }
    }

  } catch (int e) {
  }

  // Create the new regular grid field variables based on the irregular grid variables
  for (size_t v = 0; v < varsToConvert.size(); v++) {
    CDF::Variable *irregularGridVar = cdfObject->getVariable(varsToConvert[v].c_str());
    if (irregularGridVar->dimensionlinks.size() >= 2) {
#ifdef CConvertLatLonGrid_DEBUG
      CDBDebug("Converting %s", irregularGridVar->name.c_str());
#endif

      CDF::Variable *destRegularGrid = new CDF::Variable();
      cdfObject->addVariable(destRegularGrid);

      // Assign all other dims except Y and X
      for (size_t dimlinkNr = 0; dimlinkNr < irregularGridVar->dimensionlinks.size() - 2; dimlinkNr += 1) {
        auto dimlink = irregularGridVar->dimensionlinks[dimlinkNr];
        destRegularGrid->dimensionlinks.push_back(dimlink);
      }

      if (addCustomTimeDimension) {
        CDF::Dimension *timeDim = cdfObject->getDimensionNE("time");
        if (timeDim) {
          destRegularGrid->dimensionlinks.push_back(timeDim);
        }
      }

      // Assign X,Y
      destRegularGrid->dimensionlinks.push_back(dimY);
      destRegularGrid->dimensionlinks.push_back(dimX);

      // The newly allocated grid will always be a float grid and will be internally handled as a float
      destRegularGrid->setType(CDF_FLOAT);
      destRegularGrid->name = irregularGridVar->name.c_str();
      irregularGridVar->name.concat("_backup");

      // Copy variable attributes
      for (size_t j = 0; j < irregularGridVar->attributes.size(); j++) {
        CDF::Attribute *a = irregularGridVar->attributes[j];
        destRegularGrid->setAttribute(a->name.c_str(), a->getType(), a->data, a->length);
      }

      // Set destinationGrid to ADAGUC_VECTOR handling
      destRegularGrid->setAttributeText("ADAGUC_VECTOR", "true");

      // The irregularGridVar variable is not directly plotable, so skip it
      irregularGridVar->setAttributeText("ADAGUC_SKIP", "true");

      // Scale and offset are already applied
      destRegularGrid->removeAttribute("scale_factor");
      destRegularGrid->removeAttribute("add_offset");
    }
  }
  return 0;
}
