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
  CDBDebug("about to getNumDataObjects()");
  size_t nrDataObjects = dataSource->getNumDataObjects();
  if (nrDataObjects <= 0) return 1;

  CDBDebug("num data objects: %d", nrDataObjects);

  CDataSource::DataObject *dataObjects[nrDataObjects];
  for (size_t d = 0; d < nrDataObjects; d++) {
    CDBDebug("Getting dataobject %d", d);
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
  if (longitudeBnds->data == nullptr) {
    longitudeBnds->readData(CDF_DOUBLE, true);
  }
  if (latitudeBnds->data == nullptr) {
    latitudeBnds->readData(CDF_DOUBLE, true);
  }

  float fill = (float)dataObjects[0]->dfNodataValue;

  // Detect minimum and maximum values
  MinMax minMax = getMinMax(((float *)irregularGridVar[0]->data), dataObjects[0]->hasNodataValue, fill, irregularGridVar[0]->getSize());

#ifdef CConvertLatLonBnds_DEBUG
  CDBDebug("Calculated min/max : %f %f", minMax.min, minMax.max);
#endif

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
  CDBDebug("mode: %d", mode);

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

    CDBDebug("Allocating %d", fieldSize);
    // Allocate and clear data
    for (size_t d = 0; d < nrDataObjects; d++) {
      destRegularGrid[d]->setSize(fieldSize);
      CDF::allocateData(destRegularGrid[d]->getType(), &(destRegularGrid[d]->data), fieldSize);
      for (size_t j = 0; j < fieldSize; j++) {
        ((float *)dataObjects[d]->cdfVariable->data)[j] = NAN;
      }
    }

    CDBDebug("Allocated");

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
        CDBDebug("projection created");
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
        float *sourceIrregularGrid = (float *)irregularGridVar[dataObjectIndex]->data;
        float irregularGridValues[4];
        irregularGridValues[0] = sourceIrregularGrid[gridPointer];

        if (drawBilinear) {
          // Bilinear mode will use the four corner values to draw a quad with those values interpolated
          irregularGridValues[1] = sourceIrregularGrid[gridPointer];
          irregularGridValues[2] = sourceIrregularGrid[gridPointer];
          irregularGridValues[3] = sourceIrregularGrid[gridPointer];
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
    imageWarper.closereproj();
  }
#ifdef CConvertLatLonBnds_DEBUG
  CDBDebug("/convertLatLonBndsData");
#endif
  return 0;
}

// /**
//  * This function draws the virtual 2D variable into a new 2D field
//  */
// int CConvertLatLonBnds::convertLatLonBndsData(CDataSource *dataSource, int mode) {

//   CDFObject *cdfObject = dataSource->getDataObject(0)->cdfObject;CCONVERTLATLONBNDS_DEBUG
//   CDBDebug("convertLatLonBndsData");

//   CDF::Variable *pointLon;
//   CDF::Variable *pointLat;
//   try {
//     pointLon = cdfObject->getVariable("lon_bnds");
//     pointLat = cdfObject->getVariable("lat_bnds");
//   } catch (int e) {
//     CDBError("lat or lon variables not found");
//     return 1;
//   }

//   pointLon->readData(CDF_FLOAT, true);
//   pointLat->readData(CDF_FLOAT, true);

//   size_t nrDataObjects = dataSource->getNumDataObjects();

//   // Make the width and height of the new 2D adaguc field the same as the viewing window
//   dataSource->dWidth = dataSource->srvParams->Geo->dWidth;
//   dataSource->dHeight = dataSource->srvParams->Geo->dHeight;

//   if (dataSource->dWidth == 1 && dataSource->dHeight == 1) {
//     dataSource->srvParams->Geo->dfBBOX[0] = dataSource->srvParams->Geo->dfBBOX[0];
//     dataSource->srvParams->Geo->dfBBOX[1] = dataSource->srvParams->Geo->dfBBOX[1];
//     dataSource->srvParams->Geo->dfBBOX[2] = dataSource->srvParams->Geo->dfBBOX[2];
//     dataSource->srvParams->Geo->dfBBOX[3] = dataSource->srvParams->Geo->dfBBOX[3];
//   }

//   // Width needs to be at least 2 in this case.
//   if (dataSource->dWidth == 1) dataSource->dWidth = 2;
//   if (dataSource->dHeight == 1) dataSource->dHeight = 2;
//   double cellSizeX = (dataSource->srvParams->Geo->dfBBOX[2] - dataSource->srvParams->Geo->dfBBOX[0]) / double(dataSource->dWidth);
//   double cellSizeY = (dataSource->srvParams->Geo->dfBBOX[3] - dataSource->srvParams->Geo->dfBBOX[1]) / double(dataSource->dHeight);
//   double offsetX = dataSource->srvParams->Geo->dfBBOX[0];
//   double offsetY = dataSource->srvParams->Geo->dfBBOX[1];

//   if (mode == CNETCDFREADER_MODE_OPEN_ALL) {

//     CDF::Dimension *dimX;
//     CDF::Dimension *dimY;
//     CDF::Variable *varX;
//     CDF::Variable *varY;

//     // Create new dimensions and variables (X,Y,T)
//     dimX = cdfObject->getDimension("x");
//     dimX->setSize(dataSource->dWidth);

//     dimY = cdfObject->getDimension("y");
//     dimY->setSize(dataSource->dHeight);

//     varX = cdfObject->getVariable("x");
//     varY = cdfObject->getVariable("y");

//     CDF::allocateData(CDF_DOUBLE, &varX->data, dimX->length);
//     CDF::allocateData(CDF_DOUBLE, &varY->data, dimY->length);

//     // Fill in the X and Y dimensions with the array of coordinates
//     for (size_t j = 0; j < dimX->length; j++) {
//       double x = offsetX + double(j) * cellSizeX + cellSizeX / 2;
//       ((double *)varX->data)[j] = x;
//     }
//     for (size_t j = 0; j < dimY->length; j++) {
//       double y = offsetY + double(j) * cellSizeY + cellSizeY / 2;
//       ((double *)varY->data)[j] = y;
//     }

// #ifdef CCONVERTLATLONBNDS_DEBUG
//     StopWatch_Stop("Dimensions set");
// #endif
//   }

//   if (mode == CNETCDFREADER_MODE_OPEN_ALL) {

//     CDF::Variable *new2DVar[nrDataObjects];
//     CDF::Variable *pointVar[nrDataObjects];

//     for (size_t d = 0; d < nrDataObjects; d++) {
//       new2DVar[d] = dataSource->getDataObject(d)->cdfVariable;
//       CT::string origSwathName = new2DVar[d]->name.c_str();
//       origSwathName.concat("_backup");
//       pointVar[d] = dataSource->getDataObject(d)->cdfObject->getVariableNE(origSwathName.c_str());
//       if (pointVar[d] == NULL) {
//         CDBError("Unable to find orignal swath variable with name %s", origSwathName.c_str());
//         return 1;
//       }
//       pointVar[d]->readData(CDF_FLOAT);
//     }

//     CDF::Attribute *fillValue = pointVar[0]->getAttributeNE("_FillValue");
//     if (fillValue != NULL) {

//       dataSource->getDataObject(0)->hasNodataValue = true;
//       fillValue->getData(&dataSource->getDataObject(0)->dfNodataValue, 1);
// #ifdef CCONVERTLATLONBNDS_DEBUG
//       CDBDebug("_FillValue = %f", dataSource->getDataObject(0)->dfNodataValue);
// #endif
//       CDF::Attribute *fillValue2d = pointVar[0]->getAttributeNE("_FillValue");
//       if (fillValue2d == NULL) {
//         fillValue2d = new CDF::Attribute();
//         fillValue2d->name = "_FillValue";
//       }
//       float f = dataSource->getDataObject(0)->dfNodataValue;
//       fillValue2d->setData(CDF_FLOAT, &f, 1);
//     } else {
//       dataSource->getDataObject(0)->hasNodataValue = true;
//       dataSource->getDataObject(0)->dfNodataValue = NC_FILL_FLOAT;
//       float f = dataSource->getDataObject(0)->dfNodataValue;
//       pointVar[0]->setAttribute("_FillValue", CDF_FLOAT, &f, 1);
//     }

//     float fill = (float)dataSource->getDataObject(0)->dfNodataValue;
//     if (dataSource->stretchMinMax) {
//       if (dataSource->statistics == NULL) {

//         dataSource->statistics = new CDataSource::Statistics();
//         dataSource->statistics->calculate(pointVar[0]->getSize(), (float *)pointVar[0]->data, CDF_FLOAT, dataSource->getDataObject(0)->dfNodataValue, dataSource->getDataObject(0)->hasNodataValue);
//         //         dataSource->statistics->setMaximum(max);
//         //         dataSource->statistics->setMinimum(min);
//       }
//     }

//     // Allocate 2D field
//     for (size_t d = 0; d < nrDataObjects; d++) {
//       size_t fieldSize = dataSource->dWidth * dataSource->dHeight;
//       new2DVar[d]->setSize(fieldSize);
//       CDF::allocateData(new2DVar[d]->getType(), &(new2DVar[d]->data), fieldSize);
//       // CDF::fill((new2DVar[d]->data),new2DVar[d]->getType(),NAN,fieldSize);

//       // Fill in nodata
//       if (dataSource->getDataObject(d)->hasNodataValue) {
//         for (size_t j = 0; j < fieldSize; j++) {
//           ((float *)dataSource->getDataObject(d)->cdfVariable->data)[j] = NAN; //(float)dataSource->getDataObject(d)->dfNodataValue;
//         }
//       } else {
//         for (size_t j = 0; j < fieldSize; j++) {
//           ((float *)dataSource->getDataObject(d)->cdfVariable->data)[j] = NAN;
//         }
//       }
//     }

//     CImageWarper imageWarper;
//     bool projectionRequired = false;
//     if (dataSource->srvParams->Geo->CRS.length() > 0) {
//       projectionRequired = true;
//       for (size_t d = 0; d < nrDataObjects; d++) {
//         new2DVar[d]->setAttributeText("grid_mapping", "customgridprojection");
//       }
//       if (cdfObject->getVariableNE("customgridprojection") == NULL) {
//         CDF::Variable *projectionVar = new CDF::Variable();
//         projectionVar->name.copy("customgridprojection");
//         cdfObject->addVariable(projectionVar);
//         dataSource->nativeEPSG = dataSource->srvParams->Geo->CRS.c_str();
//         imageWarper.decodeCRS(&dataSource->nativeProj4, &dataSource->nativeEPSG, &dataSource->srvParams->cfg->Projection);
//         if (dataSource->nativeProj4.length() == 0) {
//           dataSource->nativeProj4 = LATLONPROJECTION;
//           dataSource->nativeEPSG = "EPSG:4326";
//           projectionRequired = false;
//         }
//         projectionVar->setAttributeText("proj4_params", dataSource->nativeProj4.c_str());
//       }
//     }

// #ifdef CCONVERTLATLONBNDS_DEBUG
//     CDBDebug("Datasource CRS = %s nativeproj4 = %s", dataSource->nativeEPSG.c_str(), dataSource->nativeProj4.c_str());
//     CDBDebug("Datasource bbox:%f %f %f %f", dataSource->srvParams->Geo->dfBBOX[0], dataSource->srvParams->Geo->dfBBOX[1], dataSource->srvParams->Geo->dfBBOX[2],
//     dataSource->srvParams->Geo->dfBBOX[3]); CDBDebug("Datasource width height %d %d", dataSource->dWidth, dataSource->dHeight);
// #endif

//     if (projectionRequired) {
//       int status = imageWarper.initreproj(dataSource, dataSource->srvParams->Geo, &dataSource->srvParams->cfg->Projection);
//       if (status != 0) {
//         CDBError("Unable to init projection");
//         return 1;
//       }
//     }

//     CT::string dmp = CDF::dump(dataSource->getDataObject(0)->cdfVariable);
//     CDBDebug("DUMP: %s", dmp.c_str());
//     float *sdata = ((float *)dataSource->getDataObject(0)->cdfVariable->data);

//     double *lonData = (double *)pointLon->data;
//     double *latData = (double *)pointLat->data;
//     double lons[4], lats[4];
//     float vals[4];

//     int swathLonLength = pointLon->dimensionlinks[pointLon->dimensionlinks.size() - 2]->getSize();

//     //     for(size_t j=0;j<pointLon->dimensionlinks.size();j++){
//     //       CDBDebug("%d %s %d",j,pointLon->dimensionlinks[j]->name.c_str(),pointLon->dimensionlinks[j]->getSize());
//     //     }
//     //
//     //     CDBDebug("swathLonWidth: %d",swathLonWidth);
//     //     CDBDebug("swathLonHeight: %d",swathLonHeight);

//     float fillValueLat = fill;
//     float fillValueLon = fill;
//     int mode = 0;
//     // for( mode=0;mode<2;mode++)
//     CDBDebug("looping");
//     {
//       for (int t = 0; t < 4; t++) {
//         for (int y = 0; y < swathLonLength; y++) {

//           size_t pSwath = t * swathLonLength + y;
//           double val = ((double *)pointVar[0]->data)[pSwath];
//           lons[0] = (float)lonData[pSwath * 4 + 0];
//           lons[1] = (float)lonData[pSwath * 4 + 1];
//           lons[2] = (float)lonData[pSwath * 4 + 3];
//           lons[3] = (float)lonData[pSwath * 4 + 2];

//           lats[0] = (float)latData[pSwath * 4 + 0];
//           lats[1] = (float)latData[pSwath * 4 + 1];
//           lats[2] = (float)latData[pSwath * 4 + 3];
//           lats[3] = (float)latData[pSwath * 4 + 2];

//           // CDBDebug("%d %d = %f %f %f", t, y, lons[0], lats[0], val);

//           vals[0] = val;
//           vals[1] = val;
//           vals[2] = val;
//           vals[3] = val;

//           bool tileHasNoData = false;

//           float lonMin, lonMax, lonMiddle = 0;
//           for (int j = 0; j < 4; j++) {
//             float lon = lons[j];
//             if (j == 0) {
//               lonMin = lon;
//               lonMax = lon;
//             } else {
//               if (lon < lonMin) lonMin = lon;
//               if (lon > lonMax) lonMax = lon;
//             }
//             lonMiddle += lon;
//             float lat = lats[j];
//             float val = vals[j];
//             if (val == fill || val == INFINITY || val == NAN || val == -INFINITY || !(val == val)) {
//               tileHasNoData = true;
//               break;
//             }
//             if (lat == fillValueLat || lat == INFINITY || lat == -INFINITY || !(lat == lat)) {
//               tileHasNoData = true;
//               break;
//             }
//             if (lon == fillValueLon || lon == INFINITY || lon == -INFINITY || !(lon == lon)) {
//               tileHasNoData = true;
//               break;
//             }
//           }
//           lonMiddle /= 4;
//           if (lonMax - lonMin >= 350) {
//             if (lonMiddle > 0) {
//               for (int j = 0; j < 4; j++)
//                 if (lons[j] < lonMiddle) lons[j] += 360;
//             } else {
//               for (int j = 0; j < 4; j++)
//                 if (lons[j] > lonMiddle) lons[j] -= 360;
//             }
//           }

//           int dlons[4], dlats[4];
//           bool projectionIsOk = true;
//           if (tileHasNoData == false) {

//             for (int j = 0; j < 4; j++) {
//               if (projectionRequired) {
//                 if (imageWarper.reprojfromLatLon(lons[j], lats[j]) != 0) projectionIsOk = false;
//               }
//               dlons[j] = int((lons[j] - offsetX) / cellSizeX);
//               dlats[j] = int((lats[j] - offsetY) / cellSizeY);
//             }
//             if (projectionIsOk) {
//               if (mode == 0) {
//                 fillQuadGouraud(sdata, vals, dataSource->dWidth, dataSource->dHeight, dlons, dlats);
//               }
//               val = 1;
//             }
//           }
//         }
//       }
//     }
//   }
// #ifdef CCONVERTLATLONBNDS_DEBUG
//   CDBDebug("/convertLatLonBndsData");
// #endif
//   return 0;
// }
