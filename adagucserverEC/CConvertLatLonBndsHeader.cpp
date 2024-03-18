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

#include "CFillTriangle.h"
#include "CImageWarper.h"
#include "CConvertLatLonBnds.h"

// #define CConvertLatLonBnds_DEBUG

/**
 * This function adjusts the cdfObject by creating virtual 2D variables
 */
int CConvertLatLonBnds::convertLatLonBndsHeader(CDFObject *cdfObject, CServerParams *) {
  CDBDebug("convertLatLonBndsHeader");
  // Check whether this is really an LatLonBnds file
  if (!isThisLatLonBndsData(cdfObject)) return 1;
  CDBDebug("Using CConvertLatLonBnds.h");

  // Standard bounding box of adaguc data is worldwide
  CDF::Variable *pointLon;
  CDF::Variable *pointLat;

  try {
    pointLon = cdfObject->getVariable("lon_bnds");
    pointLat = cdfObject->getVariable("lat_bnds");
  } catch (int e) {
    CDBError("lat or lon variables not found");
    return 1;
  }

  CDBDebug("start reading latlon coordinates");

  pointLon->readData(CDF_DOUBLE, true);
  pointLat->readData(CDF_DOUBLE, true);

#ifdef CConvertLatLonBnds_DEBUG
  StopWatch_Stop("DATA READ");
#endif
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
#ifdef CConvertLatLonBnds_DEBUG
  StopWatch_Stop("MIN/MAX Calculated");
  CDBDebug("%f,%f %f,%f", latMinMax.min, lonMinMax.min, latMinMax.max, lonMinMax.max);
#endif
  double dfBBOX[] = {lonMinMax.min - 0.5, latMinMax.min - 0.5, lonMinMax.max + 0.5, latMinMax.max + 0.5};
  // double dfBBOX[]={-180,-90,180,90};
  // CDBDebug("Datasource dfBBOX:%f %f %f %f",dfBBOX[0],dfBBOX[1],dfBBOX[2],dfBBOX[3]);

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
    varX->name.copy("x");
    varX->isDimension = true;
    varX->dimensionlinks.push_back(dimX);
    cdfObject->addVariable(varX);
    CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);

    // For y
    dimY = new CDF::Dimension();
    dimY->name = "y";
    dimY->setSize(height);
    cdfObject->addDimension(dimY);
    varY = new CDF::Variable();
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
  }

  // Make a list of variables which will be available as 2D fields
  CT::StackList<CT::string> varsToConvert;
  for (size_t v = 0; v < cdfObject->variables.size(); v++) {
    CDF::Variable *var = cdfObject->variables[v];
    if (var->isDimension == false) {
      if (var->dimensionlinks.size() >= 2 && !var->name.equals("acquisition_time") && !var->name.equals("time") && !var->name.equals("lon") && !var->name.equals("lat") &&
          !var->name.equals("longitude") && !var->name.equals("latitude") && !var->name.equals("lon_bnds") && !var->name.equals("lat_bnds")) {
        varsToConvert.add(CT::string(var->name.c_str()));
      }
    }
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
      for (size_t dimlinkNr = 0; dimlinkNr < irregularGridVar->dimensionlinks.size() - 1; dimlinkNr += 1) {
        auto dimlink = irregularGridVar->dimensionlinks[dimlinkNr];
        destRegularGrid->dimensionlinks.push_back(dimlink);
      }

      // Assign X,Y
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
