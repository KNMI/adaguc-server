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

#include "CConvertLatLonBnds.h"
#include "CFillTriangle.h"
#include "CImageWarper.h"

// #define CConvertLatLonBnds_DEBUG
/**
 * This function draws the virtual 2D variable into a new 2D field
 */
int CConvertLatLonBnds::convertLatLonBndsData(CDataSource *dataSource, int mode) {
  CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
  if (!isThisLatLonBndsData(cdfObject)) return 1;
  size_t nrDataObjects = dataSource->getNumDataObjects();
  if (nrDataObjects <= 0) return 1;

  CDataSource::DataObject *dataObjects[nrDataObjects];
  for (size_t d = 0; d < nrDataObjects; d++) {
    dataObjects[d] = dataSource->getDataObject(d);
  }
#ifdef CConvertLatLonBnds_DEBUG
  CDBDebug("convertLatLonBndsData %s", dataObjects[0]->cdfVariable->name.c_str());
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
  CDF::Variable *longitudeBnds = cdfObject->getVariableNE("lon_bnds");
  CDF::Variable *latitudeBnds = cdfObject->getVariableNE("lat_bnds");

  for (size_t d = 0; d < nrDataObjects; d++) {
    dataSource->readVariableDataForCDFDims(irregularGridVar[d], CDF_FLOAT);
    float fltFill = NC_FILL_FLOAT;
    double dblFill = NC_FILL_DOUBLE;
    CDF::Attribute *fillValue = irregularGridVar[d]->getAttributeNE("_FillValue");
    if (fillValue != NULL) {
      dataObjects[d]->hasNodataValue = true;
      fillValue->getData(&dblFill, 1);
      dataObjects[d]->dfNodataValue = dblFill;
      fltFill = (float)dblFill;
      destRegularGrid[d]->getAttribute("_FillValue")->setData(CDF_DOUBLE, &fltFill, 1);
    } else {
      dataObjects[d]->hasNodataValue = false;
    }
  }
  float fltFill = NC_FILL_FLOAT;
  if (dataObjects[0]->hasNodataValue) {
    fltFill = (float)dataObjects[0]->dfNodataValue;
  }

  // If the data was not populated in the code above, try to read it from the file
  if (longitudeBnds->data == nullptr) {
    longitudeBnds->readData(CDF_DOUBLE, true);
  }
  if (latitudeBnds->data == nullptr) {
    latitudeBnds->readData(CDF_DOUBLE, true);
  }

  // Detect minimum and maximum values
  MinMax minMax;
  minMax = getMinMax(((float *)irregularGridVar[0]->data), dataObjects[0]->hasNodataValue, (double)fltFill, irregularGridVar[0]->getSize());
  CDBDebug("minMax %f %f", minMax.min, minMax.max);

  // Set statistics
  if (dataSource->stretchMinMax) {
#ifdef CConvertLatLonBnds_DEBUG
    CDBDebug("dataSource->stretchMinMax");
#endif
    if (dataSource->statistics == NULL) {
#ifdef CConvertLatLonBnds_DEBUG
      CDBDebug("Setting statistics: min/max : %f %f", minMax.min, minMax.max);
#endif
      dataSource->statistics = new CDataSource::Statistics();
      dataSource->statistics->setMaximum(minMax.max);
      dataSource->statistics->setMinimum(minMax.min);
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

#ifdef CConvertLatLonBnds_DEBUG
  CDBDebug("Datasource bbox:%f %f %f %f", dataSource->srvParams->Geo->dfBBOX[0], dataSource->srvParams->Geo->dfBBOX[1], dataSource->srvParams->Geo->dfBBOX[2], dataSource->srvParams->Geo->dfBBOX[3]);
  CDBDebug("Datasource width height %d %d", dataSource->dWidth, dataSource->dHeight);
  CDBDebug("L2 %d %d", dataSource->dWidth, dataSource->dHeight);
#endif

  if (mode == CNETCDFREADER_MODE_OPEN_ALL) {
#ifdef CConvertLatLonBnds_DEBUG
    CDBDebug("Drawing %s", destRegularGrid[0]->name.c_str());
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

    size_t fieldSize = dataSource->dWidth * dataSource->dHeight;

    // Allocate and clear data
    for (size_t d = 0; d < nrDataObjects; d++) {
      destRegularGrid[d]->setSize(fieldSize);
      CDF::allocateData(CDF_FLOAT, &(destRegularGrid[d]->data), fieldSize);
      for (size_t j = 0; j < fieldSize; j++) {
        ((float *)dataObjects[d]->cdfVariable->data)[j] = NAN;
      }
    }

    double *lonData = (double *)longitudeBnds->data;
    double *latData = (double *)latitudeBnds->data;

    int numY = latitudeBnds->dimensionlinks[0]->getSize();
    int numX = longitudeBnds->dimensionlinks[0]->getSize();
#ifdef CConvertLatLonBnds_DEBUG
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

    CDF::Dimension *gridpoint = cdfObject->getDimensionNE("gridpoint");
    size_t nr_points = 0;
    if (gridpoint != NULL) {
      nr_points = gridpoint->getSize();
    }
#ifdef CConvertLatLonBnds_DEBUG
    CDBDebug("Start projecting numRows %d numCells %d", numY, numX);
#endif
    size_t num = numY * 4;

    proj_trans_generic(imageWarper.projLatlonToDest, PJ_FWD, lonData, sizeof(double), num, latData, sizeof(double), num, nullptr, 0, 0, nullptr, 0, 0);
#ifdef CConvertLatLonBnds_DEBUG
    CDBDebug("Done projecting numRows %d numCells %d, now start drawing", numY, numX);
#endif

    for (size_t gridPointer = 0; gridPointer < nr_points; gridPointer++) {
      double lons[4], lats[4];
      lons[0] = lonData[gridPointer * 4];     // topleft
      lons[1] = lonData[gridPointer * 4 + 1]; // topright
      lons[2] = lonData[gridPointer * 4 + 2]; // bottomleft
      lons[3] = lonData[gridPointer * 4 + 3]; // bottomright
      lats[0] = latData[gridPointer * 4];
      lats[1] = latData[gridPointer * 4 + 1];
      lats[2] = latData[gridPointer * 4 + 2];
      lats[3] = latData[gridPointer * 4 + 3];

      int dlons[4], dlats[4];
      for (size_t dataObjectIndex = 0; dataObjectIndex < nrDataObjects; dataObjectIndex++) {
        float *destinationGrid = ((float *)dataObjects[dataObjectIndex]->cdfVariable->data);
        void *sourceIrregularGrid = (void *)irregularGridVar[dataObjectIndex]->data;
        float irregularGridValues[4];
        irregularGridValues[0] = ((float *)sourceIrregularGrid)[gridPointer];

        // Bilinear and Nearest mode will use the topleft value for all values in the quad
        irregularGridValues[1] = irregularGridValues[0];
        irregularGridValues[2] = irregularGridValues[0];
        irregularGridValues[3] = irregularGridValues[0];
        bool irregularGridCellHasNoData = false;
        // Check if this is no data (irregularGridCellHasNoData)
        if (irregularGridValues[0] == fltFill || irregularGridValues[1] == fltFill || irregularGridValues[2] == fltFill || irregularGridValues[3] == fltFill) irregularGridCellHasNoData = true;

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
    imageWarper.closereproj();
  }
#ifdef CConvertLatLonBnds_DEBUG
  CDBDebug("/convertLatLonBndsData");
#endif
  return 0;
}
