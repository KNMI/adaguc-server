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
 * This function draws the virtual 2D variable into a new 2D field
 */
int CConvertLatLonGrid::convertLatLonGridData(CDataSource *dataSource, int mode) {

  CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;
  if (!isLatLonGrid(cdfObject)) return 1;
  size_t nrDataObjects = dataSource->getNumDataObjects();
  if (nrDataObjects <= 0) return 1;

  std::vector<CDataSource::DataObject *> dataObjects(nrDataObjects, nullptr);
  for (size_t d = 0; d < nrDataObjects; d++) {
    dataObjects[d] = dataSource->getDataObject(d);
  }
#ifdef CConvertLatLonGrid_DEBUG

  CDBDebug("convertLatLonGridData %s", dataObjects[0]->cdfVariable->name.c_str());
#endif
  std::vector<CDF::Variable *> destRegularGrid(nrDataObjects, nullptr);
  std::vector<CDF::Variable *> irregularGridVar(nrDataObjects, nullptr);

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
    // Read the data as CDF_FLOAT on purpose, data handling here is done in CDF_FLOAT
    dataSource->readVariableDataForCDFDims(irregularGridVar[d], CDF_FLOAT);

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

  // Detect minimum and maximum values
  MinMax minMax = getMinMax(((float *)irregularGridVar[0]->data), dataObjects[0]->hasNodataValue, fill, irregularGridVar[0]->getSize());

#ifdef CConvertLatLonGrid_DEBUG
  CDBDebug("Calculated min/max : %f %f", minMax.min, minMax.max);
#endif

  // Set statistics
  if (dataSource->stretchMinMax) {
#ifdef CConvertLatLonGrid_DEBUG
    CDBDebug("dataSource->stretchMinMax");
#endif
    if (dataSource->statistics == NULL) {
#ifdef CConvertLatLonGrid_DEBUG
      CDBDebug("Setting statistics: min/max : %f %f", minMax.min, minMax.max);
#endif
      dataSource->statistics = new CDataSource::Statistics();
      dataSource->statistics->setMaximum(minMax.max);
      dataSource->statistics->setMinimum(minMax.min);
    }
  }

  // Make the width and height of the new regular grid field the same as the viewing window
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

#ifdef CConvertLatLonGrid_DEBUG
  CDBDebug("Datasource bbox:%f %f %f %f", dataSource->srvParams->geoParams.bbox.left, dataSource->srvParams->geoParams.bbox.bottom, dataSource->srvParams->geoParams.bbox.right,
           dataSource->srvParams->geoParams.bbox.top);
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
      destRegularGrid[d]->allocateData(fieldSize);
      destRegularGrid[d]->fill(NAN);
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
    if (dataSource->srvParams->geoParams.crs.length() > 0) {
      projectionRequired = true;
      for (size_t d = 0; d < nrDataObjects; d++) {
        destRegularGrid[d]->setAttributeText("grid_mapping", "customgridprojection");
      }
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
    if (projectionRequired) {
      int status = imageWarper.initreproj(dataSource, dataSource->srvParams->geoParams, &dataSource->srvParams->cfg->Projection);
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
